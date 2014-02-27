/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

//
// Protocol Packet Data Structure with Assigned Numbers
//

#ifndef _IMIGE_STANDARD_H
#define _IMIGE_STANDARD_H

// struct I3E8023 : IEEE 802.3
// struct RFC826  : ARP
// struct RFC791  : IP
// struct RFC792  : ICMP
// struct RFC793  : TCP
// struct RFC768  : UDP

// ------ WARNING
// This File does not contain all the Protocol Standard
// Please Check RFC and related Standards for more Detail
//
// We do not support 	: IP options
//			: TCP option other than MSS
//			: many of ICMP packet types
//			: Non 6 byte Hardware Address Media for ARP
//			: .....

//
// ETHERNET Standard : IEEE 802.3
//
struct I3E8023 {
	unsigned char  destination[6];	// Destination Ethernet Address
	unsigned char  source[6];	// Source  Ethernet Address
	unsigned short protocol;	// Upper Layer Protocol
};

// Ethernet Protocol Types (network order)

#define	ETADR	(0x0608)	// ARP (Address Resolution Protocol)
#define	ETIP	(0x0008)	// IP  (Internet Protocol)

#define ETHER_HLEN	(sizeof(struct I3E8023))

//
// ARP Standard : RFC 826
//
struct RFC826 {
	struct I3E8023	ether_pkt;	// Lower Layer Ethernet
	unsigned short	hw_type;		// Hardware Media Type
	unsigned short	protocol;	// Upper Layer Protocol ID
	unsigned char	hw_len;		// Length of Hardware Address
	unsigned char	proto_len;	// Length of Protocol Address
	unsigned short	opcode;		// ARP Opcode
	unsigned char	my_hw_addr[6];	// My Hardware Address
	unsigned long	my_proto_addr;	// My Protocol Address
	unsigned char	his_hw_addr[6];	// Receiver's Hardware Address
	unsigned long	his_proto_addr;	// Receiver's Protocol Address
};

#define ARP_PLEN	(sizeof(struct RFC826))
#define ARP_HLEN	(ARP_PLEN - ETHER_HLEN)

// Byte Swapped ARP Packet Type

#define	ARETH	(0x0100)	// Ethernet (Hardware Type, see RFC826)
#define	ARIP	ETIP		// IP (Upper Layer Protocol)

// ARP Opcode

#define	ARREQ	(0x0100)	// ARP Request
#define	ARREP	(0x0200)	// ARP Reply

//
// IP Standard : RFC 791
//
struct RFC791 {
	struct I3E8023	ether_pkt;	// Ethernet Packet
	unsigned char	hdr_len_ver;	// IP Header Length and version = 0x45
	unsigned char	service_type;	// Type of Service
	unsigned short	total_len; 	// Total Packet Length incl. IP header
	unsigned short	packet_id;	// IP Packet ID for fragmentation
	unsigned short	frag_offset;	// Fragment Offset and Flag
	unsigned char	time_to_live;	// Time to live (HOP count)
	unsigned char	transport;	// Transport Layer Protocol
	unsigned short	checksum;	// IP Header Checksum
	unsigned long	source;		// My  IP Address
	unsigned long	destination;	// His IP Address
	// IP Option is here, IMIGE/TCP do not process IP options
};

#define IP_PLEN		(sizeof(struct RFC791))	// IP Packet Length
#define IP_HLEN		(IP_PLEN - ETHER_HLEN)	// IP Header Length
#define PSEUDO_HLEN	(12)	// from time_to_live to destination of IP

#define IP_TTL		(0x20)		// Dod Standard TTL

//
// ICMP Standard : RFC 792
//
struct RFC792 {	
	struct RFC791	ip_pkt;		// Ethernet and IP Header
	unsigned char	packet_type;	// ICMP Packet Type
	unsigned char	opcode;		// ICMP Opcode
	unsigned short	checksum;	// ICMP Checksum
};

#define ICMP_PLEN	(sizeof(struct RFC792))	// ICMP Packet Length
#define ICMP_HLEN	(ICMP_PLEN - IP_PLEN)	// ICMP Header Length

