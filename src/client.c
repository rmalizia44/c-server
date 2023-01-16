#include "client.h"
#include "common.h"
#include "config.h"
#include "server.h"
#include "shared.h"
#include <string.h>
#include <stdbool.h>

static void client_set_state(client_t* client, int state) {
    client->state = state;
}
static void client_on_close_handle(uv_handle_t* handle) {
    client_t* client = (client_t*)handle->data;
    handle->data = NULL;
    if(client->tcp.data == NULL && client->timer.data == NULL) {
        client_set_state(client, CLIENT_STOPPED);
    }
}
static unsigned client_id(client_t* client) {
    server_t* server = client->server;
    return client - server->clients;
}
static void client_close_handles(client_t* client) {
    bool will_close = false;
    if(client->tcp.data != NULL) {
        will_close = true;
        uv_close((uv_handle_t*)&client->tcp, client_on_close_handle);
    }
    if(client->timer.data != NULL) {
        will_close = true;
        uv_close((uv_handle_t*)&client->timer, client_on_close_handle);
    }
    client_set_state(client, will_close ? CLIENT_PARTIAL : CLIENT_STOPPED);
}
void client_close(client_t* client) {
    server_t* server = client->server;
    unsigned id = client_id(client);
    
    if(client->state != CLIENT_RUNNING) {
        LOG("trying to close not running client")
        return;
    }
    
    client_close_handles(client);
    
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
    ec = uv_timer_start(&client->timer, client_on_time, timeout, 0);
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
    ec = uv_write(req, (uv_stream_t*)&client->tcp, &buf, 1, client_on_write);
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

int client_init_start(client_t* client, server_t* server) {
    int ec;
    
    if(client->state != CLIENT_STOPPED) {
        LOG("can't initialize non stopped client");
        return UV_UNKNOWN;
    }
    
    client->server = server;
    client->tcp.data = NULL;
    client->timer.data = NULL;
    
    ec = uv_tcp_init(server->tcp->loop, &client->tcp);
    if(ec != 0) {
        ERROR_SHOW(ec);
        return ec;
    }
    
    client->tcp.data = client;
    
    ec = uv_tcp_nodelay(&client->tcp, 1);
    if(ec != 0) {
        ERROR_SHOW(ec);
        client_close_handles(client);
        return ec;
    }
    
    ec = uv_accept((uv_stream_t*)server->tcp, (uv_stream_t*)&client->tcp);
    if(ec != 0) {
        ERROR_SHOW(ec);
        client_close_handles(client);
        return ec;
    }
    
    ec = uv_timer_init(server->tcp->loop, &client->timer);
    if(ec != 0) {
        ERROR_SHOW(ec);
        client_close_handles(client);
        return ec;
    }
    client->timer.data = client;
    
    ec = uv_read_start((uv_stream_t*)&client->tcp, client_on_alloc, client_on_read);
    if(ec != 0) {
        ERROR_SHOW(ec);
        client_close_handles(client);
        return ec;
    }
    
    client_set_state(client, CLIENT_RUNNING);
    
    if(server->on_connect != NULL) {
        server->on_connect(client->server, client_id(client));
    }
    
    client_reset_timer(client);
    
    return 0;
}