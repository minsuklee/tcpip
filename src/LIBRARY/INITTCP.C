/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

int _imige_vec = 0;
int __tcp_type = -1;

int
_init_imigetcp(int flag)
{
	union   REGS _imige_reg;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);
	printf("TCP/IP Found at Vector 0x%X [%d].\n", _imige_vec, _num_sock);
	_imige_reg.h.ah = IMG_INITTCPIP;
	_imige_reg.x.bx = flag;		/* 1 : Uninstall, 0:init */
	int86(_imige_vec, &_imige_reg, &_imige_reg);
	__tcp_type = _imige_reg.x.ax;
	return(__tcp_type);
}
