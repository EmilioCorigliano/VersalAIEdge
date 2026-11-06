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
#include <semphr.h>
#include "xil_exception.h"
#include "xreg_cortexr5.h"
#include "xpseudo_asm.h"


#define DELAY_1_SECOND		1000UL
#define RX_STRING_SIZE		16

static uint32_t MEMORY_VALUE = 0x55555555;
/*-----------------------------------------------------------*/

uint32_t selfTest();
uint32_t initializeMemory();
uint32_t checkMemory();
uint32_t writeToMemory(uint32_t startAddress, uint32_t endAddress);
uint32_t compareMemory(uint32_t startAddress, uint32_t endAddress);
static int get_cpu_id(void);
/*-----------------------------------------------------------*/

static XSysMonPsv InstancePtr;
float frequency = 0;
char memoryLocation[20];
static int CORE_RUNNING = -1;

extern uint8_t __atcm0_start[];
extern uint8_t __atcm0_end[];
extern uint8_t __btcm0_start[];
extern uint8_t __btcm0_end[];
extern uint8_t __pmcram_start[];
extern uint8_t __pmcram_end[];
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
extern uint8_t __ocm_start[];
extern uint8_t __ocm_end[];


__attribute__((annotate("exclude")))
static void HardwareFault_Handler(void *CallbackRef)
{
    xil_printf("\r\n*** HARDWARE FAULT: %s in task: %s ***\r\n", (char *)CallbackRef, pcTaskGetName(NULL));
	
    // u32 dfsr, dfar;
    // mfcp(dfsr, XREG_CP15_DATA_FAULT_STATUS);
    // mfcp(dfar, XREG_CP15_DATA_FAULT_ADDRESS);
    // u32 ifsr, ifar;
    // mfcp(ifsr, XREG_CP15_INST_FAULT_STATUS);
    // mfcp(ifar, XREG_CP15_INST_FAULT_ADDRESS);

    // xil_printf("DFSR = 0x%08x   DFAR (faulting addr) = 0x%08x\r\n", dfsr, dfar);
    // xil_printf("IFSR = 0x%08x   IFAR (faulting addr) = 0x%08x\r\n", ifsr, ifar);

    while (1) { 
		portNOP();	
	}
}

__attribute__((annotate("exclude")))
void Fault_Handlers_Init(void)
{
    Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_RESET, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_RESET");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNDEFINED_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_UNDEFINED_INT");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_SWI_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_SWI_INT");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_PREFETCH_ABORT_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_PREFETCH_ABORT_INT");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_DATA_ABORT_INT");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_IRQ_INT");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT, (Xil_ExceptionHandler)HardwareFault_Handler, "XIL_EXCEPTION_ID_FIQ_INT");
}


