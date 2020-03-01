#include "net_utility.h"
#include <sys/fcntl.h>
#include <sys/socket.h>

int set_default_sockopt(const char *network, int fd, char mode)
{
    return 0;
}
//// socket utility
int set_nonblock(int socket_fd)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == 0) {
        flags = block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        flags = fcntl(socket_fd, F_SETFL, flags);
    }
    return flags;
}
