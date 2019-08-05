#include "zbytes.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

//// zbytes

struct zbytes *zb_init(struct zbytes *zb, int hint_capability)
{
    if (hint_capability<=0)
        hint_capability = (1<<16)-8;
    char *bb = malloc((size_t )hint_capability);
    if (!bb)
        return NULL;
    zb->cap = hint_capability;
    zb->pos = 0;zb->limit = 0;
    zb->raw_buffer = bb;
    return zb;
}
void zb_destroy(struct zbytes *zb)
{
    if (zb->raw_buffer) {
        free(zb->raw_buffer);
        zb->raw_buffer = NULL;
    }
    zb->pos = zb->limit = 0;
}
int zb_resize(struct zbytes *zb, size_t new_size)
{
    char *bb = realloc(zb->raw_buffer, new_size);
    if (!bb)
        return -1;
    zb->raw_buffer = bb;
    return 0;
}
void zb_move(struct zbytes *zb)
{
    if (zb->pos) {
        if (zb->pos == zb->limit) {
            zb->pos = zb->limit = 0;
        } else {
            char *bb = zb->raw_buffer;
            int pos = zb->pos;
            memmove(bb, bb + pos, zb->limit - pos);
            zb->pos = 0;
            zb->limit -= pos;
        }
    }
}
// TODO: how to handle this function
int zb_appendSocket(int fd, struct zbytes *zb)
{
    int left = zb_free_size(zb);
    ssize_t n;
retry:
    n = recv(fd, &zb->raw_buffer[zb->limit], (size_t)left, 0);
    if (n==-1) {
        if (errno==EINTR)
            goto retry;
        return -1;
    }
    zb->limit += n;
    return (int)n;
}
