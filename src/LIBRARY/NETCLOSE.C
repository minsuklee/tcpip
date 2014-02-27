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
_net_close()
{
	int i;
	struct _IMIGE_SOCKET *sp;

	if (!_imige_vec)
		if (_find_kernel() == 0)
			exit(1);

	for (sp = _imige_sock, i = 0; i <MAX_TCB; i++, sp++) {
		if (sp->so_type)
			closesocket(i);
	}
}
