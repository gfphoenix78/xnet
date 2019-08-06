#include "zbytes.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

//// zbytes
#ifdef ZB_DEBUG
void zb_Assert(int cond, const char *fmt, ...);
{
    if (!cond) {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        assert(cond && buf);
    }
}
#endif

struct zbytes *zb_init(struct zbytes *zb, int hint_capability)
{
    if (hint_capability<=0)
        hint_capability = (1<<16)-8;
    char *bb = malloc((size_t )hint_capability);
    if (!bb)
        return NULL;
    zb->cap = hint_capability;
    zb->pos = 0;zb->limit = 0;
    zb->data = bb;
    return zb;
}
void zb_destroy(struct zbytes *zb)
{
    if (zb->data) {
        free(zb->data);
        zb->data = NULL;
    }
    zb->pos = zb->limit = 0;
}
int zb_reserve(struct zbytes *zb, size_t n)
{
    if (zb_free_size(zb) >= n)
        return 0;

    size_t cap = zb->limit + n;
    if (cap < zb->cap * 2)
        cap = (size_t) (zb->cap * 2);
    return zb_resize(zb, cap);
}

int zb_resize(struct zbytes *zb, size_t new_size)
{
    char *bb = realloc(zb->data, new_size);
    if (!bb)
        return -1;
    zb->data = bb;
    return 0;
}
void zb_move(struct zbytes *zb)
{
    if (zb->pos) {
        if (zb->pos == zb->limit) {
            zb->pos = zb->limit = 0;
        } else {
            char *bb = zb->data;
            int pos = zb->pos;
            memmove(bb, bb + pos, zb->limit - pos);
            zb->pos = 0;
            zb->limit -= pos;
        }
    }
}
