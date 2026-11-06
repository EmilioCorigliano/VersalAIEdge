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
#include "simple_random.h"
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
#include "xuartpsv.h"
#include "xil_exception.h"
#include "xreg_cortexr5.h"
#include "xpseudo_asm.h"
#include "xil_printf.h"

#define lenStackInputGeneration 0x500 // 460
#define lenStackUARTOutput 0x800      // 559
#define lenStackTemp 0x300            // 194

// stack of apps
#define lenStackAcas       	0x2000    // 1825
#define lenStackAnn        	0x500     // 422
#define lenStackCanny      	0x500     // 327
#define lenStackImgScaling 	0x800     // 613
#define lenStackJpeg       	0x1000    // 799
#define lenStackLatnav     	0x800     // 514

#define DELAY_1_SECOND		1000UL

#define QUEUE_LENGTH   		1
#define RX_STRING_SIZE 		16
#define TIMEOUT_QUEUES portMAX_DELAY // Blocking queue operations


/*-----------------------------------------------------------*/

void DataCorruption_Handler()
{
    xil_printf("DataCorruption_Handler: %s\n", pcTaskGetName(NULL));
    while(1);
}

void SigMismatch_Handler()
{
    xil_printf("SigMismatch_Handler: %s\n", pcTaskGetName(NULL));
    while(1);
}

__attribute__((annotate("exclude")))
static void HardwareFault_Handler(void *CallbackRef)
{
    xil_printf("\r\n*** HARDWARE FAULT: %s in task: %s ***\r\n", (char *)CallbackRef, pcTaskGetName(NULL));

    u32 dfsr, dfar;
    dfsr = mfcp(XREG_CP15_DATA_FAULT_STATUS);
    dfar = mfcp(XREG_CP15_DATA_FAULT_ADDRESS);
    // u32 ifsr, ifar;
    // mfcp(ifsr, XREG_CP15_INST_FAULT_STATUS);
    // mfcp(ifar, XREG_CP15_INST_FAULT_ADDRESS);

    xil_printf("DFSR = 0x%08x   DFAR (faulting addr) = 0x%08x\r\n", dfsr, dfar);
	
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

// Definition of the benchmark entrypoints
uint32_t acas(SemaphoreHandle_t xMutexRandom);
uint32_t ann(SemaphoreHandle_t xMutexRandom);
uint32_t canny(SemaphoreHandle_t xMutexRandom);
uint32_t img_scaling(SemaphoreHandle_t xMutexRandom);
uint32_t jpeg_compress(SemaphoreHandle_t xMutexRandom);
uint32_t latnav(SemaphoreHandle_t xMutexRandom);

// Task entrypoints
static void readTemperature( void *pvParameters );

static void InputGeneration( void *pvParameters );
static void UARTOutput( void *pvParameters );

void init_acas();
void init_ann();
void init_canny();
void init_img_scaling();
void init_jpeg_compress();
void init_latnav();

static void AcasBench( void *pvParameters );
static void AnnBench( void *pvParameters );
static void CannyBench( void *pvParameters );
static void ImgScalingBench( void *pvParameters );
static void JpegBench( void *pvParameters );
static void LatnavBench( void *pvParameters );

__attribute__((annotate("exclude"))) 
static void Uart_RxHandler(void *CallBackRef, u32 Event, u32 EventData);
static void Uart_Init(void);

typedef struct {
	uint32_t crc;
	float tDelta;
} benchmark_result_t;


static TaskHandle_t xInputGeneration_Task;
static TaskHandle_t xUARTOutput_Task;
static TaskHandle_t xTempTask;

static TaskHandle_t xAcasTask;
static TaskHandle_t xAnnTask;
static TaskHandle_t xCannyTask;
static TaskHandle_t xImgScalingTask;
static TaskHandle_t xJpegTask;
static TaskHandle_t xLatnavTask;


// Queues
static uint8_t xRxQueueBuffer[RX_STRING_SIZE * sizeof(char)];
static uint8_t xSeedAcasQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xSeedAnnQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xSeedCannyQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xSeedImgScalingQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xSeedJpegQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xSeedLatnavQueueBuffer[QUEUE_LENGTH * sizeof(uint32_t)];
static uint8_t xCRCAcasQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];
static uint8_t xCRCAnnQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];
static uint8_t xCRCCannyQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];
static uint8_t xCRCImgScalingQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];
static uint8_t xCRCJpegQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];
static uint8_t xCRCLatnavQueueBuffer[QUEUE_LENGTH * sizeof(benchmark_result_t)];

