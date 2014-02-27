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
_read_config(struct TCPCONFIG *cfg)
{
	FILE *fp;
	char _imige_path[100], far *str;

	if ((str = getenv(IMIGETCP_ENV)) == NULL) {
		fprintf(stderr, "SET TCPIP=dir first\n");
		return(-1);
	}
	strcpy(_imige_path, str);
	strcat(_imige_path, "\\");
	strcat(_imige_path, CFGFILENAME);
	if ((fp = fopen(_imige_path, "rb")) == NULL) {
f_error:	perror(_imige_path);
		return(-1);
	} else {
		while (fgetc(fp) != 0x1a)
			;
		if (fread(cfg, sizeof(struct TCPCONFIG), 1, fp) != 1) {
			goto f_error;
		}
		fclose(fp);
	}
	return(0);
}
