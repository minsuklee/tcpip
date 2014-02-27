/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#define	MAXALIASES	5

struct hostent {
	char *h_name;		/* official name of host */
	char **h_aliases;	/* alias list */
	int	 h_addrtype;	/* host address type */
	int	 h_length;		/* length of address */
	char *h_addr;		/* list of addresses from name server */
};

struct netent {
	char *n_name;			/* official name of net */
	char **n_aliases;		/* alias list */
	int	 n_addrtype;		/* net address type */
	unsigned long n_net;	/* network # */
};

struct servent {
	char *s_name;		/* official service name */
	char **s_aliases;	/* alias list */
	int	 s_port;		/* port # */
	char *s_proto;		/* protocol to use */
};

struct protoent {
	char *p_name;		/* official protocol name */
	char **p_aliases;	/* alias list */
	int	 p_proto;		/* protocol # */
};

struct hostent *gethostbyaddr(char *addr, int len, int type);
struct hostent *gethostbyname(char *hostname);
struct hostent *gethostent(void);

void   endprotoent(void);
struct protoent *getprotobyname(char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
void   setprotoent(int stayopen);

void   endservent(void);
struct servent *getservbyname(char *servname, char *protname);
struct servent *getservbyport(int port, char *proto);
struct servent *getservent(void);
void   setservent(int stayopen);

struct netent *getnetbyname(), *getnetbyaddr(), *getnetent();

extern int h_errno;	

#define	HOST_NOT_FOUND	1 // Authoritive Answer Host not found
#define	TRY_AGAIN		2 // Non-Authoritive Host not found, or SERVERFAIL
#define	NO_RECOVERY		3 // Non recoverable errors, FORMERR, REFUSED, NOTIMP
#define NO_ADDRESS		4 // Valid host name, no address, look for MX record
