#include "common.h"
#include "config.h"
#include "server.h"
#include <stdio.h>
#include <uv.h>

#define ERROR_CHECK(expr) {\
    const int _ec = (expr);\
    if(_ec != 0) {\
        ERROR_SHOW(_ec);\
        exit(_ec);\
    }\
}

void on_new(struct server_s* server, server_buf_t* buf) {
    buf->len = 1;
    buf->ptr = heap_new(buf->len);
}
void on_del(struct server_s* server, const server_buf_t* buf) {
    heap_del(buf->ptr);
}

void on_start(server_t* server) {
    printf("[*] server started on %s:%u\n", server->config->host, server->config->port);
}
void on_terminate(server_t* server, int ec) {
    printf("[*] server terminated: %i\n", ec);
    if(ec != 0) {
        ERROR_SHOW(ec);
    }
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

void on_signal_interrupt(uv_signal_t* handle, int signum) {
    server_t* server = (server_t*)handle->data;
    LOG("interrupted");
    server_close(server, UV_EINTR);
    uv_close((uv_handle_t*)handle, NULL);
}

int main() {
    uv_loop_t loop;
    uv_signal_t signal;
    config_t config;
    server_t server;
    
    LOG("app started");
    
    ERROR_CHECK(
        uv_loop_init(&loop)
    );
    ERROR_CHECK(
        config_load(&config)
    );
    ERROR_CHECK(
        uv_signal_init(&loop, &signal)
    );
    ERROR_CHECK(
        server_init(&server, &loop, &config)
    );
    
    signal.data = &server;
    
    server.data = NULL;
    server.on_new = on_new;
    server.on_del = on_del;
    server.on_start = on_start;
    server.on_terminate = on_terminate;
    server.on_connect = on_connect;
    server.on_disconnect = on_disconnect;
    server.on_receive = on_receive;
    
    ERROR_CHECK(
        uv_signal_start(&signal, on_signal_interrupt, SIGINT)
    );
    ERROR_CHECK(
        server_start(&server)
    );
    ERROR_CHECK(
        uv_run(&loop, UV_RUN_DEFAULT)
    );
    ERROR_CHECK(
        uv_loop_close(&loop)
    );
    
    LOG("app terminated");
    
    return 0;
}