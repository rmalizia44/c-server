#include "common.h"
#include "config.h"

#include <string.h>

struct server_s;
typedef struct server_s server_t;

typedef struct client_s {
    //server_t* server;
    uv_tcp_t* tcp;
    
} client_t;

struct server_s {
    uv_tcp_t tcp;
    config_t config;
    client_t* clients;
    unsigned seed;
};

client_t* client_new(server_t* server, uv_tcp_t* tcp) {
    unsigned id;
    client_t* client;
    
    for(unsigned i = 0; i < server->config.max_clients; ++i) {
        id = server->seed;
        server->seed = (server->seed + 1) % server->config.max_clients;
        client = server->clients + id;
        
        if(client->tcp == NULL) {
            tcp->data = client;
            //client->server = server;
            client->tcp = tcp;
            return client;
        }
    }
    return NULL;
}
void client_del(client_t* client) {
    client->tcp = NULL;
}
void client_on_write(uv_write_t *req, int status) {
    if(status < 0) {
        ERROR_SHOW(status);
    }
    heap_del(req); // TODO: optimize
}

void client_send(client_t* client, const void* data, unsigned size) {
    char* ptr;
    uv_buf_t buf;
    uv_write_t* req;
    
    ptr = (char*)heap_new(sizeof(uv_write_t) + size); // TODO: optimize
    req = (uv_write_t*)ptr;
    buf = uv_buf_init(ptr + sizeof(uv_write_t), size);
    memcpy(buf.base, data, size);
    
    ERROR_CHECK(
        uv_write(req, (uv_stream_t*)client->tcp, &buf, 1, client_on_write)
    );
}

void client_on_recv(client_t* client, const void* data, unsigned size) {
    LOG("received %u bytes", size);
    client_send(client, data, size); // TODO: replace this echo
}

void client_on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = heap_new(suggested_size); // TODO: optimize
    buf->len = suggested_size;
}
void client_on_close(uv_handle_t* handle) {
    heap_del(handle);
}
void client_close(client_t* client) {
    uv_handle_t* handle = (uv_handle_t*)client->tcp;
    client_del(client);
    uv_close(handle, client_on_close);
}
void client_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if(nread < 0) {
        if(nread != UV_EOF) {
            ERROR_SHOW(nread);
        }
        client_close((client_t*)stream->data);
    } else {
        client_on_recv((client_t*)stream->data, buf->base, nread);
    }
    heap_del(buf->base); // TODO: optimize
}

void server_on_listen(uv_stream_t* server, int status) {
    uv_tcp_t* tcp;
    client_t* client;
    
    ERROR_CHECK(status);
    
    tcp = (uv_tcp_t*)heap_new(sizeof(uv_tcp_t)); // TODO: optimize
    
    ERROR_CHECK(
        uv_tcp_init(server->loop, tcp)
    );
    ERROR_CHECK(
        uv_accept(server, (uv_stream_t*)tcp)
    );
    
    client = client_new((server_t*)server, tcp);
    if(client == NULL) {
        LOG("client rejected");
        uv_close((uv_handle_t*)tcp, client_on_close);
        return;
    }
    
    ERROR_CHECK(
        uv_read_start((uv_stream_t*)tcp, client_on_alloc, client_on_read)
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
    server_t server;
    struct sockaddr_in addr;
    uv_signal_t signal;
    
    ERROR_CHECK(
        config_load(&server.config)
    );
    server.clients = (client_t*)calloc(server.config.max_clients, sizeof(client_t));
    server.seed = 0;
    
    ERROR_CHECK(
        uv_loop_init(&loop)
    );
    ERROR_CHECK(
        uv_ip4_addr(server.config.host, server.config.port, &addr)
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
        uv_listen((uv_stream_t*)&server, server.config.backlog, server_on_listen)
    );
    ERROR_CHECK(
        uv_signal_init(&loop, &signal)
    );
    ERROR_CHECK(
        uv_signal_start(&signal, server_on_signal, SIGINT)
    );
    
    LOG("server started on %s:%u", server.config.host, server.config.port);
    
    ERROR_CHECK(
        uv_run(&loop, UV_RUN_DEFAULT)
    );
    ERROR_CHECK(
        uv_loop_close(&loop)
    );
}