/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>

int
find_signature(char *sign_str, int sign_len, int v_start, int v_end)
{
	int i, j;
	char far *tsr_str;
	char *mysign;

	for (i = v_start; i <= v_end; i++) {
		tsr_str = (char far *)(*(unsigned long far *)(i * 4) + 3);
		mysign = sign_str;
		for (j = 0; j < sign_len; j++)
			if (*mysign++ != *tsr_str++)
				goto next_vec;
		return(i);
next_vec:	;
	}
	return(0);
}

void
main()
{
	union   REGS _imige_reg;
	int vec, ch;

	printf("TCP/IP Unload Utility,  Version 1.00,\n");
	printf("(C) Copyright 1998,  Imageland Inc.,  All Rights Reserved.\n");

	if (!(vec = find_signature(TSR_SIGNATURE, TSR_SIGLEN, 0x60, 0x7e))) {
		printf("TCP/IP kernel is not loaded.\n");
		goto check_slip;
	}
	printf("TCP/IP is found at vector 0x%X", vec);

	_imige_reg.h.ah = IMG_INITTCPIP;
	_imige_reg.x.bx = 1;		/* 1 : Uninstall, 0:init */
	int86(vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.x.ax != 0xFFFF)
		printf(" and unloaded.\n");

	if (_imige_reg.x.ax != IMIGETCP_VER)
		return;
check_slip:
	if (!(vec = find_signature("PKT DRVR", 8, 0x60, 0x7e)))
		return;
	_imige_reg.x.ax = 0x01FF;	// Get Driver Info
	_imige_reg.x.bx = 0xFFFF;
	int86(vec, &_imige_reg, &_imige_reg);
	if (_imige_reg.h.ch != 6)
		return;

	// There is Packet Driver and it's SLIP Driver

//	if (_imige_reg.h.cl == 0)
		printf("SLIP Driver");
//	else
//		printf("IMIGE/TCP Buffer");
	printf(" Driver is found at vector 0x%X  Unload ? (Y/N) ", vec);
	ch = getche();
	printf("\n");
	if ((ch == 'Y') || (ch == 'y')) {
		_imige_reg.h.ah = 0x05;		// Terminate
		int86(vec, &_imige_reg, &_imige_reg);
//		if (_imige_reg.h.cl == 0)
			printf("SLIP Driver");
//		else
//			printf("IMIGE/TCP Buffer");
		printf(" Driver is unloaded.\n");
	}
}
