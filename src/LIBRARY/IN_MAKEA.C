/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

unsigned long
inet_makeaddr(unsigned long net, unsigned long lna)
{
	unsigned long addr;

	if (net < 128L)
		addr = (net << IN_CLASSA_NSHIFT) | lna;
	else if (net < 65536L)
		addr = (net << IN_CLASSB_NSHIFT) | lna;
	else
		addr = (net << IN_CLASSC_NSHIFT) | lna;
	addr = htonl(addr);
	return (addr);
}
