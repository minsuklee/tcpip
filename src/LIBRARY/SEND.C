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
send(int sd, char *data, int count, int flag)
{
	struct _IMIGE_SOCKET *sp = _imige_sock + sd;
	union  REGS _imige_reg;

	//printf("[SEND %d", count);
	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);

	if ((sd < 0) || (sd >= _num_sock)) {
		errno = ENOTSOCK;
		return(-1);
	}

	if ((sp->so_state & SS_ISCONNECTED) && (sp->so_type == SOCK_DGRAM)) {
		return(sendto(sd, data, count, flag, NULL, 0));
	}

	if (sp->so_type != SOCK_STREAM) {
		errno = EBADF;
		return(-1);
	}
	if (sp->so_state & SS_CANTSENDMORE) {
		errno = ESHUTDOWN;
		return(-1);
	}
	if (!(sp->so_state & SS_ISCONNECTED)) {
		errno = ENOTCONN;
		return(-1);
	}

	if (count == 0)
		return(0);

loop_here:

	_imige_reg.h.ah = IMG_SEND;
	_imige_reg.h.al = sd;
	_imige_reg.x.si = FP_SEG(data);
	_imige_reg.x.di = FP_OFF(data);
	_imige_reg.x.cx = count;
	_imige_reg.x.bx = flag;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax == 0xFFFF) {
		errno = _get_errno(sd);
		//printf(" ER:%d]", errno);
		return(-1);
	} else if (_imige_reg.x.ax == 0) {
no_buffer:	if (sp->so_state & SS_NBIO) {
			errno = EWOULDBLOCK;
			//printf(" NB]");
			return(-1);
		} else {
			//if (kbhit()) { getch(); return(-1); }
			goto loop_here;
		}
	} else {
		//printf(":%d]", _imige_reg.x.ax);
		return(_imige_reg.x.ax);
	}
}
