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
getdomainname(char *domain, int namelen)
{
	struct TCPCONFIG config;

	_read_config(&config);
	if (namelen <= strlen(config.c_domain)) {
		errno = EFAULT;
		return(-1);
	}
	strcpy(domain, config.c_domain);
	return(strlen(config.c_domain));
}
