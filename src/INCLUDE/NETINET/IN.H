/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 /*
  * Constants and structures defined by the internet system,
  * Per RFC 790, September 1981.
  */

/*
 * Protocols
 */
#define IPPROTO_IP		0		/* dummy for IP */
#define IPPROTO_ICMP	1		/* control message protocol */
#define IPPROTO_GGP		2		/* gateway^2 (deprecated) */
#define IPPROTO_TCP		6		/* tcp */
#define IPPROTO_EGP		8		/* exterior gateway protocol */
#define IPPROTO_PUP		12		/* pup */
#define IPPROTO_UDP		17		/* user datagram protocol */
#define IPPROTO_IDP		22		/* xns idp */

#define IPPROTO_RAW		255		/* raw IP packet */
#define IPPROTO_MAX		256

/*
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#define IPPORT_RESERVED 	1024
#define IPPORT_USERRESERVED	5000

/*
 * Link numbers
 */
#define IMPLINK_IP			155
#define IMPLINK_LOWEXPER	156
#define IMPLINK_HIGHEXPER	158

/*
 * Internet address (a structure for historical reasons)
 */

struct in_addr {
    union {
		struct { unsigned char	s_b1, s_b2, s_b3, s_b4; }	S_un_b;
		struct { unsigned short s_w1, s_w2; }			  	S_un_w;
		unsigned long S_addr;
	} S_un;
#define s_addr	S_un.S_addr
#define	s_net	S_un.S_un_b.s_b1
#define	s_host	S_un.S_un_b.s_b2
#define	s_lh	S_un.S_un_b.s_b3
#define	s_impno	S_un.S_un_b.s_b4
#define	s_imp	S_un.S_un_w.s_w2
};

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define IN_CLASSA(i)		(((long)(i) & 0x80000000L) == 0L)
#define IN_CLASSA_NET		0xff000000L
#define IN_CLASSA_NSHIFT	24
#define IN_CLASSA_HOST		0x00ffffffL
#define IN_CLASSA_MAX		128

#define IN_CLASSB(i)		(((long)(i) & 0xc0000000L) == 0x80000000L)
#define IN_CLASSB_NET		0xffff0000L
#define IN_CLASSB_NSHIFT	16
#define IN_CLASSB_HOST		0x0000ffffL
#define IN_CLASSB_MAX		65536L

#define IN_CLASSC(i)		(((long)(i) & 0xc0000000L) == 0xc0000000L)
#define IN_CLASSC_NET		0xffffff00L
#define IN_CLASSC_NSHIFT	8
#define IN_CLASSC_HOST		0x000000ffL

#define INADDR_ANY			(unsigned long)0x00000000L
#define INADDR_BROADCAST	(unsigned long)0xffffffffL

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	short	sin_family;
	unsigned short sin_port;
	struct in_addr sin_addr;
	char	sin_zero[8];
};
