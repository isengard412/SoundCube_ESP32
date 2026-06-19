#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_gap_bt_api.h"
#include "equalizer.h"
#include "storage.h"

static int eq_cmd_handler(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: eq <band> <gain> | eq show | eq reset\n");
        return 1;
    }

    if (strcmp(argv[1], "show") == 0) {
        float gains[EQ_BANDS];
        equalizer_get_all(gains);
        printf(" 10-Band Equalizer:\n");
        printf(" -----------------------\n");
        for (int i = 0; i < EQ_BANDS; i++) {
            printf(" %5s: %+6.1f dB\n", EQ_LABELS[i], (double)gains[i]);
        }
        printf(" -----------------------\n");
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        equalizer_reset();
        storage_save_eq(NULL);
        printf("Equalizer reset to flat.\n");
        return 0;
    }

    if (argc < 3) {
        printf("Usage: eq <band> <gain>\n");
        return 1;
    }

    int band = atoi(argv[1]);
    float gain = (float)atof(argv[2]);

    esp_err_t ret = equalizer_set_band(band, gain);
    if (ret != ESP_OK) {
        printf("Error: band must be 0-%d, gain must be -12 to +12 dB\n", EQ_BANDS - 1);
        return 1;
    }

    float gains[EQ_BANDS];
    equalizer_get_all(gains);
    storage_save_eq(gains);
    printf("Band %d (%s) set to %.1f dB\n", band, EQ_LABELS[band], (double)gain);
    return 0;
}

static int save_cmd_handler(int argc, char **argv)
{
    float gains[EQ_BANDS];
    equalizer_get_all(gains);
    esp_err_t ret = storage_save_eq(gains);
    if (ret == ESP_OK) {
        printf("EQ settings saved to flash.\n");
    } else {
        printf("Failed to save EQ settings.\n");
    }
    return 0;
}

static int pair_cmd_handler(int argc, char **argv)
{
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    printf("Pairing mode enabled. Search for 'Soundcube' on your phone.\n");
    return 0;
}

static int help_cmd_handler(int argc, char **argv)
{
    printf("\nSoundcube Bluetooth Speaker - Commands:\n");
    printf("  eq <0-9> <gain>   Set EQ band gain (-12 to +12 dB)\n");
    printf("  eq show           Show current EQ settings\n");
    printf("  eq reset          Reset EQ to flat\n");
    printf("  save              Persist EQ settings to flash\n");
    printf("  pair              Enter Bluetooth pairing mode\n");
    printf("  help              Show this help\n");
    return 0;
}

esp_err_t console_init(void)
{
    const esp_console_cmd_t eq_cmd = {
        .command = "eq",
        .help = "Set/show/reset 10-band equalizer",
        .func = eq_cmd_handler,
    };
    const esp_console_cmd_t save_cmd = {
        .command = "save",
        .help = "Save EQ settings to flash",
        .func = save_cmd_handler,
    };
    const esp_console_cmd_t pair_cmd = {
        .command = "pair",
        .help = "Enter Bluetooth pairing mode",
        .func = pair_cmd_handler,
    };
    const esp_console_cmd_t help_cmd = {
        .command = "help",
        .help = "Show available commands",
        .func = help_cmd_handler,
    };

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "soundcube> ";
    repl_config.max_cmdline_length = 256;

    ESP_ERROR_CHECK(esp_console_cmd_register(&eq_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&save_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&pair_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&help_cmd));
    esp_console_register_help_command();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    return ESP_OK;
}