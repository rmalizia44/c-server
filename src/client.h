#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>

struct server_s;
struct uv_tcp_s;
struct uv_timer_s;
struct shared_s;

typedef struct client_s {
    struct server_s* server;
    struct uv_tcp_s* tcp;
    struct uv_timer_s* timer;
    
} client_t;

int client_init_start(client_t* client, struct server_s* server, struct uv_tcp_s* tcp);
void client_close(client_t* client);
void client_send(client_t* client, struct shared_s* shared);

#endif