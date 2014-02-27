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
setsockopt(int sd, int level, int optname, char *optval, int optlen)
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

	if (((unsigned)level != SOL_SOCKET) || (optname > 0x1000)) {
		errno = ENOPROTOOPT;
		return(-1);
	}

	if (optlen < sizeof(int)) {
		errno = EFAULT;
		return(-1);
	}

	switch (optname) {
		case SO_LINGER :
			if (optlen < sizeof(struct linger)) {
				errno = EFAULT;
				return(-1);
			}
			if (((struct linger *)optval)->l_onoff) {
				sp->so_options |= SO_LINGER;
				sp->so_linger = ((struct linger *)optval)->l_linger;
			} else {
				sp->so_options &= ~SO_LINGER;
				sp->so_linger = 0;
			}
			break;
		case SO_KEEPALIVE :
		case SO_BROADCAST :
		case SO_DONTROUTE :
		case SO_OOBINLINE :
			_imige_reg.h.ah = IMG_SETOPTION;
			_imige_reg.h.al = sd;
			_imige_reg.x.dx = optname;
			_imige_reg.x.bx = (*(int *)optval) ? 1 : 0;
			int86(_imige_vec, &_imige_reg, &_imige_reg);
			if (*(int *)optval)
				sp->so_options |= optname;
			else
				sp->so_options &= ~optname;
			break;
		default :
			if (*(int *)optval)
				sp->so_options |= optname;
			else
				sp->so_options &= ~optname;
	}
	return(0);
}
