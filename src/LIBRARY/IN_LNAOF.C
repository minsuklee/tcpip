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
inet_lnaof(struct in_addr in)
{
	unsigned long addr = ntohl(in.s_addr);

	if (IN_CLASSA(addr))
		addr &= IN_CLASSA_HOST;
	else if (IN_CLASSB(addr))
		addr &= IN_CLASSB_HOST;
	else
		addr &= IN_CLASSC_HOST;
	return(htonl(addr));
}