static StaticQueue_t qRxQueue;
static StaticQueue_t qSeedAcasQueue;
static StaticQueue_t qSeedAnnQueue;
static StaticQueue_t qSeedCannyQueue;
static StaticQueue_t qSeedImgScalingQueue;
static StaticQueue_t qSeedJpegQueue;
static StaticQueue_t qSeedLatnavQueue;
static StaticQueue_t qCRCAcasQueue;
static StaticQueue_t qCRCAnnQueue;
static StaticQueue_t qCRCCannyQueue;
static StaticQueue_t qCRCImgScalingQueue;
static StaticQueue_t qCRCJpegQueue;
static StaticQueue_t qCRCLatnavQueue;

static QueueHandle_t xRxQueue;
static QueueHandle_t xSeedAcasQueue;
static QueueHandle_t xSeedAnnQueue;
static QueueHandle_t xSeedCannyQueue;
static QueueHandle_t xSeedImgScalingQueue;
static QueueHandle_t xSeedJpegQueue;
static QueueHandle_t xSeedLatnavQueue;
static QueueHandle_t xCRCAcasQueue;
static QueueHandle_t xCRCAnnQueue;
static QueueHandle_t xCRCCannyQueue;
static QueueHandle_t xCRCImgScalingQueue;
static QueueHandle_t xCRCJpegQueue;
static QueueHandle_t xCRCLatnavQueue;

// Semaphores
static StaticSemaphore_t xIterationDoneBuffer;
static SemaphoreHandle_t xIterationDone;

// Input generation
static StaticSemaphore_t xMutexRandomBuffer;
static SemaphoreHandle_t xMutexRandom;

__attribute__((annotate("exclude"))) float T;
__attribute__((annotate("exclude"))) float minT;
__attribute__((annotate("exclude"))) float maxT;
static float frequency = 0;
static XSysMonPsv SysMonPtr;

/*-----------------------------------------------------------*/

#if (configSUPPORT_STATIC_ALLOCATION == 1)
__attribute__((annotate("exclude")))
StackType_t xInputGenerationStack[lenStackInputGeneration];
__attribute__((annotate("exclude")))
StackType_t xUARTOutputStack[lenStackUARTOutput];
__attribute__((annotate("exclude")))
StackType_t xTempStack[lenStackTemp];

__attribute__((annotate("exclude")))
StackType_t xAcasStack[lenStackAcas];
__attribute__((annotate("exclude")))
StackType_t xAnnStack[lenStackAnn];
__attribute__((annotate("exclude")))
StackType_t xCannyStack[lenStackCanny];
__attribute__((annotate("exclude")))
StackType_t xImgScalingStack[lenStackImgScaling];
__attribute__((annotate("exclude")))
StackType_t xJpegStack[lenStackJpeg];
__attribute__((annotate("exclude")))
StackType_t xLatnavStack[lenStackLatnav];

StaticTask_t xInputGenerationBuffer, xUARTOutputBuffer, xTempBuffer;
StaticTask_t xAcasBuffer, xAnnBuffer, xCannyBuffer, xImgScalingBuffer, xJpegBuffer, xLatnavBuffer;
#endif

/* Base/limit symbols from the .heap section */
extern char   _heap_start[];
extern char   _heap_end[];
extern char   __undef_stack[];
extern char   _stack_end[];
extern char   __bss_start__[];
extern char   __bss_end__[];

