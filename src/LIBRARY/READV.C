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
readv(int sd, struct iovec *iov, int iovcnt)
{
	int count, tread = 0;

	while (iovcnt--) {
		if ((count = recv(sd, iov->iov_base, iov->iov_len, 0)) != iov->iov_len)
			if ((count == -1) && !tread)
				return(-1);
			else
				return(tread + count);
		else
			tread += count;
		iov++;
	}
	return(tread);
}
