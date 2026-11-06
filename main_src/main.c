/*
    Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
    Copyright (c) 2012 - 2022 Xilinx, Inc. All Rights Reserved.
    Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
	SPDX-License-Identifier: MIT


    http://www.FreeRTOS.org
    http://aws.amazon.com/freertos


    1 tab == 4 spaces!
*/

#include "crc.h"
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
#include "Matrix.h"
#include "GenerateAngle.h"
#include "EstimateAngle.h"
#include "CalculateCoeff.h"
#include "xiltimer.h"
#include <semphr.h>
#include "xuartpsv.h"
#include "xil_exception.h"
#include "xreg_cortexr5.h"
#include "xpseudo_asm.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "wscp.h"

// #define lenStackInputGeneration 1000    //  834
// #define lenStackDoA_Weight 3500  //  3187
// #define lenStackUARTOutput 800  //  545
// #define lenStackPS2PL 450  //  318
// #define lenStackPL2PS 450  //  321
// #define lenStackTemp 350  //  194


#define lenStackReExecutor 4000    //  ?
#define lenStackInputGeneration 4000    //  834
#define lenStackDoA_Weight 8000  //  3187
#define lenStackUARTOutput 4000  //  545
#define lenStackPS2PL 4000  //  318
#define lenStackPL2PS 4000  //  321
#define lenStackTemp 4000  //  194


#define DELAY_1_SECOND		1000UL

/* Base/limit symbols from the .heap section */
extern char   _heap_start[];
extern char   _heap_end[];
extern char   __undef_stack[];
extern char   _stack_end[];
extern char   __bss_start__[];
extern char   __bss_end__[];
extern char   __data_start[];
extern char   __data_end[];
extern char   __data_backup_start0[];
extern char   __data_backup_end0[];
extern char   __data_backup_start1[];
extern char   __data_backup_end1[];
extern char   __data_backup_start2[];
extern char   __data_backup_end2[];

__attribute__((annotate("exclude"))) 
static void Uart_RxHandler(void *CallBackRef, u32 Event, u32 EventData);
static void Uart_Init(void);
static void Uart_deInit(void);
static uint32_t receiveSeed(void);

__attribute__((annotate("exclude"))) static void HardwareFault_Handler(void *CallbackRef);
__attribute__((annotate("exclude"))) static void DataCorruption_Handler();
__attribute__((annotate("exclude"))) static void SigMismatch_Handler();
/*-----------------------------------------------------------*/

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

EstimatedAngles majority_votingEstimatedAngles(EstimatedAngles *a, EstimatedAngles *b, EstimatedAngles *c)
{
	EstimatedAngles res = {0};

	for(int i = 0; i < N_SOURCES; ++i)
	{
		res.row[i] = majority_voting32(&a->row[i], &b->row[i], &c->row[i]);
		res.col[i] = majority_voting32(&a->col[i], &b->col[i], &c->col[i]);
	}

	return res;
}

void majority_votingMatrix(Matrix *a, Matrix *b, Matrix *c)
{
	majority_voting32(&a->rows, &b->rows, &c->rows);
	majority_voting32(&a->cols, &b->cols, &c->cols);
	for (int i = 0; i < a->rows * a->cols; ++i)
	{
		majority_voting32(&a->data[i], &b->data[i], &c->data[i]);
	}
}

#define portCPU_IRQ_DISABLE()										\
	__asm volatile ( "CPSID i" ::: "memory" );						\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );


#define portCPU_IRQ_ENABLE()										\
	__asm volatile ( "CPSIE i" ::: "memory" );						\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );

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

static void readTemperature( void *pvParameters );

static void InputGeneration( void *pvParameters );
static void DoA_Weight( void *pvParameters );
static void UARTOutput( void *pvParameters );
static void PS2PL( void *pvParameters );
static void PL2PS( void *pvParameters );

static void ReExecutor( void *pvParameters );


/*-----------------------------------------------------------*/

static const SUBTYPE SNR_dB 		= 20.0;
static const SUBTYPE f0 			= 299792458 / 0.003;

__attribute__((annotate("exclude"))) float T;
__attribute__((annotate("exclude"))) float minT;
__attribute__((annotate("exclude"))) float maxT;

__attribute__((annotate("exclude"))) XTime tStartAccelerators;
__attribute__((annotate("exclude"))) XTime tMatMultSw;
__attribute__((annotate("exclude"))) XTime tMatMultFPGA;
__attribute__((annotate("exclude"))) XTime tMatMultAI;
__attribute__((annotate("exclude"))) XTime tStart;
__attribute__((annotate("exclude"))) XTime tGenAng;
__attribute__((annotate("exclude"))) XTime tAngEstim;
static XUartPsv Uart_Ps;

static float frequency = 0;
static XSysMonPsv SysMonPtr;

static TaskHandle_t xReExecutor_Task;
static TaskHandle_t xInputGeneration_Task;
static TaskHandle_t xDoA_Weight_Task;
static TaskHandle_t xUARTOutput_Task;
static TaskHandle_t xPS2PL_Task;
static TaskHandle_t xPL2PS_Task;
static TaskHandle_t xTempTask;

/*----------------------- RECOVERY ---------------------*/
__attribute__((annotate("exclude"))) static uint8_t recovery0 = 0;
__attribute__((annotate("exclude"))) static uint8_t recovery1 = 0;
__attribute__((annotate("exclude"))) static uint8_t recovery2 = 0;


__attribute__((annotate("exclude"))) static uint32_t seed_backup0 = 0;
__attribute__((annotate("exclude"))) static uint32_t seed_backup1 = 0;
__attribute__((annotate("exclude"))) static uint32_t seed_backup2 = 0;

