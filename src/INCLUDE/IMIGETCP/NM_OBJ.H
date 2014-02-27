/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

// Must Check SNMP MIB for TCP/IP Kernel

#ifndef _IMIGE_NMOBJ_H
#define _IMIGE_NMOBJ_H

struct snmp_object {
	unsigned long tcpAttemptFails;	// Connection Fails
	unsigned long tcpEstabResets;	// Reset from ESTABed Connection
	unsigned long tcpRetransSegs;	// Retransmitted Segments sent
	unsigned long tcpInErrs;
	unsigned long tcpOutRsts;
	unsigned long tcpInSegs;
	unsigned long tcpOutSegs;
	unsigned long tcpActiveOpens;
	unsigned long tcpPassiveOpens;
	unsigned long TCP_rcv_bytes;	// TCP received bytes
	unsigned long TCP_snd_bytes;	// TCP sent bytes
	unsigned long TCP_dropped;		// TCP packet dropped by no port

	unsigned long UDP_rcv_bytes;	// UDP received bytes
	unsigned long UDP_snd_bytes;	// UDP sent bytes
	unsigned long UDP_no_buffer;	// Droped packet dueto buffer short

	unsigned long udpInDatagrams;	// UDP received packet
	unsigned long udpOutDatagrams;	// UDP sent packet
	unsigned long udpInErrors;		// UDP checksum error
	unsigned long udpNoPorts;		// UDP packet dropped by no port

	unsigned long icmpInMsgs;
	unsigned long icmpInErrors;
	unsigned long icmpInDestUnreachs;
	unsigned long icmpInTimeExcds;
	unsigned long icmpInParmProbs;
	unsigned long icmpInSrcQuenchs;
	unsigned long icmpInRedirects;
	unsigned long icmpInEchos;
	unsigned long icmpInTimestamps;
	unsigned long icmpInAddrMask;
	unsigned long icmpInEchoReps;

	unsigned long icmpOutMsgs;
	unsigned long icmpOutEchos;

	unsigned long ipInReceives;			// IP Received Packet
	unsigned long ipInHdrErrors;		// IP Header Checksum Error
	unsigned long ipInAddrErrors;		// IP Input Address Error
	unsigned long ipInUnknownProtos;	// IP Unknown Protocol

	unsigned long ipOutSends;
	unsigned long ipOutRequests;
	unsigned long ipOutDiscards;
	unsigned long ipOutNoRoutes;		// No gateway

	unsigned long ipReasmReqds;
	unsigned long ipReasmFails;
	unsigned long ipFragCreates;

	unsigned long arpInRequests;
	unsigned long arpOutRequests;
	unsigned long arpInReplys;

	unsigned long lanInPackets;			// LAN received packet
	unsigned long lanOutPackets;		// LAN sent packet
	unsigned long lanOutErrors;			// LAN send error
	unsigned long lanInUnknownProtos;	// LAN packet dropped
};

// Undefined Object Calculation

// udpTable : = can count by check all the TCB's

// icmpOutEchoReps	: = icmpInEchos	-- we send 100% reply
// icmpOutAddrMaskReps	: = icmpInAddrMask -- we send 100% reply
// icmpInTimestampReps	: = 0 -- we don't send TimeStampRequest

// icmpOutErrors	: = 0 -- no error
// icmpOutDestUnreachs	: = 0 -- we don't support
// icmpOutTimeExcds	: = 0 -- we don't support
// icmpOutParmProbs	: = 0 -- we don't support
// icmpOutSrcQuenchs	: = 0 -- we don't support
// icmpOutRedirects	: = 0 -- we don't support
// icmpOutAddrMasks	: = 0 -- we don't support
// icmpOutTimestamps	: = 0 -- we don't support
// icmpOutTimestampReps	: = 0 -- we don't reply to TimeStampRequest

// ipInDiscards		: ipInHdrErrors + ipInAddrErrors + ipInUnknownProtos
// ipInDelivers		: ipInReceives - ipInDiscards
// ipForwDatagrams	: = 0 -- we don't support forward
// ipOutDiscards	: = ipOutNoRoutes
// ipReasmOKs		: = ipReasmReqds - ipReasmFails;
// ipFragOKs		: = ipFragCreates
// ipFragFails		: = 0 -- We have no chance to fail fragmentation

// arpOutReply		: = arpInRequest -- we send 100% reply

#endif
