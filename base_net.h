#ifndef XNET_BASE_NET_H_
#define XNET_BASE_NET_H_

#ifdef __cplusplus
extern "C" {
#endif
struct addrinfo;
struct sockaddr;

struct BuildNetParams {
  const char *network;
  const char *local_address;
  const char *remote_address;

  // HOOK function: 0 <==> OK, -1 <==> FAIL
  // hook function after socket(), before any bind
  int (*pre_call)(int sockfd, const struct BuildNetParams *params);

  // hook function: after connect()/listen() etc.
  // do something finally.
  int (*post_call)(int sockfd, const struct BuildNetParams *params,
                   const struct addrinfo *src, const struct addrinfo *dest);

  void *arg;
};

int format_sockaddr(const struct sockaddr *sa, char *buffer, int size);

// ALL network is NOT NULL
// ALL address is NOT NULL
// return fd, -1 error
int DialTimeout(const char *network, const char *address, int ms);


int Listen_ex(const struct BuildNetParams *params);

int ListenTCP_ex(const struct BuildNetParams *params);

int ListenUDP_ex(const struct BuildNetParams *params);

int ListenUNIX_ex(const struct BuildNetParams *params);

int DialTCP_ex(const struct BuildNetParams *params);

int DialUDP_ex(const struct BuildNetParams *params);

int DialUNIX_ex(const struct BuildNetParams *params);

static inline int ListenTCP(const char *network, const char *address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = address,
  };
  return ListenTCP_ex(&params);
}

static inline int ListenUDP(const char *network, const char *address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = address,
  };
  return ListenUDP_ex(&params);
}

static inline int ListenUNIX(const char *network, const char *socket_address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = socket_address,
  };
  return ListenUNIX_ex(&params);
}

// network: tcp, tcp4, tcp6, udp, udp4, udp6, ip, ip4, ip6, unix, unixgram, unixpacket
// address: tcp[4|6], udp[4|6] host:port, port must be number
//          ip[4|6] not supported now
//          unix[gram|packet] socket file path
static inline int Listen(const char *network, const char *address) {
  if (!network)
    return -1;
  switch (network[0]) {
    case 't':
      return ListenTCP(network, address);
    case 'u':
      return network[1] == 'd' ? ListenUDP(network, address) : ListenUNIX(network, address);
    default:
      break;
  }
  return -1;
}

//========================================================================
static inline int DialTCP(const char *network, const char *remote_address) {
  struct BuildNetParams params = {
          .network = network,
          .remote_address = remote_address,
  };
  return DialTCP_ex(&params);
}

static inline int DialTCP2(const char *network, const char *local_address, const char *remote_address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = local_address,
          .remote_address = remote_address,
  };
  return DialTCP_ex(&params);
}

static inline int DialUDP(const char *network, const char *remote_address) {
  struct BuildNetParams params = {
          .network = network,
          .remote_address = remote_address,
  };
  return DialUDP_ex(&params);
}

static inline int DialUDP2(const char *network, const char *local_address, const char *remote_address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = local_address,
          .remote_address = remote_address,
  };
  return DialUDP_ex(&params);
}

static inline int DialUnix(const char *network, const char *remote_address) {
  struct BuildNetParams params = {
          .network = network,
          .remote_address = remote_address,
  };
  return DialUNIX_ex(&params);
}

static inline int DialUnix2(const char *network, const char *address) {
  struct BuildNetParams params = {
          .network = network,
          .local_address = address,
          .remote_address = address,
  };
  return DialUNIX_ex(&params);
}

static inline int Dial(const char *network, const char *address) {
  if (!network)
    return -1;
  switch (network[0]) {
    case 't':
      return DialTCP(network, address);
    case 'u':
      return network[1] == 'd' ? DialUDP(network, address) : DialUnix(network, address);
    default:
      break;
  }
  return -1;
}

#ifdef __cplusplus
}
#endif
#endif
