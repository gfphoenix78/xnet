//
// Created by Hao Wu on 8/6/19.
//

#ifndef XNET_PACKET_H
#define XNET_PACKET_H

#include "zbytes.h"

struct handler {
    struct zbytes buffer;
    int sockfd;
};
// number of bytes read from socket, or -1 if read failed
int zb_appendSocket(int fd, struct zbytes *zb);

typedef int (*zb_packet_processor_func)(char *data, int length);
#define ZBF_NOP         0
#define ZBF_MOVMEM      (1<<0)
#define ZBF_INCOMPLETE  (1<<1)
#define ZBF_XMEM        (1<<2)
typedef int (*zb_packet_checker_func)(struct zbytes *zb);


#endif //XNET_PACKET_H
