/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

struct _IMIGE_SOCKET _imige_sock[MAX_TCB];

int
socket(int family, int type, int protocol)
{
	union  REGS _imige_reg;
	struct _IMIGE_SOCKET *sp;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);
	if (family != AF_INET) {
		errno = EAFNOSUPPORT;
		return(-1);
	}
	switch (type) {
		case SOCK_STREAM :
			if (protocol && (protocol != IPPROTO_TCP))
				goto protoerror;
			_imige_reg.x.bx = IPPROTO_TCP;
			break;
		case SOCK_DGRAM :
			if (protocol && (protocol != IPPROTO_UDP))
				goto protoerror;
			_imige_reg.x.bx = IPPROTO_UDP;
			break;
		case SOCK_RAW :
			if (protocol && (protocol != IPPROTO_ICMP)) {
protoerror:			errno = EPROTONOSUPPORT;
				return(-1);
			}
			_imige_reg.x.bx = IPPROTO_ICMP;
			break;
		default :
			errno = EACCES;
			return(-1);
	}
	_imige_reg.h.ah = IMG_SOCKET;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {
		errno = ENOBUFS;
		return(-1);
	}
	sp = _imige_sock + _imige_reg.x.ax;
	sp->so_type = type;
	sp->so_options = 0;
	sp->so_state = 0;
	return(_imige_reg.x.ax);
}

int
closesocket(int sd)
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
	if (sp->so_type == 0) {
		errno = EBADF;
		return(-1);
	}
	// Check Linger, if linger wait until CLOSED
	_imige_reg.h.ah = IMG_CLOSE;
	_imige_reg.h.al = sd;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	sp->so_type = 0;
	return(0);
}
