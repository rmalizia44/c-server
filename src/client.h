#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>

struct server_s;
struct uv_tcp_s;

typedef struct client_s {
    struct server_s* server;
    struct uv_tcp_s* tcp;
    
} client_t;



client_t* client_new(struct server_s* server, struct uv_tcp_s* tcp);
int client_read_start(client_t* client);
void client_send(client_t* client, const void* data, unsigned size);
void client_close(client_t* client);

#endif