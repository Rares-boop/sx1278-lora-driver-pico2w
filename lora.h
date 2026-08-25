#ifndef LORA_DRIVER_LIBRARY_H
#define LORA_DRIVER_LIBRARY_H

#define LORA_LOG(fmt, ...) printf("[LORA] " fmt "\n", ##__VA_ARGS__)

#define REG_FIFO          0x00
#define REG_OP_MODE       0x01
#define REG_FRF_MSB       0x06
#define REG_FRF_MID       0x07
#define REG_FRF_LSB       0x08
#define REG_PA_CONFIG      0x09
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_FIFO_TX_BASE  0x0E
#define REG_FIFO_RX_BASE  0x0F
#define REG_FIFO_RX_CURRENT 0x10
#define REG_IRQ_FLAGS     0x12
#define REG_RX_NB_BYTES     0x13
#define REG_PAYLOAD_LEN   0x22
#define REG_VERSION       0x42

#include <stdint.h>

/** module initialization (433000000 freq in Hz)
 * 0 - OK  and  -1 NOT_ANSWER**/
int lora_init(uint32_t frequency);
int lora_send(const uint8_t *data, uint8_t len);
int lora_receive(uint8_t *buffer, uint8_t max_len);

#endif // LORA_DRIVER_LIBRARY_H
