#include "wscp_port.h"
#include "wscp.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "sleep.h"
#include "task.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "xsysmonpsv_hw.h"
#include "xsysmonpsv.h"
#include "xstatus.h"
#include "xparameters_ps.h"
#include "xiltimer.h"
#include "xgpio.h"
#include "xuartlite.h"
#include "xuartlite_i.h"
#include "xuartlite_l.h"
#include "xil_exception.h"
#include "xinterrupt_wrap.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "xil_cache.h"
#include "portmacro.h"

// C_LS1
//  1  2  3  4  5  6
// |x |x |TX|2 |1 |x |
// |x |x |4 |3 |x |RX|

// 1 = 0b0001
// 2 = 0b0010
// 3 = 0b0100
// 4 = 0b1000

#define XUARTLITE_BASEADDRESS	XPAR_AXI_UARTLITE_0_BASEADDR
#define TEST_BUFFER_SIZE        100

#define portCPU_IRQ_DISABLE()										\
	__asm volatile ( "CPSID i" ::: "memory" );						\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );


#define portCPU_IRQ_ENABLE()										\
	__asm volatile ( "CPSIE i" ::: "memory" );						\
	__asm volatile ( "DSB" );										\
	__asm volatile ( "ISB" );

static XUartLite UartLite;
static XUartLite_Config *UartCfg;

static XGpio gpio;

static XSysMonPsv SysMonPtr;

__attribute__((section(".persistent")))
static uint8_t data_backup_init_0 = 0;

__attribute__((section(".persistent")))
static uint8_t data_backup_init_1 = 0;

__attribute__((section(".persistent")))
static uint8_t data_backup_init_2 = 0;

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

typedef struct {
    uint32_t watchdog_reason;
    uint32_t powman_chip_reset;
    uint32_t watchdog_caused;
    uint32_t watchdog_enable_caused;

    uint32_t scratch0;
    uint32_t scratch1;
    uint32_t scratch2;
    uint32_t scratch3;
} reset_diag_t;

volatile reset_diag_t reset_diag;

// TODO!
void wscp_port_capture_reset_reason(void)
{
    // reset_diag.watchdog_reason =
    //     watchdog_hw->reason;

    // reset_diag.powman_chip_reset =
    //     powman_hw->chip_reset;

    // reset_diag.watchdog_caused =
    //     watchdog_caused_reboot();

    // reset_diag.watchdog_enable_caused =
    //     watchdog_enable_caused_reboot();

    // reset_diag.scratch0 = watchdog_hw->scratch[0];
    // reset_diag.scratch1 = watchdog_hw->scratch[1];
    // reset_diag.scratch2 = watchdog_hw->scratch[2];
    // reset_diag.scratch3 = watchdog_hw->scratch[3];
}

#ifndef WSCP_UART_INSTANCE
#define WSCP_UART_INSTANCE XUARTLITE_BASEADDRESS
#endif

#ifndef WSCP_UART_BAUD_RATE
#define WSCP_UART_BAUD_RATE 9600
#endif

// #ifndef WSCP_UART_TX_PIN
// #define WSCP_UART_TX_PIN (0)
// #endif

// #ifndef WSCP_UART_RX_PIN
// #define WSCP_UART_RX_PIN (1)
// #endif

#ifndef WSCP_READY_PIN
#define WSCP_READY_PIN (0b0001)
#endif

#ifndef WSCP_FAULT_PIN
#define WSCP_FAULT_PIN (0b0010)
#endif 

#ifndef WSCP_TX_SIG
#define WSCP_TX_SIG (0b0100)
#endif 

#ifndef WSCP_RUN_PIN
#define WSCP_RUN_PIN (0b1000)
#endif

#ifndef DUT_CONFIG_NAME
#define DUT_CONFIG_NAME "versal-0"
#endif

volatile uint32_t input = -1;        // It was marked extern
volatile unsigned short output = 0; // It was marked extern
unsigned short output_to_send = 0xbeef;
volatile uint64_t begin_time = 0;
volatile uint64_t end_time = 0;
volatile uint64_t exec_time = 0;

wscp_finfo_t wscp_port_finfo = {
    .finfo_code = NO_ERR,
};

wscp_finfo_desc_t wscp_port_finfo_desc = {
    .finfo_code = NO_ERR,
    .task_id = 0,
    .fault_addr = 0,
    .cause = 0,
};

wscp_dut_info_t wscp_port_dut_info = {
    .prot_version = 1,
    .dut_name = DUT_CONFIG_NAME,
};

