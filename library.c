#include "library.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

static void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (fmt[strlen(fmt)-1] != '\n')
        printf("\n");
}

// network: tcp, tcp4, tcp6, udp, udp4, udp6, ip, ip4, ip6, unix, unixgram, unixpacket
// address: tcp[4|6], udp[4|6] host:port, port must be number
//          ip[4|6] not supported now
//          unix[gram|packet] socket file path
int Dial(const char *network, const char *address)
{
    if (!network)
        return -1;
    switch(network[0]) {
        case 't': return DialTCP(network, address);
        case 'u':
            return network[1]=='d' ? DialUDP(network, NULL, address) : DialUnix(network, address);
        default:
            break;
    }
    return -1;
}
int DialTimeout(const char *network, const char *address, int ms)
{
    return -1;
}
int Listen(const char *network, const char *address)
{
    if (!network) {
        log_error("null network");
        return -1;
    }
    switch(network[0]) {
        case 't': return ListenTCP(network, address);
        case 'u':
            return network[1]=='d' ? ListenUDP(network, address)
                    : ListenUnix(network, address);
        default:
            break;
    }
    return -1;
}
static char *split_address(const char *address, char *node_service)
{
    char *p;
    int n = strlen(address);
    if (n>=600 || n<5)
        return NULL;
    strcpy(node_service, address);
    p = node_service + n-1;
    while (*p != ':' && p>node_service)
        --p;
    if (*p != ':')
        return NULL;
    *p = '\0';
    return p+1;
}
int DialTCP(const char *network, const char *address)
{
    char node_service[600], *node, *service;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd, rc;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_V4MAPPED;
    if (strcmp(network, "tcp") == 0) {
        hints.ai_family = AF_UNSPEC;
    } else if (strcmp(network, "tcp4") == 0) {
        hints.ai_family = AF_INET;
    } else if (strcmp(network, "tcp6") == 0) {
        hints.ai_family = AF_INET6;
    } else {
        log_error("invalid network:%s\n", network);
        return -1;
    }
    service = split_address(address, node_service);
    if (!service) {
        log_error("bad address(%s)\n", address);
        return -1;
    }
    node = node_service[0]=='\0' ? NULL : node_service;

    rc = getaddrinfo(node, service, &hints, &result);
    if (rc != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (fd == -1)
            continue;

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(fd);
    }
    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        log_error("Could not bind(%s)\n", strerror(errno));
        return -1;
    }

    return fd;
}
// if address is not null, call connect to bind this address to the created socket
// FIXME: local_address used to bind to local address
// FIXME: remote_address is used to connect to the server address
int DialUDP(const char *network, const char *local_address, const char *remote_address)
{
    if (!network)
        return -1;
    char node_service[600], *node, *service;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd, rc;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    if (strcmp(network, "udp") == 0) {
        hints.ai_family = AF_UNSPEC;
    } else if (strcmp(network, "udp4") == 0) {
        hints.ai_family = AF_INET;
    } else if (strcmp(network, "udp6") == 0) {
        hints.ai_family = AF_INET6;
    } else {
        log_error("invalid network:%s\n", network);
        return -1;
    }
    if (!remote_address) {
        fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
        goto out;
    }

    service = split_address(remote_address, node_service);
    if (!service) {
        log_error("bad address(%s)\n", remote_address);
        return -1;
    }
    node = node_service[0]=='\0' ? NULL : node_service;

    rc = getaddrinfo(node, service, &hints, &result);
    if (rc != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (fd == -1)
            continue;

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(fd);
    }
    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        log_error("Could not bind\n");
        return -1;
    }

out:
    return fd;
}
int DialUnix(const char *network, const char *address)
{
    struct sockaddr_un sockaddr;
    int fd, sotype;
    socklen_t socklen;
    if (!network || !address) {
        log_error("null network or address");
        return -1;
    }
    if (strcmp(network, "unix")==0)
        sotype = SOCK_STREAM;
    else if (strcmp(network, "unixgram")==0)
        sotype = SOCK_DGRAM;
    else if (strcmp(network, "unixpacket")==0)
        sotype = SOCK_SEQPACKET;
    else {
        log_error("unknown network for unix-socket(%s)", network);
        return -1;
    }
    if (strlen(address) >= sizeof(sockaddr.sun_path)) {
        log_error("address(%s) is too long(%d)", address, strlen(address));
        return -1;
    }
    fd = socket(AF_UNIX, sotype, 0);
    if (fd == -1) {
        log_error("socket failed:%s", strerror(errno));
        return -1;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, address);
    socklen = sizeof(sockaddr);
    if (connect(fd, (const struct sockaddr*)&sockaddr, socklen)) {
        log_error("connect error");
        goto out;
    }
    return fd;

out:
    close(fd);
    return -1;
}

int ListenTCP(const char *network, const char *address)
{
    if (!network || !address) {
        log_error("invalid parameters: network(%s), address(%s)",
                network, address);
        return -1;
    }
    char node_service[600], *node, *service;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, fd=-1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
    if (strcmp(network, "tcp") == 0) {
        hints.ai_family = AF_UNSPEC;
    } else if (strcmp(network, "tcp4") == 0) {
        hints.ai_family = AF_INET;
    } else if (strcmp(network, "tcp6") == 0) {
        hints.ai_family = AF_INET6;
    } else {
        log_error("invalid network:%s\n", network);
        return -1;
    }
    service = split_address(address, node_service);
    if (!service) {
        log_error("bad address(%s)\n", address);
        return -1;
    }
    node = node_service[0]=='\0' ? NULL : node_service;
    if (!node && hints.ai_family == AF_UNSPEC)
        hints.ai_family = AF_INET6;

    rc = getaddrinfo(node, service, &hints, &result);
    if (rc != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (fd == -1)
            continue;

        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(fd, 128) == 0)
                break;
        }
        close(fd);
    }

    freeaddrinfo(result);           /* No longer needed */
    return fd;
}
// if address is not null, bind address to the socket, so
int ListenUDP(const char *network, const char *address)
{

    return -1;
}
int ListenUnix(const char *network, const char *address)
{
    int fd, sotype;
    struct sockaddr_un sockaddr;
    if (!network || !address) {
        log_error("null parameters: network(%s), address(%s)", network, address);
        return -1;
    }
    if (strcmp(network, "unix") == 0)
        sotype = SOCK_STREAM;
    else if (strcmp(network, "unixpacket") == 0)
        sotype = SOCK_SEQPACKET;
//    else if (strcmp(network, "unixgram") == 0)
//        socktype = SOCK_DGRAM;
    else {
        log_error("unknown network(%s)", network);
        return -1;
    }
    if (strlen(address) >= sizeof(sockaddr.sun_path)) {
        log_error("address is too long(%d)", strlen(address));
        return -1;
    }
    fd = socket(AF_UNIX, sotype, 0);
    if (fd == -1) {
        log_error("socket failed(%s)", strerror(errno));
        return -1;
    }
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, address);
    if (bind(fd, (const struct sockaddr*)&sockaddr, (socklen_t) sizeof(sockaddr)) == -1) {
        log_error("bind error(%s)", strerror(errno));
        goto out;
    }
    if (listen(fd, 128) == -1) {
        log_error("listen error(%s)", strerror(errno));
        goto out;
    }
    return fd;

out:
    close(fd);
    return -1;
}
int set_default_sockopt(const char *network, int fd, char mode)
{
    return 0;
}
//// socket utility
int setblock(int socket_fd, bool block)
{
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == 0) {
        flags = block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        flags = fcntl(socket_fd, F_SETFL, flags);
    }
    return flags;
}
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