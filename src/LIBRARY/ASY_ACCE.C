/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

// key == FFFF  : do not check keyboard
//	     0  : Any key return
//	  00xx  : scan code
//	  FFxx  : Extended Code

// return  0..n : accepted (accepted socket in return value)
//        -1 : error occured
//	  -2 : user aborted
//	  -3 : timeout
int
__async_accept(int sd, struct sockaddr *peer, int *addrlen, int sec, unsigned key)
{
        fd_set ready;
	struct timeval time;
	int input;

	if (sec <= 10000)
		sec *= 10;
	if (key != 0xFFFF) {
		while (bioskey(1))
			bioskey(0);
	}
	printf("Wait accept, any key to escape, ! = DOS\n");
	time.tv_sec = 0L;
	time.tv_usec = 100000L;		// 100msec
        FD_ZERO(&ready);
	while (sec) {
                FD_SET(sd, &ready);
                if (select(32, &ready, NULL, NULL, &time) < 0) {
			printf("Select Error\n");
			return(-1);
                }
		if (FD_ISSET(sd, &ready)) {
			return(accept(sd, peer, addrlen));
    		} else if ((key != 0xFFFF) && bioskey(1)) {
			input = bioskey(0) & 0xff;
			if (input == 0) {	// function key
				input = bioskey(0) & 0xff;
				if (!key || (((key & 0xff00) == 0xff00) && (key == input)))
					goto match;
			}
			if (input == '!') {
				system("command.com");
				continue;
			}			
			if ((input == key) || !key) {
match:				printf("Accept Aborted by User\n");
				while (bioskey(1))
					bioskey(0);
				errno = ECONNABORTED;
				return(-2);
			}
		}
		sec--;
	}
	errno = ETIMEDOUT;
	return(-3);
}
