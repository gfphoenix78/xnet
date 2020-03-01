#ifndef XNET_BASE_NET_H_
#define XNET_BASE_NET_H_
struct addrinfo;
struct sockaddr;
typedef int (*setsockopt_fn)(int sockfd);

int format_sockaddr(const struct sockaddr *sa, char *buffer, int size);

// ALL network is NOT NULL
// ALL address is NOT NULL
// return fd, -1 error
int Dial(const char *network, const char *address);
int DialTimeout(const char *network, const char *address, int ms);
int Listen(const char *network, const char *address);


int DialTCP_ex(const char *network, const char *local_address, const char *remote_address, setsockopt_fn fn);
int DialUDP_ex(const char *network, const char *local_address, const char *remote_address, setsockopt_fn fn);
static inline int DialTCP(const char *network, const char *remote_address)
{
  return DialTCP_ex(network, (void*)0, remote_address, (void*)0);
}
static inline int DialUDP(const char *network, const char *local_address, const char *remote_address)
{
  return DialUDP_ex(network, local_address, remote_address, (void*)0);
};

int DialUnix(const char *network, const char *address);

int ListenTCP(const char *network, const char *address);
int ListenUDP(const char *network, const char *address);
int ListenUnix(const char *network, const char *address);


#endif
