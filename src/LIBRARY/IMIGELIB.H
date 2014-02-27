/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

//
// Header for IMIGE System Library USE Only
//
#include <imigetsr.h>
#include <imigetcp/imigetcp.h>

#include <sys/ioctl.h>

#include <bios.h>
#include <memory.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <io.h>

#include <arpa/inet.h>
#include <netdb.h>

extern int _imige_vec;
extern int __tcp_type;
extern int _num_sock;

int _find_kernel(void);				// Library
int _tcb_stat(int sd, struct TCB *stat);	// Kernel Call
int _get_errno(int sd);				// Kernel Call
int _resv_bind(int sd);				// Kernel Call

// Domain Name Resolver
struct hostent *_resolve_name(char *hostname, unsigned long addr, int type);

struct _IMIGE_SOCKET {
	int so_type;		// socket type
	int so_state;		// socket state
	int so_options;		// socket options
	int so_linger;		// socket options
};

extern struct _IMIGE_SOCKET _imige_sock[MAX_TCB];
extern int errno;

// Socket state bits definition

#define	SS_NOFDREF			0x0001	// no file table ref any more
#define	SS_ISCONNECTED		0x0002	// socket connected to a peer
#define	SS_ISCONNECTING		0x0004	// in process of connecting to peer
#define	SS_ISDISCONNECTING	0x0008	// in process of disconnecting
#define	SS_CANTSENDMORE		0x0010	// can't send more data to peer
#define	SS_CANTRCVMORE		0x0020	// can't receive more data from peer
#define	SS_RCVATMARK		0x0040	// at mark on input

#define	SS_PRIV				0x0080	// privileged for broadcast, raw...
#define	SS_NBIO				0x0100	// non-blocking ops
#define	SS_ASYNC			0x0200	// async i/o notify 

#define SS_ISLISTENNING		0x0400  // being passive opened
#define SS_ACCEPTING		0x0800  // being accepting connection
