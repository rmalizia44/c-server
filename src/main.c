#include "common.h"
#include "config.h"

#include <string.h>

typedef struct server_s {
    uv_tcp_t tcp;
    
} server_t;

typedef struct client_s {
    uv_tcp_t tcp;
    
} client_t;

client_t* client_new(server_t* server) {
    client_t* client;
    client = (client_t*)malloc(sizeof(client_t)); // TODO: optimize
    LOG("client connected");
    return client;
}
void client_del(client_t* client) {
    LOG("client closed");
    free(client); // TODO: optimize
}

void on_client_write(uv_write_t *req, int status) {
    if(status < 0) {
        ERROR_SHOW(status);
    }
    free(req); // TODO: optimize
}

void client_send(client_t* client, const void* data, unsigned size) {
    char* ptr;
    uv_buf_t buf;
    uv_write_t* req;
    
    ptr = (char*)malloc(sizeof(uv_write_t) + size); // TODO: optimize
    req = (uv_write_t*)ptr;
    buf = uv_buf_init(ptr + sizeof(uv_write_t), size);
    memcpy(buf.base, data, size);
    
    ERROR_CHECK(
        uv_write(req, (uv_stream_t*)client, &buf, 1, on_client_write)
    );
}

void on_client_recv(client_t* client, const void* data, unsigned size) {
    LOG("received %u bytes", size);
    client_send(client, data, size); // TODO: replace this echo
}

void on_client_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size); // TODO: optimize
    buf->len = suggested_size;
}
void on_client_close(uv_handle_t* handle) {
    client_t* client = (client_t*)handle;
    client_del(client);
}
void client_kick(client_t* client) {
    uv_close((uv_handle_t*)client, on_client_close);
}
void on_client_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if(nread < 0) {
        if(nread != UV_EOF) {
            ERROR_SHOW(nread);
        }
        client_kick((client_t*)stream);
    } else {
        on_client_recv((client_t*)stream, buf->base, nread);
    }
    free(buf->base); // TODO: optimize
}
void on_server_reject(uv_handle_t* handle) {
    free(handle); // TODO: optimize
}
void on_server_listen(uv_stream_t* server, int status) {
    client_t* client;
    uv_tcp_t* tcp;
    
    ERROR_CHECK(status);
    
    client = client_new((server_t*)server);
    if(client == NULL) {
        tcp = (uv_tcp_t*)malloc(sizeof(uv_tcp_t)); // TODO: optimize
    } else {
        tcp = (uv_tcp_t*)client;
    }
    ERROR_CHECK(
        uv_tcp_init(server->loop, tcp)
    );
    ERROR_CHECK(
        uv_accept(server, (uv_stream_t*)tcp)
    );
    if(client == NULL) {
        LOG("client rejected");
        uv_close((uv_handle_t*)tcp, on_server_reject);
        return;
    }
    ERROR_CHECK(
        uv_read_start((uv_stream_t*)tcp, on_client_alloc, on_client_read)
    );
}
void server_on_signal(uv_signal_t* handle, int signum) {
    switch(signum) {
        case SIGINT:
            LOG("server interrupted by SIGINT");
            break;
        default:
            LOG("server interrupted by unexpected signal: %i", signum);
    }
    exit(0);
}

int main() {
    uv_loop_t loop;
    config_t config;
    server_t server;
    struct sockaddr_in addr;
    uv_signal_t signal;
    
    ERROR_CHECK(
        config_load(&config)
    );
    
    ERROR_CHECK(
        uv_loop_init(&loop)
    );
    ERROR_CHECK(
        uv_ip4_addr(config.host, config.port, &addr)
    );
    ERROR_CHECK(
        uv_tcp_init(&loop, (uv_tcp_t*)&server)
    );
    ERROR_CHECK(
        uv_tcp_nodelay((uv_tcp_t*)&server, 1)
    );
    ERROR_CHECK(
        uv_tcp_bind((uv_tcp_t*)&server, (const struct sockaddr*)&addr, 0)
    );
    ERROR_CHECK(
        uv_listen((uv_stream_t*)&server, config.backlog, on_server_listen)
    );
    ERROR_CHECK(
        uv_signal_init(&loop, &signal)
    );
    ERROR_CHECK(
        uv_signal_start(&signal, server_on_signal, SIGINT)
    );
    
    LOG("server started on %s:%u", config.host, config.port);
    
    ERROR_CHECK(
        uv_run(&loop, UV_RUN_DEFAULT)
    );
    ERROR_CHECK(
        uv_loop_close(&loop)
    );
}