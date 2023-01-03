#include "config.h"
#include <stdio.h> // snprintf

int config_load(config_t* config) {
    snprintf(config->host, sizeof(config->host), "0.0.0.0");
    config->port = 7171;
    config->backlog = 1024;
    return 0;
}
