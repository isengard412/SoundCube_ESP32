#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "equalizer.h"
#define SAMPLE_RATE 44100

static const char *TAG = "soundcube_eq";

const char *EQ_LABELS[EQ_BANDS] = {
    "31Hz", "62Hz", "125Hz", "250Hz", "500Hz",
    "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"
};

static const float EQ_FREQS[EQ_BANDS] = {
    31.0f, 62.0f, 125.0f, 250.0f, 500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

#define Q_FACTOR 1.0f
#define MIN_GAIN -12.0f
#define MAX_GAIN 12.0f

typedef struct {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} biquad_t;

typedef struct {
    biquad_t left[EQ_BANDS];
    biquad_t right[EQ_BANDS];
    float gains[EQ_BANDS];
    bool active[EQ_BANDS];   /* true when gain != 0 dB */
    int  active_count;       /* number of bands with non-zero gain */
} eq_state_t;

static eq_state_t s_eq;

static void biquad_peaking(biquad_t *f, float freq, float gain_db, float Q, float fs)
{
    float A = powf(10.0f, gain_db / 40.0f);
    float omega = 2.0f * (float)M_PI * freq / fs;
    float alpha = sinf(omega) / (2.0f * Q);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosf(omega);
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosf(omega);
    float a2 = 1.0f - alpha / A;

    float inv_a0 = 1.0f / a0;
    f->b0 = b0 * inv_a0;
    f->b1 = b1 * inv_a0;
    f->b2 = b2 * inv_a0;
    f->a1 = a1 * inv_a0;
    f->a2 = a2 * inv_a0;
}

static void biquad_reset(biquad_t *f)
{
    f->x1 = f->x2 = f->y1 = f->y2 = 0.0f;
}

static float biquad_process(biquad_t *f, float x)
{
    float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
              - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;
    return y;
}

static void eq_recompute(eq_state_t *eq)
{
    eq->active_count = 0;
    for (int i = 0; i < EQ_BANDS; i++) {
        eq->active[i] = (fabsf(eq->gains[i]) > 0.05f);
        if (eq->active[i]) {
            eq->active_count++;
            biquad_peaking(&eq->left[i],  EQ_FREQS[i], eq->gains[i], Q_FACTOR, SAMPLE_RATE);
            biquad_peaking(&eq->right[i], EQ_FREQS[i], eq->gains[i], Q_FACTOR, SAMPLE_RATE);
        }
        biquad_reset(&eq->left[i]);
        biquad_reset(&eq->right[i]);
    }
}

esp_err_t equalizer_init(void)
{
    memset(&s_eq, 0, sizeof(s_eq));
    equalizer_reset();
    ESP_LOGI(TAG, "Equalizer initialized (%d bands, %d Hz)", EQ_BANDS, SAMPLE_RATE);
    return ESP_OK;
}

void equalizer_process(int16_t *buffer, uint32_t samples)
{
    /* samples is the total number of int16_t values; each stereo frame = 2 values */
    uint32_t frames = samples / 2;

#ifdef EQUALIZER_MONO_MIX
    for (uint32_t i = 0; i < frames; i++) {
        /* Sum L+R into mono before EQ — only one filter chain needed */
        float mono = ((float)buffer[i * 2] + (float)buffer[i * 2 + 1]) * 0.5f;

        for (int b = 0; b < EQ_BANDS; b++) {
            if (!s_eq.active[b]) continue;
            mono = biquad_process(&s_eq.left[b], mono);
        }

        int16_t out = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, mono));
        buffer[i * 2]     = out;
        buffer[i * 2 + 1] = out;
    }
#else
    /* Fast path: nothing to do when all bands are flat */
    if (s_eq.active_count == 0) return;

    for (uint32_t i = 0; i < frames; i++) {
        float left  = (float)buffer[i * 2];
        float right = (float)buffer[i * 2 + 1];

        for (int b = 0; b < EQ_BANDS; b++) {
            if (!s_eq.active[b]) continue;
            left  = biquad_process(&s_eq.left[b],  left);
            right = biquad_process(&s_eq.right[b], right);
        }

        buffer[i * 2]     = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, left));
        buffer[i * 2 + 1] = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, right));
    }
#endif
}

float equalizer_get_band(int band)
{
    if (band < 0 || band >= EQ_BANDS) return 0.0f;
    return s_eq.gains[band];
}

esp_err_t equalizer_set_band(int band, float gain_db)
{
    if (band < 0 || band >= EQ_BANDS) return ESP_ERR_INVALID_ARG;
    gain_db = fmaxf(MIN_GAIN, fminf(MAX_GAIN, gain_db));
    s_eq.gains[band] = gain_db;
    s_eq.active[band] = (fabsf(gain_db) > 0.05f);
    s_eq.active_count = 0;
    for (int i = 0; i < EQ_BANDS; i++) {
        if (s_eq.active[i]) s_eq.active_count++;
    }
    biquad_peaking(&s_eq.left[band],  EQ_FREQS[band], gain_db, Q_FACTOR, SAMPLE_RATE);
    biquad_peaking(&s_eq.right[band], EQ_FREQS[band], gain_db, Q_FACTOR, SAMPLE_RATE);
    biquad_reset(&s_eq.left[band]);
    biquad_reset(&s_eq.right[band]);
    ESP_LOGI(TAG, "Band %d (%s) set to %.1f dB", band, EQ_LABELS[band], gain_db);
    return ESP_OK;
}

void equalizer_get_all(float *gains)
{
    memcpy(gains, s_eq.gains, sizeof(s_eq.gains));
}

void equalizer_set_all(const float *gains)
{
    memcpy(s_eq.gains, gains, sizeof(s_eq.gains));
    eq_recompute(&s_eq);
}

void equalizer_reset(void)
{
    for (int i = 0; i < EQ_BANDS; i++) {
        s_eq.gains[i] = 0.0f;
    }
    eq_recompute(&s_eq);
    ESP_LOGI(TAG, "Equalizer reset to flat");
}