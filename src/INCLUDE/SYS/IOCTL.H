/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#define F_GETFL		3	/* get file flags */
#define F_SETFL		4	/* set file flags */

#define	FREAD		0x0001	/* descriptor is readable */
#define	FWRITE		0x0002	/* descriptor is writeable */
#define	FNDELAY		0x0004	/* non-blocking */
#define	FAPPEND		0x0008	/* append on each write */
#define	FASYNC		0x0080	/* signal when data ready */

#define	FIONREAD	0x0005
#define	FIONBIO		0x0006
#define	FIOASYNC	0x0007
