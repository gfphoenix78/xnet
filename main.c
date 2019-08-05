//
// Created by Hao Wu on 8/2/19.
//
#include <stdio.h>
#include <sys/socket.h>
#include "base_net.h"

int main(int ac, char *av[])
{
    int fd = Listen(av[1], av[2]);
    int cfd;
    printf("fd = %d\n", fd);
    struct sockaddr_storage cli_addr;
    socklen_t len;
    cfd = accept(fd, (struct sockaddr*)&cli_addr, &len);
    printf("cfd = %d\n", cfd);
    return 0;
}
