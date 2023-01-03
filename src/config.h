#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_s {
    char host[16];
    unsigned port;
    unsigned backlog;
} config_t;


int config_load(config_t* config);

#endif