#ifndef XNET_NET_UTILITY_H_
#define XNET_NET_UTILITY_H_
#include <stdbool.h>

// mode is 'D' for dial, 'L' for listen
int set_default_sockopt(const char *network, int fd, char mode);
// socket utilities
// 0 : success, -1 fail
int setblock(int socket_fd, bool block);

#endif
