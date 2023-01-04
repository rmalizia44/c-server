#ifndef SERVER_H
#define SERVER_H

struct client_s;
struct config_s;
struct uv_tcp_s;
struct uv_signal_s;
struct uv_loop_s;
struct server_s;

typedef struct server_reactor_s {
    void (*on_connect)(struct server_s* self, unsigned id);
    void (*on_disconnect)(struct server_s* self, unsigned id);
    void (*on_receive)(struct server_s* self, unsigned id, const char* data, unsigned size);
} server_reactor_t;

typedef struct server_s {
    struct uv_tcp_s* tcp;
    struct uv_signal_s* signal;
    const server_reactor_t* reactor;
    struct client_s* clients;
    unsigned max_clients;
    unsigned seed;
} server_t;

int server_init(server_t* self, struct uv_loop_s* loop, const struct config_s* config, const server_reactor_t* reactor);
void server_send(server_t* self, unsigned id, const char* data, unsigned size);
void server_broadcast(server_t* self, unsigned exclude, const char* data, unsigned size);
void server_kick(server_t* self, unsigned id);

#endif