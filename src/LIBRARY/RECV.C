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
recv(int sd, char *data, int count, int flag)
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

	if ((sp->so_state & SS_ISCONNECTED) && (sp->so_type == SOCK_DGRAM)) {
		return(recvfrom(sd, data, count, flag, NULL, NULL));
	}

	if (sp->so_type != SOCK_STREAM) {
		errno = EBADF;
		return(-1);
	}

	if (sp->so_state & SS_CANTRCVMORE) {
		errno = ESHUTDOWN;
		return(-1);
	}

loop_here:
	_imige_reg.h.ah = IMG_RECV;
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
	} else if (_imige_reg.x.ax == 0xFFFF) {
		errno = _get_errno(sd);
		return(-1);
	} else
		return(_imige_reg.x.ax);
}
