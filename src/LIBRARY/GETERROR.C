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
_get_errno(int sd)
{
	union  REGS _imige_reg;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);
	_imige_reg.h.ah = IMG_GETERROR;
	_imige_reg.h.al = sd;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	return(_imige_reg.x.ax);
}