wscp_dut_temp_t wscp_port_dut_temp = {
    .num_temperatures = WSCP_PORT_NUM_TEMPERATURES,
    .temps = 0.0,
};

void wscp_port_handle_message(void) {
    xil_printf("UART interrupt received\r\n");
    /* Disable UART interrupt */
    XUartLite_DisableInterrupt(&UartLite);
    
    wscp_handle_message();
    
    /* Enable UART interrupt */
    XUartLite_EnableInterrupt(&UartLite);
    xil_printf("UART interrupt received end\r\n");
}

uint8_t get_pin_value(uint32_t pin) {
    return XGpio_DiscreteRead(&gpio, 1) & pin;
}

void set_pin(uint32_t pin) {

	u32 val = XGpio_DiscreteRead(&gpio, 1);

	XGpio_DiscreteWrite(&gpio, 1, (val | pin) & 0b1111);
}

void reset_pin(uint32_t pin) {

	u32 val = XGpio_DiscreteRead(&gpio, 1);

	XGpio_DiscreteWrite(&gpio, 1, (val & ~pin) & 0b1111);
}

static u32     uart_intr_id;
static UINTPTR uart_intr_parent;
static int     uart_initialized = 0;

int wscp_port_uart_deinitialize(void) {
    xil_printf("Deinitializing UART...\r\n");
    if (!uart_initialized) return XST_SUCCESS;

    // while (!XUartLite_IsTransmitEmpty(UartLite.RegBaseAddress)) {}

    /* 1. Stop the peripheral from raising interrupts */
    XUartLite_DisableInterrupt(&UartLite);

    /* 2. Mask the line at the GIC and remove the handler */
    XDisableIntrId(uart_intr_id, uart_intr_parent);
    XDisconnectInterruptCntrl(uart_intr_id, uart_intr_parent);

    /* 3. Drain/clear TX and RX FIFOs (also drops any stale pending byte) */
    XUartLite_ResetFifos(&UartLite);

    /* 4. Clear the driver state so no half-finished transfer survives */
    memset(&UartLite, 0, sizeof(UartLite));
    uart_initialized = 0;
    xil_printf("Deinitialized UART\r\n");

    return XST_SUCCESS;
}

// int wscp_port_uart_deinitialize(void) {
// 	XUartLite_Config *CfgPtr;

// 	CfgPtr = XUartLite_LookupConfig(WSCP_UART_INSTANCE);

//     XDisconnectInterruptCntrl(CfgPtr->IntrId, CfgPtr->IntrParent);
//     XDisableIntrId(CfgPtr->IntrId, CfgPtr->IntrParent);
//     XStopInterruptCntrl(CfgPtr->IntrParent);
//     XUartLite_DisableInterrupt(&UartLite);
//     XUartLite_ResetFifos(&UartLite);
// }

static void UartLiteSendHandler(void *CallBackRef, unsigned int EventData)
{
	(void)CallBackRef;
	(void)EventData;
	// TotalSentCount = EventData;
}

static void UartLiteRecvHandler(void *CallBackRef, unsigned int EventData)
{
	(void)CallBackRef;
	(void)EventData;
}

int wscp_port_uart_initialize(void) {
    wscp_port_capture_reset_reason();

    /* UART */
    int Status;
	int Index;
	XUartLite_Config *CfgPtr;

	CfgPtr = XUartLite_LookupConfig(WSCP_UART_INSTANCE);

	if (XUartLite_Initialize(&UartLite, WSCP_UART_INSTANCE) != XST_SUCCESS) {
		xil_printf("UART Lite initialization failed!\n");
		return XST_FAILURE;
	}

	if (XUartLite_SelfTest(&UartLite) != XST_SUCCESS) {
		xil_printf("UART Lite Self Test failed!\n");
		return XST_FAILURE;
	}

    Status = XSetupInterruptSystem(&UartLite,
                                   wscp_port_handle_message,
                                   CfgPtr->IntrId,
                                   CfgPtr->IntrParent,
                                   XINTERRUPT_DEFAULT_PRIORITY);

    if (Status != XST_SUCCESS) {
        xil_printf("UARTLite interrupt setup failed\r\n");
        return XST_FAILURE;
    }

    uart_intr_id     = CfgPtr->IntrId;
    uart_intr_parent = CfgPtr->IntrParent;
    uart_initialized = 1;

    // /*
	//  * Setup the handlers for the UartLite that will be called from the
	//  * interrupt context when data has been sent and received,
	//  * specify a pointer to the UartLite driver instance as the callback
	//  * reference so the handlers are able to access the instance data.
	//  */
	// XUartLite_SetSendHandler(&UartLite, UartLiteSendHandler, &UartLite);
	// XUartLite_SetRecvHandler(&UartLite, UartLiteRecvHandler, &UartLite);


    /* Enable UARTLite interrupt generation */
    XUartLite_EnableInterrupt(&UartLite);

    /* GPIOs */
	if (XGpio_Initialize(&gpio, XPAR_XGPIO_0_BASEADDR) != XST_SUCCESS) {
		xil_printf("GPIO not initialized!\n");
		return XST_FAILURE;
	}

	XGpio_SetDataDirection(&gpio, 1, 0x0);   // 0x0 = all 4 bits are outputs; 0xF = all 4 bits are inputs
    reset_pin(WSCP_FAULT_PIN | WSCP_TX_SIG | WSCP_RUN_PIN);
    set_pin(WSCP_READY_PIN);

    /* Temp Sensor */
	XSysMonPsv_Init(&SysMonPtr, NULL);

    // riscv_timer_set_enabled(1);
    return 1;
}

