#include "client.h"
#include "common.h"
#include "config.h"
#include "server.h"
#include "shared.h"
#include <uv.h>
#include <string.h>

static void client_on_close(uv_handle_t* handle) {
    heap_del(handle);
}
static unsigned client_id(client_t* client) {
    server_t* server = client->server;
    return client - server->clients;
}
void client_close(client_t* client) {
    server_t* server = client->server;
    unsigned id = client_id(client);
    
    if(client->tcp == NULL) {
        return;
    }
    
    uv_close((uv_handle_t*)client->tcp, client_on_close);
    uv_close((uv_handle_t*)client->timer, client_on_close);
    
    client->tcp = NULL;
    client->timer = NULL;
    
    if(server->on_disconnect != NULL) {
        server->on_disconnect(server, id);
    }
}
static void client_on_time(uv_timer_t* handle) {
    client_close((client_t*)handle->data);
}
static void client_reset_timer(client_t* client) {
    int ec;
    unsigned timeout = client->server->config->timeout;
    ec = uv_timer_start(client->timer, client_on_time, timeout, 0);
    if(ec != 0) {
        ERROR_SHOW(ec);
        client_close(client);
    }
}
static void client_on_write(uv_write_t *req, int status) {
    if(status < 0) {
        ERROR_SHOW(status);
    }
    shared_del((shared_t*)req->data);
    heap_del(req); // TODO: optimize
}
void client_send(client_t* client, shared_t* shared) {
    int ec;
    uv_buf_t buf;
    uv_write_t* req;
    
    req = (uv_write_t*)heap_new(sizeof(uv_write_t)); // TODO: optimize
    if(req == NULL) {
        ERROR_SHOW(UV_ENOMEM);
        client_close(client);
        return;
    }
    buf = uv_buf_init(shared->data, shared->size);
    ec = uv_write(req, (uv_stream_t*)client->tcp, &buf, 1, client_on_write);
    if(ec != 0) {
        ERROR_SHOW(ec);
        heap_del(req);
        client_close(client);
    }
    req->data = shared_copy(shared);
}
static void client_on_recv(client_t* client, const void* data, unsigned size) {
    server_t* server = client->server;
    unsigned id = client_id(client);
    
    client_reset_timer(client);
    if(server->on_receive != NULL) {
        server->on_receive(server, id, data, size);
    }
}
static void client_on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    server_buf_t buffer;
    client_t* client = (client_t*)handle->data;
    server_t* server = client->server;
    
    if(server->on_new != NULL) {
        server->on_new(server, &buffer);
    } else {
        buffer.ptr = heap_new(suggested_size); // TODO: optimize
        buffer.len = suggested_size;
    }
    
    buf->base = buffer.ptr;
    buf->len = buffer.len;
}
static void client_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    server_buf_t buffer;
    client_t* client = (client_t*)stream->data;
    server_t* server = client->server;
    
    buffer.ptr = buf->base;
    buffer.len = buf->len;
    
    if(nread < 0) {
        if(nread != UV_EOF) {
            ERROR_SHOW(nread);
        }
        client_close(client);
    } else {
        client_on_recv(client, buf->base, nread);
    }
    
    if(server->on_del != NULL) {
        server->on_del(server, &buffer);
    } else {
        heap_del(buffer.ptr); // TODO: optimize
    }
    
}
int client_init_start(client_t* client, server_t* server, struct uv_tcp_s* tcp) {
    int ec = UV_ENOMEM;
    uv_timer_t* timer;
    
    timer = (uv_timer_t*)heap_new(sizeof(uv_timer_t));
    if(timer == NULL) {
        ERROR_SHOW(ec);
        return ec;
    }
    
    ec = uv_timer_init(tcp->loop, timer);
    if(ec != 0) {
        ERROR_SHOW(ec);
        heap_del(timer);
        return ec;
    }
    
    ec = uv_read_start((uv_stream_t*)tcp, client_on_alloc, client_on_read);
    if(ec != 0) {
        ERROR_SHOW(ec);
        uv_close((uv_handle_t*)timer, client_on_close);
        return ec;
    }
    
    client->server = server;
    client->tcp = tcp;
    client->tcp->data = client;
    client->timer = timer;
    client->timer->data = client;
    
    if(server->on_connect != NULL) {
        server->on_connect(client->server, client_id(client));
    }
    
    client_reset_timer(client);
    
    return 0;
}