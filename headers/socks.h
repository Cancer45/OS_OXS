#ifndef SOCKS_H
#define SOCKS_H

#define SOCK_PATH "/tmp/server.sock"
#define BACKLOG 5

int uds_socket_init();
int uds_socket_bind(int, const char*);

#endif
