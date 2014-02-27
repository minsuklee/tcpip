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
getlogin()
{
	struct TCPCONFIG config;
	static char name[CONF_USERLEN];

	_read_config(&config);
	//printf("GETLOGIN = '%s'\n", config.c_userid);
	strcpy(name, config.c_userid);
	return(name);
}