void wscp_port_fill_dut_temp() {
    float T = 0;
	XSysMonPsv_ReadTempProcessed(&SysMonPtr, XSYSMONPSV_TEMP, &T);

    wscp_port_dut_temp.temps[0] = T;
}

int wscp_port_read_state_interrupt() {
    return XUartLite_ReadReg(UartLite.RegBaseAddress, XUL_STATUS_REG_OFFSET) & XUL_SR_INTR_ENABLED;
}

int wscp_port_uart_send(void *buf, size_t size) {
    if (buf == NULL && size != 0) return 0;

    uint32_t toBeSent = size;
    uint32_t sent = 0;

    while(sent < toBeSent) {
        sent += XUartLite_Send(&UartLite, &((uint8_t *)buf)[sent], toBeSent - sent);
    }

    while(XUartLite_IsSending(&UartLite)) {
        // wait until the transmission is complete
    }

    return 1;
}

int wscp_port_uart_send_byte(uint8_t byte) {
    wscp_port_uart_send(&byte, 1);
    return 1;
}

uint8_t wscp_port_uart_read_byte(void) {
    uint8_t byte = 0;
    uint8_t recv = 0;
    
    while(recv <= 0 ) {
        recv = XUartLite_Recv(&UartLite, &byte, 1);
    }

    return byte;
}

int wscp_port_uart_read(void *buf, size_t size) {
    if (buf == NULL && size != 0) return 0;

    uint8_t recv = 0;

    while(recv < size) {
        recv += XUartLite_Recv(&UartLite, &((uint8_t *)buf)[recv], size - recv);
    }

    return 1;
}

int wscp_port_uart_data_available(void) {
    uint8_t SR = XUartLite_GetSR(&UartLite);
    
    return SR & XUL_SR_RX_FIFO_VALID_DATA;
}

// GPIO-side protocol

uint8_t wscp_port_get_rdy(void) {
    return get_pin_value(WSCP_READY_PIN);
}

void wscp_port_set_rdy(void) {
    set_pin(WSCP_READY_PIN);
}

void wscp_port_clear_rdy(void) {
    reset_pin(WSCP_READY_PIN);
}

void wscp_port_add_delay_before_msg() {
    usleep(350);
}

uint8_t wscp_port_get_tx_sig(void) {
    return get_pin_value(WSCP_TX_SIG);
}

void wscp_port_set_tx_sig(void) {
    set_pin(WSCP_TX_SIG);
}

void wscp_port_clear_tx_sig(void) {
    reset_pin(WSCP_TX_SIG);
}

uint8_t wscp_port_get_run(void) {
    return get_pin_value(WSCP_RUN_PIN);
}

void wscp_port_set_run(void) {
    set_pin(WSCP_RUN_PIN);

    XTime_GetTime(&begin_time);
}

void wscp_port_clear_run(void) {
    reset_pin(WSCP_RUN_PIN);

    XTime_GetTime(&end_time);

    uint64_t dT;

    if(end_time < begin_time) {
        dT = UINT32_MAX - (begin_time - end_time);
    } else {
        dT = end_time - begin_time;
    }

    exec_time = (float)(dT) / COUNTS_PER_SECOND;
}

uint8_t wscp_port_get_fault(void) {
    return get_pin_value(WSCP_FAULT_PIN);
}

void wscp_port_set_fault(void) {
    set_pin(WSCP_FAULT_PIN);
}

void wscp_port_clear_fault(void) {
    reset_pin(WSCP_FAULT_PIN);
}

// Command implementation

