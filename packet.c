//
// Created by Hao Wu on 8/6/19.
//
#include "packet.h"
#include <errno.h>
#include <sys/socket.h>

// TODO: how to handle this function
int zb_appendSocket(int fd, struct zbytes *zb)
{
    int left = zb_free_size(zb);
    ssize_t n;
    retry:
    n = recv(fd, &zb->data[zb->limit], (size_t)left, 0);
    if (n==-1) {
        if (errno==EINTR)
            goto retry;
        return -1;
    }
    zb->limit += n;
    return (int)n;
}
