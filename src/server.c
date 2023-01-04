#include "server.h"
#include "common.h"
#include "config.h"
#include "client.h"
#include <uv.h>

static void server_on_reject(uv_handle_t* handle) {
    heap_del(handle);
}
static void server_on_listen(uv_stream_t* stream, int status) {
    uv_tcp_t* tcp;
    client_t* client;
    
    ERROR_CHECK(status);
    
    tcp = (uv_tcp_t*)heap_new(sizeof(uv_tcp_t)); // TODO: optimize
    
    ERROR_CHECK(
        uv_tcp_init(stream->loop, tcp)
    );
    ERROR_CHECK(
        uv_accept(stream, (uv_stream_t*)tcp)
    );
    
    client = client_new((server_t*)stream->data, tcp);
    if(client == NULL) {
        LOG("client rejected");
        uv_close((uv_handle_t*)tcp, server_on_reject);
        return;
    }
    
    ERROR_CHECK(
        client_start(client)
    );
}
static void server_on_signal(uv_signal_t* handle, int signum) {
    server_t* server = (server_t*)handle->data;
    
    /*switch(signum) {
        case SIGINT:
            LOG("server interrupted by SIGINT");
            break;
        default:
            LOG("server interrupted by unexpected signal: %i", signum);
    }*/
    
    if(server->on_terminate != NULL) {
        server->on_terminate(server);
    }
    
    exit(0);
}
static client_t* server_get_client(server_t* server, unsigned id) {
    client_t* client;
    
    if(id >= server->max_clients) {
        return NULL;
    }
    
    client = server->clients + id;
    if(client->tcp == NULL) {
        return NULL;
    }
    
    return client;
}
void server_send(server_t* server, unsigned id, const char* data, unsigned size) {
    client_t* client = server_get_client(server, id);
    if(client == NULL) {
        LOG("invalid client id to send: %u", id)
        return;
    }
    client_send(client, data, size);
}
void server_broadcast(server_t* server, unsigned exclude, const char* data, unsigned size) {
    client_t* client;
    for(unsigned id = 0; id < server->max_clients; ++id) {
        if(id == exclude) {
            continue;
        }
        client = server_get_client(server, id);
        if(client != NULL) {
            client_send(client, data, size);
        }
    }
}
void server_kick(server_t* server, unsigned id) {
    client_t* client = server_get_client(server, id);
    if(client == NULL) {
        LOG("invalid client id to kick: %u", id)
        return;
    }
    client_close(client);
}
int server_init(server_t* server, uv_loop_t* loop, const config_t* config) {
    struct sockaddr_in addr;
    
    server->config = config;
    server->seed = 0;
    server->tcp = (uv_tcp_t*)heap_new(sizeof(uv_tcp_t));
    server->tcp->data = server;
    server->signal = (uv_signal_t*)heap_new(sizeof(uv_signal_t));
    server->signal->data = server;
    server->max_clients = config->max_clients;
    server->clients = (client_t*)heap_new(server->max_clients * sizeof(client_t));
    
    memset(server->clients, 0, server->max_clients * sizeof(client_t));
    
    ERROR_CHECK(
        uv_ip4_addr(config->host, config->port, &addr)
    );
    ERROR_CHECK(
        uv_tcp_init(loop, server->tcp)
    );
    ERROR_CHECK(
        uv_tcp_nodelay(server->tcp, 1)
    );
    ERROR_CHECK(
        uv_tcp_bind(server->tcp, (const struct sockaddr*)&addr, 0)
    );
    ERROR_CHECK(
        uv_listen((uv_stream_t*)server->tcp, config->backlog, server_on_listen)
    );
    ERROR_CHECK(
        uv_signal_init(loop, server->signal)
    );
    ERROR_CHECK(
        uv_signal_start(server->signal, server_on_signal, SIGINT)
    );
    
    if(server->on_start != NULL) {
        server->on_start(server);
    }
    
    return 0;
}