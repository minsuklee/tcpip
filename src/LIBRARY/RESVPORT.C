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
rresvport(int *port)
{
	union  REGS _imige_reg;
	int sd;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
		return (-1);
	_imige_reg.h.ah = IMG_RBIND;
	_imige_reg.h.al = sd;
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	*port = _imige_reg.x.ax;
	return(sd);
}
