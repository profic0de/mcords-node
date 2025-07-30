#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "config.h"

// #define CONFIG_FILE "server.properties"
// #define DEFAULT_CONTENT "max-players=100\ndestroy-player-on-leave=true\nenforce-target-connection=false\ntarget-ip=127.0.0.1\ntarget-port=25565\n"

#define MAX_CONFIG_LINES 128
#define MAX_LINE_LENGTH 256

static char *config_lines[MAX_CONFIG_LINES] = {0};
static int config_loaded = 0;

void create_default_config_and_exit() {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        perror("Failed to create config file");
        exit(EXIT_FAILURE);
    }
    fputs(DEFAULT_CONTENT, f);
    fclose(f);
    printf("Config file '%s' created with default values. Please edit it and restart the server.\n", CONFIG_FILE);
    exit(EXIT_SUCCESS);
}

void load_config_if_needed() {
    if (config_loaded) return;

    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        printf("Config file '%s' not found. Creating default config and exiting.\n", CONFIG_FILE);
        create_default_config_and_exit();
    }

    char line[MAX_LINE_LENGTH];
    int count = 0;

    while (fgets(line, sizeof(line), f) && count < MAX_CONFIG_LINES) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        config_lines[count++] = strdup(line);
    }

    fclose(f);
    config_loaded = 1;
}

const char* find_config_value(const char *key) {
    load_config_if_needed();

    size_t key_len = strlen(key);
    for (int i = 0; config_lines[i]; i++) {
        if (strncmp(config_lines[i], key, key_len) == 0 && config_lines[i][key_len] == '=') {
            return config_lines[i] + key_len + 1; // pointer to value
        }
    }

    printf("Key '%s' not found in config. Exiting.\n", key);
    exit(EXIT_FAILURE);
}

int read_config_int(const char *key) {
    const char *val = find_config_value(key);
    return atoi(val);
}

bool read_config_bool(const char *key) {
    const char *val = find_config_value(key);

    char lower_val[16];
    strncpy(lower_val, val, sizeof(lower_val) - 1);
    lower_val[sizeof(lower_val) - 1] = '\0';

    for (char *p = lower_val; *p; ++p) *p = tolower(*p);

    if (strcmp(lower_val, "true") == 0 || strcmp(lower_val, "1") == 0) {
        return true;
    } else if (strcmp(lower_val, "false") == 0 || strcmp(lower_val, "0") == 0) {
        return false;
    } else {
        printf("Invalid boolean value for '%s': '%s'. Expected true/false or 1/0.\n", key, val);
        exit(EXIT_FAILURE);
    }
}

const char* read_config_string(const char *key) {
    return find_config_value(key);
}
