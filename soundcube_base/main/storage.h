#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"

esp_err_t storage_init(void);
esp_err_t storage_load_eq(float *gains);
esp_err_t storage_save_eq(const float *gains);

#endif