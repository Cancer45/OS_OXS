#ifndef SOCKS_H
#define SOCKS_H

#include "structs.h"

#define BACKLOG 5
#define MAX_PACKET (1024 * 100)
#define SOCK_PATH "/tmp/server.sock"

#define OK                   200
#define CREATED              201
#define NO_CONTENT           204
#define BAD_REQUEST          400
#define FORBIDDEN            403
#define NOT_FOUND            404
#define INTERNAL_SERVER_ERROR 500

int set_nonblocking(int);
int uds_socket_init();
int uds_socket_bind(int, const char*);
int uds_connect(int, const char*);

#endif
