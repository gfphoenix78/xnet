#include "base_net.h"
#include <assert.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdbool.h>
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
  if (n >= NI_MAXHOST + NI_MAXSERV + 3 || n<1)
    return -1;
  service[0] = '\0';
  if (address[0] == '[') {
    char *br = strchr(address, ']');
    // invalid ipv6 address in brace, or empty in []
    if (br == NULL || address + 1 == br)
      return -1;
    if ((size_t)(br - address - 1) >= NI_MAXHOST)
      return -1; // too long host
    memcpy(node, address + 1, br - address - 1);
    node[br - address] = '\0';
    if (br[1] == ':') {
      if (strlen(br+2) >= NI_MAXSERV)
        return -1; // too long serv/port
      strcpy(service, br+2);
    } else if (br[1] != '\0') {
      return -1;
    }
  } else {
    char *br = strrchr(address, ':');
    if (br) {
      n = (size_t)(br - address);
      if (n >= NI_MAXHOST)
        return -1; // too long host
      memcpy(node, address, n);
      node[n] = '\0';
      if (strlen(br + 1) >= NI_MAXSERV)
        return -1;
      strcpy(service, br + 1);
    } else {
      if (n >= NI_MAXHOST)
        return -1;
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

int DialTimeout(const char *network, const char *address, int ms)
{
  return -1;
}

int Listen_ex(const struct BuildNetParams *params)
{
  assert(params && params->network);
  const char *network = params->network;
  switch(network[0]) {
    case 't': return ListenTCP_ex(params);
    case 'u':
              return network[1]=='d' ? ListenUDP_ex(params)
                : ListenUNIX_ex(params);
    default:
              break;
  }
  return -1;
}
static struct addrinfo *_getaddrinfo(const char *node_service, const struct addrinfo *hints)
{
  char node_[NI_MAXHOST], service[NI_MAXSERV];
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
static int _connect(const struct addrinfo *hints, const struct BuildNetParams *params)
{
  struct addrinfo *result, *rp;
  int sockfd = -1;
  const char *remote = params->remote_address;
  result = _getaddrinfo(remote, hints);
  if (result == NULL)
    return -1;
  for (rp = result; rp; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1) continue;
    if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
        connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0 &&
        (params->post_call == NULL || params->post_call(sockfd, params, NULL, rp) == 0))
      break;
    close(sockfd);
  }
  freeaddrinfo(result);
  return rp ? sockfd : -1;
}
int BindConnect(const struct addrinfo *hints, const struct BuildNetParams *params)
{
  struct addrinfo *result_remote, *rp_remote = NULL;
  struct addrinfo *result_local, *rp_local;
  const char *local = params->local_address;
  const char *remote = params->remote_address;
  int sockfd = -1;
  if (local == NULL)
    return _connect(hints, params);

  result_local = _getaddrinfo(local, hints);
  result_remote = _getaddrinfo(remote, hints);
  if (result_local == NULL || result_remote == NULL)
    goto null_out;

  for (rp_remote = result_remote; rp_remote; rp_remote = rp_remote->ai_next) {
    for (rp_local = result_local; rp_local; rp_local = rp_local->ai_next) {
      if (rp_remote->ai_family != rp_local->ai_family)
        continue;

      sockfd = socket(rp_remote->ai_family, rp_remote->ai_socktype, rp_remote->ai_protocol);
      if (sockfd == -1)
        break;
      if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
          bind(sockfd, rp_local->ai_addr, rp_local->ai_addrlen) == 0 &&
          connect(sockfd, rp_remote->ai_addr, rp_remote->ai_addrlen) == 0 &&
          (params->post_call == NULL || params->post_call(sockfd, params, rp_local, rp_remote) == 0))
        break;
      close(sockfd);
    }
    if (sockfd != -1 && rp_local)
      break;
  }
null_out:
  if (result_remote)
    freeaddrinfo(result_remote);
  if (result_local)
    freeaddrinfo(result_local);
  return rp_remote ? sockfd : -1;
}
int DialTCP_ex(const struct BuildNetParams *params)
{
  assert(params && params->network && params->remote_address);
  assert(memcmp(params->network, "tcp", 3) == 0);
  //  const char *network, const char *local_address, const char *remote_address, setsockopt_fn fn
  const char *network = params->network;
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
  return BindConnect(&hints, params);
}

int DialUDP_ex(const struct BuildNetParams *params)
{
  assert(params->network != NULL);
  assert(params->remote_address != NULL);
  const char *network = params->network;
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
  return BindConnect(&hints, params);
}

