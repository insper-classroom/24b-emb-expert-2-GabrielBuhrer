#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ssd1306.h"
#include "gfx.h"

#define SPI_PORT spi0
#define SSD1306_DATA_CMD_SEL 20 
#define OLED_CS 17
#define OLED_DC 16
#define OLED_RESET 21

ssd1306_t disp;
int dma_chan;

void init_dma_for_spi() {
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_index(SPI_PORT) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
    dma_channel_configure(
        dma_chan,
        &c,
        &spi_get_hw(SPI_PORT)->dr,
        NULL,
        0,
        false
    );
}

void ssd1306_put_page_dma(uint8_t *data, uint8_t page, uint8_t column, uint8_t width) {
    ssd1306_set_page_address(page);
    ssd1306_set_column_address(column);

    gpio_put(SSD1306_DATA_CMD_SEL, 1);
    spi_cs_select();

    dma_channel_set_read_addr(dma_chan, data, false);
    dma_channel_set_transfer_count(dma_chan, width);
    dma_channel_start(dma_chan);
    dma_channel_wait_for_finish_blocking(dma_chan);

    spi_cs_deselect();
}

void gfx_show(ssd1306_t *p) {
    for (uint8_t page = 0; page < p->pages; page++) {
        ssd1306_put_page_dma(p->buffer + (page * p->width), page, 0, p->width);
    }
}

void vTaskADC(void *pvParameters) {
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    while (1) {
        uint16_t result = adc_read();
        printf("ADC Value: %d\n", result);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vTaskOLED(void *pvParameters) {
    // Inicializa o OLED
    ssd1306_init();

    while (1) {
        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

   
    init_dma_for_spi();

   
    TaskHandle_t xHandleADC = NULL;
    xTaskCreate(vTaskADC, "ADC Task", 256, NULL, 1, &xHandleADC);
    vTaskCoreAffinitySet(xHandleADC, 1 << 0); // Core 0

   
    TaskHandle_t xHandleOLED = NULL;
    xTaskCreate(vTaskOLED, "OLED Task", 256, NULL, 1, &xHandleOLED);
    vTaskCoreAffinitySet(xHandleOLED, 1 << 1); // Core 1

    
    vTaskStartScheduler();

    while (1);
}
