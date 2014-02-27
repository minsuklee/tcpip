/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

struct timeval {
	unsigned long tv_sec;
	unsigned long tv_usec;
};

struct timezone {
	short tz_minuteswest;
	short tz_dsttime;
};
