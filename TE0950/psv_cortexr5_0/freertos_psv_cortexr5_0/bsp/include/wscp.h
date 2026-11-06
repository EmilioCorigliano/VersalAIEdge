#ifndef WSCP_H
#define WSCP_H

#ifndef WSCP_PORT_H 
  #include "wscp_port.h"
#endif

#define WSCP_VERSION 1.0

#define WSCP_ESCAPE_CHAR      0x67
#define WSCP_MSG_BEGIN_CHAR   0xD5

#define WSCP_MAX_MEMWRITE_SIZE (2 * 0x400) // 2 * 1KB

#include "stdint.h"
#include "cbor/cbor.h"

enum wscp_finfo_code_e {
    NO_ERR = 0,
    DE_DETECTED = 1,
    CFE_DETECTED = 2,
    HARD_FAULT = 3,
    BUS_FAULT = 4,
    USAGE_FAULT = 5,
    UNKNOWN_FAULT = 6,
    TIMING_FAULT = 7,
    RECOVERED = 0b10000000,
};

enum wscp_msg_id_e {
  // MB -> DUT messages
  MB_INPUT = 0,   // send the task input to DUT
  MB_OUTPUT,      // read the task output from the DUT
  MB_F_INFO,
  MB_F_INFO_DESC,
  MB_MEM_READ,
  MB_MEM_WRITE,
  MB_DUT_INFO,
  MB_PING,
  MB_SOFT_RESET,
  MB_TEMP_READ,
  // DUT -> MB messages
  DUT_ACK = 128,  // returned when a message is received correctly
  DUT_OUTPUT,     // returns the output to the MB
  DUT_F_INFO,
  DUT_F_INFO_DESC,
  DUT_MEM_READ,
  DUT_DUT_INFO = 133,
  DUT_PONG,
  DUT_TEMP = 136,
  DUT_NACK = 255,

  // this is used to invalidate the type ID and return a NACK
  DUT_INVALID = 127
};

enum wscp_nack_reason_e {
  UNSUPPORTED_MSG,
  INVALID_CRC,
  CBOR_ERR,
  REPEATED_INPUT
};

typedef struct __attribute__((packed)) {
  uint8_t preamble;
  uint8_t type_ID;     // 8b
  uint16_t pl_length;   // 16b
  uint8_t payload[WSCP_MAX_PAYLOAD_SIZE];
  uint16_t crc;                           // CRC is the CRC-16-CCITT of the entire message, preamble included, up to the payload size
} wscp_msg_t;

typedef struct __attribute__((packed)) {
  uint8_t nack_reason;
} wscp_reason_t;

typedef struct __attribute__((packed))  {
  uint32_t prot_version;
  uint8_t dut_name[16];
} wscp_dut_info_t;

typedef struct __attribute__((packed)) {
  uint8_t num_temperatures;
  float temps[WSCP_PORT_NUM_TEMPERATURES]; // questa cosa possiamo anche evitarla, possiamo semplicemente assumere che ci siano 2 termometri :)
} wscp_dut_temp_t;

typedef struct __attribute__((packed)) {
  uint8_t finfo_code;
} wscp_finfo_t;

typedef struct __attribute__((packed)) {
  uint8_t finfo_code;
  uint8_t task_id;
  uint32_t fault_addr;
  uint32_t cause;
} wscp_finfo_desc_t;

typedef struct __attribute__((packed)) {
  uint32_t address;
  uint32_t length;
} wscp_memread_payload_t;

typedef struct __attribute__((packed)) {
  uint32_t address;
  uint8_t data[WSCP_MAX_MEMWRITE_SIZE];
} wscp_memwrite_payload_t;


// user-accessible port-dependent structs for diagnostics
extern wscp_finfo_t wscp_port_finfo;
extern wscp_finfo_desc_t wscp_port_finfo_desc;
extern wscp_dut_info_t wscp_port_dut_info;
extern wscp_dut_temp_t wscp_port_dut_temp;

int wscp_init();

int wscp_handle_message();

cbor_error_t wscp_reason_serialize_cbor(cbor_writer_t *w,
                                           const wscp_reason_t *s);


cbor_error_t wscp_dut_info_serialize_cbor(cbor_writer_t *w,
                                             const wscp_dut_info_t *s);


cbor_error_t wscp_dut_temp_serialize_cbor(cbor_writer_t *w,
                                             const wscp_dut_temp_t *s);                                             

cbor_error_t wscp_finfo_serialize_cbor(cbor_writer_t *w,
                                               const wscp_finfo_t *s);

cbor_error_t wscp_finfo_desc_serialize_cbor(cbor_writer_t *w,
                                               const wscp_finfo_desc_t *s);


cbor_error_t wscp_byte_array_serialize_cbor(const void *src,
                                             size_t src_len,
                                             void *dst_cbor,
                                             size_t dst_cbor_capacity,
                                             size_t *dst_cbor_len);
                                      
cbor_error_t wscp_byte_array_unserialize_cbor(const void *src_cbor,
                                               size_t src_cbor_len,
                                               void *dst,
                                               size_t dst_capacity,
                                               size_t *dst_len);

cbor_error_t wscp_memwrite_payload_t_unserialize_cbor(
    const void *pl,
    size_t pl_length,
    wscp_memwrite_payload_t *payload,
    size_t max_data_len,
    size_t *data_len);

cbor_error_t wscp_payload_t_unserialize_cbor_array(
    const void *pl,
    size_t pl_length,
    uint8_t *data,
    size_t max_data_len,
    size_t *data_len);


cbor_error_t wscp_memread_payload_t_unserialize_cbor(
    const void *pl,
    size_t pl_length,
    wscp_memread_payload_t *payload);

#endif
