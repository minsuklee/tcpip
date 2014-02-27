/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

void
_net_stat(struct snmp_object *stat, int clearflag)
{
	union   REGS _imige_reg;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);
	_imige_reg.h.ah = IMG_TCP_STAT;
	_imige_reg.x.bx = FP_SEG(stat);
	_imige_reg.x.cx = FP_OFF(stat);
	_imige_reg.h.al = clearflag;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
}
