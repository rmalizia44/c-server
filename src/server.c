#include "server.h"
#include "common.h"
#include "config.h"
#include "client.h"
#include <uv.h>

static client_t* server_client_unused(server_t* server) {
    unsigned id;
    client_t* client;
    
    for(unsigned i = 0; i < server->max_clients; ++i) {
        id = server->seed;
        server->seed = (server->seed + 1) % server->max_clients;
        client = server->clients + id;
        
        if(client->tcp == NULL) {
            return client;
        }
    }
    
    return NULL;
}
static void server_on_reject(uv_handle_t* handle) {
    heap_del(handle);
}
static void server_on_listen(uv_stream_t* stream, int ec) {
    uv_tcp_t* tcp;
    client_t* client;
    server_t* server = (server_t*)stream->data;
    
    if(ec != 0) {
        ERROR_SHOW(ec);
        server_close(server, ec);
        return;
    }
    
    tcp = (uv_tcp_t*)heap_new(sizeof(uv_tcp_t)); // TODO: optimize
    if(tcp == NULL) {
        ERROR_SHOW(UV_ENOMEM);
        return;
    }
    ec = uv_tcp_init(stream->loop, tcp)
        || uv_accept(stream, (uv_stream_t*)tcp);
    if(ec != 0) {
        heap_del(tcp);
        ERROR_SHOW(ec);
        return;
    }
    
    client = server_client_unused(server);
    if(client == NULL) {
        LOG("client rejected");
        uv_close((uv_handle_t*)tcp, server_on_reject);
        return;
    }
    
    ec = client_init_start(client, server, tcp);
    if(ec != 0) {
        ERROR_SHOW(ec);
        uv_close((uv_handle_t*)tcp, server_on_reject);
        return;
    }
}
static void server_on_close(uv_handle_t* handle) {
    server_t* server = (server_t*)handle->data;
    
    if(server->on_terminate) {
        server->on_terminate(server, server->ec);
    }
    
    heap_del(server->clients);
    heap_del(handle);
}

static client_t* server_get_client_bound_unchecked(server_t* server, unsigned id) {
    client_t* client;
    
    client = server->clients + id;
    if(client->tcp == NULL) {
        return NULL;
    }
    
    return client;
}
static client_t* server_get_client(server_t* server, unsigned id) {
    if(id >= server->max_clients) {
        return NULL;
    }
    return server_get_client_bound_unchecked(server, id);
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
        client = server_get_client_bound_unchecked(server, id);
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
void server_kick_all(server_t* server) {
    client_t* client;
    for(unsigned id = 0; id < server->max_clients; ++id) {
        client = server_get_client_bound_unchecked(server, id);
        if(client != NULL) {
            client_close(client);
        }
    }
}

int server_init(server_t* server, uv_loop_t* loop, const config_t* config) {
    int ec = UV_ENOMEM;
    
    memset(server, 0, sizeof(server_t));
    
    server->seed = 0;
    server->config = config;
    server->max_clients = config->max_clients;
    
    server->tcp = (uv_tcp_t*)heap_new(sizeof(uv_tcp_t));
    if(server->tcp != NULL) {
        server->tcp->data = server;
        
        server->clients = (client_t*)heap_new(server->max_clients * sizeof(client_t));
        if(server->clients != NULL) {
            memset(server->clients, 0, server->max_clients * sizeof(client_t));
            
            ec = uv_tcp_init(loop, server->tcp);
            if(ec == 0) {
                return 0;
            }
            
            heap_del(server->clients);
        }
        
        heap_del(server->tcp);
    }
    
    return ec;
}
int server_start(server_t* server) {
    int ec;
    struct sockaddr_in addr;
    
    ec = uv_ip4_addr(server->config->host, server->config->port, &addr);
    if(ec != 0) { return ec; }
    
    ec = uv_tcp_nodelay(server->tcp, 1);
    if(ec != 0) { return ec; }
    
    ec = uv_tcp_bind(server->tcp, (const struct sockaddr*)&addr, 0);
    if(ec != 0) { return ec; }
    
    ec = uv_listen((uv_stream_t*)server->tcp, server->config->backlog, server_on_listen);
    if(ec != 0) { return ec; }
    
    if(server->on_start != NULL) {
        server->on_start(server);
    }
    
    return 0;
}
void server_close(server_t* server, int ec) {
    if(server->tcp != NULL) {
        server->ec = ec;
        server_kick_all(server);
        uv_close((uv_handle_t*)server->tcp, server_on_close);
        server->tcp = NULL;
    }
}