void wscp_port_set_input(void *new_input, size_t size) {
    if (new_input == NULL || size == 0) return;

    int value = 0;
    size_t copy_size = size <= sizeof(value) ? size : sizeof(value);
    for (int i=0; i<copy_size; i++) {
        ((char*)&value)[copy_size-(i+1)] = ((char*)new_input)[i];
    }

    input = value;
    wscp_port_finfo.finfo_code = NO_ERR;

    wscp_port_finfo_desc.finfo_code = NO_ERR;
    wscp_port_finfo_desc.task_id = 0;
    wscp_port_finfo_desc.fault_addr = 0;
}

void *wscp_port_get_output(uint16_t *size) {
    if (size != NULL) *size = (uint16_t)sizeof(output);
    for (int i=0; i<*size; i++) {
        ((char*)&output_to_send)[*size-(i+1)] = ((char*)&output)[i];
    }
    return &output_to_send;
}

// void *wscp_port_get_input(uint16_t *size) {
//     *size = sizeof(input);
//     return &input;
// }


bool wscp_port_take_input(int *value)
{
    if (value == NULL) {
        return false;
    }

    /* Disable UART interrupt */
    XUartLite_DisableInterrupt(&UartLite); // TODO: check if is fine to remove this
    
    if (input == -1) {
        /* Enable UART interrupt */
        XUartLite_EnableInterrupt(&UartLite); // TODO: check if is fine to remove this
        return false;
    }

    *value = input;

    /*
     * Consume it while we still own the critical section.
     */
    input = -1;

    /* Enable UART interrupt */
    XUartLite_EnableInterrupt(&UartLite); // TODO: check if is fine to remove this

    return true;
}

volatile int curr_input;
void *wscp_port_get_input(uint16_t *size) {
    int i = 0;
    int j = 0;
    while(!wscp_port_take_input(&curr_input)) {
        usleep(100);
        // j++;
        // if (j % 1000 == 0) {
        //     i++;
        //     gpio_put(25, i%2);
        // }
    }
    *size = 4;
    return &curr_input;
}

extern uint8_t majority_voting8(uint8_t *a, uint8_t *b, uint8_t *c);

extern uint32_t majority_voting32(uint32_t *a, uint32_t *b, uint32_t *c);

int wscp_init_data_section_backups() {
    uintptr_t data_start = (uintptr_t)__data_start;
	uintptr_t data_backup_start0 = (uintptr_t)__data_backup_start0;
	uintptr_t data_backup_start1 = (uintptr_t)__data_backup_start1;
	uintptr_t data_backup_start2 = (uintptr_t)__data_backup_start2;
    
    uintptr_t data_end   = (uintptr_t)__data_end;
    uintptr_t data_backup_end0   = (uintptr_t)__data_backup_end0;
    uintptr_t data_backup_end1   = (uintptr_t)__data_backup_end1;
    uintptr_t data_backup_end2   = (uintptr_t)__data_backup_end2;
    
    size_t    data_size  = data_end - data_start;
    size_t    data_backup_size0  = data_backup_end0 - data_backup_start0;
    size_t    data_backup_size1  = data_backup_end1 - data_backup_start1;
    size_t    data_backup_size2  = data_backup_end2 - data_backup_start2;

	if(data_size != data_backup_size0 || data_size != data_backup_size1 || data_size != data_backup_size2) {
		xil_printf("Error: data size and backup size do not match!\n");
		while(1);
	}
	
	if(majority_voting8(&data_backup_init_0, &data_backup_init_1, &data_backup_init_2) != 1) {
		memcpy((void *)data_backup_start0, (void *)data_start, data_size);
		data_backup_init_0 = 1;
		memcpy((void *)data_backup_start1, (void *)data_start, data_size);
		data_backup_init_1 = 1;
		memcpy((void *)data_backup_start2, (void *)data_start, data_size);
		data_backup_init_2 = 1;
		memcpy((void *)data_start, (void *)data_backup_start0, data_size);
        return 1;
	}

    return 0;
}


