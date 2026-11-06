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

#include "wscp.h"

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

uint8_t majority_voting8(uint8_t *a, uint8_t *b, uint8_t *c)
{
	uint8_t res = ((*a & *b) | (*a & *c) | (*b & *c));
	*a = res;
	*b = res;
	*c = res;
	return res;
}

uint32_t majority_voting32(uint32_t *a, uint32_t *b, uint32_t *c)
{
	uint32_t res = ((*a & *b) | (*a & *c) | (*b & *c));
	*a = res;
	*b = res;
	*c = res;
	return res;
}


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
	wscp_init_data_section_backups();

	// Read the divider set for the clock frequency of the RPU and calculate the actual frequency
	CORE_RUNNING = get_cpu_id();

	// msleep(4000);

	// Init GPIOs and UART
	wscp_init();

	char SendBuffer[] = "provaprova123prova\r\n";
	char ReceiveBuffer[64] = {0};
	int toBeSent = sizeof(SendBuffer);

	while(1) {
		char menu[] = 	"0) Toggle WSCP_READY_PIN\n"
						"1) Toggle WSCP_FAULT_PIN\n"
						"2) Toggle WSCP_TX_SIG   \n"
						"3) Toggle WSCP_RUN_PIN  \n"
						"4) Send dummy message through SFERAv2 Serial\n"
						"R) Soft reset the board\n";

		wscp_port_uart_send(menu, sizeof(menu));

		wscp_port_uart_read(ReceiveBuffer, 2);
		xil_printf("Received: %s\n", ReceiveBuffer);

		if(strcmp(ReceiveBuffer, "0\n") == 0) {
			wscp_port_get_rdy() ? wscp_port_clear_rdy() : wscp_port_set_rdy();
		} else if (strcmp(ReceiveBuffer, "1\n") == 0) {
			wscp_port_get_fault()? wscp_port_clear_fault() : wscp_port_set_fault();
		} else if (strcmp(ReceiveBuffer, "2\n") == 0) {
			wscp_port_get_tx_sig() ? wscp_port_clear_tx_sig() : wscp_port_set_tx_sig();
		} else if (strcmp(ReceiveBuffer, "3\n") == 0) {
			wscp_port_get_run() ? wscp_port_clear_run() : wscp_port_set_run();
		} else if (strcmp(ReceiveBuffer, "4\n") == 0) {
			// wscp_send_ack();
		} else if (strcmp(ReceiveBuffer, "R\n") == 0) {
			wscp_port_reset();
		} else {
			wscp_port_uart_send("Option not recognized\n", sizeof("Option not recognized\n"));
		}
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
