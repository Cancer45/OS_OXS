#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#define BUF_SIZE 200
#define BACKLOG 5
#define SOCK_PATH "/tmp/serversock"

void perr_exit(char* err)
{
    perror(err);
    exit(1);
}

int main()
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
    if (remove(SOCK_PATH) == -1 && errno != ENOENT)
        perr_exit("remove");

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) -1);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
        perr_exit("bind");
    if (listen(sfd, BACKLOG) == -1)
        perr_exit("listen");

    for (;;)
    {
        cfd = accept(sfd, NULL, NULL);
        if (cfd == -1)
            perr_exit("accept");

        while((num_read = read(cfd, buf, BUF_SIZE)) > 0)
            if( write(STDOUT_FILENO, buf, num_read) != num_read)
                perr_exit("partial/failed write");

        if (num_read == -1)
            perr_exit("read");

        if (close(cfd) == -1)
            perr_exit("close");
    }
}
