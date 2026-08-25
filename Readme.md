# sx1278-lora-driver-pico

Bare metal C driver for SX1278 (Ra-02) LoRa modules on Raspberry Pi Pico 2 W using the Pico SDK.

## Hardware

- Raspberry Pi Pico 2 W
- Ra-02 SX1278 LoRa 433MHz module

## Wiring

| Pico 2 W | Ra-02 (SX1278) |
|----------|----------------|
| GP2      | SCK            |
| GP3      | MOSI           |
| GP4      | MISO           |
| GP5      | NSS (CS)       |
| 3V3(OUT) | VCC            |
| GND      | GND            |

## Usage

```c
#include "lora.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    if (lora_init(433000000) != 0) {
        printf("LoRa init failed\n");
        return -1;
    }

    // send
    lora_send((uint8_t *)"hello", 5);

    // receive
    uint8_t buf[255];
    int len = lora_receive(buf, sizeof(buf));
    if (len > 0) {
        printf("Received %d bytes\n", len);
    }
}
```

## API

- `int lora_init(uint32_t frequency)` — initialize SPI and configure the module. Returns 0 on success.
- `int lora_send(const uint8_t *data, uint8_t len)` — transmit data. Returns 0 on success.
- `int lora_receive(uint8_t *buffer, uint8_t max_len)` — receive data. Returns number of bytes received, 0 on timeout, -1 on error.

## Build

Requires [Pico SDK](https://github.com/raspberrypi/pico-sdk). Set `PICO_SDK_PATH` environment variable.

```bash
mkdir build && cd build
cmake ..
make
```