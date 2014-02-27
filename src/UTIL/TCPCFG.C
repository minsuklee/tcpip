/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>

#define BLOCKADJUST

char *title = "TCP/IP  Configuration Utility";
char *version = "Version 1.00";
char *copyright = "(C) Copyright 1995, IMIGE Systems Inc., All Rights Reserved.\n";

struct TCPCONFIG config;

int  input_string(char *argv[], char *s, int n);
int  input_addr(char *argv[], char *ipaddr);
int  input_hex(char *argv[], char *ipaddr);
void write_config(void), show_config(void), get_defaults(void);
void adjust_mask(int entry);

struct cmd {
	char *cmd;
	int (* func)();
	char *dest;
	char len;
};

#define MAXCONF		11

#define ADDR_ENTRY	0
#define NETMASK_ENTRY	1

struct cmd cmdtab[MAXCONF] = {

	// Kernel Configuration DO NOT CHANGE ORDER

	{ "ADDR",	input_addr,   (char *)&config.c_kernel.c_myip  ,  0 },
	{ "NETMASK",	input_hex,    (char *)&config.c_kernel.c_subnet,  0 },

	{ "GATEWAY",	input_string, config.gate_way , CONF_HOSTLEN },

	// Library Configuration

	{ "DNS",	input_addr,   (char *)&config.c_dns,	0 },
	{ "HOST",	input_string, config.c_myname,  CONF_USERLEN },
	{ "DOMAIN",	input_string, config.c_domain,  CONF_HOSTLEN },
	{ "USER",	input_string, config.c_userid,  CONF_USERLEN },

	// Application Configuration

	{ "MAIL",	input_string, config.c_defmail,	CONF_HOSTLEN },
	{ "PRINTER",	input_string, config.c_deflp,	CONF_HOSTLEN },
	{ "NFS",	input_string, config.c_defnfs,	CONF_HOSTLEN },
	{ "DEFAULT",	input_string, config.c_defhost,	CONF_HOSTLEN }
};

char _imige_path[100];
FILE *fp;

void
main(argc, argv)
int	argc;
char	*argv[];
{
	int i;
	char *str;
	struct cmd *tab = cmdtab;

	printf("%s,  %s,\n%s\n", title, version, copyright);

	if ((str = getenv(IMIGETCP_ENV)) == NULL) {
		fprintf(stderr, "SET TCPIP=dir first\n");
		exit(1);
	}
	strcpy(_imige_path, str);
	strcat(_imige_path, "\\");
	strcat(_imige_path, CFGFILENAME);

	if ((fp = fopen(_imige_path, "rb")) == NULL) {
		get_defaults();
	} else {
		while (fgetc(fp) != 0x1a)
			;
		fread(&config, sizeof(config), 1, fp);
		fclose(fp);
	}

	if (argc != 3) {
usage:		show_config();
		fprintf(stderr, "\nUsage : TCPCFG field value	(ex,  TCPCFG addr 193.10.9.1)\n");
		exit(1);
	}
	for (i = 0; i < MAXCONF; i++) {
		if (strcmp(strupr(argv[1]), tab->cmd) == 0) {
			if ((*(tab->func))(argv, tab->dest, tab->len) == 0) {
#ifndef BLOCKADJUST
				if (i == ADDR_ENTRY || i == NETMASK_ENTRY)
					adjust_mask(i);
#endif
				write_config();
				show_config();
				exit(0);
			} else {
				goto usage;
			}
		} else {
			tab++;
		}
	}
	goto usage;
}

void
adjust_mask(int entry)
{
	if (IN_CLASSA(ntohl(config.c_kernel.c_myip))) {		/* CLASS A */
		config.c_kernel.c_subnet |= htonl(IN_CLASSA_NET);
		if (entry == ADDR_ENTRY)
			config.c_kernel.c_subnet = htonl(IN_CLASSA_NET);
		return;
	}
	if (IN_CLASSB(ntohl(config.c_kernel.c_myip))) {		/* CLASS B */
		config.c_kernel.c_subnet |= htonl(IN_CLASSB_NET);
		if (entry == ADDR_ENTRY)
			config.c_kernel.c_subnet = htonl(IN_CLASSB_NET);
		return;
	}
	if (IN_CLASSC(ntohl(config.c_kernel.c_myip))) {		/* CLASS C */
		config.c_kernel.c_subnet |= htonl(IN_CLASSC_NET);
		if (entry == ADDR_ENTRY)
			config.c_kernel.c_subnet = htonl(IN_CLASSC_NET);
		return;
	}	
}

