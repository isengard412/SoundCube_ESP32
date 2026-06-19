#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button.h"

static const char *TAG = "soundcube_btn";

#define BUTTON_GPIO         39
#define DEBOUNCE_MS         50
#define LONG_PRESS_MS       3000

static button_cb_t s_callback = NULL;

static void button_task(void *arg)
{
    int last_state = 1;
    TickType_t press_start = 0;
    bool press_reported = false;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);
        if (level != last_state) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            level = gpio_get_level(BUTTON_GPIO);
            if (level != last_state) {
                last_state = level;
                if (level == 0) {
                    press_start = xTaskGetTickCount();
                    press_reported = false;
                    ESP_LOGI(TAG, "Button pressed");
                } else {
                    TickType_t elapsed = xTaskGetTickCount() - press_start;
                    if (elapsed < pdMS_TO_TICKS(LONG_PRESS_MS) && !press_reported) {
                        press_reported = true;
                        if (s_callback) {
                            ESP_LOGI(TAG, "Short press -> pairing mode");
                            s_callback();
                        }
                    }
                }
            }
        }

        if (last_state == 0 && !press_reported) {
            TickType_t elapsed = xTaskGetTickCount() - press_start;
            if (elapsed >= pdMS_TO_TICKS(LONG_PRESS_MS)) {
                press_reported = true;
                if (s_callback) {
                    ESP_LOGI(TAG, "Long press -> pairing mode");
                    s_callback();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t button_init(button_cb_t on_pairing_request)
{
    s_callback = on_pairing_request;
    xTaskCreatePinnedToCore(button_task, "btn_task", 2048, NULL, 5, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "Button initialized on GPIO %d", BUTTON_GPIO);
    return ESP_OK;
}