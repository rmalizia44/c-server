#include "client.h"
#include "common.h"
#include "server.h"
#include <uv.h>
#include <string.h>

unsigned client_id(client_t* client) {
    server_t* server = client->server;
    return client - server->clients;
}
client_t* client_new(server_t* server, uv_tcp_t* tcp) {
    unsigned id;
    client_t* client;
    
    for(unsigned i = 0; i < server->max_clients; ++i) {
        id = server->seed;
        server->seed = (server->seed + 1) % server->max_clients;
        client = server->clients + id;
        
        if(client->tcp == NULL) {
            tcp->data = client;
            client->server = server;
            client->tcp = tcp;
            server->reactor->on_connect(client->server, id);
            return client;
        }
    }
    return NULL;
}
void client_del(client_t* client) {
    server_t* server = client->server;
    unsigned id = client_id(client);
    
    client->tcp = NULL;
    server->reactor->on_disconnect(server, id);
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
    server_t* server = client->server;
    unsigned id = client_id(client);
    
    server->reactor->on_receive(server, id, data, size);
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
int client_read_start(client_t* client) {
    ERROR_CHECK(
        uv_read_start((uv_stream_t*)client->tcp, client_on_alloc, client_on_read)
    );
    return 0;
}