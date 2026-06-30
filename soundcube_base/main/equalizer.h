#ifndef EQUALIZER_H
#define EQUALIZER_H

#include <stdint.h>
#include "esp_err.h"

#define EQ_BANDS 10

/* Define to mix both stereo input channels into a single mono signal before
 * EQ processing. Both output channels will carry the identical L+R mix.
 * Comment out to restore independent stereo processing. */
#define EQUALIZER_MONO_MIX

extern const char *EQ_LABELS[EQ_BANDS];

esp_err_t equalizer_init(void);
void equalizer_process(int16_t *buffer, uint32_t samples);
float equalizer_get_band(int band);
esp_err_t equalizer_set_band(int band, float gain_db);
void equalizer_get_all(float *gains);
void equalizer_set_all(const float *gains);
void equalizer_reset(void);

#endif