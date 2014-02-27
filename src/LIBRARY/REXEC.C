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
rexec(char **host, int rport, char *name, char *pass, char *cmd, int *fd2p)
{
	int s, timo = 1, s3;
	struct sockaddr_in sin, sin2, from;
	char c;
	short port;
	struct hostent *hp;

	hp = gethostbyname(*host);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", *host);
		return(-1);
	}
	*host = hp->h_name;
	ruserpass(hp->h_name, &name, &pass);
retry:
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("rexec: socket");
		return(-1);
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = rport;
	sin.sin_addr.s_addr = *(unsigned long *)(hp->h_addr);
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		if (errno == ECONNREFUSED && timo <= 16) {
			closesocket(s);
			goto retry;
		}
		perror(hp->h_name);
		return(-1);
	}
	if (fd2p == 0) {
		send(s, "", 1, 0);
		port = 0;
	} else {
		char num[8];
		int s2, sin2len;
		
		s2 = socket(AF_INET, SOCK_STREAM, 0);
		if (s2 < 0) {
			closesocket(s);
			return(-1);
		}
		listen(s2, 1);
		sin2len = sizeof (sin2);
		if (getsockname(s2, (struct sockaddr *)&sin2, &sin2len) < 0) {
			perror("getsockname");
			closesocket(s2);
			goto bad;
		}
		port = ntohs((u_short)sin2.sin_port);
		sprintf(num, "%d", port);
		send(s, num, strlen(num) + 1, 0);
		{
			int len = sizeof (from);
			s3 = accept(s2, (struct sockaddr *)&from, &len);
			closesocket(s2);
			if (s3 < 0) {
				perror("accept");
				port = 0;
				goto bad;
			}
		}
		*fd2p = s3;
	}
	send(s, name, strlen(name) + 1, 0);
	send(s, pass, strlen(pass) + 1, 0);
	send(s, cmd, strlen(cmd) + 1, 0);
	if (recv(s, &c, 1, 0) != 1) {
		perror(*host);
		goto bad;
	}
	if (c != 0) {
		while (recv(s, &c, 1, 0) == 1) {
			send(2, &c, 1, 0);
			if (c == '\n')
				break;
		}
		goto bad;
	}
	return(s);
bad:	if (port)
		closesocket(*fd2p);
	closesocket(s);
	return(-1);
}
