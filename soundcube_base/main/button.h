#ifndef BUTTON_H
#define BUTTON_H

#include "esp_err.h"

typedef void (*button_cb_t)(void);

esp_err_t button_init(button_cb_t on_pairing_request);

#endif