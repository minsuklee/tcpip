/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 typedef unsigned short	u_int;
typedef unsigned char	u_char;
typedef unsigned long	u_long;
typedef unsigned long	ulong;
typedef unsigned short	u_short;
typedef unsigned short	ushort; 	/* sys III compat */

typedef struct	_quad { long val[2]; } quad;
typedef long	daddr_t;
typedef char *	caddr_t;
typedef long	swblk_t;

struct iovec {
	char far *iov_base;
	short	 iov_len;
};

typedef unsigned long fd_set;

#define NBBY	8

#ifndef _INO_T_DEFINED
typedef unsigned short ino_t;	/* i-node number (not used on DOS) */
#define _INO_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif

#ifndef _DEV_T_DEFINED
typedef short dev_t;			/* device code */
#define _DEV_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef long off_t;			/* file offset value */
#define _OFF_T_DEFINED
#endif

#define	FD_SET(n, p)	(*(p) |= (1<<(n)))
#define	FD_CLR(n, p)	(*(p) &= ~(1<<(n)))
#define	FD_ISSET(n, p)	(*(p) & (1<<(n)))
#define FD_ZERO(p)		(*(p) = 0)
