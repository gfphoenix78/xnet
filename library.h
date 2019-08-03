#ifndef XNET_LIBRARY_H
#define XNET_LIBRARY_H
// return fd, -1 error
int Dial(const char *network, const char *address);
int DialTimeout(const char *network, const char *address, int ms);
int Listen(const char *network, const char *address);

int DialTCP(const char *network, const char *address);
int DialUDP(const char *network, const char *address);
int DialUnix(const char *network, const char *address);

int ListenTCP(const char *network, const char *address);
int ListenUDP(const char *network, const char *address);

// flags family socktype protocol addrlen
// 0 30 1 6 28 OK 0x30 0x39 000
// 0 2 1 6 16 FAIL
#endif