int main( void )
{
	Fault_Handlers_Init();

	uintptr_t heap_start = (uintptr_t)_heap_start;
    uintptr_t heap_end   = (uintptr_t)_heap_end;
    size_t    heap_size  = heap_end - heap_start;

	uintptr_t stack_start = (uintptr_t)__undef_stack;
    uintptr_t stack_end   = (uintptr_t)_stack_end;
    size_t    stack_size  = -(stack_end - stack_start);

	uintptr_t bss_start = (uintptr_t)__bss_start__;
    uintptr_t bss_end   = (uintptr_t)__bss_end__;
    size_t    bss_size  = bss_end - bss_start;

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

#if ( configSUPPORT_STATIC_ALLOCATION == 0 ) /* Normal or standard use case */
	xil_printf( "Using dynamic memory for tasks, queue and timer creations. \r\n" );

	#error "Dynamic memory allocation is not supported in this project. Please set configSUPPORT_STATIC_ALLOCATION to 1 in FreeRTOSConfig.h"

#else
	xil_printf( "Using static memory for tasks, queue and timer creations. \r\n" );

	// Semaphore creation to signal when another angle can be generated
	xIterationDone = xSemaphoreCreateBinaryStatic(&xIterationDoneBuffer);
	xMutexRandom = xSemaphoreCreateBinaryStatic(&xMutexRandomBuffer);
	xSemaphoreGive(xMutexRandom); // Initialize mutex

	/* Create only the queues used by InputGeneration and UARTOutput */
	xil_printf( "Creating the minimal queues statically. \r\n" );

	xRxQueue = xQueueCreateStatic(RX_STRING_SIZE, sizeof(char), xRxQueueBuffer, &qRxQueue);

	xSeedAcasQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedAcasQueueBuffer, &qSeedAcasQueue);
	xSeedAnnQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedAnnQueueBuffer, &qSeedAnnQueue);
	xSeedCannyQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedCannyQueueBuffer, &qSeedCannyQueue);
	xSeedImgScalingQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedImgScalingQueueBuffer, &qSeedImgScalingQueue);
	xSeedJpegQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedJpegQueueBuffer, &qSeedJpegQueue);
	xSeedLatnavQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t), xSeedLatnavQueueBuffer, &qSeedLatnavQueue);

	xCRCAcasQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCAcasQueueBuffer, &qCRCAcasQueue);
	xCRCAnnQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCAnnQueueBuffer, &qCRCAnnQueue);
	xCRCCannyQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCCannyQueueBuffer, &qCRCCannyQueue);
	xCRCImgScalingQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCImgScalingQueueBuffer, &qCRCImgScalingQueue);
	xCRCJpegQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCJpegQueueBuffer, &qCRCJpegQueue);
	xCRCLatnavQueue = xQueueCreateStatic(QUEUE_LENGTH, sizeof(benchmark_result_t), xCRCLatnavQueueBuffer, &qCRCLatnavQueue);

	xil_printf( "Queues created. \r\n" );

	// Create the tasks statically: input, uart output, apps and temperature reader
	xInputGeneration_Task = xTaskCreateStatic( InputGeneration,
				( const char * ) "Input",
				lenStackInputGeneration,
				NULL,
				tskIDLE_PRIORITY+1,
				xInputGenerationStack,
				&xInputGenerationBuffer );
	xUARTOutput_Task = xTaskCreateStatic( UARTOutput,
				( const char * ) "UARTOut",
				lenStackUARTOutput,
				NULL,
				tskIDLE_PRIORITY+1,
				xUARTOutputStack,
				&xUARTOutputBuffer );
	xTempTask = xTaskCreateStatic( readTemperature,
				( const char * ) "Temper",
				lenStackTemp,
				NULL,
				tskIDLE_PRIORITY+1,
				xTempStack,
				&xTempBuffer );

	// App tasks
	xAcasTask = xTaskCreateStatic( AcasBench,
				( const char * ) "Acas",
				lenStackAcas,
				NULL,
				tskIDLE_PRIORITY+1,
				xAcasStack,
				&xAcasBuffer );
	xAnnTask = xTaskCreateStatic( AnnBench,
				( const char * ) "Ann",
				lenStackAnn,
				NULL,
				tskIDLE_PRIORITY+1,
				xAnnStack,
				&xAnnBuffer );
	xCannyTask = xTaskCreateStatic( CannyBench,
				( const char * ) "Canny",
				lenStackCanny,
				NULL,
				tskIDLE_PRIORITY+1,
				xCannyStack,
				&xCannyBuffer );
	xImgScalingTask = xTaskCreateStatic( ImgScalingBench,
				( const char * ) "ImgScale",
				lenStackImgScaling,
				NULL,
				tskIDLE_PRIORITY+1,
				xImgScalingStack,
				&xImgScalingBuffer );
	xJpegTask = xTaskCreateStatic( JpegBench,
				( const char * ) "Jpeg",
				lenStackJpeg,
				NULL,
				tskIDLE_PRIORITY+1,
				xJpegStack,
				&xJpegBuffer );
	xLatnavTask = xTaskCreateStatic( LatnavBench,
				( const char * ) "Latnav",
				lenStackLatnav,
				NULL,
				tskIDLE_PRIORITY+1,
				xLatnavStack,
				&xLatnavBuffer );

	xil_printf( "Tasks created. \r\n" );
	