int main( void )
{
	Fault_Handlers_Init();

	// Read the divider set for the clock frequency of the RPU and calculate the actual frequency
	CORE_RUNNING = get_cpu_id();

	#if defined(__aarch64__)
		printf("Cortex-A72\n");
		frequency = 1000. / (((*((volatile unsigned int *)(XPAR_PSV_CRF_0_BASEADDR + 0x10c))) >> 8) & 0x3ff);
	#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7A__)
		printf("%d Cortex-R5\n", CORE_RUNNING);
		frequency = 400. / (((*((volatile unsigned int *)(XPAR_PSV_CRL_0_BASEADDR + 0x10c))) >> 8) & 0x3ff);
	#else
		printf("unknown arch\n");
	#endif

	XSysMonPsv_Init(&InstancePtr, NULL);
	
	// If selfTest returns errors, repeat it
	sleep(3);

	float T, minT, maxT;
	int counter = 0;
	XTime tStart=0, tEnd=0, dT=0;

	initializeMemory();
	printf("%d Memory initialized\n", CORE_RUNNING);

	for( ;; )
	{

		xil_printf("\nWaiting seed\n");

		counter = checkMemory();

		XSysMonPsv_ReadTempProcessed(&InstancePtr, XSYSMONPSV_TEMP, &T);
		XSysMonPsv_ReadTempProcessed(&InstancePtr, XSYSMONPSV_TEMP_MIN, &minT);
		XSysMonPsv_ReadTempProcessed(&InstancePtr, XSYSMONPSV_TEMP_MAX, &maxT);

		// if(counter % 10 == 0) {
		// 	printf("%.3f [%.3f MHz] curr: %.3f\tmax:%.3f\n", (double)(dT) / COUNTS_PER_SECOND, frequency, T, maxT);
		// }
		XTime_GetTime(&tEnd);
		if(tEnd < tStart) {
			dT = UINT32_MAX - (tStart - tEnd);
		} else {
			dT = tEnd - tStart;
		}
		XTime_GetTime(&tStart);


		printf("%.3f [%.3f MHz] i:%d\tcrcInput:0x%08x crcBF:0x%08x crcOutputSw:0x%08x crcOutputFPGA:0x%08x crcOutputAI:0x%08x tGenAng:%.4fs tAngEstim:%.4fs tCoeff:%.4fs tMatMultAI:%.4fs tMatMultFPGA:%.4fs tTot:%.4fs angles:", 
			T,
			frequency,
			counter,
			0,
			0,
			0,
			0,
			0,
			(float)(dT) / COUNTS_PER_SECOND,
			(float)(dT) / COUNTS_PER_SECOND,
			(float)(dT) / COUNTS_PER_SECOND,
			(float)(dT) / COUNTS_PER_SECOND,
			(float)(dT) / COUNTS_PER_SECOND,
			(float)(dT) / COUNTS_PER_SECOND);

		for (int i = 0; i < 2; ++i)
		{
			printf("[(%+2.2f, %+2.2f)->(%+2d, %+2d)] ", 0., 0., 0, 0);
		}
		printf("\n");

		msleep(500);
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

uint32_t selfTest() {
	uint32_t errors = 0;
	uint32_t OLD_MEMORY_VALUE = MEMORY_VALUE;
	printf("%d Starting self-test...\n", CORE_RUNNING);
	printf("%d Pattern: 0x00000000\n", CORE_RUNNING);
	MEMORY_VALUE = 0x00000000;
	errors += initializeMemory();
	errors += checkMemory();
	printf("%d Pattern: 0xFFFFFFFF\n", CORE_RUNNING);
	MEMORY_VALUE = 0xFFFFFFFF;
	errors += initializeMemory();
	errors += checkMemory();
	printf("%d Pattern: 0xAAAAAAAA\n", CORE_RUNNING);
	MEMORY_VALUE = 0xAAAAAAAA;
	errors += initializeMemory();
	errors += checkMemory();
	printf("%d Pattern: 0x55555555\n", CORE_RUNNING);
	MEMORY_VALUE = 0x55555555;
	errors += initializeMemory();
	errors += checkMemory();
	MEMORY_VALUE = OLD_MEMORY_VALUE;
	printf("%d Self-test completed with %d errors.\n", CORE_RUNNING, errors);

	return errors;
}

uint32_t initializeMemory() {
	uint32_t errors = 0;
	// TCM
	// strcpy(memoryLocation, "__atcm0");
	// errors += writeToMemory((uintptr_t)__atcm0_start, (uintptr_t)__atcm0_end);
	// strcpy(memoryLocation, "__btcm0");
	// errors += writeToMemory((uintptr_t)__btcm0_start, (uintptr_t)__btcm0_end);
	strcpy(memoryLocation, "__atcm_global0");
	errors += writeToMemory((uintptr_t)__atcm_global0_start, (uintptr_t)__atcm_global0_end);
	strcpy(memoryLocation, "__atcm_global1");
	errors += writeToMemory((uintptr_t)__atcm_global1_start, (uintptr_t)__atcm_global1_end);
	// strcpy(memoryLocation, "__atcm_lockstep0");
	// errors += writeToMemory((uintptr_t)__atcm_lockstep0_start, (uintptr_t)__atcm_lockstep0_end);
	strcpy(memoryLocation, "__btcm_global0");
	errors += writeToMemory((uintptr_t)__btcm_global0_start, (uintptr_t)__btcm_global0_end);
	strcpy(memoryLocation, "__btcm_global1");
	errors += writeToMemory((uintptr_t)__btcm_global1_start, (uintptr_t)__btcm_global1_end);
	// strcpy(memoryLocation, "__btcm_lockstep0");
	// errors += writeToMemory((uintptr_t)__btcm_lockstep0_start, (uintptr_t)__btcm_lockstep0_end);
	// strcpy(memoryLocation, "__pmcram");
	// errors += writeToMemory((uintptr_t)__pmcram_start, (uintptr_t)__pmcram_end);

	// OCM
	strcpy(memoryLocation, "__ocm");
	errors += writeToMemory((uintptr_t)__ocm_start, (uintptr_t)__ocm_end);
	return errors;
}

uint32_t checkMemory() {
	uint32_t errors = 0;
	// TCM
	// strcpy(memoryLocation, "__atcm0");
	// errors += compareMemory((uintptr_t)__atcm0_start, (uintptr_t)__atcm0_end);
	// strcpy(memoryLocation, "__btcm0");
	// errors += compareMemory((uintptr_t)__btcm0_start, (uintptr_t)__btcm0_end);
	strcpy(memoryLocation, "__atcm_global0");
	errors += compareMemory((uintptr_t)__atcm_global0_start, (uintptr_t)__atcm_global0_end);
	strcpy(memoryLocation, "__atcm_global1");
	errors += compareMemory((uintptr_t)__atcm_global1_start, (uintptr_t)__atcm_global1_end);
	// strcpy(memoryLocation, "__atcm_lockstep0");
	// errors += compareMemory((uintptr_t)__atcm_lockstep0_start, (uintptr_t)__atcm_lockstep0_end);
	strcpy(memoryLocation, "__btcm_global0");
	errors += compareMemory((uintptr_t)__btcm_global0_start, (uintptr_t)__btcm_global0_end);
	strcpy(memoryLocation, "__btcm_global1");
	errors += compareMemory((uintptr_t)__btcm_global1_start, (uintptr_t)__btcm_global1_end);
	// strcpy(memoryLocation, "__btcm_lockstep0");
	// errors += compareMemory((uintptr_t)__btcm_lockstep0_start, (uintptr_t)__btcm_lockstep0_end);
	// strcpy(memoryLocation, "__pmcram");
	// errors += compareMemory((uintptr_t)__pmcram_start, (uintptr_t)__pmcram_end);


	// OCM
	strcpy(memoryLocation, "__ocm");
	errors += compareMemory((uintptr_t)__ocm_start, (uintptr_t)__ocm_end);

	return errors;
}

uint32_t writeToMemory(uint32_t startAddress, uint32_t endAddress) {
	uint32_t errors = 0;
	for(uint32_t mem = startAddress; mem < endAddress-4; mem += 4) {
		*((volatile uint32_t *)(mem)) = MEMORY_VALUE;
		if(*((volatile uint32_t *)(mem)) != MEMORY_VALUE) {
			printf("%d [%s] Rewrite of 0x%08x failed: expected 0x%08x, got 0x%08x\n", CORE_RUNNING, memoryLocation, mem, MEMORY_VALUE, *((volatile uint32_t *)(mem)));
			errors++;
		}
	}
	return errors;
}

uint32_t compareMemory(uint32_t startAddress, uint32_t endAddress) {
	uint32_t errors = 0;
	for(uint32_t mem = startAddress; mem < endAddress-4; mem += 4) {
		if(*((volatile uint32_t *)(mem)) != MEMORY_VALUE) {
			printf("%d [%s] Flip at 0x%08x: 0x%08x (^0x%08x)\n", CORE_RUNNING, memoryLocation, mem, *((volatile uint32_t *)(mem)), *((volatile uint32_t *)(mem)) ^ MEMORY_VALUE);
			*((volatile uint32_t *)(mem)) = MEMORY_VALUE;
			errors++;
		}
	}
	return errors;
}

static int get_cpu_id(void)
{
    uint32_t mpidr;
    __asm__ volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
    return mpidr & 0x3;
}