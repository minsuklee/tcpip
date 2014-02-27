/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <stdio.h>
#include <io.h>
#include <dos.h>
#include <stdlib.h>	/* for sys_errlist, sys_nerr */
#include <string.h>

#include <errno.h>

extern int errno;

static struct _tcp_error {
	int Err_no;
	char *Err_msg;
};

struct _tcp_error  ___nerror[] = {
	{ EWOULDBLOCK, "Operation would block" },
	{ EINPROGRESS, "Operation now in progress" },
	{ EALREADY, "Operation already in progress" },
	{ ENOTSOCK, "Socket operation on non-socket" },
	{ EDESTADDRREQ, "Destination address required" },
	{ EMSGSIZE, "Message too long" },
	{ EPROTOTYPE, "Protocol wrong type for socket" },
	{ ENOPROTOOPT, "Protocol not available" },
	{ EPROTONOSUPPORT, "Protocol not supported" },
	{ ESOCKTNOSUPPORT, "Socket type not supported" },
	{ EOPNOTSUPP, "Operation not supported on socket" },
	{ EPFNOSUPPORT, "Protocol family not supported" },
	{ EAFNOSUPPORT, "Address family not supported by protocol family" },
	{ EADDRINUSE, "Address already in use" },
	{ EADDRNOTAVAIL, "Can't assign requested address" },
	{ ENETDOWN, "Network is down" },
	{ ENETUNREACH, "Network is unreachable" },
	{ ENETRESET, "Network dropped connection on reset" },
	{ ECONNABORTED, "Software caused connection abort" },
	{ ECONNRESET, "Connection reset by peer" },
	{ ENOBUFS, "No buffer space available" },
	{ EISCONN, "Socket is already connected" },
	{ ENOTCONN, "Socket is not connected" },
	{ ESHUTDOWN, "Can't send after socket shutdown" },
	{ ETOOMANYREFS, "Too many references: can't splice" },
	{ ETIMEDOUT, "Connection timed out" },
	{ ECONNREFUSED, "Connection refused" },
	{ ELOOP, "Too many Levels of symbolic links" },
	{ ENAMETOOLONG, "File name too long" },
	{ EHOSTDOWN, "Host is down" },
	{ EHOSTUNREACH, "No route to host" },
	{ ENOTEMPTY, "Directory not empty" },
	{ ESTALE, "Stale NFS file handle" },
	{ EREMOTE, "Too many levels of remote in path" },
};

static char *Unknown_error = "Unknown error";

void _Cdecl
perror(const char *s)
{
	int i;

	write(2, (char *)s, strlen(s));
	write(2, ": ", 2);
	if (errno < EWOULDBLOCK) {
		write(2, sys_errlist[errno], strlen(sys_errlist[errno]));
		goto done;
	}
	if (errno <= EREMOTE) {
	   for (i = 0; i < sizeof(___nerror)/sizeof(struct _tcp_error); i++) {
		if (___nerror[i].Err_no == errno) {
		   write(2, ___nerror[i].Err_msg, strlen(___nerror[i].Err_msg));
		   goto done;
		}
	   }
	}
	write(2, Unknown_error, strlen(Unknown_error));
done:
	write(2, "\n", 1);
}
