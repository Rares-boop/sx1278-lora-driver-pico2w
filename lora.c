#define SPI_PORT  spi0
#define PIN_SCK   2
#define PIN_MOSI  3
#define PIN_MISO  4
#define PIN_CS    5
#define STD_BAUDRATE 5000000
#define MAX_PAYLOAD_SIZE 255
#define RX_TIMEOUT_MS 5000

#include "lora.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <stdio.h>

static void cs_select(void) {
    gpio_put(PIN_CS, 0);
    sleep_us(5);
}

static void cs_deselect(void) {
    gpio_put(PIN_CS, 1);
    sleep_us(5);
}

static void write_register(uint8_t addr, uint8_t value) {
    if (addr > 0x7F) {
        LORA_LOG("write_register invalid address: 0x%02X", addr);
        return;
    }

    uint8_t tx = addr | 0x80;

    cs_select();
    spi_write_blocking(SPI_PORT, &tx, 1);
    spi_write_blocking(SPI_PORT, &value, 1);
    cs_deselect();

}

static uint8_t read_register(uint8_t addr) {
    if (addr > 0x7F) {
        LORA_LOG("read_register invalid address: 0x%02X", addr);
        return 0x00;
    }

    uint8_t tx = addr & 0x7F;
    uint8_t rx;

    cs_select();
    spi_write_blocking(SPI_PORT, &tx, 1);
    spi_read_blocking(SPI_PORT, 0X00, &rx, 1);
    cs_deselect();

    return rx;
}

static int write_register_verified(uint8_t addr, uint8_t value) {
    write_register(addr, value);
    uint8_t check = read_register(addr);
    if (check != value) {
        LORA_LOG("write verify FAIL at 0x%02X: wrote 0x%02X, read 0x%02X", addr, value, check);
        return -1;
    }
    return 0;
}

int lora_init(uint32_t frequency) {
    /** init SPI **/
    spi_init(SPI_PORT, STD_BAUDRATE);
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);

    /** Initialize GPIO pins for SPI0 **/
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    /** check lora module **/
    uint8_t check_lora_module = read_register(REG_VERSION);
    if ( check_lora_module != 0X12 ) {
        LORA_LOG("lora init lora module initialization failure REG_VERSION %d\n", check_lora_module);
        return -1;
    }

    write_register(REG_OP_MODE, 0x80);
    sleep_ms(10);

    /** set the frequency **/
    uint32_t frf = (uint32_t)((uint64_t)frequency * 524288 / 32000000);
    int frf_msb = write_register_verified(REG_FRF_MSB, (frf >> 16) & 0xFF);
    int frf_mid = write_register_verified(REG_FRF_MID, (frf >> 8) & 0xFF);
    int frf_lsb = write_register_verified(REG_FRF_LSB, frf  & 0xFF);

    if (frf_msb != 0 || frf_mid != 0 || frf_lsb != 0) {
        LORA_LOG("lora init lora module initialization failure FREQUENCY SETTING REGISTER FAILURE %d\n", check_lora_module);
        return -1;
    }

    /** SET FIFO BUFFER
     * base address TX 0
     * base address RX 0
     * |        FIFO = 256 bytes        |  split   |   TX = 128 bytes     |     RX = 128 bytes    |**/
    int fifo_tx_base = write_register_verified(REG_FIFO_TX_BASE, 0x00);
    if (fifo_tx_base != 0) {
        LORA_LOG("lora init Error setting TX buffer to 0x00\n");
        return -1;
    }

    int fifo_rx_base = write_register_verified(REG_FIFO_RX_BASE, 0x00);
    if (fifo_rx_base != 0) {
        LORA_LOG("lora init Error setting RX buffer to 0x00\n");
        return -1;
    }

    int pa_config = write_register_verified(REG_PA_CONFIG, 0x8F);
    if (pa_config != 0) {
        LORA_LOG("lora init Error setting PA CONFIG register\n");
        return -1;
    }

    write_register(REG_OP_MODE, 0X81);
    sleep_ms(10);

    return 0;
}

