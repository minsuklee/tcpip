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
recvfrom(int sd, char *data, int count, int flag, struct sockaddr *from, int *addrlen)
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
	if (sp->so_type == SOCK_STREAM) {
		errno = EBADF;
		return(-1);
	}
	if (sp->so_state & SS_CANTRCVMORE) {
		errno = ESHUTDOWN;
		return(-1);
	}
	if (from && *addrlen < 8) {
		errno = EFAULT;
		return(-1);
	}

loop_here:
	_imige_reg.h.ah = IMG_RECVFROM;
	_imige_reg.h.al = sd;
	_imige_reg.x.si = FP_SEG(data);
	_imige_reg.x.di = FP_OFF(data);
	_imige_reg.x.cx = count;
	_imige_reg.x.bx = flag;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0) {
		if (sp->so_state & SS_NBIO) {
			errno = EWOULDBLOCK;	
			return(-1);
		} else
			goto loop_here;
	}
	// IMG_RECVFROM Does not return with minus value

	if (from) {
		((struct sockaddr_in *)from)->sin_family = AF_INET;
		((struct sockaddr_in *)from)->sin_addr.s_addr =
			(unsigned long)_imige_reg.x.dx * 0x10000L +
			(unsigned long)_imige_reg.x.bx;
		((struct sockaddr_in *)from)->sin_port = _imige_reg.x.cx;
		*addrlen = 8;
	}
	return(_imige_reg.x.ax);
}
