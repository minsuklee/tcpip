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
rcmd(char **host, unsigned port, char *lusr, char *rusr, char *cmd, int *fd2p)
{
	int s, timo = 1, oldmask;
	struct sockaddr_in sin, sin2, from;
	char c;
	int lport, ret;
	struct hostent *hp;

	hp = gethostbyname(*host);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", *host);
		return(-1);
	}
	*host = hp->h_name;
	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			perror("rcmd: socket");
			return(-1);
		}
		sin.sin_family = hp->h_addrtype;
		sin.sin_addr.s_addr = *(unsigned long *)(hp->h_addr);
		sin.sin_port = port;
		if ((ret = __async_connect(s, (struct sockaddr *)&sin, sizeof (sin), 5, 3)) >= 0)
			break;
		closesocket(s);
		if (ret == -2) {
			goto c_error;
		}
		if ((errno == ECONNREFUSED) && (timo <= 20)) {
			timo++;
			continue;
		}
c_error:	perror(hp->h_name);
		return(-1);
	}
	//printf("CONNECTED to %08lx lport = %d\n", sin.sin_addr.s_addr, lport);
	if (fd2p == 0) {
		send(s, "", 1, 0);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;
		int len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		sprintf(num, "%d", lport);
		if (send(s, num, strlen(num) + 1, 0) != strlen(num) + 1) {
			perror("send: setting up stderr");
			closesocket(s2);
			goto bad;
		}
		printf("Wait for second socket = %d\n", lport);
		s3 = __async_accept(s2, (struct sockaddr *)&from, &len, 10, 3);
		closesocket(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		from.sin_port = ntohs((unsigned short)from.sin_port);
		if (from.sin_family != AF_INET || from.sin_port >= IPPORT_RESERVED) {
			fprintf(stderr, "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
		printf("Accepted socket = %d\n", s3);
	}
	printf("send strings '%s' '%s' '%s'\n", lusr, rusr, cmd);
	send(s, lusr, strlen(lusr) + 1, 0);
	send(s, rusr, strlen(rusr) + 1, 0);
	send(s, cmd,  strlen(cmd)  + 1, 0);
	printf("receive strings\n");
	if (recv(s, &c, 1, 0) != 1) {
		perror(*host);
		goto bad2;
	}
	if (c != 0) {
		while (recv(s, &c, 1, 0) == 1) {
			write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	printf("string received\n");
	return(s);
bad2:	if (lport)
		closesocket(*fd2p);
bad:	closesocket(s);
	return(-1);
}