__attribute__((annotate("exclude"))) static EstimatedAngles estimatedAngles0 = {0};
__attribute__((annotate("exclude"))) static EstimatedAngles estimatedAngles1 = {0};
__attribute__((annotate("exclude"))) static EstimatedAngles estimatedAngles2 = {0};

__attribute__((annotate("exclude"))) static uint8_t CheckPointReached0 = 0;
__attribute__((annotate("exclude"))) static uint8_t CheckPointReached1 = 0;
__attribute__((annotate("exclude"))) static uint8_t CheckPointReached2 = 0;


// These shall become local if start pipelining (if we have one global matrix, 
// it will be overritten by the next task before the previous one has finished using it)
Matrix BF;
Matrix outputMatrix;

/*-----------------------------------------------------------*/

/**
 * Queue counts:
 * 2x Input signal matrix: N_A * N_SNAPSHOTS * sizeof(TYPE)
 * 2x Coefficients: N_SOURCES * N_A * sizeof(TYPE)
 * 1x DoA: N_SOURCES * sizeof(PairInt)
 * 1x outputs: N_SOURCES * N_SNAPSHOTS * sizeof(TYPE)
 */

#define QUEUE_LENGTH 1
#define RX_STRING_SIZE 16

#define MATRIX_SIZE (sizeof(Matrix))

// #define TIMEOUT_QUEUES pdMS_TO_TICKS(1000) // 1 second timeout for queue operations
#define TIMEOUT_QUEUES portMAX_DELAY // Blocking queue operations

// #define SIGNAL_MATRIX_SIZE (N_A * N_SNAPSHOTS * sizeof(TYPE))
// #define COEFFICIENTS_SIZE (N_SOURCES * N_A * sizeof(TYPE))
// #define DOA_SIZE (N_SOURCES * sizeof(PairInt))
// #define OUTPUTS_SIZE (N_SOURCES * N_SNAPSHOTS * sizeof(TYPE))

