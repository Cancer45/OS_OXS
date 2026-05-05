#ifndef HANDLERS_H
#define HANDLERS_H

#include "structs.h"

void* request_handler(void*);

//create
void create_user(struct ClientHandlerParams);
void create_exam(struct ClientHandlerParams);
void create_session(struct ClientHandlerParams);

void get_user(struct ClientHandlerParams params);
void admit_candidate(struct ClientHandlerParams);
void collect_candidate(struct ClientHandlerParams);

//handle
void handle_accept(int efd, int sfd);
int  handle_client(int efd, int cfd, MYSQL *connection);

typedef void (*Handler)(struct ClientHandlerParams);

#endif
