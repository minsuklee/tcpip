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
listen(int sd, int backlog)
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
	if (!(sp->so_type)) {
		errno = EBADF;
		return(-1);
	}
	if (sp->so_type != SOCK_STREAM) {
		errno = EOPNOTSUPP;
		return(-1);
	}
	if (sp->so_state & (SS_ISCONNECTED | SS_ISCONNECTING | SS_ISLISTENNING)) {
		errno = EISCONN;
		return(-1);
	}
	if (backlog > SOMAXCONN)
		backlog = SOMAXCONN;

	_imige_reg.h.ah = IMG_LISTEN;
	_imige_reg.h.al = sd;
	_imige_reg.x.bx = backlog;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	sp->so_state |= SS_ISLISTENNING;
	return(0);
}
