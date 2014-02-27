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
sendto(int sd, char *data, int count, int flag, struct sockaddr *to, int addrlen)
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
	if (sp->so_state & SS_CANTSENDMORE) {
		errno = ESHUTDOWN;
		return(-1);
	}

	if (!(sp->so_state & SS_ISCONNECTED))
		if (!to || addrlen < 8) {
			errno = EFAULT;
			return(-1);
		}

	if (flag) {			// We dont support UDP flags
//		errno = EFAULT;
//		return(-1);
	}

	if (count == 0)
		return(0);
loop_here:

	// we dont need IMG_UDP_DEST if this socket is a connected one

	if (!(sp->so_state & SS_ISCONNECTED)) {
		_imige_reg.h.ah = IMG_UDP_DEST;
		_imige_reg.h.al = sd;
		_imige_reg.x.dx = __highw(((struct sockaddr_in *)to)->sin_addr.s_addr);
		_imige_reg.x.cx = __loww(((struct sockaddr_in *)to)->sin_addr.s_addr);
		_imige_reg.x.bx = ((struct sockaddr_in *)to)->sin_port;
		int86(_imige_vec, &_imige_reg, &_imige_reg);
		if (_imige_reg.x.ax == 0xFFFF) {// Previous Data not sent yet
			errno = _get_errno(sd);
			if (errno == EWOULDBLOCK)
				goto no_buffer;
			else
				return(-1);
		}
	}

	_imige_reg.h.ah = IMG_SENDTO;
	_imige_reg.h.al = sd;
	_imige_reg.x.si = FP_SEG(data);
	_imige_reg.x.di = FP_OFF(data);
	_imige_reg.x.bx = count;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {	// Kernel doest return with -1
		errno = _get_errno(sd);
		return(-1);
	} else if (_imige_reg.x.ax == 0) {
no_buffer:	if (sp->so_state & SS_NBIO) {
			errno = EWOULDBLOCK;	
			return(-1);
		} else {
			//if (kbhit()) { getch(); return(-1); }
			goto loop_here;
		}
	} else
		return(_imige_reg.x.ax);
}
