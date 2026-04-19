#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char ip[64];
    int start_port;
    int end_port;
    int interval;
    int web_port;
} Config;

int load_config(const char* filename, Config* config);

#endif