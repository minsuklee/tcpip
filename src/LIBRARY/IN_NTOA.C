/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

char *
inet_ntoa(struct in_addr in)
{
	static char tmp_str[16]; 
	unsigned char *tmp = (char *)&in;

	sprintf(tmp_str, "%d.%d.%d.%d", *tmp, *(tmp+1), *(tmp+2), *(tmp+3));
	return(tmp_str);
}
