/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

// We should Optimize BSWAP

unsigned short
bswap(unsigned short input)
{
	return((input / 256) + (input % 256) * 256);
}
