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
inet_addr(char *addr_str)
{
	unsigned long addr, n, a, b, c, d;

	n = sscanf(addr_str, "%ld.%ld.%ld.%ld", &a, &b, &c, &d);
	switch (n) {
	   case 1:	// a       -- 32 bits
		addr = a;
		break;
	   case 2:	// a.b     -- 8.24 bits
		addr = (a << 24) | (b & 0xffffff);
		break;
	   case 3:	// a.b.c   -- 8.8.16 bits
		addr = (a << 24) | ((b & 0xff) << 16) | (c & 0xffff);
		break;
	   case 4:	// a.b.c.d -- 8.8.8.8 bits
		addr = (a << 24) | ((b & 0xff) << 16) | ((c & 0xff) << 8) | (d & 0xff);
		break;
	   default:
		return (0xFFFFFFFFL);
	}
	return(htonl(addr));	// to Network Order
}
