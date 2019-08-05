#ifndef XNET_BASE_NET_H_
#define XNET_BASE_NET_H_

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

#endif
