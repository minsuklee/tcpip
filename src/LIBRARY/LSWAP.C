/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

// We should Optimize LSWAP

unsigned long
lswap(unsigned long input)
{
	return( (input / 0x1000000L) +
		((input / 0x10000L) % 0x100L) * 0x100L +
		((input / 0x100L) % 0x100L) * 0x10000L +
		((input % 0x100) * 0x1000000L));
}