void
write_config()
{
//	unsigned long addr;
//	if ((addr = inet_addr(config.gate_way)) == 0xffffffffL) {
//		struct hostent *hp;
//		if (strlen(config.gate_way)) {
//			if ((hp = gethostbyname(config.gate_way)) == NULL) {
//				printf("\nGateway '%s' unknown.\n", config.gate_way);
//				addr = 0L;
//			} else
//				addr= *(unsigned long *)(hp->h_addr);
//		} else
//			addr = 0L;
//	}
//	config.c_kernel.c_defgw = addr;

	config.c_ver = IMIGETCP_VER;
	if ((fp = fopen(_imige_path, "wb")) == NULL) {
		fprintf(stderr, "Cannot Open %s\n", _imige_path);
		exit(1);
	}
	fprintf(fp, "TCP/IP  Configuration File,  Version 1.00,\r\n%s\r%c", copyright, 26);
	fwrite(&config, 1, sizeof(config), fp);
	fclose(fp);
}

void
show_config()
{
	printf("Configuration File : %s\n", _imige_path);
	printf("Configuration Version : %d.%02d\n\n",
		config.c_ver / 100, config.c_ver % 100);
	printf("My Host (user@host.domain) : %s@%s.%s\n",
		config.c_userid, config.c_myname, config.c_domain);
	printf("My Internet Address (addr) : %s\n",
		inet_ntoa(*(struct in_addr *)&config.c_kernel.c_myip));
	printf("Network Mask     (netmask) : 0x%08lX\n",
		ntohl(config.c_kernel.c_subnet));
	printf("\nDefault Gateway  (gateway) : %s", config.gate_way);
	{
		unsigned long addr;
		if ((addr = inet_addr(config.gate_way)) == 0xffffffffL) {
			struct hostent *hp;
			if (strlen(config.gate_way)) {
				if ((hp = gethostbyname(config.gate_way)) == NULL)
					addr = 0L;
				else
					addr= *(unsigned long *)(hp->h_addr);
			} else
				addr = 0L;
			printf(" (%s)", inet_ntoa(*(struct in_addr *)&addr));
		}
	}
	printf("\n");
	printf("Domain Name Server   (dns) : %s\n",
		inet_ntoa(*(struct in_addr *)&config.c_dns));
	printf("Mail Relay          (mail) : %s\n", config.c_defmail);
	printf("Network Printer  (printer) : %s\n", config.c_deflp);
	printf("Network File Server  (nfs) : %s\n", config.c_defnfs);
	printf("Default Host     (default) : %s\n", config.c_defhost);
}

void
get_defaults()
{
	config.c_kernel.c_myip = 0x100007fL;
	config.c_kernel.c_defgw = 0L;
	config.c_kernel.c_subnet = 0xFFL;

	config.c_ver = IMIGETCP_VER;
	config.c_dns = 0L;

	strcpy(config.gate_way, "gateway");
	strcpy(config.c_myname, "pcname");
	strcpy(config.c_domain, "domain");
	strcpy(config.c_userid, "username");

	strcpy(config.c_defmail, "mail relay");
	strcpy(config.c_deflp,   "printer spooler");
	strcpy(config.c_defnfs,  "nfs server");
	strcpy(config.c_defhost, "default application host");
	write_config();
}

int
input_addr(char *argv[], char *ipaddr)
{
	unsigned long tmp;

	if ((tmp = inet_addr(argv[2])) == 0xFFFFFFFFL)
		return(1);
	else {
		*(unsigned long *)ipaddr = tmp;
		return(0);
	}
}

int
input_hex(char *argv[], char *ipaddr)
{
	int a,b,c,d;

	if (sscanf(argv[2], "0x%02x%02x%02x%02x", &a, &b, &c, &d) == 4)
		goto good;
	else if (sscanf(argv[2], "0X%02x%02x%02x%02x", &a, &b, &c, &d) == 4)
		goto good;
	else if (sscanf(argv[2], "%02x%02x%02x%02x", &a, &b, &c, &d) == 4) {
good:		/* printf("0x%02x%02x%02x%02x", a, b, c, d); */
		if (d == 0xff)
			return(1);
		if (c != 0 && (b != 0xff || a != 0xff))
			return(1);
		if (b != 0 && a != 0xff)
			return(1);
		*ipaddr++ = a;
		*ipaddr++ = b;
		*ipaddr++ = c;
		*ipaddr   = d;
		return(0);
	}
	return(1);
}

int
input_string(char *argv[], char *s, int n)
{
	int i;
	char *v = argv[2];

	switch (toupper(argv[1][0])) {
	  case 'U' :
		i = 0;
		while ((*v != 0) && (*v != '@')) {
			config.c_userid[i++] = *v++;
		}
		config.c_userid[i] = 0;
		if (*v == 0)
			break;
		else
			v++;
	  case 'H' :
		i = 0;
		while ((*v != 0) && (*v != '.'))
			config.c_myname[i++] = *v++;
		config.c_myname[i] = 0;
		if (*v == 0)
			break;
		else
			strncpy(config.c_domain, ++v, n);
		break;
	  default  :
		strncpy(s, argv[2], n);
		break;
	}
	return(0);
}
