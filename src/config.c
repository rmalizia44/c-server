#include "config.h"
#include <stdio.h> // snprintf

int config_load(config_t* self) {
    snprintf(self->host, sizeof(self->host), "0.0.0.0");
    self->port = 7171;
    self->backlog = 1024;
    self->max_clients = 100;
    return 0;
}
