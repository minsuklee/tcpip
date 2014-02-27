/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

// IMIGE/TCP for WINDOW Configuration Structure

#ifndef _IMIGE_CONFIG_H
#define _IMIGE_CONFIG_H

#define IMIGETCP_VER 300		// 3.00 for IMIGE/TCP for Window

// . IMIGE/TCP Kernel gets the SW INT number as a command line argument.
// . NETDB's File Path is defined as ENVIRONMENT VARIABLE by
//   DOS command 'set' like "set IMIGETCP=C:\IMIGETCP" in startup-batch.

// All the "unsigned long" Addresses must be in network byte order

// Configuration data used by Kernel

struct TCPIPCFG {
	unsigned long c_myip;		// My IP address
	unsigned long c_subnet;		// Subnet Mask
	unsigned long c_defgw;		// Gateway Address
};

// Configuration data used by Kernel and Application

#define	CONF_USERLEN	16
#define CONF_HOSTLEN	40
#define CONF_PATHLEN	64

typedef struct TCPCONFIG {
	struct TCPIPCFG c_kernel;		// Kernel Configuration

	unsigned short c_ver;			// IMIGE/TCP Version
	unsigned long  c_dns;			// DNS Domain Name Server Address

	char gate_way[CONF_HOSTLEN];	// Symbolic Gateway Name
	char c_myname[CONF_USERLEN];	// My host Name
	char c_domain[CONF_HOSTLEN];	// My domain Name
	char c_userid[CONF_USERLEN];	// My user name

	// Next hostnames are used as a default for applcation program.
	// if there is no explicit command-line argument for the application

	char c_defmail[CONF_HOSTLEN];	// Mail relay host
	char c_deflp[CONF_HOSTLEN];		// Printer spooler
	char c_defnfs[CONF_HOSTLEN];	// NFS Server host
	char c_defhost[CONF_HOSTLEN];	// Default Application Host
};

#define TSR_SIGNATURE	"TCP/IP"
#define TSR_SIGLEN		6

#define CFGFILENAME		"TCPIP.CFG"

#define IMIGETCP_ENV	"TCPIP"

#endif