#endif
	
	xil_printf("Starting the scheduler.\n");

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

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

__attribute__((annotate("exclude"))) XTime tStartAcas;
__attribute__((annotate("exclude"))) XTime tEndAcas;
__attribute__((annotate("exclude"))) XTime tStartAnn;
__attribute__((annotate("exclude"))) XTime tEndAnn;
__attribute__((annotate("exclude"))) XTime tStartCanny;
__attribute__((annotate("exclude"))) XTime tEndCanny;
__attribute__((annotate("exclude"))) XTime tStartImgScaling;
__attribute__((annotate("exclude"))) XTime tEndImgScaling;
__attribute__((annotate("exclude"))) XTime tStartJpeg;
__attribute__((annotate("exclude"))) XTime tEndJpeg;
__attribute__((annotate("exclude"))) XTime tStartLatnav;
__attribute__((annotate("exclude"))) XTime tEndLatnav;
static XUartPsv Uart_Ps;

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
    XUartPsv_Config *Config = XUartPsv_LookupConfig(0xff010000);
    XUartPsv_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);

    XUartPsv_SetHandler(&Uart_Ps, Uart_RxHandler, &Uart_Ps);
    XUartPsv_SetInterruptMask(&Uart_Ps, XUARTPSV_UARTIMSC_RXIM | XUARTPSV_UARTIMSC_RTIM);

    xPortInstallInterruptHandler(XPAR_XUARTPSV_0_INTERRUPTS,
                                  (XInterruptHandler)XUartPsv_InterruptHandler,
                                  &Uart_Ps);
    vPortEnableInterrupt(XPAR_XUARTPSV_0_INTERRUPTS);
}

static void InputGeneration( void *pvParameters )
{
	(void) pvParameters;
	Uart_Init();

	// Generate random angles
	while(1)
	{
		// Waiting for stdin for seed setup
		u8 seedInput[16] = {0};
        int idx = 0;
        u8 c = 0;

		xil_printf("\nWaiting seed\n");

        while (idx < (int)sizeof(seedInput) - 1) {
            xQueueReceive(xRxQueue, &c, portMAX_DELAY);
            if (c < '0' || c > '9') {
                if (idx == 0) 
					continue;
                else 
					break;
            }
            seedInput[idx++] = c;
        }
        seedInput[idx] = '\0';

        u32 seed = atoi((char *)seedInput);

		// Seed the random number generator
		xSemaphoreTake(xMutexRandom, TIMEOUT_QUEUES);
		srand(seed);
		random_set_seed(seed);
		xSemaphoreGive(xMutexRandom);
		
		xQueueSend(xSeedAcasQueue, &seed, portMAX_DELAY);

		xSemaphoreTake(xIterationDone, TIMEOUT_QUEUES);
	}
}