void wscp_port_reset(void) {
	xil_printf("Rebooting the system...\n");

	// De-initializes the UART
	// Uart_deInit();
    wscp_port_uart_deinitialize();

	portCPU_IRQ_DISABLE();

	uintptr_t data_start = (uintptr_t)__data_start;
	uintptr_t data_end   = (uintptr_t)__data_end;
	size_t    data_size  = data_end - data_start;

	uintptr_t data_backup_start0 = (uintptr_t)__data_backup_start0;
	uintptr_t data_backup_start1 = (uintptr_t)__data_backup_start1;
	uintptr_t data_backup_start2 = (uintptr_t)__data_backup_start2;

	uintptr_t bss_start = (uintptr_t)__bss_start__;
	uintptr_t bss_end   = (uintptr_t)__bss_end__;
	size_t    bss_size  = bss_end - bss_start;
    
	/* Majority voting bit a bit for the data section */
	for (size_t i = 0; i < data_size / 4; ++i)
	{
		*((uint32_t *)data_start + i) = majority_voting32((uint32_t *)data_backup_start0 + i, (uint32_t *)data_backup_start1 + i, (uint32_t *)data_backup_start2 + i);
	}
	memset((void *)bss_start, 0, bss_size);

	/* Ensure caches are coherent with memory before jumping to _boot.
	 * Without this, the CPU's instruction prefetch or data cache may see
	 * stale/stale-instruction/data while the RAM contents were just rewritten,
	 * causing prefetch aborts or corrupted behavior when early init runs.
	 */    
    Xil_DCacheFlush();
    Xil_DCacheInvalidate();
    Xil_ICacheInvalidate();

	asm volatile (
        "cpsid if\n"
        "cps   #0x13\n"                    /* SVC mode */
        "ldr\tpc,=_boot");
}

void wscp_port_mem_write(uint32_t address,
                         const uint8_t *data,
                         size_t length) {
    if (data == NULL && length != 0) return;
    memcpy((void *)(uintptr_t)address, data, length);
}

const uint8_t *wscp_port_mem_read(uint32_t address, uint32_t length) {
    (void)length;
    return (const uint8_t *)(uintptr_t)address;
}

// Exception handling

static void HardwareFault_Handler(void *ExceptionIdParam)
{
    uint32_t ExceptionId = (uint32_t) ExceptionIdParam;

    // Signal that something has happened
    wscp_port_set_fault();

    u32 dfsr, dfar;
    dfsr = mfcp(XREG_CP15_DATA_FAULT_STATUS);
    dfar = mfcp(XREG_CP15_DATA_FAULT_ADDRESS);
    // u32 ifsr, ifar;
    // mfcp(ifsr, XREG_CP15_INST_FAULT_STATUS);
    // mfcp(ifar, XREG_CP15_INST_FAULT_ADDRESS);

    wscp_port_finfo.finfo_code = HARD_FAULT;

    wscp_port_finfo_desc.finfo_code = HARD_FAULT;
    wscp_port_finfo_desc.task_id = uxTaskGetTaskNumber(NULL);
    wscp_port_finfo_desc.fault_addr = dfar;
    wscp_port_finfo_desc.cause = ExceptionId;

    char *ExceptionIdStr;
    switch (ExceptionId)
    {
        case XIL_EXCEPTION_ID_RESET:
            ExceptionIdStr = "XIL_EXCEPTION_ID_RESET";
            break;
        case XIL_EXCEPTION_ID_UNDEFINED_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_UNDEFINED_INT";
            break;
        case XIL_EXCEPTION_ID_SWI_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_SWI_INT";
            break;
        case XIL_EXCEPTION_ID_PREFETCH_ABORT_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_PREFETCH_ABORT_INT";
            break;
        case XIL_EXCEPTION_ID_DATA_ABORT_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_DATA_ABORT_INT";
            break;
        case XIL_EXCEPTION_ID_IRQ_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_IRQ_INT";
            break;
        case XIL_EXCEPTION_ID_FIQ_INT:
            ExceptionIdStr = "XIL_EXCEPTION_ID_FIQ_INT";
            break;
        default:
            ExceptionIdStr = "UNKNOWN";
            break;
    }

    xil_printf("\r\n*** HARDWARE FAULT: %s in task: %s ***\r\n", ExceptionIdStr, pcTaskGetName(NULL));
	
    xil_printf("DFSR = 0x%08x   DFAR (faulting addr) = 0x%08x\r\n", dfsr, dfar);
    // xil_printf("IFSR = 0x%08x   IFAR (faulting addr) = 0x%08x\r\n", ifsr, ifar);

    // const uint uart_irq = UART_IRQ_NUM(WSCP_UART_INSTANCE);
    wscp_port_set_rdy();
    // irq_set_enabled(uart_irq, true);

    while (1) { 
		portNOP();	
	}
}

int wscp_port_exp_initialize() {
    Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_RESET, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_RESET);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNDEFINED_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_UNDEFINED_INT);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_SWI_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_SWI_INT);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_PREFETCH_ABORT_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_PREFETCH_ABORT_INT);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_DATA_ABORT_INT);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_IRQ_INT);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT, (Xil_ExceptionHandler)HardwareFault_Handler, (void *)XIL_EXCEPTION_ID_FIQ_INT);

    return 1;
}
