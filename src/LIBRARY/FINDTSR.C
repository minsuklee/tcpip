/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

#define	FIRST_VEC	0x60
#define	LAST_VEC	0x7E

int _num_sock;

int
_find_kernel()
{
	int i, j;
	char far *tsr_str;
	char *mysign;
	struct POINTERS ptr;
	union  REGS _imige_reg;

	for (i = FIRST_VEC; i <= LAST_VEC; i++) {
		tsr_str = (char far *)(*(unsigned long far *)(i * 4) + 3);
		mysign = TSR_SIGNATURE;
		for (j = 0; j < TSR_SIGLEN; j++)
			if (*mysign++ != *tsr_str++)
				goto next_vec;
		_imige_vec = i;
		_imige_reg.h.ah = IMG_GETPOINTER;
		_imige_reg.x.bx = _imige_reg.x.cx = 0;
		int86(_imige_vec, &_imige_reg, &_imige_reg);
		_num_sock = _imige_reg.x.ax;
		return(i);
next_vec:	;
	}
	fprintf(stderr, "TCP/IP Kernel Not Loaded.\n");
	return(0);
}
