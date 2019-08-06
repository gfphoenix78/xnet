#ifndef XNET_ZBYTES_H_
#define XNET_ZBYTES_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef ZB_DEBUG
void zb_Assert(int cond, const char *fmt, ...);

#else
#define zb_Assert(cond, fmt, ...) do{}while(0)
#endif
// valid data is [pos, limit), free space is cap-limit
// pos is a read/write pointer

struct zbytes {
    char *data;
    int pos;
    int limit;
    int cap;
};
struct zbytes * zb_init(struct zbytes *zb, int hint_capability);
void zb_destroy(struct zbytes *zb);
static inline char *zb_data(const struct zbytes *zb)
{
    return zb->data + zb->pos;
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
static inline void zb_zero(struct zbytes *zb)
{
    zb->pos = zb->limit = 0;
}

int zb_reserve(struct zbytes *zb, size_t n);
int zb_resize(struct zbytes *zb, size_t new_size);
void zb_move(struct zbytes *zb);

//// APPEND DATA INTO zbytes
// all append and read is unsafe, you should reserve enough room for data by yourself
static inline void zb_append(struct zbytes *zb, const void *data, size_t len)
{
    memcpy(zb->data+zb->limit, data, len);
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_cstring(struct zbytes *zb, const char *str)
{
    zb_append(zb, str, strlen(str));
}
static inline void zb_append_char(struct zbytes *zb, char c)
{
    zb->data[zb->limit++] = c;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_uint(struct zbytes *zb, unsigned int val)
{
    *(unsigned int*)(zb->data+zb->limit) = val;
    zb->limit += sizeof(val);
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_uint8(struct zbytes *zb, uint8_t val)
{
    *(zb->data+zb->limit++) = (char)val;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_uint16(struct zbytes *zb, uint16_t val)
{
    *(uint16_t *)(zb->data+zb->limit) = val;
    zb->limit += 2;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_uint32(struct zbytes *zb, uint32_t val)
{
    *(uint32_t *)(zb->data+zb->limit) = val;
    zb->limit += 4;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_uint64(struct zbytes *zb, uint64_t val)
{
    *(uint64_t *)(zb->data+zb->limit) = val;
    zb->limit += 8;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_int(struct zbytes *zb, int val)
{
    *(int*)(zb->data+zb->limit) = val;
    zb->limit += sizeof(val);
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_int8(struct zbytes *zb, int8_t val)
{
    *(zb->data+zb->limit++) = (char)val;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_int16(struct zbytes *zb, int16_t val)
{
    *(int16_t *)(zb->data+zb->limit) = val;
    zb->limit += 2;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_int32(struct zbytes *zb, int32_t val)
{
    *(int32_t *)(zb->data+zb->limit) = val;
    zb->limit += 4;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}
static inline void zb_append_int64(struct zbytes *zb, int64_t val)
{
    *(int64_t *)(zb->data+zb->limit) = val;
    zb->limit += 8;
    zb_Assert(zb->limit<=zb->cap, "append overflow");
}

//// READ DATA from buffer
static inline char zb_read_char(struct zbytes *zb)
{
    zb_Assert(zb_available(zb)>0, "read an empty zbytes");
    return zb->data[zb->pos++];
}
static inline int zb_read_int(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= sizeof(int), "read is not ready");
    int v = *(int*)(zb->data+zb->pos);
    zb->pos += sizeof(int);
    return v;
}
static inline int8_t zb_read_int8(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 1, "read is not ready");
    return (int8_t)(zb->data[zb->pos++]);
}
static inline int16_t zb_read_int16(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 2, "read is not ready");
    int16_t v = *(int16_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline int32_t zb_read_int32(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 4, "read is not ready");
    int32_t v = *(int32_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline int64_t zb_read_int64(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 8, "read is not ready");
    int64_t v = *(int64_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline unsigned int zb_read_uint(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= sizeof(int), "read is not ready");
    unsigned int v = *(unsigned int*)(zb->data+zb->pos);
    zb->pos += sizeof(int);
    return v;
}
static inline uint8_t zb_read_uint8(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 1, "read is not ready");
    return (uint8_t)(zb->data[zb->pos++]);
}
static inline uint16_t zb_read_uint16(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 2, "read is not ready");
    uint16_t v = *(uint16_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline uint32_t zb_read_uint32(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 4, "read is not ready");
    uint32_t v = *(uint32_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline uint64_t zb_read_uint64(struct zbytes *zb)
{
    zb_Assert(zb_available(zb) >= 8, "read is not ready");
    uint64_t v = *(uint64_t*)(zb->data+zb->pos);
    zb->pos += sizeof(v);
    return v;
}
static inline int zb_skip(struct zbytes *zb, int offset)
{
    int pos = zb->pos;
    zb->pos += offset;
    zb_Assert(zb->pos>=0 && zb->pos<=zb->limit, "pos is out of bound");
    return pos;
}

#endif /* XNET_ZBYTES_H_ */
