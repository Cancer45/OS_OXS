#ifndef HANDLERS_H
#define HANDLERS_H

#include "structs.h"

void* request_handler(void*);

int create_user(struct ClientHandlerParams);
int create_exam(struct ClientHandlerParams);
int create_session(struct ClientHandlerParams);
int admit_candidate(struct ClientHandlerParams);
int collect_candidate(struct ClientHandlerParams);

void handle_accept(int efd, int sfd);
int  handle_client(int efd, int cfd, MYSQL *connection);
int get_user(struct ClientHandlerParams params);
int get_exam(struct ClientHandlerParams params);
int get_question(struct ClientHandlerParams params);
int get_option(struct ClientHandlerParams params);
int create_question(struct ClientHandlerParams params);
int create_option(struct ClientHandlerParams params);
typedef int (*Handler)(struct ClientHandlerParams);

#endif
