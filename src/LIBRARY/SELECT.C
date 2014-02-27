/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

// This Implementation implys fd_set is unsigned long type

#define NULL_DS	((fd_set *)0L)

int
select(int n, fd_set *read_ds, fd_set *write_ds, fd_set *except_ds, struct timeval *timeout)
{
	union  REGS _imige_reg;
	struct _IMIGE_SOCKET *sp;
	struct SELECT sel_str;

	fd_set width = 0, oread = 0, owrite = 0, oexcept = 0, which = 0;
	int i, count_selected = 0;
	long old_time, new_time, remained_time; // in milli sec order

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);

	if (n > _num_sock)
		n = _num_sock;

	for (i = 0; i < n; i++) FD_SET(i, &width);

	if (read_ds)	which |= *read_ds;
	if (write_ds)	which |= *write_ds;
	if (except_ds)	which |= *except_ds;
	which &= width;

	remained_time = timeout ? ((timeout->tv_sec) * 1000L + (timeout->tv_usec) / 1000L) : 1;
	old_time = biostime(0, 0L);

loop_here:
	_imige_reg.h.ah = IMG_SELECT;
	_imige_reg.x.dx = FP_SEG(&sel_str);
	_imige_reg.x.si = FP_OFF(&sel_str);
	_imige_reg.x.bx = __highw(which);
	_imige_reg.x.cx = __loww(which);
	int86(_imige_vec, &_imige_reg, &_imige_reg);

	for (sp = _imige_sock, i = 0; i < n; i++, sp++) {
		if (FD_ISSET(i, &which) && FD_ISSET(i, &(sel_str.closed))) {
			//printf("[select: socket %d CLOSED]", i);
			errno = EBADF;
			return(-1);
		}
		if ((read_ds == NULL_DS) || !FD_ISSET(i, read_ds))
			goto test_write;
		if ((sp->so_state & SS_ISLISTENNING) &&
		    FD_ISSET(i, &(sel_str.readfds))) {
			goto readable;
		} else if (((sp->so_type != SOCK_STREAM) ||
			    ((sp->so_state & SS_ISCONNECTED) &&
		             !(sp->so_state & SS_CANTRCVMORE))) &&
			   FD_ISSET(i, &(sel_str.readfds))) {
readable:		FD_SET(i, &oread);
			count_selected++;
		}
test_write:	if ((write_ds == NULL_DS) || !FD_ISSET(i, write_ds))
			goto test_except;
		if ((sp->so_state & SS_ISCONNECTING) &&
		    FD_ISSET(i, &(sel_str.connect))) {
			sp->so_state &= ~SS_ISCONNECTING;
			sp->so_state |= SS_ISCONNECTED;
			goto writable;
		} else if ((sp->so_state & SS_ISCONNECTED) &&
		    !(sp->so_state & SS_CANTSENDMORE) &&
		    FD_ISSET(i, &(sel_str.writefds))) {
writable:		FD_SET(i, &owrite);
			count_selected++;
		}
test_except:	if ((except_ds == NULL_DS) || !FD_ISSET(i, except_ds))
			continue;
		if ((sp->so_type == SOCK_STREAM) &&
		    (sp->so_state & SS_ISCONNECTED) &&
		    !(sp->so_state & SS_CANTRCVMORE) &&
		    FD_ISSET(i, &(sel_str.exceptfds))) {
			FD_SET(i, &oexcept);
			count_selected++;
		}
	}
	if (count_selected || (timeout && (remained_time <= 0))) {
		*(unsigned *)read_ds = oread;
		*(unsigned *)write_ds = owrite;
		*(unsigned *)except_ds = oexcept;
		return(count_selected);
	}
	if (timeout != NULL) {
		new_time = biostime(0,0L);
		remained_time -= ((new_time - old_time) * 54);
		old_time = new_time;
	}
	//printf("[sel t=%ld]", remained_time);
	goto loop_here;	// If (timeout == NULL), Blocking Poll
}
