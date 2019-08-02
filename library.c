#include "library.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

void hello(void) {
    printf("Hello, World!\n");
}

static void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
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
            return network[1]=='d' ? DialUDP(network, address) : DialUnix(network, address);
        default:
            break;
    }
    return -1;
}
int DialTimeout(const char *network, const char *address, int ms);
int Listen(const char *network, const char *address, int *fds, int fd_n)
{
    if (!network || !fds || fd_n<1) {
        log_error("invalid parameters: network(%s),fds(%s),fd_n(%s)", network, fds, fd_n);
        return -1;
    }
    switch(network[0]) {
        case 't': return ListenTCP(network, address, fds, fd_n);
        case 'u':
//            return network[1]=='d' ? DialUDP(network, address) : DialUnix(network, address);
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
    if (!network || !address)
        return -1;
    char node_service[600], *node, *service;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd, rc;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
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
        log_error("Could not bind\n");
        return -1;
    }

    return fd;
}
// if address is not null, call connect to bind this address to the created socket
int DialUDP(const char *network, const char *address)
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
    if (!address) {
        fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
        goto out;
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
        log_error("Could not bind\n");
        return -1;
    }

out:
    return fd;
}
int DialUnix(const char *network, const char *address)
{
    return -1;
}

int ListenTCP(const char *network, const char *address, int *fds, int fd_n)
{
    if (!network || !address || !fds || fd_n<1) {
        log_error("invalid parameters: network(%s), address(%s), fds(%p), fd_n(%d)",
                network, address, fds, fd_n);
        return -1;
    }
    char node_service[600], *node, *service;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, count=0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
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

    for (rp = result; rp != NULL && count < fd_n; rp = rp->ai_next) {
        fds[count] = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (fds[count] == -1)
            continue;

        if (bind(fds[count], rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(fds[count], 128) == 0) {
                count++;
                continue;
            }
        }
        close(fds[count]);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (count == 0) {               /* No address succeeded */
        log_error("Could not bind\n");
        return -1;
    }

    return count;
}
// if address is not null, bind address to the socket, so
int ListenUDP(const char *network, const char *address, int *fds, int fd_n)
{

    return -1;
}
