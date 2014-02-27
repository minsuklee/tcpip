/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigelib.h"

// Definition based on rfc883

#define PACKETSZ	512	// maximum packet size
#define MAXDNAME	256	// maximum domain name
#define MAXCDNAME	255	// maximum compressed domain name
#define MAXLABEL	63	// maximum length of domain label
#define QFIXEDSZ	4	// Size of fixed size data in query structure
#define RRFIXEDSZ	10	// Size of fixed size data in resource record

#define NAMESERVER_PORT	53	// Internet nameserver port number

// defined opcodes

#define QUERY		0x0	// standard query
#define IQUERY		0x1	// inverse query
#define STATUS		0x2	// nameserver status query
#define UPDATEA		0x9	// add resource record
#define UPDATED		0xa	// delete a specific resource record
#define UPDATEDA	0xb	// delete all nemed resource record
#define UPDATEM		0xc	// modify a specific resource record
#define UPDATEMA	0xd	// modify all named resource record

#define ZONEINIT	0xe	// initial zone transfer
#define ZONEREF		0xf	// incremental zone referesh

// defined response codes

#define NOERROR		0	// no error
#define FORMERR		1	// format error
#define SERVFAIL	2	// server failure
#define NXDOMAIN	3	// non existent domain
#define NOTIMP		4	// not implemented
#define REFUSED		5	// query refused
#define NOCHANGE	0xf	// update failed to change db

// Type values for resources and queries

#define T_A		1	// host address
#define T_NS		2	// authoritative server
#define T_MD		3	// mail destination
#define T_MF		4	// mail forwarder
#define T_CNAME		5	// connonical name
#define T_SOA		6	// start of authority zone
#define T_MB		7	// mailbox domain name
#define T_MG		8	// mail group member
#define T_MR		9	// mail rename name
#define T_NULL		10	// null resource record
#define T_WKS		11	// well known service
#define T_PTR		12	// domain name pointer
#define T_HINFO		13	// host information
#define T_MINFO		14	// mailbox information
#define T_MX		15	// mail routing information

// non standard

#define T_UINFO		100	// user (finger) information
#define T_UID		101	// user ID
#define T_GID		102	// group ID
#define T_UNSPEC	103	// Unspecified format (binary data)

// Query type values which do not appear in resource records

#define T_AXFR		252	// transfer zone of authority
#define T_MAILB		253	// transfer mailbox records
#define T_MAILA		254	// transfer mail agent records
#define T_ANY		255	// wildcard match

// Values for class field

#define C_IN		1	// the arpa internet
#define C_CHAOS		3	// for chaos net at MIT

// Query class values which do not appear in resource records

#define C_ANY		255	// wildcard match

// Status return codes for T_UNSPEC conversion routines

#define CONV_SUCCESS	 0
#define CONV_OVERFLOW	-1
#define CONV_BADFMT	-2
#define CONV_BADCKSUM	-3
#define CONV_BADBUFLEN	-4

// Structure for query header

typedef struct {
	unsigned short	id;		// query identification number
/* 3 */	unsigned	rd:1;		// recursion desired
	unsigned	tc:1;		// truncated message
	unsigned	aa:1;		// authoritive answer
	unsigned	opcode:4;	// purpose of message
	unsigned	qr:1;		// response flag
/* 4 */	unsigned	rcode:4;	// response code
	unsigned	unused:2;	// unused bits
	unsigned	pr:1;		// primary server required (non standard)
	unsigned	ra:1;		// recursion available
/* 5 */	unsigned short	qdcount;	// number of question entries
	unsigned short	ancount;	// number of answer entries
	unsigned short	nscount;	// number of authority entries
	unsigned short	arcount;	// number of resource entries
} HEADER;

// Defines for handling compressed domain names
#define INDIR_MASK	0xc0

// Structure for passing resource records around.
struct rrec {
	short	r_zone;		// zone number
	short	r_class;	// class number
	short	r_type;		// type number
	unsigned long	r_ttl;	// time to live
	int	r_size;		// size of data area
	char	*r_data;	// pointer to data
};

#define ADDRSORT		1	// enable the address-sorting option
#define MAXADDR			10	// max # addresses to sort by
#define	MAXNS			3	// max # name servers we'll track
#define	MAXDNSRCH		3	// max # default domain levels to try
#define	LOCALDOMAINPARTS	2	// min levels in name that is "local"

