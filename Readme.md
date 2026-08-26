# sx1278-lora-driver-pico

Minimal C driver for SX1278 (Ra-02) LoRa modules on Raspberry Pi Pico, using the Pico SDK.

> **Status:** MVP. SPI and register access verified on hardware.
> Wireless TX/RX between two modules not yet tested.

## Wiring

| Pico | Ra-02 |
|------|-------|
| GP2  | SCK   |
| GP3  | MOSI  |
| GP4  | MISO  |
| GP5  | NSS   |
| 3V3  | VCC   |
| GND  | GND   |

Pins are defined at the top of `lora.c`.

## Usage

```c
#include "lora.h"

lora_init(434000000);

// send
lora_send((const uint8_t *)"hello", 5);

// receive
uint8_t buf[255];
int len = lora_receive(buf, sizeof(buf));
if (len > 0) {
    printf("got %d bytes\n", len);
}
```

Both modules must use the same frequency.

## API

- `int lora_init(uint32_t frequency)` — 0 on success, -1 on failure.
- `int lora_send(const uint8_t *data, uint8_t len)` — 0 on success, -1 on failure.
- `int lora_receive(uint8_t *buffer, uint8_t max_len)` — byte count, 0 on timeout, -1 on error. Blocks up to 5 seconds.

## Notes

- Connect an antenna before transmitting. Transmitting without one can damage the module.
- In the EU, use 433.05–434.79 MHz with a 10% duty cycle limit.
- Point-to-point only: no addressing, acknowledgement or retry.
- Not thread-safe.

## Build

Requires the Pico SDK.

```cmake
add_library(lora lora.c)
target_link_libraries(lora pico_stdlib hardware_spi hardware_gpio)
```