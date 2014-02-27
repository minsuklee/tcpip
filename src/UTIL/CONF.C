/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>

void
main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	struct TCPCONFIG config;
	char _imige_path[100], *str;
	int i;

	printf("TCP/IP CONFIGURATION Utility,  Version 1.00,\n");
	printf("(C) Copyright 1998, IMIGE Systems Inc.,  All Rights Reserved.\n");

	if ((str = getenv(IMIGETCP_ENV)) == NULL) {
		fprintf(stderr, "SET TCPIP=dir first\n");
		exit(1);
	}
	strcpy(_imige_path, str);
	strcat(_imige_path, "\\");
	strcat(_imige_path, CFGFILENAME);
	if (argc != 2) {
		fprintf(stderr, "Default Configuration: 147.46.133.35\n");
	}
	if (argc == 1) {
		config.c_kernel.c_myip = 0x23852e93L;
		config.c_kernel.c_subnet = 0xffffL;
		config.c_kernel.c_defgw = 0x63502e93L;
	} else {
		unsigned a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
			printf("Illegal Address => use a.b.c.d (dec)\n");
			exit(1);
		}
		config.c_kernel.c_myip = (unsigned long)(d * 256 + c) * 0x10000L + b * 256 + a;
		config.c_kernel.c_subnet = 0xffffffL;
		config.c_kernel.c_defgw = 0L;
	}
	if ((fp = fopen(_imige_path, "wb")) == NULL) {
		perror(_imige_path);
		exit(1);
	}
	printf("TCP/IP Configuration File : %s\n", _imige_path);
	fprintf(fp, "TCP/IP Configuration File\n\r%c", 0x1a);
	fwrite(&config, sizeof(struct TCPCONFIG), 1, fp);
	fclose(fp);
	printf("CONFIGURATION :\nMYIP:%08lX, NETMASK:%08lX GATEWAY:%08lX",
		config.c_kernel.c_myip,
		config.c_kernel.c_subnet, config.c_kernel.c_defgw);
}
