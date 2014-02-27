/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

int
bind(int sd, struct sockaddr *myaddr, int addrlen)
{
	struct _IMIGE_SOCKET *sp = _imige_sock + sd;
	union  REGS _imige_reg;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);

	if ((sd < 0) || (sd >= _num_sock)) {
		errno = ENOTSOCK;
		return(-1);
	}
	if (!sp->so_type) {
		errno = EBADF;
		return(-1);
	}

	// Socket Library allow bin() call to RAW socket

	if (addrlen < 8) {
		errno = EFAULT;
		return(-1);
	}

	if (sp->so_state & (SS_ISCONNECTED | SS_ISCONNECTING)) {
		errno = EINVAL;
		return(-1);
	}
	if (myaddr->sa_family != AF_INET) {
		errno = EADDRNOTAVAIL;
		return(-1);
	}		

	_imige_reg.h.ah = IMG_BIND;
	_imige_reg.h.al = sd;
	_imige_reg.x.bx = ((struct sockaddr_in *)myaddr)->sin_port;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {
		errno = EADDRINUSE;
		return(-1);
	}
	return(0);
}
