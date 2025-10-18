#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* key;
    char* value;
} config_pair;

typedef struct {
    config_pair** configs;
    int nconfigs;
} config;

config* load(char* file, int len, char* fallback) {
    FILE *f = fopen(file, "r");
    int l = len;
    if (!len) l = 512;
    if (!f) {
        // file missing — create with defaults
        printf("%s not found, creating default file...\n",file);
        f = fopen(file, "w");
        if (!f) {
            perror("Failed to create file");
            return NULL;
        }
        fputs(fallback, f);
        fclose(f);
        f = fopen(file, "r");
    }

    char* line = malloc(l+1);
    char* key = malloc(l+1);
    char* value = malloc(l+1);

    int cpy = 0;
    int ksf = 0;

    config* c = malloc(sizeof(config));
    config_pair** configs = NULL;
    int nconfigs = 0;

    c->configs = configs;
    c->nconfigs = nconfigs;

    while (fgets(line, l+1, f)) {
        memset(key, 0, l+1);
        memset(value, 0, l+1);
        if (line[0] == '\n' || line[0] == '#') continue;
        cpy = 0;
        ksf = 0;
        // printf("%c\n",line[strlen(line)+1]);

        for (int i = 0; i < strlen(line); i++) {
            if (cpy && line[i]!='\n') value[ksf++] = line[i];
            // if (cpy && line[i]=='\n') {value[ksf++] = '\\';value[ksf++] = 'n';}

            if (!cpy && line[i]=='=') {
                strncpy(key, line, i);
                cpy = 1;
            }
        }

        nconfigs += 1;

        // if (configs) 
        configs = realloc(configs, sizeof(config_pair*)*nconfigs);
        // if (!configs) configs = malloc(sizeof(config_pair*));

        config_pair* pair = malloc(sizeof(config_pair));
        configs[nconfigs-1] = pair;
        pair->key=strdup(key);
        pair->value=strdup(value);

        // printf("\n");
        // printf("%s = %s\n", key, value);
        // printf("%s", line);
        memset(line, 0, l+1);
    }

    c->configs = configs;
    c->nconfigs = nconfigs;

    free(value);
    free(key);
    free(line);

    fclose(f);
    return c;
}

char* get_config(config* config, char* key) {
    for (int i = 0; i < config->nconfigs; i++) {
        // if (configs[i]->key==key) {
        if (!strcmp(config->configs[i]->key, key)) {
            return config->configs[i]->value;
        } 
    }
    return NULL;
}

int free_config(config* config) {
    config_pair** configs = config->configs;
    int nconfigs = config->nconfigs;

    for (int i = 0; i < nconfigs; i++) {
        free(configs[i]->value);
        free(configs[i]->key);
        free(configs[i]);
    }
    free(configs);
    nconfigs = 0;
    configs = NULL;

    free(config);

    return 0;
}

// int main() {
//     config* c = load("server.properties", 0,
//             "# Default Minecraft server properties\n"
//             "motd=My Minecraft Server\n"
//             "max-players=20\n"
//             "online-mode=true\n"
//             "view-distance=10\n"
//             "server-port= 35567 \n");

//     int port;
//     char* ports = get_config(c, "server-port");
//     sscanf(ports?ports:"25568", "%i", &port);
    
//     printf("Port: [%i]", port);
    
//     return 0;
// }