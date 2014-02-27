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
inet_netof(struct in_addr in)
{
	unsigned long addr = ntohl(in.s_addr);

	if (IN_CLASSA(addr)) {
		addr &= IN_CLASSA_NET;
		addr >>= IN_CLASSA_NSHIFT;
	} else if (IN_CLASSB(addr)) {
		addr &= IN_CLASSB_NET;
		addr >>= IN_CLASSB_NSHIFT;
	} else {
		addr &= IN_CLASSC_NET;
		addr >>= IN_CLASSC_NSHIFT;
	}
	return (addr);
}
