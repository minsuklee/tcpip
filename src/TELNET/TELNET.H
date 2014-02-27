/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 /*
 * Definitions for the TELNET protocol.
 */
#define	IAC		255		/* interpret as command: */
#define	DONT	254		/* you are not to use option */
#define	DO		253		/* please, you use option */
#define	WONT	252		/* I won't use option */
#define	WILL	251		/* I will use option */
#define	SB		250		/* interpret as subnegotiation */
#define	GA		249		/* you may reverse the line */
#define	EL		248		/* erase the current line */
#define	EC		247		/* erase the current character */
#define	AYT		246		/* are you there */
#define	AO		245		/* abort output--but let prog finish */
#define	IP		244		/* interrupt process--permanently */
#define	BREAK	243		/* break */
#define	DM		242		/* data mark--for connect. cleaning */
#define	NOP		241		/* nop */
#define	SE		240		/* end sub negotiation */
#define EOR     239     /* end of record (transparent mode) */
#define ABORT   238     /* Abort process */
#define SUSP    237     /* Suspend process */
#define xEOF    236     /* End of file: EOF is already used... */
#define SYNCH	242		/* for telfunc calls */

/* telnet options */
#define TELOPT_BINARY	0	/* 8-bit data path */
#define TELOPT_ECHO		1	/* echo */
#define	TELOPT_RCP		2	/* prepare to reconnect */
#define	TELOPT_SGA		3	/* suppress go ahead */
#define	TELOPT_NAMS		4	/* approximate message size */
#define	TELOPT_STATUS	5	/* give status */
#define	TELOPT_TM		6	/* timing mark */
#define	TELOPT_RCTE		7	/* remote controlled transmission and echo */
#define TELOPT_NAOL 	8	/* negotiate about output line width */
#define TELOPT_NAOP 	9	/* negotiate about output page size */
#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
#define TELOPT_NAOHTS	11	/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD	12	/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS	14	/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD	15	/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD	16	/* negotiate about output LF disposition */
#define TELOPT_XASCII	17	/* extended ascic character set */
#define	TELOPT_LOGOUT	18	/* force logout */
#define	TELOPT_BM		19	/* byte macro */
#define	TELOPT_DET		20	/* data entry terminal */
#define	TELOPT_SUPDUP	21	/* supdup protocol */
#define	TELOPT_SUPDUPOUTPUT 22	/* supdup output */
#define	TELOPT_SNDLOC	23	/* send location */
#define	TELOPT_TTYPE	24	/* terminal type */
#define	TELOPT_EOR		25	/* end or record */
#define TELOPT_TUID		26	/* TACACS user identification */
#define TELOPT_OUTMRK	27	/* output marking */
#define TELOPT_TTYLOC	28	/* terminal location number */
#define TELOPT_3270REGIME 29	/* 3270 regime */
#define TELOPT_X3PAD	30	/* X.3 PAD */
#define TELOPT_NAWS	31	/* window size */
#define TELOPT_TSPEED	32	/* terminal speed */
#define TELOPT_LFLOW	33	/* remote flow control */
#define TELOPT_LINEMODE 34      /* Linemode option */
#define TELOPT_XDISPLOC	35	/* X Display Location */
#define TELOPT_ENVIRON  36      /* Environment variables */
#define TELOPT_AUTHENTICATION 45 /* XXX Auto Authenticate */
#define TELOPT_EXOPL	255	/* extended-options-list */

/* sub-option qualifiers */
#define	TELQUAL_IS		0	/* option is... */
#define	TELQUAL_SEND	1	/* send option */
#define TELQUAL_INFO    2   /* ENVIRON: informational version of IS */

#define	SUBOPTSIZE	256

/*
 * We keep track of each side of the option negotiation.
 */

#define	MY_STATE_WILL		0x01
#define	MY_WANT_STATE_WILL	0x02
#define	MY_STATE_DO			0x04
#define	MY_WANT_STATE_DO	0x08

/*
 * Macros to check the current state of things
 */

#define	my_st_do(x, o)			((x)->opts[o]&MY_STATE_DO)
#define	my_st_will(x, o)		((x)->opts[o]&MY_STATE_WILL)
#define my_want_st_do(x, o)		((x)->opts[o]&MY_WANT_STATE_DO)
#define my_want_st_will(x, o)	((x)->opts[o]&MY_WANT_STATE_WILL)

#define	my_st_dont(x, o)		(!my_st_do(x, o))
#define	my_st_wont(x, o)		(!my_st_will(x, o))
#define my_want_st_dont(x, o)	(!my_want_st_do(x, o))
#define my_want_st_wont(x, o)	(!my_want_st_will(x, o))

#define	set_my_state_do(x, o)			{(x)->opts[o] |= MY_STATE_DO;}
#define	set_my_state_will(x, o)			{(x)->opts[o] |= MY_STATE_WILL;}
#define	set_my_want_state_do(x, o)		{(x)->opts[o] |= MY_WANT_STATE_DO;}
#define	set_my_want_state_will(x, o)	{(x)->opts[o] |= MY_WANT_STATE_WILL;}

#define	set_my_state_dont(x, o)			{(x)->opts[o] &= ~MY_STATE_DO;}
#define	set_my_state_wont(x, o)			{(x)->opts[o] &= ~MY_STATE_WILL;}
#define	set_my_want_state_dont(x, o)	{(x)->opts[o] &= ~MY_WANT_STATE_DO;}
#define	set_my_want_state_wont(x, o)	{(x)->opts[o] &= ~MY_WANT_STATE_WILL;}
