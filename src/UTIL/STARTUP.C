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
main()
{
	FILE *fp;
	struct TCPCONFIG config;
	char _imige_path[100], *str;
	union   REGS _imige_reg;
	int i;
	unsigned long addr;

	printf("TCP/IP Start-up Utility,  Version 3.00,\n");
	printf("(C) Copyright 1998, IMIGE Systems Inc.,  All Rights Reserved.\n");

	i = _init_imigetcp(0);
	if (i != IMIGETCP_VER) {
		fprintf(stderr, "It's Not the TCP/IP Kernel.\n");
		exit(1);
	}

	if (_read_config(&config) < 0)
		exit(1);

	if ((addr = inet_addr(config.gate_way)) == 0xffffffffL) {
		struct hostent *hp;
		if (strlen(config.gate_way)) {
			if ((hp = gethostbyname(config.gate_way)) == NULL) {
				printf("Gateway '%s' unknown.\n", config.gate_way);
				addr = 0L;
			} else
				addr= *(unsigned long *)(hp->h_addr);
		} else
			addr = 0L;
	}
	config.c_kernel.c_defgw = addr;

#ifdef PRINTCONF
	printf("CONFIGURATION : MyIP:%08lX, Netmask:0x%08lX, Gateway:%08lX\n",
		config.c_kernel.c_myip,
		config.c_kernel.c_subnet, config.c_kernel.c_defgw);
#endif
	_imige_reg.h.ah = IMG_LOADCFG;
	_imige_reg.x.bx = FP_SEG(&config);
	_imige_reg.x.cx = FP_OFF(&config);
	int86(_imige_vec, &_imige_reg, &_imige_reg);

	printf("TCP/IP Started\n");
}
