#include "common.h"
#include "config.h"
#include "server.h"
#include <stdio.h>
#include <uv.h>

void on_start(server_t* server) {
    printf("[*] server started on %s:%u\n", server->config->host, server->config->port);
}
void on_terminate(server_t* server) {
    printf("[*] server terminated\n");
}

void on_connect(server_t* server, unsigned id) {
    printf("[%u] connected\n", id);
}
void on_disconnect(server_t* server, unsigned id) {
    printf("[%u] disconnected\n", id);
}
void on_receive(server_t* server, unsigned id, const char* data, unsigned size) {
    printf("[%u] received %u bytes\n", id, size);
    server_broadcast(server, id, data, size);
}

int main() {
    uv_loop_t loop;
    config_t config = {0};
    server_t server = {0};
    
    server.data = NULL;
    server.on_start = on_start;
    server.on_terminate = on_terminate;
    server.on_connect = on_connect;
    server.on_disconnect = on_disconnect;
    server.on_receive = on_receive;
    
    ERROR_CHECK(
        uv_loop_init(&loop)
    );
    ERROR_CHECK(
        config_load(&config)
    );
    ERROR_CHECK(
        server_init(&server, &loop, &config)
    );
    
    ERROR_CHECK(
        uv_run(&loop, UV_RUN_DEFAULT)
    );
    ERROR_CHECK(
        uv_loop_close(&loop)
    );
    return 0;
}