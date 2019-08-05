#ifndef XNET_LIBRARY_H
#define XNET_LIBRARY_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
// ALL network is NOT NULL
// ALL address is NOT NULL
// return fd, -1 error
int Dial(const char *network, const char *address);
int DialTimeout(const char *network, const char *address, int ms);
int Listen(const char *network, const char *address);

int DialTCP(const char *network, const char *address);
int DialUDP(const char *network, const char *local_address, const char *remote_address);
int DialUnix(const char *network, const char *address);

int ListenTCP(const char *network, const char *address);
int ListenUDP(const char *network, const char *address);
int ListenUnix(const char *network, const char *address);

// mode is 'D' for dial, 'L' for listen
int set_default_sockopt(const char *network, int fd, char mode);
// socket utilities
// 0 : success, -1 fail
int setblock(int socket_fd, bool block);


struct zbytes {
    char *raw_buffer;
    int pos;
    int limit;
    int cap;
};
struct zbytes * zb_init(struct zbytes *zb, int hint_capability);
void zb_destroy(struct zbytes *zb);
static inline char *zb_data(const struct zbytes *zb)
{
    return zb->raw_buffer + zb->pos;
}
static inline int zb_available(const struct zbytes *zb)
{
    return zb->limit - zb->pos;
}
static inline int zb_free_size(const struct zbytes *zb)
{
    return zb->cap - zb->limit;
}
static inline bool zb_empty(const struct zbytes *zb)
{
    return zb->pos == zb->limit;
}
// callers must be honest to report the number of bytes they consume.
static inline void zb_consume(struct zbytes *zb, size_t nbytes)
{
    assert(zb_available(zb)>=nbytes);
    zb->pos += nbytes;
    if (zb_empty(zb))
        zb->pos = zb->limit = 0;
}
int zb_resize(struct zbytes *zb, size_t new_size);
void zb_move(struct zbytes *zb);
// number of bytes read from socket, or -1 if read failed
int zb_appendSocket(int fd, struct zbytes *zb);

typedef int (*zb_packet_processor_func)(char *data, int length);
#define ZBF_NOP         0
#define ZBF_MOVMEM      (1<<0)
#define ZBF_INCOMPLETE  (1<<1)
#define ZBF_XMEM        (1<<2)
typedef int (*zb_packet_checker_func)(struct zbytes *zb);

#endif