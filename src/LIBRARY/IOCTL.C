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
ioctlsocket(int sd, int request, char *arg)
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
	switch (request) {
		case FIONBIO :
			if (*(int *)arg)
				sp->so_state |= SS_NBIO;
			else
				sp->so_state &= ~SS_NBIO;
			break;
		case FIONREAD :
			_imige_reg.h.ah = IMG_SOCK_STAT;
			_imige_reg.h.al = sd;
			_imige_reg.x.bx = _imige_reg.x.cx = 0;
			int86(_imige_vec, &_imige_reg, &_imige_reg);
			*(long *)arg = (long)_imige_reg.x.ax;
			break;
		default :
			errno = EINVAL;
			return(-1);
	}
	return(0);
}