// ICMP Packet Type

#define ECHO_REPLY		(0)
#define DEST_UNREACHABLE	(3)
#define SOURCE_QUENCH		(4)
#define GATEWAY_REDIRECT	(5)
#define ECHO_REQUEST		(8)
#define TIME_EXCEED		(11)

// Code Definition

#define ICMP_NETUNREACH		(0)
#define ICMP_HOSTUNREACH	(1)

// ICMP Echo Request/Reply Packet

struct ICMP_ECHO {
	struct RFC792	icmp_pkt;	// ICMP Header
	unsigned short	identification;	// ICMP Echo Request Identification
	unsigned short	sequence;	// ICMP Packet Sequence Number
};

#define ECHO_PLEN	(sizeof(struct ICMP_ECHO))	// Ping Length
#define ECHO_HLEN	(ECHO_PLEN - IP_PLEN)		// Ping Header Length

// ICMP Gateway Redirection Packet

struct ICMP_REDIRECT {
	struct RFC792	icmp_pkt;	// ICMP Header
	unsigned long	new_gateway;	// Other Gateway's IP address
};

#define REDIRECT_PLEN	(sizeof(struct ICMP_REDIRECT))	// Packet Length
#define REDIRECT_HLEN	(REDIRECT_PLEN - IP_PLEN)	// Packet Header Length

//
// TCP Standard : RFC 793
//
				// Long arithmetic resolves wrapped seq number 
typedef	long	tcp_seq;	// TCP Sequence Number

struct RFC793 {
	struct RFC791	ip_pkt;		// Ethernet and IP Header
	unsigned short	source;		// My  Port Number
	unsigned short	destination;	// His Port Number
	tcp_seq		sequence;	// Sequence Number for this Segment
	tcp_seq		acknowledge;	// Acknowledgement when Flags & TH_ACK
	unsigned	xxxxxx:4,	// Reserved Bits
			offset:4;	// Header Length = Offset for TCP Data
	unsigned char	flags;		// Type of this Segment
	unsigned short	window_adv;	// Window Advertisement of Sender
	unsigned short	checksum;	// Header Checksum (+ Pseudo Header)
	unsigned short	urgent_ptr;	// Urgent Pointer
	unsigned char	option_code;	// Option Opcode
	unsigned char	option_len;	// Option Length
	unsigned short	max_segment;	// Data for Maximum Segment Size Option
};

#define TCPO_PLEN	(sizeof(struct RFC793))	// TCP Packet Length + option
#define TCPO_HLEN	(TCPO_PLEN - IP_PLEN)	// TCP Header Length + option

#define TCP_PLEN	(TCPO_PLEN - 4)		// TCP Packet Length
#define TCP_HLEN	(TCP_PLEN - IP_PLEN)	// TCP Header Length

#define TCP_MSS		(536)	// RFC 1122, Default Maximum Segment Size

// TCP Segment types

#define	TH_FIN	(0x01)		// FINISH	: Close Request
#define	TH_SYN	(0x02)		// SYNCHRONIZE	: Connection Request
#define	TH_RST	(0x04)		// RESET	: Reset Connection
#define	TH_PUSH	(0x08)		// PUSH		: Last data in buffer
#define	TH_ACK	(0x10)		// ACKNOWLEDGE	: Decode Acknowledge Field
#define	TH_URG	(0x20)		// URGENT	: Out-Of-Band Urgent Data

// TCP Option Code
// We use only MSS option while SYN bit in flags is set

#define	TCPOPT_MAXSEG	(2)	// My Maximum Segment Size Reporting

//
// UDP Standard : RFC 768
//
struct RFC768 {
	struct RFC791	ip_pkt;		// Ethernet and IP Header
	unsigned short	source;		// My  Port Number
	unsigned short	destination;	// His Port Number
	unsigned short	length;		// UDP Segment(+hedaer) Length
	unsigned short	checksum;	// UDP Checksum
};

#define UDP_PLEN	(sizeof(struct RFC768))	// UDP Packet Length
#define UDP_HLEN	(UDP_PLEN - IP_PLEN)	// UDP Header Length

#endif
