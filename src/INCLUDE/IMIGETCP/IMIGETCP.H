/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#ifndef _IMIGE_TCP_H
#define _IMIGE_TCP_H

#include <imigetcp/nm_obj.h>
#include <imigetcp/tcpcfg.h>

int  _init_imigetcp(int flag);		// Flag must be 0, returns Kernel Type
int  _read_config(struct TCPCONFIG *cfg);	// Get TCP Configuration
void _net_stat(struct snmp_object *stat, int clearflag);	// Get TCP/IP statistics
int  _asyncsocket(int sd, void far (* user_func)());
int  __async_connect(int sd, struct sockaddr *sin, int len, int sec, unsigned key);
int  __async_accept(int sd, struct sockaddr *peer, int *addrlen, int sec, unsigned key);
void _net_close(void);	// close all open sockets
int _get_errno(int sd);

// Configration Related Functions

char *getlogin(void);		// Login Username
unsigned long gethostid(void);	// My Host Address (in Network Order)
int gethostname(char *hostname, int namelen);	// My symbolic Hostname
int getdomainname(char *domain, int namelen);	// My symbolic Domain Name

// Function Prototypes for Socket library

int socket(int family, int type,int protocol);
int accept(int sd, struct sockaddr *peer, int *addrlen);
int bind(int sd, struct sockaddr *myaddr, int addrlen);
int listen(int sd, int backlog);
int connect(int sd, struct sockaddr *toaddr, int addrlen);
int select(int n, fd_set *read_ds, fd_set *write_ds, fd_set *except_ds, struct timeval *timeout);
int shutdown(int sd, int how);
int closesocket(int sd);

int recv(int sd, char *data, int count, int flag);
int recvfrom(int sd, char *data, int count, int flag, struct sockaddr *from, int *addrlen);
int send(int sd, char *data, int count, int flag);
int sendto(int sd, char *data, int count, int flag, struct sockaddr *to, int addrlen);

int setsockopt(int sd, int level, int optname, char *optval, int optlen);
int getsockopt(int sd, int level, int optname, char *optval, int *optlen);
int ioctlsocket(int sd, int request, char *arg);

int getpeername(int sd, struct sockaddr *peer, int *addrlen);
int getsockname(int sd, struct sockaddr *myname, int *addrlen);

// BSD support complex I/O funtions

int readv(int sd, struct iovec *iov, int iovcnt);
int writev(int sd, struct iovec *iov, int iovcnt);

int rresvport(int *port);
int rcmd(char **host, unsigned port, char *lusr, char *rusr, char *cmd, int *fd2p);
int rexec(char **host, int rport, char *name, char *pass, char *cmd, int *fd2p);

void bcopy(char *src, char *dst, int count);
int  bcmp(char *src, char *dst, int count);
void bzero(char *dst, int count);

// Macros for number representation conversion.

unsigned long  lswap(unsigned long input);
unsigned short bswap(unsigned short input);

#define	ntohl(x) lswap((x))
#define	ntohs(x) bswap((x))
#define	htonl(x) lswap((x))
#define	htons(x) bswap((x))

#endif
