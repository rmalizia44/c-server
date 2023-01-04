#include "common.h"
#include "config.h"
#include "server.h"
#include <stdio.h>
#include <uv.h>

void on_connect(server_t* self, unsigned id) {
    printf("[%u] connected\n", id);
}
void on_disconnect(server_t* self, unsigned id) {
    printf("[%u] disconnected\n", id);
}
void on_receive(server_t* self, unsigned id, const char* data, unsigned size) {
    printf("[%u] received %u bytes\n", id, size);
    server_broadcast(self, id, data, size);
}

int main() {
    uv_loop_t loop;
    config_t config;
    server_t server;
    server_reactor_t reactor = { on_connect, on_disconnect, on_receive };
    
    ERROR_CHECK(
        uv_loop_init(&loop)
    );
    ERROR_CHECK(
        config_load(&config)
    );
    ERROR_CHECK(
        server_init(&server, &loop, &config, &reactor)
    );
    
    printf("server started on %s:%u\n", config.host, config.port);
    
    ERROR_CHECK(
        uv_run(&loop, UV_RUN_DEFAULT)
    );
    ERROR_CHECK(
        uv_loop_close(&loop)
    );
    return 0;
}