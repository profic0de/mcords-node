#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// Path to the config file
#define CONFIG_FILE "server.properties"

// Default contents for the config file
#define DEFAULT_CONTENT \
    "max-players=100\n" \
    "destroy-player-on-leave=true\n" \
    "enforce-target-connection=false\n" \
    "target-ip=127.0.0.1\n" \
    "target-port=25565\n"

void create_default_config_and_exit(void);
int read_config_int(const char *key);
bool read_config_bool(const char *key);
const char *read_config_string(const char *key);

#endif // CONFIG_H