static void UARTOutput( void *pvParameters )
{
	(void) pvParameters;
	xil_printf("Starting UARTOutput task.\n");

	uint32_t counter = 0;

	benchmark_result_t ResultAcas;
	benchmark_result_t ResultAnn;
	benchmark_result_t ResultCanny;
	benchmark_result_t ResultImgScaling;
	benchmark_result_t ResultJpeg;
	benchmark_result_t ResultLatnav;

	while(1)
	{
		xQueueReceive(xCRCAcasQueue, &ResultAcas, TIMEOUT_QUEUES);
		xQueueReceive(xCRCAnnQueue, &ResultAnn, TIMEOUT_QUEUES);
		xQueueReceive(xCRCCannyQueue, &ResultCanny, TIMEOUT_QUEUES);
		xQueueReceive(xCRCImgScalingQueue, &ResultImgScaling, TIMEOUT_QUEUES);
		xQueueReceive(xCRCJpegQueue, &ResultJpeg, TIMEOUT_QUEUES);
		xQueueReceive(xCRCLatnavQueue, &ResultLatnav, TIMEOUT_QUEUES);

		printf("%.3f [%.3f MHz] i:%d\tcrcAcas:0x%08x crcAnn:0x%08x crcCanny:0x%08x crcImgScaling:0x%08x crcJpeg:0x%08x crcLatnav:0x%08x tAcas:%.5fs tAnn:%.5fs tCanny:%.5fs tImgScaling:%.5fs tJpeg:%.5fs tLatnav:%.5fs\n", 
			T,
			frequency,
			++counter,
			ResultAcas.crc,
			ResultAnn.crc,
			ResultCanny.crc,
			ResultImgScaling.crc,
			ResultJpeg.crc,
			ResultLatnav.crc,
			(float)ResultAcas.tDelta / COUNTS_PER_SECOND,
			(float)ResultAnn.tDelta / COUNTS_PER_SECOND,
			(float)ResultCanny.tDelta / COUNTS_PER_SECOND,
			(float)ResultImgScaling.tDelta / COUNTS_PER_SECOND,
			(float)ResultJpeg.tDelta / COUNTS_PER_SECOND,
			(float)ResultLatnav.tDelta / COUNTS_PER_SECOND);

		// dup N_SOURCES 1 | 2 (563)
		xil_printf("InputGeneration: %d\n", (lenStackInputGeneration - uxTaskGetStackHighWaterMark2(xInputGeneration_Task))); // xInputGeneration_Task: 744 | 812
		xil_printf("UARTOutput: %d\n", (lenStackUARTOutput - uxTaskGetStackHighWaterMark2(xUARTOutput_Task))); // xInputGeneration_Task: 744 | 812
		xil_printf("Temp: %d\n", (lenStackTemp - uxTaskGetStackHighWaterMark2(xTempTask))); // xInputGeneration_Task: 744 | 812
		xil_printf("Acas: %d\n", (lenStackAcas - uxTaskGetStackHighWaterMark2(xAcasTask))); // xInputGeneration_Task: 744 | 812
		xil_printf("Ann: %d\n", (lenStackAnn - uxTaskGetStackHighWaterMark2(xAnnTask))); // xDoA_Weight_Task: 3021 | 3083
		xil_printf("Canny: %d\n", (lenStackCanny - uxTaskGetStackHighWaterMark2(xCannyTask))); // xUARTOutput_Task: 465 | 603
		xil_printf("ImgScaling: %d\n", (lenStackImgScaling - uxTaskGetStackHighWaterMark2(xImgScalingTask))); // xPS2PL_Task: 308 | 402
		xil_printf("Jpeg: %d\n", (lenStackJpeg - uxTaskGetStackHighWaterMark2(xJpegTask))); // xPL2PS_Task: 158 | 158
		xil_printf("Latnav: %d\n", (lenStackLatnav - uxTaskGetStackHighWaterMark2(xLatnavTask))); // xTempTask: 186 | 186

		xSemaphoreGive(xIterationDone);
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
static void AcasBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultAcas;
	while(1) {
		xQueueReceive(xSeedAcasQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartAcas);
		srand(seed);
		random_set_seed(seed);
		init_acas();
		ResultAcas.crc = acas(xMutexRandom);
		XTime_GetTime(&tEndAcas);
		ResultAcas.tDelta = tEndAcas - tStartAcas;
		xQueueSend(xCRCAcasQueue, &ResultAcas, TIMEOUT_QUEUES);

		xQueueSend(xSeedAnnQueue, &seed, portMAX_DELAY);
	} 
}

static void AnnBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultAnn;
	while(1) {
		xQueueReceive(xSeedAnnQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartAnn);
		srand(seed);
		random_set_seed(seed);
		init_ann();
		ResultAnn.crc = ann(xMutexRandom);
		XTime_GetTime(&tEndAnn);
		ResultAnn.tDelta = tEndAnn - tStartAnn;
		xQueueSend(xCRCAnnQueue, &ResultAnn, TIMEOUT_QUEUES);

		xQueueSend(xSeedCannyQueue, &seed, portMAX_DELAY);
	} 
}

