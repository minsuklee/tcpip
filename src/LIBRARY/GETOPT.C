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
getsockopt(int sd, int level, int optname, char *optval, int *optlen)
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

	switch (optname) {
		case SO_LINGER :
			if (*optlen < sizeof(struct linger)) {
				errno = EFAULT;
				return(-1);
			}
			if (sp->so_options & SO_LINGER) {
				((struct linger *)optval)->l_onoff = 1;
			} else {
				((struct linger *)optval)->l_onoff = 0;
			}
			((struct linger *)optval)->l_linger = sp->so_linger;
			*optlen = sizeof(struct linger);
			break;
		default :
			if (*optlen < sizeof(int)) {
				errno = EFAULT;
				return(-1);
			}
			*optlen = sizeof(int);
			*(int *)optval = (sp->so_options & optname) ? 1 : 0;
	}
	return(0);
}