static uint8_t ucQSignalMatrixPS2PL[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQSignalMatrixDoAEstimator[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQSignalMatrixUARTOutput[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQCoefficientsPS2PL[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQCoefficientsUARTOutput[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQDoAUARTOutput[QUEUE_LENGTH * sizeof(EstimatedAngles)];
static uint8_t ucQOutputsUARTOutput[QUEUE_LENGTH * MATRIX_SIZE];
static uint8_t ucQCRCInputUARTOutput[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t ucQCRCBFUARTOutput[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t ucQCRCOutputSwUARTOutput[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t ucQCRCOutputFPGAUARTOutput[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t ucQCRCOutputAIUARTOutput[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t ucQRealAnglesUARTOutput[QUEUE_LENGTH * sizeof(RealAngles)];
static uint8_t xRxQueueBuffer[RX_STRING_SIZE * sizeof(char)];
static uint8_t xTaskReExecutorBuffer[QUEUE_LENGTH * sizeof(TaskHandle_t *)];

static StaticQueue_t qSignalMatrixPS2PL;
static StaticQueue_t qSignalMatrixDoAEstimator;
static StaticQueue_t qSignalMatrixUARTOutput;
static StaticQueue_t qCoefficientsPS2PL;
static StaticQueue_t qCoefficientsUARTOutput;
static StaticQueue_t qDoAUARTOutput;
static StaticQueue_t qOutputsUARTOutput;
static StaticQueue_t qCRCInputUARTOutput;
static StaticQueue_t qCRCBFUARTOutput;
static StaticQueue_t qCRCOutputSwUARTOutput;
static StaticQueue_t qCRCOutputFPGAUARTOutput;
static StaticQueue_t qCRCOutputAIUARTOutput;
static StaticQueue_t qRealAnglesUARTOutput;
static StaticQueue_t qRxQueue;
static StaticQueue_t qTaskReExecutor;

static QueueHandle_t xQSignalMatrixPS2PL;
static QueueHandle_t xQSignalMatrixDoAEstimator;
static QueueHandle_t xQSignalMatrixUARTOutput;
static QueueHandle_t xQCoefficientsPS2PL;
static QueueHandle_t xQCoefficientsUARTOutput;
static QueueHandle_t xQDoAUARTOutput;
static QueueHandle_t xQOutputsUARTOutput;
static QueueHandle_t xQCRCInputUARTOutput;
static QueueHandle_t xQCRCBFUARTOutput;
static QueueHandle_t xQCRCOutputSwUARTOutput;
static QueueHandle_t xQCRCOutputFPGAUARTOutput;
static QueueHandle_t xQCRCOutputAIUARTOutput;
static QueueHandle_t xQRealAnglesUARTOutput;
static QueueHandle_t xRxQueue;
static QueueHandle_t xTaskReExecutor;

static StaticSemaphore_t xIterationDoneBuffer;
static SemaphoreHandle_t xIterationDone;

// Temporary
static StaticSemaphore_t xPS_PLCommunicationBuffer;
static SemaphoreHandle_t xPS_PLCommunication;

/*-----------------------------------------------------------*/

__attribute__((annotate("exclude")))
StackType_t xReExecutorStack[lenStackReExecutor];

__attribute__((annotate("exclude")))
StackType_t xInputGenerationStack[lenStackInputGeneration];

__attribute__((annotate("exclude")))
StackType_t xDoA_WeightStack[lenStackDoA_Weight];

__attribute__((annotate("exclude")))
StackType_t xUARTOutputStack[lenStackUARTOutput];

__attribute__((annotate("exclude")))
StackType_t xPS2PLStack[lenStackPS2PL];

__attribute__((annotate("exclude")))
StackType_t xPL2PSStack[lenStackPL2PS];

__attribute__((annotate("exclude")))
StackType_t xTempStack[lenStackTemp];

StaticTask_t xReExecutorBuffer, xInputGenerationBuffer, xDoA_WeightBuffer, xUARTOutputBuffer, xPS2PLBuffer, xPL2PSBuffer, xTempBuffer;

__attribute__((section(".persistent")))
static uint32_t data_backup_init_0 = 0;

__attribute__((section(".persistent")))
static uint32_t data_backup_init_1 = 0;

__attribute__((section(".persistent")))
static uint32_t data_backup_init_2 = 0;

int main( void )
{
	wscp_init_data_section_backups();


	// msleep(4000);

	// Init GPIOs and UART
	wscp_init();


	uintptr_t heap_start = (uintptr_t)_heap_start;
    uintptr_t heap_end   = (uintptr_t)_heap_end;
    size_t    heap_size  = heap_end - heap_start;

	uintptr_t stack_start = (uintptr_t)__undef_stack;
    uintptr_t stack_end   = (uintptr_t)_stack_end;
    size_t    stack_size  = -(stack_end - stack_start);

	uintptr_t bss_start = (uintptr_t)__bss_start__;
    uintptr_t bss_end   = (uintptr_t)__bss_end__;
    size_t    bss_size  = bss_end - bss_start;

	uintptr_t data_start = (uintptr_t)__data_start;
    uintptr_t data_end   = (uintptr_t)__data_end;
    size_t    data_size  = data_end - data_start;

	uintptr_t data_backup_start0 = (uintptr_t)__data_backup_start0;
	uintptr_t data_backup_start1 = (uintptr_t)__data_backup_start1;
	uintptr_t data_backup_start2 = (uintptr_t)__data_backup_start2;
    uintptr_t data_backup_end0   = (uintptr_t)__data_backup_end0;
    uintptr_t data_backup_end1   = (uintptr_t)__data_backup_end1;
    uintptr_t data_backup_end2   = (uintptr_t)__data_backup_end2;
    size_t    data_backup_size0  = data_backup_end0 - data_backup_start0;
    size_t    data_backup_size1  = data_backup_end1 - data_backup_start1;
    size_t    data_backup_size2  = data_backup_end2 - data_backup_start2;

	if(data_size != data_backup_size0 || data_size != data_backup_size1 || data_size != data_backup_size2) {
		xil_printf("Error: data size and backup size do not match!\n");
		while(1);
	}

	const HeapRegion_t xHeapRegions[] =  
	{  
		{ ( uint8_t * ) heap_start, heap_size },  
		{ NULL, 0 } /* Terminates the array. */  
	};

	/* Pass the array into vPortDefineHeapRegions(). */  
	vPortDefineHeapRegions( xHeapRegions );  

	msleep(400);

	xil_printf("stack : 0x%8x - 0x%8x (0x%x)\n", stack_start, stack_end, stack_size);
	xil_printf("bss   : 0x%8x - 0x%8x (0x%x)\n", bss_start, bss_end, bss_size);
	xil_printf("heap  : 0x%8x - 0x%8x (0x%x)\n", heap_start, heap_end, heap_size);
	xil_printf("data  : 0x%8x - 0x%8x (0x%x) crc32: 0x%08x\n", data_start, data_end, data_size, crc32((uint8_t *)data_start, data_size));
	xil_printf("data_0: 0x%8x - 0x%8x (0x%x) crc32: 0x%08x\n", data_backup_start0, data_backup_end0, data_backup_size0, crc32((uint8_t *)data_backup_start0, data_size));
	xil_printf("data_1: 0x%8x - 0x%8x (0x%x) crc32: 0x%08x\n", data_backup_start1, data_backup_end1, data_backup_size1, crc32((uint8_t *)data_backup_start1, data_size));
	xil_printf("data_2: 0x%8x - 0x%8x (0x%x) crc32: 0x%08x\n", data_backup_start2, data_backup_end2, data_backup_size2, crc32((uint8_t *)data_backup_start2, data_size));
	
	// Read the divider set for the clock frequency of the RPU and calculate the actual frequency
	#if defined(__aarch64__)
		xil_printf("Cortex-A72\n");
		frequency = 1000. / (((*((volatile unsigned int *)(XPAR_PSV_CRF_0_BASEADDR + 0x10c))) >> 8) & 0x3ff);
	#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7A__)
		xil_printf("Cortex-R5\n");
		frequency = 400. / (((*((volatile unsigned int *)(XPAR_PSV_CRL_0_BASEADDR + 0x10c))) >> 8) & 0x3ff);
	#else
		xil_printf("unknown arch\n");
	#endif

	XSysMonPsv_Init(&SysMonPtr, NULL);

	xil_printf( "Using static memory for tasks, queue and timer creations. \r\n" );

	// Semaphore creation to signal when another angle can be generated
	xIterationDone = xSemaphoreCreateBinaryStatic(&xIterationDoneBuffer);
	xPS_PLCommunication = xSemaphoreCreateBinaryStatic(&xPS_PLCommunicationBuffer);

	/* Create the queue statically */
	xil_printf( "Creating the queues statically. \r\n" );
	xTaskReExecutor = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(TaskHandle_t),
		xTaskReExecutorBuffer,
		&qTaskReExecutor
	);
	xQSignalMatrixPS2PL = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQSignalMatrixPS2PL,
		&qSignalMatrixPS2PL
	);
	xQSignalMatrixDoAEstimator = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQSignalMatrixDoAEstimator,
		&qSignalMatrixDoAEstimator
	);
	xQSignalMatrixUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQSignalMatrixUARTOutput,
		&qSignalMatrixUARTOutput
	);
	xQCoefficientsPS2PL = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQCoefficientsPS2PL,
		&qCoefficientsPS2PL
	);
	xQCoefficientsUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQCoefficientsUARTOutput,
		&qCoefficientsUARTOutput
	);
	xQDoAUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(EstimatedAngles),
		ucQDoAUARTOutput,
		&qDoAUARTOutput
	);
	xQOutputsUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		MATRIX_SIZE,
		ucQOutputsUARTOutput,
		&qOutputsUARTOutput
	);

	// CRCs
	xQCRCInputUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(uint32_t),
		ucQCRCInputUARTOutput,
		&qCRCInputUARTOutput
	);

	xQCRCBFUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(uint32_t),
		ucQCRCBFUARTOutput,
		&qCRCBFUARTOutput
	);

	xQCRCOutputSwUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(uint32_t),
		ucQCRCOutputSwUARTOutput,
		&qCRCOutputSwUARTOutput
	);

	xQCRCOutputFPGAUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(uint32_t),
		ucQCRCOutputFPGAUARTOutput,
		&qCRCOutputFPGAUARTOutput
	);

	xQCRCOutputAIUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(uint32_t),
		ucQCRCOutputAIUARTOutput,
		&qCRCOutputAIUARTOutput
	);

	xQRealAnglesUARTOutput = xQueueCreateStatic(
		QUEUE_LENGTH,
		sizeof(RealAngles),
		ucQRealAnglesUARTOutput,
		&qRealAnglesUARTOutput
	);

	xRxQueue = xQueueCreateStatic(
		RX_STRING_SIZE,
		sizeof(char),
		xRxQueueBuffer,
		&qRxQueue
	);


	xil_printf( "Queues created. \r\n" );

	// Create the tasks statically
	xReExecutor_Task = xTaskCreateStatic(	ReExecutor,
					( const char * ) "ReExecutor",
					lenStackReExecutor,
					NULL,
					tskIDLE_PRIORITY+3,
					xReExecutorStack,
					&xReExecutorBuffer );
	xInputGeneration_Task = xTaskCreateStatic(	InputGeneration,
					( const char * ) "Input",
					lenStackInputGeneration,
					NULL,
					tskIDLE_PRIORITY+1,
					xInputGenerationStack,
					&xInputGenerationBuffer );
	xDoA_Weight_Task = xTaskCreateStatic(	DoA_Weight,
					( const char * ) "DoA",
					lenStackDoA_Weight,
					NULL,
					tskIDLE_PRIORITY+1,
					xDoA_WeightStack,
					&xDoA_WeightBuffer );
	xUARTOutput_Task = xTaskCreateStatic(	UARTOutput,
					( const char * ) "UARTOut",
					lenStackUARTOutput,
					NULL,
					tskIDLE_PRIORITY+1,
					xUARTOutputStack,
					&xUARTOutputBuffer );
	xPS2PL_Task = xTaskCreateStatic(	PS2PL,
					( const char * ) "PS2PL",
					lenStackPS2PL,
					NULL,
					tskIDLE_PRIORITY+2,
					xPS2PLStack,
					&xPS2PLBuffer );
	xPL2PS_Task = xTaskCreateStatic(	PL2PS,
					( const char * ) "PL2PS",
					lenStackPL2PS,
					NULL,
					tskIDLE_PRIORITY+2,
					xPL2PSStack,
					&xPL2PSBuffer );

	xTempTask = xTaskCreateStatic( readTemperature,
					( const char * ) "Temper",
					lenStackTemp,
					NULL,
					tskIDLE_PRIORITY+1,
					xTempStack,
					&xTempBuffer );
				 
	xil_printf( "Tasks created. \r\n" );

	/* Create the matrices */
	BF = createMatrix(N_SOURCES, N_A);
	outputMatrix = createMatrix(N_SOURCES, N_SNAPSHOTS);
	xil_printf("Starting the scheduler.\n");

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	free_matrix(BF);
	free_matrix(outputMatrix);

	xil_printf("************************** Should never reach here! #####################################\n");

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; )
	;
}

