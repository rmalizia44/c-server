#ifndef SERVER_H
#define SERVER_H

struct client_s;
struct config_s;
struct uv_tcp_s;
struct uv_signal_s;
struct uv_loop_s;
struct server_s;
struct shared_s;

typedef struct server_buf_s {
    void* ptr;
    unsigned len;
} server_buf_t;

typedef struct server_s {
    const struct config_s* config;
    struct uv_tcp_s* tcp;
    struct client_s* clients;
    void (*on_new)(struct server_s* server, server_buf_t* buf);
    void (*on_del)(struct server_s* server, const server_buf_t* buf);
    void (*on_start)(struct server_s* server);
    void (*on_terminate)(struct server_s* server, int ec);
    void (*on_connect)(struct server_s* server, unsigned id);
    void (*on_disconnect)(struct server_s* server, unsigned id);
    void (*on_receive)(struct server_s* server, unsigned id, const char* data, unsigned size);
    unsigned max_clients;
    unsigned seed;
    int ec;
    void* data;
} server_t;

int server_init(server_t* server, struct uv_loop_s* loop, const struct config_s* config);
int server_start(server_t* server);
void server_close(server_t* server, int ec);
void server_send(server_t* server, unsigned id, struct shared_s* shared);
void server_broadcast(server_t* server, unsigned exclude, struct shared_s* shared);
void server_kick(server_t* server, unsigned id);
void server_kick_all(server_t* server);

#endif