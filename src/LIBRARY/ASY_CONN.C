/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

// sec : wait time in second

// key == FFFF  : do not check keyboard
//	     0  : Any key return
//	  00xx  : scan code
//	  FFxx  : Extended Code

// return  0 : connected
//        -1 : error occured
//	  -2 : user aborted
//	  -3 : timeout

int
__async_connect(int sd, struct sockaddr *sin, int len, int sec, unsigned key)
{
	struct _IMIGE_SOCKET *sp = _imige_sock + sd;

	int nbio = (sp->so_state & SS_NBIO);
	int data = 1, input;

	fd_set ready;
	struct timeval time;

	ioctlsocket(sd, FIONBIO, (char *)&data);
	if (connect(sd, sin, len) < 0) {
		if (errno != EINPROGRESS)
			return(-1);
	}

	if (sec <= 10000)
		sec *= 10;
	if (key != 0xFFFF) {
		while (bioskey(1))
			bioskey(0);
	}
	time.tv_sec = 0L;
	time.tv_usec = 100000L;		// 100msec
        FD_ZERO(&ready);
	//printf("WHILE LOOP - \n");
	while (sec) {
		FD_ZERO(&ready);
                FD_SET(sd, &ready);
                if (select(32, NULL, &ready, NULL, &time) < 0) {
closed:			//printf("SELECT ERROR\n");
			return(-1);
                }
		if (FD_ISSET(sd, &ready)) {
			//printf("Connected\n");
			break;
		}
		else if ((key != 0xFFFF) && bioskey(1)) {
			input = bioskey(0) & 0xff;
			if (input == 0) {	// function key
				input = bioskey(0) & 0xff;
				if (!key || (((key & 0xff00) == 0xff00) && (key == input)))
					goto match;
			}
			//if (input == '!') {
			//	system("command.com");
			//	continue;
			//}			
			if ((input == key) || !key) {
match:				//printf("Accept Aborted by User\n");
				while (bioskey(1))
					bioskey(0);
				errno = ECONNABORTED;
				return(-2);
			}
		}
		sec--;
	}
	//printf("LOOP END \n");
	if (!nbio) {
		data = 0;
		ioctlsocket(sd, FIONBIO, (char *)&data);
	}
	if (sec == 0) {
		errno = ETIMEDOUT;
		return(-3);
	}
	errno = _get_errno(sd);
	if (errno)
		return(-1);
	return(0);
}
