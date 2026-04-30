#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../headers/structs.h"

#define BUF_SIZE 200
#define BACKLOG 5
#define SOCK_PATH "/tmp/serversock"

void perr_exit(char* err)
{
    perror(err);
    exit(1);
}

int main(int argc, char* argv[])
{
    // create socket: int socket(int domain, int type, int protocol);
    // bind socket: int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    // passivise socket: int listen(int sockfd, int backlog);
    // accept client socket: int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    const char *SOCKNAME = "/tmp/mysock";
    int sfd, cfd;
    ssize_t num_read;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
        perr_exit("socket");

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) -1);

    if (connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perr_exit("connect");
        return 1;
    }
        

    struct RequestHeader rh = {31, 1, 0};
    struct User user;
    strcpy(user.username, argv[1]);

    write(sfd, &rh, sizeof(struct RequestHeader));
    write(sfd, user.username, sizeof(user.username)); 
    read(sfd, &user, sizeof(struct User));
    printf("\nRECEIVED USER\nuser_id\t%d\nstatus\t%d\nname\t%s\nusername\t%s\nregistration_date\t%s\n",
            user.user_id, user.status, user.name, user.username, user.registration_date);

    exit(EXIT_SUCCESS);
}
