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
connect(int sd, struct sockaddr *toaddr, int addrlen)
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
	if ((sp->so_state & SS_ISCONNECTING) && (sp->so_state & SS_NBIO)) {
		errno = EALREADY;
		return(-1);
	}

	// UDP socket allows multiple connection
	if ((sp->so_type == SOCK_STREAM) &&
	    (sp->so_state & (SS_ISCONNECTING | SS_ISCONNECTED))) {
		errno = EISCONN;
		return(-1);
	}

	if (!toaddr || addrlen < 8) {
		errno = EFAULT;
		return(-1);
	}

	if (toaddr->sa_family != AF_INET) {
		errno = EAFNOSUPPORT;
		return(-1);
	}		

	sp->so_state |= SS_ISCONNECTING;

	_imige_reg.h.ah = IMG_CONNECT;
	_imige_reg.h.al = sd;
	_imige_reg.x.dx = __highw(((struct sockaddr_in *)toaddr)->sin_addr.s_addr);
	_imige_reg.x.cx = __loww(((struct sockaddr_in *)toaddr)->sin_addr.s_addr);
	_imige_reg.x.bx = ((struct sockaddr_in *)toaddr)->sin_port;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {
		errno = _get_errno(sd);
	} else if (sp->so_type == SOCK_DGRAM) {
		if ((((struct sockaddr_in *)toaddr)->sin_addr.s_addr == 0L) &&
		    ((struct sockaddr_in *)toaddr)->sin_port) {
			sp->so_state &= ~(SS_ISCONNECTED | SS_ISCONNECTING);
		} else
			goto connected;
	} else if (sp->so_state & SS_NBIO) {
		errno = EINPROGRESS;	// Now Connection in progress
	} else {
		struct TCB thistcb;

loop_here:	_imige_reg.h.ah = IMG_SOCK_STAT;
		_imige_reg.h.al = sd;
		_imige_reg.x.bx = FP_SEG(&thistcb);
		_imige_reg.x.cx = FP_OFF(&thistcb);
		int86(_imige_vec, &_imige_reg, &_imige_reg);

		if (thistcb.status & TCB_TCP_SYN) {
			// ARP is undone, yet
			goto loop_here;
		}
		switch (thistcb.t_state) {
			case TCPS_ESTABLISHED :
connected:			sp->so_state &= ~SS_ISCONNECTING;
				sp->so_state |= SS_ISCONNECTED;
				return(0);
			case TCPS_SYN_SENT :
				goto loop_here;
			case TCPS_CLOSED :
				sp->so_state &= ~SS_ISCONNECTING;
				errno = _get_errno(sd);
				break;
			default :
				errno = EFAULT;
				break;
		}
	}
	return(-1);
}
