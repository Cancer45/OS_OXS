#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "../headers/structs.h"
#include "../headers/handlers.h"
#include "../headers/socks.h"

#define BUF_SIZE 200
#define MAX_EVENTS 10

int main()
{
    //init socket FDs and epoll structs
    struct epoll_event ev, events[MAX_EVENTS];
    int  efd, sfd = uds_socket_init();

    //bind server socket to path
    uds_socket_bind(sfd, SOCK_PATH);

    //set server socket to passive/listening
    if (listen(sfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    //set server socket to not block
    set_nonblocking(sfd);

    //create epoll instance
    if ((efd = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        exit(1);
    }

    //set epoll to unblock on incoming 
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sfd;

    //add server socket to epoll tree
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) == -1)
    {
        perror("epoll_ctl: sfd");
        exit(1);
    }
    printf("server listening on %s...\n", SOCK_PATH);

    //init client_handler_params and connection_slots
    struct ConnectionSlot connection_slots[DB_CONN_LIMIT];
    int total_connections = 0;

    //semaphore free_slots tracks number of available connections
    sem_t free_slots;
    sem_init(&free_slots, 0, 0);

    //slots_lock guards connection_slots from concurrent access
    pthread_mutex_t slots_lock;
    pthread_mutex_init(&slots_lock, NULL);

    pthread_t worker_threads[MAX_EVENTS];
    struct RequestHandlerParams *req_h_params_array[MAX_EVENTS];
    int nfds;                                                                                                                                                                 
    while (1)
    {
        if ((nfds = epoll_wait(efd, events, MAX_EVENTS, -1)) == -1)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            req_h_params_array[i] = malloc(sizeof(struct RequestHandlerParams));
            *req_h_params_array[i] = (struct RequestHandlerParams) 
            { 
                .efd = efd, .sfd = sfd, .rfd = events[i].data.fd,
                .total_connections = &total_connections,
                .free_slots = &free_slots,
                .slots_lock = &slots_lock,
                .connection_slots = connection_slots, 
            };

            pthread_create(&worker_threads[i], NULL, request_handler, req_h_params_array[i]);
        }

        //join worker_threads around here
        for (int i = 0; i < nfds; i++)
        {
            pthread_join(worker_threads[i], NULL);
            free(req_h_params_array[i]);
        }
    }
}
