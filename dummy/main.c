/*
    Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
    Copyright (c) 2012 - 2022 Xilinx, Inc. All Rights Reserved.
    Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
	SPDX-License-Identifier: MIT


    http://www.FreeRTOS.org
    http://aws.amazon.com/freertos


    1 tab == 4 spaces!
*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "sleep.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "xsysmonpsv_hw.h"
#include "xsysmonpsv.h"
#include "xstatus.h"
#include "xparameters_ps.h"
#include "xiltimer.h"
#include "xgpio.h"
#include "xuartlite.h"
#include "xil_exception.h"
#include "xinterrupt_wrap.h"

// default
//  1  2  3  4  5  6
// |x |x |TX|2 |1 |x |
// |x |x |4 |3 |x |RX|

// 1 = 0b0001
// 2 = 0b0010
// 3 = 0b0100
// 4 = 0b1000


#define DELAY_1_SECOND			1000UL
#define XUARTLITE_BASEADDRESS	XPAR_AXI_UARTLITE_0_BASEADDR
#define TEST_BUFFER_SIZE        100


/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int UartLiteIntrExample(XUartLite *UartLiteInstancePtr,
			UINTPTR BaseAddress);

/************************** Variable Definitions *****************************/

static XUartLite UartLite;
static XUartLite_Config *UartCfg;

static uint32_t MEMORY_VALUE = 0x55555555;
/*-----------------------------------------------------------*/

static int get_cpu_id(void);
/*-----------------------------------------------------------*/

static XSysMonPsv InstancePtr;
float frequency = 0;
char memoryLocation[20];

extern uint8_t __atcm_global0_start[];
extern uint8_t __atcm_global0_end[];
extern uint8_t __atcm_global1_start[];
extern uint8_t __atcm_global1_end[];
extern uint8_t __atcm_lockstep0_start[];
extern uint8_t __atcm_lockstep0_end[];
extern uint8_t __btcm_global0_start[];
extern uint8_t __btcm_global0_end[];
extern uint8_t __btcm_global1_start[];
extern uint8_t __btcm_global1_end[];
extern uint8_t __btcm_lockstep0_start[];
extern uint8_t __btcm_lockstep0_end[];

static int CORE_RUNNING = -1;

static int get_cpu_id(void)
{
    uint32_t mpidr;
    __asm__ volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
    return mpidr & 0x3;
}

int main( void )
{
	// Read the divider set for the clock frequency of the RPU and calculate the actual frequency
	CORE_RUNNING = get_cpu_id();

	msleep(4000);

	XGpio gpio;
	int Status;
	xil_printf("Starting GPIOs.\n");

	Status = XGpio_Initialize(&gpio, XPAR_XGPIO_0_BASEADDR); // or device ID, depending on your BSP
	if (Status != XST_SUCCESS) {
		xil_printf("GPIO not initialized!\n");
		// handle error
	}

	XGpio_SetDataDirection(&gpio, 1, 0x0);   // 0x0 = all 4 bits are outputs
											// 0xF = all 4 bits are inputs

	// Write 4 GPIO bits
	XGpio_DiscreteWrite(&gpio, 1, 0b0001);   // drives bits high

	// Read back the 4 bits
	u32 val = XGpio_DiscreteRead(&gpio, 1);

	/*
	 * Run the UartLite Interrupt example, specify the Device ID that is
	 * generated in xparameters.h.
	 */
	Status = UartLiteIntrExample(&UartLite, XUARTLITE_BASEADDRESS);

	if (Status != XST_SUCCESS) {
		xil_printf("Uartlite interrupt Example Failed\r\n");
		return XST_FAILURE;
	}
	
	frequency = 400. / (((*((volatile unsigned int *)(XPAR_PSV_CRL_0_BASEADDR + 0x10c))) >> 8) & 0x3ff);
	printf("\n%d [%f] Cortex-R5 running!\n", CORE_RUNNING, frequency);

	for( ;; )
	{
		portNOP();
	}

	// ----|tEnd---------------------------------|tStart----


	printf("Starting the scheduler.\n");

	/* start the timer with a block time of 0 ticks. This means as soon
	   as the schedule starts the timer will start running and will expire after
	   10 seconds */
	// xTimerStart( xTimer, 0 );

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	printf("************************** Should never reach here! #####################################\n");

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}

// ---------------------------------------------------------------------------------------------------------------------------------- //
// https://github.com/Xilinx/embeddedsw/blob/master/XilinxProcessorIPLib/drivers/uartlite/examples/xuartlite_intr_example.c#L101
int UartLiteIntrExample(XUartLite *UartLiteInstPtr, UINTPTR BaseAddress)
{
	int Status;
	int Index;
	XUartLite_Config *CfgPtr;
	(void)UartLiteInstPtr;


	CfgPtr = XUartLite_LookupConfig(BaseAddress);
	Status = XUartLite_Initialize(&UartLite, BaseAddress);

	if (Status != XST_SUCCESS) {
		xil_printf("UART Lite initialization failed!\n");
		return XST_FAILURE;
	}

	Status = XUartLite_SelfTest(&UartLite);
	if (Status != XST_SUCCESS) {
		xil_printf("UART Lite Self Test failed!\n");
		return XST_FAILURE;
	}


	char SendBuffer[] = "provaprova123prova\r\n";
	char ReceiveBuffer[64] = {0};
	int toBeSent = sizeof(SendBuffer);

	while(1) {
		xil_printf(SendBuffer);
		int sent = 0;
		int recv = 0;
		while(sent != toBeSent) {
			sent += XUartLite_Send(&UartLite, &SendBuffer[sent], toBeSent - sent);
		}
		while(recv <= 0 || ReceiveBuffer[recv-1] != '\n') {
			recv += XUartLite_Recv(&UartLite, &ReceiveBuffer[recv], 64);
		}
		toBeSent = recv;
		memcpy(SendBuffer, ReceiveBuffer, recv);
	}
}