typedef union {
    HEADER q_buf1;
    char q_buf2[PACKETSZ];
} query_buf;

static union {
    long a_long;
    char a_char;
} align;

#define RES_INIT	0x0001		// address initialized
#define RES_DEBUG	0x0002		// print debug messages
#define RES_AAONLY	0x0004		// authoritative answers only
#define RES_USEVC	0x0008		// use virtual circuit
#define RES_PRIMARY	0x0010		// query primary server only
#define RES_IGNTC	0x0020		// ignore trucation errors
#define RES_RECURSE	0x0040		// recursion desired
#define RES_DEFNAMES	0x0080		// use default domain name
#define RES_STAYOPEN	0x0100		// Keep TCP socket open
#define RES_DNSRCH	0x0200		// search up local domain tree

#define RES_DEFAULT	(RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

// Resolver state default settings

#define RES_TIMEOUT 2

static struct sockaddr_in dns_addr;
static int query_id = 0;

extern struct hostent __ahost;
extern unsigned long __h_inaddr;
extern char __h_name[CONF_HOSTLEN];

// Expand compressed domain name 'comp_dn' to full domain name.
// 'msg' is a pointer to the begining of the message,
// 'eomorig' points to the first location after the message,
// 'exp_dn' is a pointer to a buffer of size 'length' for the result.
// Return size of compressed name or -1 if there was an error.
int
dn_expand(char *msg, char *eomorig, char *comp_dn, char *exp_dn, int length)
{
	char *cp, *dn;
	int n, c;
	char *eom;
	int len = -1;

	dn = exp_dn;
	cp = comp_dn;
	eom = exp_dn + length - 1;

	// fetch next label in domain name
	while ((n = *cp++) != 0) {
		// Check for indirection
		switch (n & INDIR_MASK) {
		case 0:
			if (dn != exp_dn) {
				if (dn >= eom)
					return (-1);
				*dn++ = '.';
			}
			if (dn+n >= eom)
				return (-1);
			while (--n >= 0) {
				if ((c = *cp++) == '.') {
					if (dn+n+1 >= eom)
						return (-1);
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)	// out of range
					return(-1);
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = cp - comp_dn + 1;
			cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	// out of range
				return(-1);
			break;

		default:
			return (-1);			// flag error
		}
	}
	*dn = '\0';
	if (len < 0)
		len = cp - comp_dn;
	return (len);
}

// Search for expanded name from a list of previously compressed names.
// Return the offset from msg if found or -1.
int
dn_find(char *exp_dn, char *msg, char **dnptrs, char **lastdnptr)
{
	char *dn, *cp, **cpp;
	int n;
	char *sp;

	for (cpp = dnptrs + 1; cpp < lastdnptr; cpp++) {
		dn = exp_dn;
		sp = cp = *cpp;
		while ((n = *cp++) != 0) {
			// check for indirection
			switch (n & INDIR_MASK) {
			case 0:		// normal case, n == len
				while (--n >= 0) {
					if (*dn == '\\')
						dn++;
					if (*dn++ != *cp++)
						goto next;
				}
				if ((n = *dn++) == '\0' && *cp == '\0')
					return (sp - msg);
				if (n == '.')
					continue;
				goto next;

			default:	// illegal type
				return (-1);

			case INDIR_MASK:	// indirection
				cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			}
		}
		if (*dn == '\0')
			return (sp - msg);
	next:	;
	}
	return (-1);
}

// Compress domain name 'exp_dn' into 'comp_dn'.
// Return the size of the compressed name or -1.
// 'length' is the size of the array pointed to by 'comp_dn'.
// 'dnptrs' is a list of pointers to previous compressed names. dnptrs[0]
// is a pointer to the beginning of the message. The list ends with NULL.
// 'lastdnptr' is a pointer to the end of the arrary pointed to
// by 'dnptrs'. Side effect is to update the list of pointers for
// labels inserted into the message as we compress the name.
// If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
// is NULL, we don't update the list.
int
dn_comp(char *exp_dn, char *comp_dn, int length, char **dnptrs, char **lastdnptr)
{
	char *cp, *dn;
	int c, l;
	char **cpp, **lpp, *sp, *eob;
	char *msg;

	dn = exp_dn;
	cp = comp_dn;
	eob = cp + length;
	if (dnptrs != NULL) {
		if ((msg = *dnptrs++) != NULL) {
			for (cpp = dnptrs; *cpp != NULL; cpp++)
				;
			lpp = cpp;	// end of list to search
		}
	} else
		msg = NULL;
	for (c = *dn++; c != '\0'; ) {
		// look to see if we can use pointers
		if (msg != NULL) {
			if ((l = dn_find(dn-1, msg, dnptrs, lpp)) >= 0) {
				if (cp+1 >= eob)
					return (-1);
				*cp++ = (l >> 8) | INDIR_MASK;
				*cp++ = l % 256;
				return (cp - comp_dn);
			}
			// not found, save it
			if (lastdnptr != NULL && cpp < lastdnptr-1) {
				*cpp++ = cp;
				*cpp = NULL;
			}
		}
		sp = cp++;	// save ptr to length byte
		do {
			if (c == '.') {
				c = *dn++;
				break;
			}
			if (c == '\\') {
				if ((c = *dn++) == '\0')
					break;
			}
			if (cp >= eob)
				return (-1);
			*cp++ = c;
		} while ((c = *dn++) != '\0');
		// catch trailing '.'s but not '..'
		if ((l = cp - sp - 1) == 0 && c == '\0') {
			cp--;
			break;
		}
		if (l <= 0 || l > MAXLABEL)
			return (-1);
		*sp = l;
	}
	if (cp >= eob)
		return (-1);
	*cp++ = '\0';
	return (cp - comp_dn);
}

// Skip over a compressed domain name. Return the size or -1.
int
dn_skip(char *comp_dn)
{
	char *cp;
	int n;

	cp = comp_dn;
	while ((n = *cp++) != 0) {
		// check for indirection
		switch (n & INDIR_MASK) {
		case 0:		// normal case, n == len
			cp += n;
			continue;
		default:	// illegal type
			return (-1);
		case INDIR_MASK:	// indirection
			cp++;
		}
		break;
	}
	return (cp - comp_dn);
}

unsigned short
getshort(char *msgp)
{
	unsigned char *p = (unsigned char *) msgp;
	unsigned short u;

	u = *p++ << 8;
	return ((unsigned short)(u | *p));
}

void
putshort(unsigned short s, char *msgp)
{
	msgp[1] = s;
	msgp[0] = s >> 8;
}

static int
makequery(char *dname, int type, char *buf, int buflen)
{
	char *cp, *dnptrs[10], **dpp, **lastdnptr;
	int len;

	// Initialize header fields.

	//printf("MAKE QUERY\n");
	((HEADER *)buf)->id = htons(++query_id);
	((HEADER *)buf)->opcode = QUERY;
	((HEADER *)buf)->qr = ((HEADER *)buf)->aa =
			((HEADER *)buf)->tc = ((HEADER *)buf)->ra =
					((HEADER *)buf)->pr = 0;
	((HEADER *)buf)->rd = 1;
	((HEADER *)buf)->rcode = NOERROR;
	((HEADER *)buf)->qdcount = ((HEADER *)buf)->ancount = ((HEADER *)buf)->nscount = ((HEADER *)buf)->arcount = 0;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);

	buflen -= QFIXEDSZ;
	if ((len = dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
		return (-1);
	cp += len;
	buflen -= len;

	putshort(type, cp); cp += sizeof(unsigned short);
	putshort(C_IN, cp); cp += sizeof(unsigned short);

	((HEADER *)buf)->qdcount = HTONS(1);
	return (cp - buf);
}

res_send(char *buf, int blen, char *answer, int alen)
{
	int n, sd;
	int retry, resplen, ns, s;
	unsigned short id, len;
	char *cp;
	fd_set rdfds;
	struct timeval timeout;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;

	id = hp->id;
	timeout.tv_usec = 100000;

	//printf("_RES_SEND\n");
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return(-1);

	// Send request, RETRY times, or until successful
	for (retry = 0; retry < RES_TIMEOUT; retry++) {
		//printf("SEND\n");
		if (sendto(sd, buf, blen, 0, (struct sockaddr *)&dns_addr, sizeof(dns_addr)) != blen)
			continue;
		// Wait for reply
		timeout.tv_sec = 2 * retry;
wait:		FD_ZERO(&rdfds);
		FD_SET(sd, &rdfds);
		if ((n = select(sd + 1, &rdfds, NULL, NULL, &timeout)) < 0)
			break;
		if (n == 0) {		// timeout -> resend
			if (_get_errno(sd))
				goto leave;
			continue;
		}
		if ((resplen = recvfrom(sd, answer, alen, 0, NULL, 0)) <= 0)
			continue;
		if (id != anhp->id)	// response from old query, ignore it
			goto wait;
		if (anhp->tc) {
			retry = 0;
			continue;
		}
		//printf("ANSWER RECEIVED %d bytes\n", resplen);
		closesocket(sd);
		return (resplen);
	}
	errno = ETIMEDOUT;
leave:	closesocket(sd);
	return (-1);
}

static char *h_addr_ptrs[MAXALIASES];
static char *host_aliases[MAXALIASES];
static char hostbuf[BUFSIZ+1];

static struct hostent *
getanswer(char *msg, int msglen, int iquery)
{
	HEADER *hp;
	char *cp;
	static query_buf answer;
	char *eom, *bp, **ap;
	int n, type, class, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;

	//printf("GET ANSWER\n");
	if ((n = res_send(msg, msglen, (char *)&answer, sizeof(answer))) < 0) {
		return (NULL);
	}
	eom = (char *)&answer + n;
	// find first satisfactory answer
	hp = (HEADER *) &answer;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	if (hp->rcode != NOERROR || ancount == 0) {
		return (NULL);
	}
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = (char *)&answer + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0) {
				return (NULL);
			}
			cp += n + QFIXEDSZ;
			__ahost.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += dn_skip(cp) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skip(cp) + QFIXEDSZ;
	} else if (iquery) {
		return (NULL);
	}
	ap = host_aliases;
	__ahost.h_aliases = host_aliases;
	hap = h_addr_ptrs;
//	MUST BE CLEARED
//	__ahost.h_addr_list = h_addr_ptrs;
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = getshort(cp);
 		cp += sizeof(unsigned short);
		class = getshort(cp);
 		cp += sizeof(unsigned short) + sizeof(unsigned long);
		n = getshort(cp);
		cp += sizeof(unsigned short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (type == T_PTR) {
			if ((n = dn_expand((char *)&answer, eom,
			    cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			cp += n;
			__ahost.h_name = bp;
			__ahost.h_addr = h_addr_ptrs[0];
			return(&__ahost);
		}
		if (type != T_A)  {
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != __ahost.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			__ahost.h_length = n;
			getclass = class;
			__ahost.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				__ahost.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += ((unsigned long)bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
		*hap = NULL;
		__ahost.h_addr = h_addr_ptrs[0];
		return (&__ahost);
	} else {
		return (NULL);
	}
}

// Type : 0 = name -> address
//	  1 = address -> name
struct hostent *
_resolve_name(char *hostname, unsigned long addr, int type)
{
	struct TCPCONFIG config;
	char dnbuf[CONF_HOSTLEN];
	query_buf *tmp_buf;
	struct hostent *rhost = NULL;
	int n;

	_read_config(&config);
	//printf("START RESOLVE_NAME SERVER:%s\n", inet_ntoa(*(struct in_addr *)&(config.c_dns)));
 	if (!config.c_dns) {
		//printf("Name server not configured\n");
		goto leave2;
	}

	dns_addr.sin_family = AF_INET;
	dns_addr.sin_addr.s_addr = config.c_dns;
	dns_addr.sin_port = HTONS(NAMESERVER_PORT);

	if (!(tmp_buf = (query_buf *)malloc(sizeof(query_buf)))) {
		//printf("malloc FAILED\n");
		goto leave2;
	}

	if (type) {	// Address -> name
		//printf("ADDR->NAME\n");
		sprintf(dnbuf, "%s.in-addr.arpa", inet_ntoa(*(struct in_addr *)&addr));
		if ((n = makequery(dnbuf, T_PTR, (char *)tmp_buf, sizeof(query_buf))) < 0)
			goto leave1;
		if (!(rhost = getanswer((char *)&tmp_buf, n, 1)))
			goto leave1;
		rhost->h_addrtype = AF_INET;
		rhost->h_length = sizeof(unsigned long);
	} else {	// name -> address
		//printf("NAME(%s)->ADDR\n", hostname);
		if (!strchr(hostname, '.')) {	// append the default domain
			sprintf(dnbuf, "%s.%s", hostname, config.c_domain);
			hostname = dnbuf;
		}
		if ((n = makequery(hostname, T_A, (char *)tmp_buf, sizeof(query_buf))) < 0)
			goto leave1;
		rhost = getanswer((char *)tmp_buf, n, 0);
	}
leave1:	free(tmp_buf);
leave2:	return(rhost);
}