static void CannyBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultCanny;
	while(1) {
		xQueueReceive(xSeedCannyQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartCanny);
		srand(seed);
		random_set_seed(seed);
		init_canny();
		ResultCanny.crc = canny(xMutexRandom);
		XTime_GetTime(&tEndCanny);
		ResultCanny.tDelta = tEndCanny - tStartCanny;
		xQueueSend(xCRCCannyQueue, &ResultCanny, TIMEOUT_QUEUES);

		xQueueSend(xSeedImgScalingQueue, &seed, portMAX_DELAY);
	} 
}

static void ImgScalingBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultImgScaling;
	while(1) {
		xQueueReceive(xSeedImgScalingQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartImgScaling);
		srand(seed);
		random_set_seed(seed);
		init_img_scaling();
		ResultImgScaling.crc = img_scaling(xMutexRandom);
		XTime_GetTime(&tEndImgScaling);
		ResultImgScaling.tDelta = tEndImgScaling - tStartImgScaling;
		xQueueSend(xCRCImgScalingQueue, &ResultImgScaling, TIMEOUT_QUEUES);

		xQueueSend(xSeedJpegQueue, &seed, portMAX_DELAY);
	} 
}

static void JpegBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultJpeg;
	while(1) {
		xQueueReceive(xSeedJpegQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartJpeg);
		srand(seed);
		random_set_seed(seed);
		init_jpeg_compress();
		ResultJpeg.crc = jpeg_compress(xMutexRandom);
		XTime_GetTime(&tEndJpeg);
		ResultJpeg.tDelta = tEndJpeg - tStartJpeg;
		xQueueSend(xCRCJpegQueue, &ResultJpeg, TIMEOUT_QUEUES);

		xQueueSend(xSeedLatnavQueue, &seed, portMAX_DELAY);
	} 
}

static void LatnavBench( void *pvParameters ) {
	(void) pvParameters; 
	
	uint32_t seed;
	benchmark_result_t ResultLatnav;
	while(1) {
		xQueueReceive(xSeedLatnavQueue, &seed, TIMEOUT_QUEUES);
		XTime_GetTime(&tStartLatnav);
		srand(seed);
		random_set_seed(seed);
		init_latnav();
		ResultLatnav.crc = latnav(xMutexRandom);
		XTime_GetTime(&tEndLatnav);
		ResultLatnav.tDelta = tEndLatnav - tStartLatnav;
		xQueueSend(xCRCLatnavQueue, &ResultLatnav, TIMEOUT_QUEUES);
	} 
}
