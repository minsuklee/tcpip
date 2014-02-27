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
gethostid()
{
	struct TCPCONFIG config;

	_read_config(&config);
	return(config.c_kernel.c_myip);	
}