int lora_send(const uint8_t *data, uint8_t len) {
    if (len == 0 || len > MAX_PAYLOAD_SIZE) {
        LORA_LOG("lora_send invalid length: %d", len);
        return -1;
    }

    /* put lora module in standby */
    write_register(REG_OP_MODE, 0X81);
    sleep_ms(10);

    /* set fifo ptr la baza TX  */
    int set_tx_base = write_register_verified(REG_FIFO_ADDR_PTR, 0x00);
    if (set_tx_base != 0) {
        LORA_LOG("lora send Error setting TX base to 0x00\n");
        return -1;
    }

    /* write data to FIFO */
    for (int i=0;i<len;i++) {
        write_register(REG_FIFO, data[i]);
    }

    /* set payload length */
    write_register(REG_PAYLOAD_LEN, len);

    /* put lora module in TX mode */
    write_register(REG_OP_MODE, 0x83);

    /* wait for TxDone */
    int time_to_sleep_ms = 300;
    uint8_t irq_flags = 0x00;
    while (time_to_sleep_ms > 0) {
        irq_flags = read_register(REG_IRQ_FLAGS);
        if (irq_flags & 0x08) {
            break;
        }
        sleep_ms(10);
        time_to_sleep_ms -= 10;
    }

    if (!(irq_flags & 0x08)) {
        LORA_LOG("lora_send TX timeout");
        write_register(REG_OP_MODE, 0x81);
        return -1;
    }

    /* clear the IRQ flag */
    write_register(REG_IRQ_FLAGS, 0x08);

    /* put lora module on stand by */
    write_register(REG_OP_MODE, 0X81);
    sleep_ms(10);

    return 0;
}

int lora_receive(uint8_t *buffer, uint8_t max_len) {
    if (buffer == NULL || max_len == 0) {
        LORA_LOG("lora_receive invalid buffer");
        return -1;
    }

    /* put the lora module in standby */
    write_register(REG_OP_MODE, 0X81);
    sleep_ms(10);

    /* set FIFO ptr to base RX  */
    int set_rx_base = write_register_verified(REG_FIFO_ADDR_PTR, 0x00);
    if (set_rx_base != 0) {
        LORA_LOG("lora receive Error setting RX base to 0x00\n");
        return -1;
    }

    /* set RX module to continuous */
    write_register(REG_OP_MODE, 0x85);

    /* wait for RxDone */
    int timeout_rx_ms = RX_TIMEOUT_MS;
    uint8_t irq_flags = 0x00;
    while (timeout_rx_ms > 0) {
        irq_flags = read_register(REG_IRQ_FLAGS);
        if (irq_flags & 0x40) {
            break;
        }
        sleep_ms(10);
        timeout_rx_ms -= 10;
    }

    if (!(irq_flags & 0x40)) {
        LORA_LOG("lora_receive RX timeout");
        write_register(REG_IRQ_FLAGS, 0xFF);
        write_register(REG_OP_MODE, 0x81);
        return 0;
    }

    /* check crc error */
    if (irq_flags & 0x20) {
        LORA_LOG("lora receive Package is corrupt");
        write_register(REG_IRQ_FLAGS, 0xFF);
        write_register(REG_OP_MODE, 0x81);
        return -1;
    }

    /* get number of sent bytes */
    uint8_t received_len = read_register(REG_RX_NB_BYTES);
    if (received_len == 0) {
        LORA_LOG("lora receive Bytes have not been received");
        write_register(REG_IRQ_FLAGS, 0xFF);
        write_register(REG_OP_MODE, 0x81);
        return -1;
    }

    /* get FIFO current address */
    uint8_t rx_addr = read_register(REG_FIFO_RX_CURRENT);

    /* set FIFO ptr to current FIFO address */
    write_register(REG_FIFO_ADDR_PTR, rx_addr);

    if (received_len > max_len) {
        LORA_LOG("lora_receive buffer too small: got %d, max %d", received_len, max_len);
        received_len = max_len;
    }

    /* read in buffer every byte */
    for (int i=0;i<received_len;i++) {
        buffer[i] = read_register(REG_FIFO);
    }

    /* clear all the flags */
    write_register(REG_IRQ_FLAGS, 0xFF);

    /* put lora module on stand by */
    write_register(REG_OP_MODE, 0X81);
    sleep_ms(10);

    return received_len;
}

