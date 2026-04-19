#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

int load_config(const char* filename, Config* config) {
    FILE* fp = fopen(filename, "r");
    char line[256];

    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char key[128];
        char value[128];

        if (sscanf(line, "%127[^=]=%127s", key, value) != 2) {
            continue;
        }

        if (strcmp(key, "ip") == 0) {
            strncpy(config->ip, value, sizeof(config->ip) - 1);
            config->ip[sizeof(config->ip) - 1] = '\0';
        } else if (strcmp(key, "start_port") == 0) {
            config->start_port = atoi(value);
        } else if (strcmp(key, "end_port") == 0) {
            config->end_port = atoi(value);
        } else if (strcmp(key, "interval") == 0) {
            config->interval = atoi(value);
        } else if (strcmp(key, "web_port") == 0) {
            config->web_port = atoi(value);
        } 
    }

    fclose(fp);
    return 0;
}