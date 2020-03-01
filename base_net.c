#include "base_net.h"
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/errno.h>
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
static int split_address(const char *address, char *node, char *service)
{
    while (*address == ' ')
        address++;
    size_t n = strlen(address);
    if (n>=600 || n<1)
        return -1;
    service[0] = '\0';
    if (address[0] == '[') {
        char *br = strchr(address, ']');
        // invalid ipv6 address in brace, or empty in []
        if (!br || address + 1 == br)
            return -1;
        memcpy(node, address + 1, br - address - 1);
        node[br - address] = '\0';
        if (br[1] == ':') {
            strcpy(service, br+2);
        } else if (br[1] != '\0') {
            return -1;
        }
    } else {
        char *br = strrchr(address, ':');
        if (br) {
            n = (size_t)(br - address);
            memcpy(node, address, n);
            node[n] = '\0';
            strcpy(service, br + 1);
        } else {
            memcpy(node, address, n);
        }
    }
    n = strlen(service);
    for (int i = (int)(n - 1); i >= 0; i--) {
        if (service[i] != ' ')
            break;
        service[i] = '\0';
    }
    if (service[0] == '\0') {
        service[0] = '0';
        service[1] = '\0';
    }
    return 0;
}

// format the sockaddr into buffer with style:
// "xx.yy.zz.ww:port" for IPv4
// "[::1]:port" for IPv6
int format_sockaddr(const struct sockaddr *sa, char *buffer, int size)
{
    char        host[NI_MAXHOST];
    char        port[NI_MAXSERV];
    int rc;

    rc = getnameinfo(sa, sa->sa_len,
            host, sizeof(host),
            port, sizeof(port),
            NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc != 0) {
        snprintf(buffer, size, "?host?:?port?");
    } else {
        if (sa->sa_family == AF_INET6)
            snprintf(buffer, size, "[%s]:%s", host, port);
        else
            snprintf(buffer, size, "%s:%s", host, port);

    }
    return rc;
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
static struct addrinfo *_getaddrinfo(const char *node_service, const struct addrinfo *hints)
{
    char node_[600], service[32];
    char *node = node_;
    struct addrinfo *result;
    int rc;
    rc = split_address(node_service, node_, service);
    if (rc != 0)
        return NULL;
    if (node_[0] == '\0' || strcmp(node_, "*") == 0)
        node = NULL;
    rc = getaddrinfo(node, service, hints, &result);
    if (rc != 0)
        return NULL;
    return result;
}
static int _connect(const struct addrinfo *hints, const char *remote, setsockopt_fn fn)
{
    struct addrinfo *result, *rp;
    int sockfd = -1;
    result = _getaddrinfo(remote, hints);
    if (!result)
        return -1;
    for (rp = result; rp; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;
        if (!fn(sockfd))
            goto cont;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
cont:
        close(sockfd);
    }
    freeaddrinfo(result);
    return rp ? sockfd : -1;
}
int BindConnect(const struct addrinfo *hints, const char *local, const char *remote, setsockopt_fn fn)
{
    struct addrinfo *result, *rp;
    struct addrinfo *result_local, *rp_local;

    char node_[600], service[32];
    char *node = node_;
    int rc, sockfd = -1;
    if (!local)
        return _connect(hints, remote, fn);
    rp = NULL;
    result_local = _getaddrinfo(local, hints);
    if (!result_local)
        return -1;
    result = _getaddrinfo(remote, hints);
    if (!result)
        goto out1;
    for (rp = result; rp; rp = rp->ai_next) {
        sockfd = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
        if (sockfd < 0)
            return -1;
        for (rp_local = result_local; rp_local; rp_local = rp_local->ai_next) {
            if (fn && !fn(sockfd))
                continue;
            if (bind(sockfd, rp_local->ai_addr, rp_local->ai_addrlen) == 0)
                break;
        }
        if (rp_local && connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        // connect failed.
        close(sockfd);
    }
    freeaddrinfo(result);
out1:
    freeaddrinfo(result_local);
    return rp ? sockfd : -1;
}
int DialTCP_ex(const char *network, const char *local_address, const char *remote_address, setsockopt_fn fn)
{
    struct addrinfo hints;
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
    return BindConnect(&hints, local_address, remote_address, fn);
}

int DialUDP_ex(const char *network, const char *local_address, const char *remote_address, setsockopt_fn fn)
{
    // assert(network != NULL)
    // assert(remote_address != NULL)
    if (!network)
        return -1;
    struct addrinfo hints;
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
    return BindConnect(&hints, local_address, remote_address, fn);
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
        log_error("connect error(%s)", strerror(errno));
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
    char node_[600], *node, service[32];
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, sockfd=-1;
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
    rc = split_address(address, node_, service);
    if (rc != 0) {
        log_error("bad address(%s)\n", address);
        return -1;
    }
    node = (node_[0] == '\0' || node_[0] == '*') ? NULL : node_;

    rc = getaddrinfo(node, service, &hints, &result);
    if (rc != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(sockfd, 128) == 0)
                break;
        }
        close(sockfd);
    }
    if (rp) {
        printf("listen on %s %s, ", node_, service);
        format_sockaddr(rp->ai_addr, node_, sizeof(node_));
        printf("address : %s\n",node_);
    }
    freeaddrinfo(result);           /* No longer needed */
    return rp ? sockfd : -1;
}
// if address is not null, bind address to the socket, so
int ListenUDP(const char *network, const char *address)
{
    if (!network || !address) {
        log_error("invalid parameters: network(%s), address(%s)",
                  network, address);
        return -1;
    }
    char node_[600], *node, service[32];
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, sockfd=-1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
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
    rc = split_address(address, node_, service);
    if (rc != 0) {
        log_error("bad address(%s)\n", address);
        return -1;
    }
    node = (node_[0] == '\0' || node_[0] == '*') ? NULL : node_;

    rc = getaddrinfo(node, service, &hints, &result);
    if (rc != 0) {
        log_error("getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sockfd);
    }
    if (rp) {
        printf("listen on %s %s, ", node_, service);
        format_sockaddr(rp->ai_addr, node_, sizeof(node_));
        printf("address : %s\n",node_);
    }
    freeaddrinfo(result);           /* No longer needed */
    return sockfd;
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