static int unix_common_prepare(const struct BuildNetParams *params, struct addrinfo *info)
{
  assert(params && params->network);
  assert(params->local_address || params->remote_address);
  assert(!(params->local_address && params->remote_address) ||
      strcmp(params->local_address, params->remote_address) == 0);
  const char *network = params->network;
  const char *address = params->remote_address;
  if (address == NULL)
    address = params->local_address;
  struct sockaddr_un sockaddr;
  int sockfd;

  if (strcmp(network, "unix") == 0)
    info->ai_socktype = SOCK_STREAM;
  else if (strcmp(network, "unixgram") == 0)
    info->ai_socktype = SOCK_DGRAM;
  else if (strcmp(network, "unixpacket") == 0)
    info->ai_socktype = SOCK_SEQPACKET;
  else {
    log_error("unknown network for unix-socket(%s)", network);
    return -1;
  }
  if (strlen(address) >= sizeof(sockaddr.sun_path)) {
    log_error("address(%s) is too long(%d)", address, strlen(address));
    return -1;
  }
  sockfd = socket(AF_UNIX, info->ai_socktype, info->ai_protocol);
  if (sockfd == -1) {
    log_error("socket failed:%s", strerror(errno));
    return -1;
  }

  sockaddr.sun_family = AF_UNIX;
  strcpy(sockaddr.sun_path, address);

  return sockfd;
}
int DialUnix_ex(const struct BuildNetParams *params)
{
  struct sockaddr_un sockaddr = {
    .sun_family = AF_UNIX,
  };
  struct addrinfo info = {
    .ai_family = AF_UNIX,
    .ai_addr = (struct sockaddr*)&sockaddr,
    .ai_addrlen = sizeof(sockaddr),
  };
  int sockfd = unix_common_prepare(params, &info);
  if (sockfd == -1)
    return -1;

  if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
      (params->local_address == NULL || bind(sockfd, info.ai_addr, info.ai_addrlen) == 0) &&
      connect(sockfd, info.ai_addr, info.ai_addrlen) == 0 &&
      (params->post_call == NULL || params->post_call(sockfd, params, &info, &info) == 0))
    return sockfd;

  log_error("connect error(%s)", strerror(errno));
  close(sockfd);
  return -1;
}


int ListenTCP_ex(const struct BuildNetParams *params)
{

  assert(params && params->network && params->local_address);
  assert(memcmp(params->network, "tcp", 3) == 0);
  const char *network = params->network;
  const char *address = params->local_address;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sockfd=-1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
  switch (network[3]) {
    case '\0': hints.ai_family = AF_UNSPEC;
               break;
    case '4': hints.ai_family = AF_INET;
              break;
    case '6': hints.ai_family = AF_INET6;
              break;
    default:
              return -1;
  }

  result = _getaddrinfo(address, &hints);
  if (result == NULL)
    return -1;

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype,
        rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
        bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0 &&
        listen(sockfd, 128) == 0 &&
        (params->post_call == NULL || params->post_call(sockfd, params, rp, NULL) == 0))
      break;
    close(sockfd);
  }
  freeaddrinfo(result);           /* No longer needed */
  return rp ? sockfd : -1;
}
// if address is not null, bind address to the socket, so
int ListenUDP_ex(const struct BuildNetParams *params)
{
  const char *network;
  const char *address;
  assert(params && params->network && params->local_address);
  assert(memcmp(params->network, "udp", 3) == 0);
  network = params->network;
  address = params->local_address;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int rc, sockfd=-1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE | AI_V4MAPPED;
  switch (network[3]) {
    case '\0': hints.ai_family = AF_UNSPEC;
               break;
    case '4': hints.ai_family = AF_INET;
              break;
    case '6': hints.ai_family = AF_INET6;
              break;
    default:
              return -1;
  }

  result = _getaddrinfo(address, &hints);
  if (result == NULL)
    return -1;

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1)
      continue;
    if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
        bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0 &&
        (params->post_call == NULL && params->post_call(sockfd, params, rp, NULL) == 0))
      break;
    close(sockfd);
  }
  freeaddrinfo(result);           /* No longer needed */
  return rp ? sockfd : -1;
}
int ListenUNIX_ex(const struct BuildNetParams *params)
{
  struct sockaddr_un sockaddr = {
    .sun_family = AF_UNIX,
  };
  struct addrinfo info = {
    .ai_family = AF_UNIX,
    .ai_addr = (struct sockaddr*)&sockaddr,
    .ai_addrlen = sizeof(sockaddr),
  };

  int sockfd = unix_common_prepare(params, &info);
  if (sockfd == -1)
    return -1;

  if ((params->pre_call == NULL || params->pre_call(sockfd, params) == 0) &&
      bind(sockfd, info.ai_addr, info.ai_addrlen) == 0 &&
      (info.ai_socktype == SOCK_DGRAM || listen(sockfd, 128) == 0) &&
      (params->post_call == NULL || params->post_call(sockfd, params, &info, &info) == 0))
    return sockfd;

  close(sockfd);
  return -1;
}