/*-----------------------------------------------------------*/

// ASPIS: Exclude since the interrupt calls the Uart_RxHandler_dup with only the original arguments
__attribute__((annotate("exclude"))) 
static void Uart_RxHandler(void *CallBackRef, u32 Event, u32 EventData) {
    if (Event == XUARTPSV_EVENT_RECV_DATA || Event == XUARTPSV_EVENT_RECV_TOUT) {
        u32 rx_byte = 0;
		char c = 0;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Assumes RX FIFO trigger level = 1, or you drain EventData bytes
        while (XUartPsv_IsReceiveData(Uart_Ps.Config.BaseAddress)) {
            rx_byte = XUartPsv_ReadReg(Uart_Ps.Config.BaseAddress, XUARTPSV_UARTDR_OFFSET);
			c = (char)rx_byte;
            xQueueSendFromISR(xRxQueue, &c, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void Uart_Init(void) {
	fflush(stdout);
	portCPU_IRQ_DISABLE();
    XUartPsv_Config *Config = XUartPsv_LookupConfig(0xff010000);
    XUartPsv_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
	XUartPsv_SetBaudRate(&Uart_Ps, Config->BaudRate);

    XUartPsv_SetHandler(&Uart_Ps, Uart_RxHandler, &Uart_Ps);
    XUartPsv_SetInterruptMask(&Uart_Ps, XUARTPSV_UARTIMSC_RXIM | XUARTPSV_UARTIMSC_RTIM);

    xPortInstallInterruptHandler(XPAR_XUARTPSV_0_INTERRUPTS,
                                  (XInterruptHandler)XUartPsv_InterruptHandler,
                                  &Uart_Ps);
    vPortEnableInterrupt(XPAR_XUARTPSV_0_INTERRUPTS);
	portCPU_IRQ_ENABLE();
}


void Uart_deInit(void) {
  	fflush(stdout);
	portCPU_IRQ_DISABLE();
	vPortDisableInterrupt(XPAR_XUARTPSV_0_INTERRUPTS);

	XUartPsv_CleanupRx(&Uart_Ps);
	XUartPsv_CleanupTx(&Uart_Ps);

    XUartPsv_SetInterruptMask(&Uart_Ps, 0);

	if (xRxQueue != NULL) {
		char discard;
		while (xQueueReceiveFromISR(xRxQueue, &discard, 0) == pdTRUE) {
			/* drop */;
		}
	}
	portCPU_IRQ_ENABLE();
}


static uint32_t receiveSeed( void ) {
	u8 seedInput[16] = {0};
	int idx = 0;
	u8 c = 0;

	xil_printf("\nWaiting seed\n");

	uint16_t size_input = 0;
	uint32_t *input = wscp_port_get_input(&size_input);

	xil_printf("\n seed received: %u\n", *input);

	return *input;

	// while (idx < (int)sizeof(seedInput) - 1) {
	// 	xQueueReceive(xRxQueue, &c, portMAX_DELAY);
	// 	if (c < '0' || c > '9') {
	// 		if (idx == 0) 
	// 			continue;
	// 		else 
	// 			break;
	// 	}
	// 	seedInput[idx++] = c;
	// }
	// seedInput[idx] = '\0';

	// return atoi((char *)seedInput);
}

static void InputGeneration( void *pvParameters )
{
	(void) pvParameters;
	Uart_Init();
	uint32_t seed = 0;

	sleep(10);

	while(1) {
		xil_printf("InputGeneration: Waiting for seed\n");
		wscp_port_uart_send("Waiting for seed\n", 18);
		sleep(1);
	}

	while(1)
	{
		if(majority_voting8(&recovery0, &recovery1, &recovery2) == 1) {
			xil_printf("InputGeneration: Recovery triggered\n");

			seed = majority_voting32(&seed_backup0, &seed_backup1, &seed_backup2);
		} else {
			// Waiting for stdin for seed setup
			seed = receiveSeed();
			seed_backup0 = seed;
			seed_backup1 = seed;
			seed_backup2 = seed;

			if(seed == 0) {
				wscp_port_reset();
			} else if (majority_voting32(&seed_backup0, &seed_backup1, &seed_backup2) == 1) {
				DataCorruption_Handler();
			}
		}
		 

		// Seed the random number generator
		srand(seed);

		XTime_GetTime(&tStart);

		Matrix input;
		RealAngles RealAngles = {0};
		for(int i = 0; i < N_SOURCES; ++i) {
			SUBTYPE theta_true 	= -40.0 + ((SUBTYPE)rand() / (SUBTYPE)RAND_MAX) * 80.0;
			SUBTYPE phi_true 	= -40.0 + ((SUBTYPE)rand() / (SUBTYPE)RAND_MAX) * 80.0;
			RealAngles.row[i] = theta_true;
			RealAngles.col[i] = phi_true;
			
			Matrix inputSource = GenerateAngle(theta_true, phi_true, SNR_dB, f0);
			if(i == 0) {
				input = inputSource;
			} else {
				// sum the matrixes
				for(int rows = 0; rows < inputSource.rows; ++rows) {
					for(int cols = 0; cols < inputSource.cols; ++cols) {
						m_set(input, rows, cols, m_at(input, rows, cols) + m_at(inputSource, rows, cols));
					}
				}

				free_matrix(inputSource);
			}
		}

		xQueueSend(xQRealAnglesUARTOutput, &RealAngles, TIMEOUT_QUEUES);

		// Send the input matrix to the DoA estimator task and PS2PL task
		xQueueSend(xQSignalMatrixPS2PL, &input, TIMEOUT_QUEUES);
		xQueueSend(xQSignalMatrixDoAEstimator, &input, TIMEOUT_QUEUES);
		xQueueSend(xQSignalMatrixUARTOutput, &input, TIMEOUT_QUEUES);

		XTime_GetTime(&tGenAng);

		unsigned int crcInput = crc32((const unsigned char *)input.data, input.cols * input.rows * sizeof(TYPE));
		xQueueSend(xQCRCInputUARTOutput, &crcInput, TIMEOUT_QUEUES);
		xSemaphoreTake(xIterationDone, TIMEOUT_QUEUES);
	}
}


static void DoA_Weight( void *pvParameters )
{
	(void) pvParameters;
	Matrix input;

	while(1)
	{
		EstimatedAngles estimatedAngles = {0};

		// Receive the input matrix from the InputGeneration task
		xQueueReceive(xQSignalMatrixDoAEstimator, &input, TIMEOUT_QUEUES);

		if(majority_voting8(&recovery0, &recovery1, &recovery2) == 1 && 
			majority_voting8(&CheckPointReached0, &CheckPointReached1, &CheckPointReached2) == 1) {
			xil_printf("DoA_Weight: Recovery triggered with checkpoint\n");
			estimatedAngles = majority_votingEstimatedAngles(&estimatedAngles0, &estimatedAngles1, &estimatedAngles2);
		} else {
 			if (majority_voting8(&recovery0, &recovery1, &recovery2) == 0 && majority_voting32(&seed_backup0, &seed_backup1, &seed_backup2) == 2) {
				DataCorruption_Handler();
			}

			estimatedAngles = AngleEstimate(input, f0); // Check for multiple sources

			estimatedAngles0 = estimatedAngles;
			estimatedAngles1 = estimatedAngles;
			estimatedAngles2 = estimatedAngles;

			CheckPointReached0 = 1;
			CheckPointReached1 = 1;
			CheckPointReached2 = 1;
			if (majority_voting32(&seed_backup0, &seed_backup1, &seed_backup2) == 3) {
				DataCorruption_Handler();
			}
		}

		XTime_GetTime(&tAngEstim);

		xQueueSend(xQDoAUARTOutput, &estimatedAngles, TIMEOUT_QUEUES);

		// Checkpoint

		SUBTYPE theta[N_SOURCES];
		SUBTYPE phi[N_SOURCES];

		// convert DoA in rad
		for (int i = 0; i < N_SOURCES; i++)
		{
			theta[i] = (SUBTYPE)estimatedAngles.row[i] * M_PI / 180.0;
			phi[i] = (SUBTYPE)estimatedAngles.col[i] * M_PI / 180.0;
		}

		ZFBF_weights_UPA(&BF, theta, phi);

		// Send the coefficients to the UARTOutput task
		xQueueSend(xQCoefficientsPS2PL, &BF, TIMEOUT_QUEUES);
		xQueueSend(xQCoefficientsUARTOutput, &BF, TIMEOUT_QUEUES);

		uint32_t crcBF = crc32((const unsigned char *)BF.data, N_SOURCES * N_A * sizeof(TYPE));
		xQueueSend(xQCRCBFUARTOutput, &crcBF, TIMEOUT_QUEUES);

		// outputMatrix = BF * input
		for (int i = 0; i < N_SOURCES; i++)
		{
			for (int j = 0; j < N_SNAPSHOTS; j++)
			{
				m_set(outputMatrix, i, j, 0);
				TYPE acc = 0;
				for (int k = 0; k < N_A; k++)
				{	
					acc += m_at(input, k, j) * m_at(BF, i, k);
				}
				m_set(outputMatrix, i, j, acc);
			}
		}

		uint32_t crcOutput = crc32((const unsigned char *)outputMatrix.data, N_SOURCES * N_SNAPSHOTS * sizeof(TYPE));
		xQueueSend(xQCRCOutputSwUARTOutput, &crcOutput, TIMEOUT_QUEUES);

		XTime_GetTime(&tMatMultSw);
	}
}

static void matchAngles(RealAngles *real, EstimatedAngles *est, int matchedIdx[N_SOURCES])
{
    int used[N_SOURCES] = {0};

    for (int i = 0; i < N_SOURCES; ++i)
    {
        SUBTYPE bestDist = INFINITY;
        int bestJ = -1;
        for (int j = 0; j < N_SOURCES; ++j)
        {
            if (used[j]) continue;
            SUBTYPE dRow = real->row[i] - (SUBTYPE)est->row[j];
            SUBTYPE dCol = real->col[i] - (SUBTYPE)est->col[j];
            SUBTYPE dist = dRow * dRow + dCol * dCol; /* squared distance, no need for sqrt */
            if (dist < bestDist)
            {
                bestDist = dist;
                bestJ = j;
            }
        }
        matchedIdx[i] = bestJ;
        used[bestJ] = 1;
    }
}

static void UARTOutput( void *pvParameters )
{
	(void) pvParameters;
	xil_printf("Starting UARTOutput task.\n");

	uint32_t counter = 0;

	RealAngles RealAngles;
	uint32_t CRCInput;
	EstimatedAngles DoA;
	uint32_t CRCBF;
	uint32_t CRCOutputSoftware;
	uint32_t CRCOutputFPGA;
	uint32_t CRCOutputAI;
	Matrix input;

	while(1)
	{
		// Receive the input matrix from the InputGeneration task
		xQueueReceive(xQSignalMatrixUARTOutput, &input, TIMEOUT_QUEUES);
		xQueueReceive(xQRealAnglesUARTOutput, &RealAngles, TIMEOUT_QUEUES);
		xQueueReceive(xQCRCInputUARTOutput, &CRCInput, TIMEOUT_QUEUES);
		xQueueReceive(xQDoAUARTOutput, &DoA, TIMEOUT_QUEUES);
		xQueueReceive(xQCRCBFUARTOutput, &CRCBF, TIMEOUT_QUEUES);
		xQueueReceive(xQCoefficientsUARTOutput, &BF, TIMEOUT_QUEUES);
		xQueueReceive(xQCRCOutputSwUARTOutput, &CRCOutputSoftware, TIMEOUT_QUEUES);
		xQueueReceive(xQCRCOutputFPGAUARTOutput, &CRCOutputFPGA, TIMEOUT_QUEUES);
		xQueueReceive(xQCRCOutputAIUARTOutput, &CRCOutputAI, TIMEOUT_QUEUES);

		int matchedIdx[N_SOURCES];
		matchAngles(&RealAngles, &DoA, matchedIdx);
		for (int i = 0; i < N_SOURCES; ++i)
		{
			int j = matchedIdx[i];
			if (fabs(RealAngles.row[i] - DoA.row[j]) > 5 || fabs(RealAngles.col[i] - DoA.col[j]) > 5)
			{
				printf(" Error %d: angles mismatch (%+d, %+d) != (%+.2f, %+.2f)\n",
					i, DoA.row[j], DoA.col[j], RealAngles.row[i], RealAngles.col[i]);
			}
		}

		printf("%.3f [%.3f MHz] i:%d\tcrcInput:0x%08x crcBF:0x%08x crcOutputSw:0x%08x crcOutputFPGA:0x%08x crcOutputAI:0x%08x tGenAng:%.4fs tAngEstim:%.4fs tCoeff:%.4fs tMatMultAI:%.4fs tMatMultFPGA:%.4fs tTot:%.4fs angles:", 
			T,
			frequency,
			++counter,
			CRCInput,
			CRCBF,
			CRCOutputSoftware,
			CRCOutputFPGA,
			CRCOutputAI,
			(SUBTYPE)(tGenAng - tStart) / COUNTS_PER_SECOND,
			(SUBTYPE)(tAngEstim - tGenAng) / COUNTS_PER_SECOND,
			(SUBTYPE)(tMatMultSw - tAngEstim) / COUNTS_PER_SECOND,
			(SUBTYPE)(tMatMultAI - tStartAccelerators) / COUNTS_PER_SECOND,
			(SUBTYPE)(tMatMultFPGA - tStartAccelerators) / COUNTS_PER_SECOND,
			(SUBTYPE)(tMatMultSw - tStart) / COUNTS_PER_SECOND);

		for (int i = 0; i < N_SOURCES; ++i)
		{
			int j = matchedIdx[i];
			printf("[(%+2.2f, %+2.2f)->(%+2d, %+2d)] ", RealAngles.row[i], RealAngles.col[i], DoA.row[j], DoA.col[j]);
		}
		printf("\n");

		// dup N_SOURCES 1 | 2 (563)
		// xil_printf("xInputGeneration_Task: %d\n", (lenStackInputGeneration - uxTaskGetStackHighWaterMark2(xInputGeneration_Task))); // xInputGeneration_Task: 744 | 812
		// xil_printf("xDoA_Weight_Task: %d\n", (lenStackDoA_Weight - uxTaskGetStackHighWaterMark2(xDoA_Weight_Task))); // xDoA_Weight_Task: 3021 | 3083
		// xil_printf("xUARTOutput_Task: %d\n", (lenStackUARTOutput - uxTaskGetStackHighWaterMark2(xUARTOutput_Task))); // xUARTOutput_Task: 465 | 603
		// xil_printf("xPS2PL_Task: %d\n", (lenStackPS2PL - uxTaskGetStackHighWaterMark2(xPS2PL_Task))); // xPS2PL_Task: 308 | 402
		// xil_printf("xPL2PS_Task: %d\n", (lenStackPL2PS - uxTaskGetStackHighWaterMark2(xPL2PS_Task))); // xPL2PS_Task: 158 | 158
		// xil_printf("xTempTask: %d\n", (lenStackTemp - uxTaskGetStackHighWaterMark2(xTempTask))); // xTempTask: 186 | 186

		free_matrix(input);

		recovery0 = 0;
		recovery1 = 0;
		recovery2 = 0;

		CheckPointReached0 = 0;
		CheckPointReached1 = 0;
		CheckPointReached2 = 0;
		xSemaphoreGive(xIterationDone);
	}
}


static void PS2PL( void *pvParameters )
{
	(void) pvParameters;
	Matrix BF;
	Matrix Coefficients;

	while(1) {
		xQueueReceive(xQSignalMatrixPS2PL, &BF, TIMEOUT_QUEUES);
		xQueueReceive(xQCoefficientsPS2PL, &Coefficients, TIMEOUT_QUEUES);

		// setup for FPGA and AI engines processing
		XTime_GetTime(&tStartAccelerators);
		xSemaphoreGive(xPS_PLCommunication);
	}
}


static void PL2PS( void *pvParameters )
{
	(void) pvParameters;

	while(1) {
		uint32_t crcOutputFPGA = 0;
		uint32_t crcOutputAI = 0;

		// Wait for FPGA and AI engines processing ending
		xSemaphoreTake(xPS_PLCommunication, TIMEOUT_QUEUES);

		XTime_GetTime(&tMatMultAI);
		XTime_GetTime(&tMatMultFPGA);

		xQueueSend(xQCRCOutputFPGAUARTOutput, &crcOutputFPGA, TIMEOUT_QUEUES);
		xQueueSend(xQCRCOutputAIUARTOutput, &crcOutputAI, TIMEOUT_QUEUES);
	}
}


/*-----------------------------------------------------------*/
static void readTemperature( void *pvParameters ) {
	(void) pvParameters;
	const TickType_t delay = pdMS_TO_TICKS( DELAY_1_SECOND / 5);

	for( ;; )
	{
		XSysMonPsv_ReadTempProcessed(&SysMonPtr, XSYSMONPSV_TEMP, &T);
		XSysMonPsv_ReadTempProcessed(&SysMonPtr, XSYSMONPSV_TEMP_MIN, &minT);
		XSysMonPsv_ReadTempProcessed(&SysMonPtr, XSYSMONPSV_TEMP_MAX, &maxT);
		/* Delay for 1 second. */
		vTaskDelay( delay );
	}
}


/*-----------------------------------------------------------*/

static void ReExecutor( void *pvParameters ) {
	// remember mutexes
	// Understand what happens if mutex blocked
	// Detect whether the fault is applicaiton or OS related
	(void) pvParameters;
	TaskHandle_t task = NULL;

	for(;;) {
		xQueueReceive(xTaskReExecutor, &task, TIMEOUT_QUEUES);

		portCPU_IRQ_DISABLE();
		if(task == xTempTask) {
			// If TempTask to be recovered, just restart the task			
			vTaskDelete(xTempTask);
			xTempTask = xTaskCreateStatic( readTemperature,
					( const char * ) "Temper",
					lenStackTemp,
					NULL,
					tskIDLE_PRIORITY+1,
					xTempStack,
					&xTempBuffer );
		} else if(task == xReExecutor_Task) {
			// If the re-executor is to be recovered, reboot the system
			wscp_port_reset();
		} else if(task == xInputGeneration_Task || task == xDoA_Weight_Task || task == xPS2PL_Task || task == xPL2PS_Task || task == xUARTOutput_Task) {
			if (majority_voting8(&recovery0, &recovery1, &recovery2) == 1) {
				if(majority_voting8(&CheckPointReached0, &CheckPointReached1, &CheckPointReached2) == 1) {
					// If tried to recover from checkpoint and failed, retry without checkpoint
					CheckPointReached0 = 0;
					CheckPointReached1 = 0;
					CheckPointReached2 = 0;
				} else {
					// If tried to recover without checkpoint and failed, reboot the system
					wscp_port_reset();
				}
			}

			xQueueReset(xQSignalMatrixPS2PL);
			xQueueReset(xQSignalMatrixDoAEstimator);
			xQueueReset(xQSignalMatrixUARTOutput);
			xQueueReset(xQCoefficientsPS2PL);
			xQueueReset(xQCoefficientsUARTOutput);
			xQueueReset(xQDoAUARTOutput);
			xQueueReset(xQOutputsUARTOutput);
			xQueueReset(xQCRCInputUARTOutput);
			xQueueReset(xQCRCBFUARTOutput);
			xQueueReset(xQCRCOutputSwUARTOutput);
			xQueueReset(xQCRCOutputFPGAUARTOutput);
			xQueueReset(xQCRCOutputAIUARTOutput);
			xQueueReset(xQRealAnglesUARTOutput);
			xQueueReset(xRxQueue);
			xQueueReset(xTaskReExecutor);

			// If it is another application task, restart all the tasks after the checkpoint
			recovery0 = 1;
			recovery1 = 1;
			recovery2 = 1;

			Uart_deInit();

			vTaskDelete(xInputGeneration_Task);
			vTaskDelete(xDoA_Weight_Task);
			vTaskDelete(xPS2PL_Task);
			vTaskDelete(xPL2PS_Task);
			vTaskDelete(xUARTOutput_Task);
			xInputGeneration_Task = xTaskCreateStatic(	InputGeneration,
					( const char * ) "Input",
					lenStackInputGeneration,
					NULL,
					tskIDLE_PRIORITY+1,
					xInputGenerationStack,
					&xInputGenerationBuffer );
			xDoA_Weight_Task = xTaskCreateStatic(	DoA_Weight,
					( const char * ) "DoA",
					lenStackDoA_Weight,
					NULL,
					tskIDLE_PRIORITY+1,
					xDoA_WeightStack,
					&xDoA_WeightBuffer );
			xPS2PL_Task = xTaskCreateStatic(	PS2PL,
					( const char * ) "PS2PL",
					lenStackPS2PL,
					NULL,
					tskIDLE_PRIORITY+1,
					xPS2PLStack,
					&xPS2PLBuffer );
			xPL2PS_Task = xTaskCreateStatic(	PL2PS,
					( const char * ) "PL2PS",
					lenStackPL2PS,
					NULL,
					tskIDLE_PRIORITY+1,
					xPL2PSStack,
					&xPL2PSBuffer );
			xUARTOutput_Task = xTaskCreateStatic(	UARTOutput,
					( const char * ) "UARTOut",
					lenStackUARTOutput,
					NULL,
					tskIDLE_PRIORITY+1,
					xUARTOutputStack,
					&xUARTOutputBuffer );
	 	} else {
			xil_printf("Unknown task to re-execute: %p\n", task);

			// If the re-executor is to be recovered, reboot the system
			wscp_port_reset();
		}

		portCPU_IRQ_ENABLE();
	}
}


__attribute__((annotate("exclude")))
static void HardwareFault_Handler(void *CallbackRef)
{
    xil_printf("\r\n*** HARDWARE FAULT: %s in task: %s ***\r\n", (char *)CallbackRef, pcTaskGetName(NULL));
	

	// IFDEF per reset completo sistema per ripartire da seed

    u32 dfsr, dfar;
    dfsr = mfcp(XREG_CP15_DATA_FAULT_STATUS);
    dfar = mfcp(XREG_CP15_DATA_FAULT_ADDRESS);
    // u32 ifsr, ifar;
    // mfcp(ifsr, XREG_CP15_INST_FAULT_STATUS);
    // mfcp(ifar, XREG_CP15_INST_FAULT_ADDRESS);

    xil_printf("DFSR = 0x%08x   DFAR (faulting addr) = 0x%08x\r\n", dfsr, dfar);
    // xil_printf("IFSR = 0x%08x   IFAR (faulting addr) = 0x%08x\r\n", ifsr, ifar);
	wscp_port_reset();

    while (1) { 
		portNOP();	
	}
}

__attribute__((annotate("exclude")))
void DataCorruption_Handler()
{
    xil_printf("DataCorruption_Handler: %s\n", pcTaskGetName(NULL));

	// IFDEF per reset completo sistema per ripartire da seed

	if(strcmp(pcTaskGetName(NULL), "ReExecutor") == 0) {
		xil_printf("DataCorruption_Handler on ReExecutor: rebooting\n");
		wscp_port_reset();
	} else {
		TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
		xQueueSend(xTaskReExecutor, &currentTask, TIMEOUT_QUEUES);
	}

	while(1);
}

__attribute__((annotate("exclude")))
void SigMismatch_Handler()
{
    xil_printf("SigMismatch_Handler: %s\n", pcTaskGetName(NULL));

	// IFDEF per reset completo sistema per ripartire da seed

	if(strcmp(pcTaskGetName(NULL), "ReExecutor") == 0) {
		xil_printf("SigMismatch_Handler on ReExecutor: rebooting\n");
		wscp_port_reset();
	} else {
		TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
		xQueueSend(xTaskReExecutor, &currentTask, TIMEOUT_QUEUES);
	}

    while(1);
}