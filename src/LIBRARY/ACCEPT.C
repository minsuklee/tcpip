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
accept(int sd, struct sockaddr *peer, int *addrlen)
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
	if (sp->so_type != SOCK_STREAM) {
		errno = EBADF;
		return(-1);
	}
	if (!(sp->so_state & SS_ISLISTENNING)) {
		errno = EOPNOTSUPP;
		return(-1);
	}
	if (peer && *addrlen < 8) {
		errno = EFAULT;
		return(-1);
	}

loop_here:

	_imige_reg.h.ah = IMG_ACCEPT;
	_imige_reg.h.al = sd;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {
		if (sp->so_state & SS_NBIO) {
			errno = EWOULDBLOCK;
			return(-1);
		} else {
			//if (kbhit()) { getch(); return(-1); }
			goto loop_here;
		}
	} else {
		int accepted = _imige_reg.x.ax;
		struct _IMIGE_SOCKET *accept_sp = _imige_sock + accepted;

		accept_sp->so_type = sp->so_type;
		accept_sp->so_state = SS_ISCONNECTED;
		accept_sp->so_options = sp->so_options;
		if (peer) {
			struct TCB thistcb;

			_imige_reg.h.ah = IMG_SOCK_STAT;
			_imige_reg.h.al = accepted;
			_imige_reg.x.bx = FP_SEG(&thistcb);
			_imige_reg.x.cx = FP_OFF(&thistcb);
			int86(_imige_vec, &_imige_reg, &_imige_reg);
			((struct sockaddr_in *)peer)->sin_family = AF_INET;
			((struct sockaddr_in *)peer)->sin_addr.s_addr = thistcb.IP_HDR.destination;
			((struct sockaddr_in *)peer)->sin_port = thistcb.remote_port;
			*addrlen = 8;
		}
		return(accepted);
	}
}
