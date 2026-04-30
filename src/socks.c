#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>

void perr_exit(char* err)
{
    perror(err);
    exit(1);
}


int uds_socket_init()
{
    int sfd;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        perr_exit("socket");
        return 0;
    }

    return sfd;
}

int uds_socket_bind(int sfd, const char* pathname)
{
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, pathname, sizeof(addr.sun_path) -1);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
    {
        perr_exit("bind");
        return 0;
    }

    return 1;
}
