#ifndef WSCP_PORT_H
#define WSCP_PORT_H

#include <stdlib.h>
#include <stdint.h>

#ifndef WSCP_PORT_NUM_TEMPERATURES
  #define WSCP_PORT_NUM_TEMPERATURES 1
#endif

/* Maximum size in bytes of the payload */
#define WSCP_MAX_INPUT_SIZE 4 // bytes -> 1 int
#define WSCP_MAX_PAYLOAD_SIZE 0xFFFF // bytes

/**
 * @brief Initialize the UART communication with the master.
 * @returns 1 if the uart was initialized correctly, 0 otherwise.
 */
int wscp_port_uart_initialize();


/**
 * @brief Initialize the exception handlers for communicating hard faults.
 * @returns 1 if all handlers were registered correctly, 0 otherwise.
 */
int wscp_port_exp_initialize();

/**
 * @brief Sends bytes to the master, via UART.
 * @param buf is the buffer containing the data to send.
 * @param size is the size of the data to send.
 */
int wscp_port_uart_send(void *buf, size_t size);

/**
 * @brief Sends bytes to the master, via UART.
 * @param byte is the byte to send.
 */
int wscp_port_uart_send_byte(uint8_t byte);

/**
 * Fills the dut temp data structure.
 */
void wscp_port_fill_dut_temp();

int wscp_port_uart_deinitialize(void);

int wscp_port_read_state_interrupt();

/**
 * @brief Sends bytes to the master, via UART.
 * @param byte is the byte to send.
 */
uint8_t wscp_port_uart_read_byte();

/**
 * @brief Receives bytes from the master, via UART.
 * @param buf is the buffer that will contain the data read.
 * @param size is the size (in bytes) of the data to read.
 */
int wscp_port_uart_read(void *buf, size_t size);

/**
 * Checks, without blocking, whether at least one received byte is pending
 */
int wscp_port_uart_data_available();

/**
 * @brief Sets the application-specific task input
 * @param input is the input of the task
 * @param size is the size of the provided input
 */
void wscp_port_set_input(void* input, size_t size);

/**
 * @brief Gets the application-specific task output
 * @returns A pointer to the output of the task
 * @param size is a pointer where to store the size of the output
 */
void *wscp_port_get_output(uint16_t *size);

/**
 * @brief Gets the application-specific task input
 * @returns A pointer to the input of the task
 * @param size is a pointer where to store the size of the input
 */
void *wscp_port_get_input(uint16_t *size);

/**
 * Get the RDY GPIO.
 */
uint8_t wscp_port_get_rdy();

/**
 * Set the RDY GPIO pin to 1, signaling the master that the DUT is ready to receive a message.
 */
void wscp_port_set_rdy();

/**
 * Set the RDY GPIO pin to 0, signaling the master that the DUT is busy handling a message.
 */
void wscp_port_clear_rdy();

/**
 * Get the FAULT GPIO.
 */
uint8_t wscp_port_get_fault();

/**
 * Set the FAULT GPIO pin to 1, signaling the master that the DUT has encountered a fault.
 */
void wscp_port_set_fault();

/**
 * Set the FAULT GPIO pin to 0, clearing the fault signal to the master.
 */
void wscp_port_clear_fault();

/**
 * Get the TX_SIG pin.
 */
uint8_t wscp_port_get_tx_sig();

/**
 * Set the TX_SIG pin to 1, signaling the master that the DUT has encountered a fault.
 */
void wscp_port_set_tx_sig();

/**
 * Set the TX_SIG pin to 0, clearing the fault signal to the master.
 */
void wscp_port_clear_tx_sig();

/**
 * Get the RUN GPIO.
 */
uint8_t wscp_port_get_run();

/**
 * Set the RUN GPIO pin to 1, signaling the master that the DUT has encountered a fault.
 */
void wscp_port_set_run();

/**
 * Set the RUN GPIO pin to 0, clearing the fault signal to the master.
 */
void wscp_port_clear_run();

/**
 * Set the different backups of the .data sections to allow for a soft reset
 */
int wscp_init_data_section_backups();

/**
 * Perform a software reset of the DUT
 */
void wscp_port_reset();

/**
 * Add a delay because it may be that the MB has some problems and we want to wait before responding
 */
void wscp_port_add_delay_before_msg();

/**
 * @brief Write bytes into the DUT's address space.
 * @param address destination address as reported by the MB
 * @param data    source buffer
 * @param length  number of bytes to write
 */
void wscp_port_mem_write(uint32_t address, const uint8_t *data, size_t length);

/**
 * @brief Read bytes from the DUT's address space.
 * @param address source address as reported by the MB
 * @param length  number of bytes to read
 * @returns pointer to the data (valid until next port call), NULL on error
 */
const uint8_t *wscp_port_mem_read(uint32_t address, uint32_t length);

#endif
