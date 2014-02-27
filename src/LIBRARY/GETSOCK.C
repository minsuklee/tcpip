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
getsockname(int sd, struct sockaddr *myname, int *addrlen)
{
	struct _IMIGE_SOCKET *sp = _imige_sock + sd;
	union  REGS _imige_reg;
	struct TCB thistcb;

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

	if (*addrlen < 8) {
		errno = ENOBUFS;
		return(-1);
	}

	_imige_reg.h.ah = IMG_SOCK_STAT;
	_imige_reg.h.al = sd;
	_imige_reg.x.bx = FP_SEG(&thistcb);
	_imige_reg.x.cx = FP_OFF(&thistcb);
	int86(_imige_vec, &_imige_reg, &_imige_reg);

	((struct sockaddr_in *)myname)->sin_family = AF_INET;
	((struct sockaddr_in *)myname)->sin_addr.s_addr = thistcb.IP_HDR.source;
	((struct sockaddr_in *)myname)->sin_port = thistcb.local_port;
	return(0);
}
