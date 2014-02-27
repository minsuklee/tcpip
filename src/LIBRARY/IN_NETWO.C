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
inet_network(char *addr_str)
{
	int n;
	unsigned long a, b, c, d, addr;

	n = sscanf(addr_str, "%ld.%ld.%ld.%ld", &a, &b, &c, &d);
	switch (n) {
		case 0 :
			return(-1L);
		case 1 :
			addr = a;
			break;
		case 2 :
			addr = a * 0x1000000L + b;
			break;
		case 3 :
			addr = a * 0x1000000L + b * 0x10000L + c;
			break;
		case 4 :
			addr = a * 0x1000000L + b * 0x10000L + c * 0x100L + d;
			break;
	}
	return(ntohl(addr));
}
