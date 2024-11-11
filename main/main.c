#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "ssd1306.h"
#include "gfx.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/dma.h"

#define CORE_0 (1 << 0)
#define CORE_1 (1 << 1)

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

QueueHandle_t xQueueADCValue;

// Inicialização dos pinos dos botões e LEDs
void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);
    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);
    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);
    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);
    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

// Modificação para uso de DMA no OLED
void ssd1306_put_page_dma(uint8_t *data, uint8_t page, uint8_t column, uint8_t width) {
    ssd1306_set_page_address(page);
    ssd1306_set_column_address(column);

    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, DREQ_SPI0_TX);

    dma_channel_configure(
        dma_chan,
        &c,
        &spi_get_hw(SPI_PORT)->dr, // Endereço de destino: registrador SPI
        data,                       // Endereço de origem: buffer de dados
        width,                      // Número de bytes a transferir
        true                        // Iniciar imediatamente
    );

    dma_channel_wait_for_finish_blocking(dma_chan);
    dma_channel_unclaim(dma_chan);
}

// Task para leitura do ADC, atribuída ao CORE 0
void adc_task(void *p) {
    adc_init();
    adc_gpio_init(26); // GPIO26 para ADC
    adc_select_input(0); // Seleciona o canal de ADC 0 (conectado ao GPIO26)
    uint16_t adc_value;

    while (true) {
        adc_value = adc_read(); // Lê o valor do ADC
        xQueueSend(xQueueADCValue, &adc_value, 0); // Envia o valor para a fila
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay de 100 ms
    }
}

// Task para atualizar o OLED, atribuída ao CORE 1
void oled_task(void *p) {
    ssd1306_init();
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);
    oled1_btn_led_init();

    uint16_t adc_value;
    float voltage;
    char buffer[20];

    while (true) {
        if (xQueueReceive(xQueueADCValue, &adc_value, portMAX_DELAY) == pdTRUE) {
            gfx_clear_buffer(&disp);

            // Calcula a tensão com base no valor do ADC
            voltage = (adc_value / 4095.0) * 3.3;
            sprintf(buffer, "Voltage: %.2f V", voltage);
            gfx_draw_string(&disp, 0, 0, 1, buffer);

            // Desenha uma barra proporcional ao valor do ADC
            int bar_length = (adc_value / 4095.0) * 128;
            gfx_draw_line(&disp, 0, 20, bar_length, 20);

            gfx_show(&disp);
        }
    }
}

int main() {
    stdio_init_all();
    xQueueADCValue = xQueueCreate(32, sizeof(uint16_t));

    TaskHandle_t xHandleADC;
    TaskHandle_t xHandleOLED;

    // Cria as tasks sem definir afinidade explícita
    xTaskCreate(adc_task, "ADC Task", 256, NULL, 1, &xHandleADC);
    xTaskCreate(oled_task, "OLED Task", 256, NULL, 1, &xHandleOLED);

    // Define afinidade das tasks para os núcleos
    vTaskCoreAffinitySet(xHandleADC, CORE_0); // Task ADC no CORE 0
    vTaskCoreAffinitySet(xHandleOLED, CORE_1); // Task OLED no CORE 1

    vTaskStartScheduler();

    while (true) {
        ;
    }
}
