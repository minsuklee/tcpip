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
gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	tp->tv_sec = time(NULL);
	tp->tv_usec = 0L;
	tzp->tz_minuteswest = -9 * 60;
	tzp->tz_dsttime = 0;
	return(0);
}
