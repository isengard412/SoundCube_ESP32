#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "equalizer.h"
#include "storage.h"

static const char *TAG = "soundcube_storage";
static const char *NVS_NAMESPACE = "soundcube";
static const char *NVS_KEY_EQ = "eq_gains";

esp_err_t storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) return ret;
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "NVS initialized");
    return ESP_OK;
}

esp_err_t storage_load_eq(float *gains)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No EQ settings found, using flat");
        return ESP_ERR_NOT_FOUND;
    }

    size_t size = EQ_BANDS * sizeof(float);
    ret = nvs_get_blob(handle, NVS_KEY_EQ, gains, &size);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded EQ settings from NVS");
    } else {
        ESP_LOGW(TAG, "No saved EQ settings, using flat");
    }
    return ret;
}

esp_err_t storage_save_eq(const float *gains)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    if (gains) {
        ret = nvs_set_blob(handle, NVS_KEY_EQ, gains, EQ_BANDS * sizeof(float));
    } else {
        float flat[EQ_BANDS] = {0};
        ret = nvs_set_blob(handle, NVS_KEY_EQ, flat, EQ_BANDS * sizeof(float));
    }

    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "EQ settings saved to NVS");
    }
    return ret;
}