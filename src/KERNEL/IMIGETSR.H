/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#ifndef _IMIGE_TSR_H
#define _IMIGE_TSR_H

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

// BSD Socket Library Headers

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

// IMIGE/TCP Specific Headers

#include <imigetcp/tcpcfg.h>		// TCP/IP Configuration
#include <imigetcp/nm_obj.h>		// SNMP Statistics Object List
#include <imigetcp/imigetcp.h>

#include "standard.h"			// TCP/IP Kernel Standard Structure
#include "imgtcpif.h"

// Other Configuration Parameters
//
#define MAX_TCB		20		// Allowable Maximum Socket

// Buffer Structure
//
// UDP Buffer contains full size packet from the begining in any cases
//	so, no start pointer is required
//
#define MAX_BUFSIZE	1460
#define DEF_BUFSIZE	128

//#define PERMIT_MONOPOLY
extern int MAX_RX_ALLOCATE;
extern int MAX_TX_ALLOCATE;

struct BUFFER {
	struct BUFFER near *next;	// Next Link
	int buffer_type;		// Buffer Type
	unsigned short r_count;		// Data Byte Count in this Buffer
	union {
		struct {
			unsigned short start;	// Unread Data Start Point
			tcp_seq	sequence;	// Seqence Number of First Byte
		} tcp_buffer;
		struct {
			unsigned long source_ip;	// Source IP Address
			unsigned short source_port;	// Source Port Number
		} raw_buffer;
	} protocol;
	unsigned char near *data;	// Real Data
};

#define r_start protocol.tcp_buffer.start
#define s_seq	protocol.tcp_buffer.sequence
#define r_ip	protocol.raw_buffer.source_ip
#define r_port	protocol.raw_buffer.source_port

// Buffer Type Encoding
//
#define FREE_BUFFER	0x00	// Free (unallocated) Buffer
#define RX_BUFFER	0x01	// TCP/UDP RX Buffer
#define TX_BUFFER	0x02	// TCP/UDP TX Buffer
#define LOCK_BUFFER	0x04	// Lock buffer for linked list
#define FRAG_BUFFER	0x08	// Buffer for IP Assembly
#define MORE_FRAGMENT	0x80	// Next Buffer is from the same Segment

// Timer

struct TIMER {
	struct TIMER near *next;	// Next TIMER structure
	unsigned long tick;		// Absolute Tick to timeout
	tcp_seq  seq;	// Associated TCP Sequence Number ==
			// Last byte in Buffer to be ACKed until this timer
};

#define MAX_TIMER_COUNT		(150)
extern struct TIMER TIMERS[MAX_TIMER_COUNT];

#define TICK_TO_SEC	18
#define ONE_SECOND	6
#define TIME_FOREVER	0xFFFFFFFFL

#define DEL_ACK_TIME	(ONE_SECOND / 3)	// Delayed ACK time
#define KEEPALIVE_TIME	(ONE_SECOND * 20)	// Keep Alive Time
#define MAX_SEG_TIME	(ONE_SECOND * 60)	// Maximum Segment Life Time
#define TWO_MSL_TIME	(2 * MAX_SEG_TIME)	// 2 * MAX_SEG_TIME
#define CLOSE_WAIT_TIME	(MAX_SEG_TIME * 2)	// Maximum Wait Time to Close

#define ROUND_TRIP_TIME(tp)	(tp->round_trip / (TICK_TO_SEC / ONE_SECOND))

// Retransmition Timer use tick Unit

#define MIN_RETX_TIME	(TICK_TO_SEC / 2)	// Minimum Retransmit Time
#define MAX_RETX_TIME	(TICK_TO_SEC * 60)	// Maximum Retransmit Time


//
//  TCP Control Block Definition
//

struct TCB {
	int		last_error;	// Last Error for this socket

	// Socket Management Variables Here

	unsigned short	status;		// tcb Status
	unsigned short	option;		// Socket Option see <sys/socket.h>

	unsigned short	local_port;	// My TCP/UDP Port
	unsigned short	remote_port;	// Remote Host's Port

	char	backlog[SOMAXCONN];	// Backlog Queue for Listen Socket
	char	logcount;		// Queued(unaccepted) connection count

	int		t_state;	// TCP state

	// TCP Standard Variables

	short		round_trip;	// Round Trip Time measured

	tcp_seq		snd_una;	// send and unacked number
	tcp_seq		snd_nxt;	// send sequence number
	unsigned short	snd_wnd;	// his window size
	unsigned short	snd_mss;	// Maximum Segment Size Reported

	tcp_seq		rcv_nxt;	// recv sequence number
	short		rcv_wnd;	// My Receive Window size
	unsigned short	rcv_adv;	// Last Advertizement of rcv_wnd
	unsigned short	rcv_mss;	// Maximum Segment Size Requested

	struct BUFFER near *rx, near *rxe;	// Receive Buffer start, end
	struct BUFFER near *tx, near *txe;	// Retransmit Buffer start, end
	struct BUFFER near *nexttx;		// Buffer pointed bu snd_una
	int rx_len, tx_len;			// TX, RX Buffer list length
	struct TIMER  near *ts, near *te;	// Retransmit Timer start, end

	int		timeout_count;	// Retry Count
	unsigned short	del_ack_timer;	// Delayed ACK Timer
	unsigned short	syn_fin_timer;	// SYN, FIN ACK wait Timer
	unsigned short	persist_timer;	// Send Window Polling Timer
	unsigned short	cleanup_timer;	// Close wait/ Retry Timer
	unsigned short	kpalive_timer;	// Keep Alive Timer

	// Socket Specific Event handler
	void far (* async_func)(int sd, unsigned event, unsigned arg);

	struct RFC791 IP_HDR;		// IP Header Stub for TX Performance
};

#define	arp_retrytimer	cleanup_timer	// Retry Timer for ARP

// Gateway Address for this Connection
#define arp_target(tp)	((unsigned long)(tp->rcv_nxt))

#define ARP_RETRY	3		// three times of ARP test
#define SYN_RETRY	3		// three times of SYN Sent

#define ETH_STUB(p)	((struct I3E8023 *)(&((p)->IP_HDR)))
#define IP_STUB(p)	((struct RFC791  *)(&((p)->IP_HDR)))

// TCB Status Bit Encoding

#define TCB_ARP_ING		0x8000		// Wait ARP Reply
#define TCB_ARP_ED		0x4000		// ARP is done for this Channel
#define TCB_ECHO_ING		0x2000		// Wait Echo Reply
#define TCB_ECHO_ED		0x1000		// Echo Reply Received

#define TCB_TCP_WACC		0x0800		// Connected but not Accepted
#define TCB_TCP_SYN		0x0400		// TCP wait ARP before SYN
#define TCB_TCP_FIN		0x0200		// Application Closed Channel
#define TCB_TCP_ACK		0x0100		// Send Delayed ACK
#define TCB_TCP_RST		0x0080		// Send Reset Packet
#define TCB_TCP_DATA		0x0040		// Unsent Data in Buffer
#define TCB_TCP_POLL		0x0020		// TCP is polling Peer's Window
#define TCB_TCP_KEEP		0x0010		// Send Keep Alive Packet

#define TCB_UDP_CONNECTED	0x0002		// This is Connected UDP socket
#define TCB_SENDTO_WARP		0x0001		// RAW packet to send queued

// Redefine Interrupt enable/disable for readability

#define enable_interrupt()	enable()
#define disable_interrupt()	disable()

#define set_status(tp, flag)	 { (tp)->status |= (flag); }
#define clr_status(tp, flag)	 { (tp)->status &= ~(flag); }

// TCP_states Definition

#define	TCPS_CLOSED		0	// closed
#define	TCPS_LISTEN		1	// listening for connection
#define	TCPS_SYN_SENT		2	// active, have sent syn
#define	TCPS_SYN_RECVD		3	// have send and received syn
#define	TCPS_ESTABLISHED	4	// established
#define	TCPS_CLOSE_WAIT		5	// rcvd fin, waiting for close
#define	TCPS_FIN_WAIT_1		6	// have closed, sent fin
#define	TCPS_CLOSING		7	// closed xchd FIN; await FIN ACK
#define	TCPS_LAST_ACK		8	// had fin and close; await FIN ACK
#define	TCPS_FIN_WAIT_2		9	// have closed, fin is acked
#define	TCPS_TIME_WAIT		10	// in 2*msl quiet wait after close

// Protocol Header Pointer Definition for a Received Packet
// packet_buffer is also be used as immediate reply packet buffer
//			for ARP Reply, ICMP Echo Reply, TCP RST.

#define ETH_P	((struct I3E8023	*)packet_buffer)	// ETHERNET
#define ARP_P	((struct RFC826		*)packet_buffer)	// ARP
#define IP_P	((struct RFC791		*)packet_buffer)	// IP
#define ICMP_P	((struct RFC792		*)packet_buffer)	// ICMP
#define ECHO_P	((struct ICMP_ECHO	*)packet_buffer)	// ECHO
#define GATE_P	((struct ICMP_REDIRECT	*)packet_buffer)	// REDIRECT
#define TCP_P	((struct RFC793		*)packet_buffer)	// TCP
#define UDP_P	((struct RFC768		*)packet_buffer)	// UDP

#define ETH_S	((struct I3E8023	*)send_buffer)	// ETHERNET
#define ARP_S	((struct RFC826		*)send_buffer)	// ARP
#define IP_S	((struct RFC791		*)send_buffer)	// IP
#define ICMP_S	((struct RFC792		*)send_buffer)	// ICMP
#define ECHO_S	((struct ICMP_ECHO	*)send_buffer)	// ECHO
#define GATE_S	((struct ICMP_REDIRECT	*)send_buffer)	// REDIRECT
#define TCP_S	((struct RFC793		*)send_buffer)	// TCP
#define UDP_S	((struct RFC768		*)send_buffer)	// UDP

// ICMP Gateway Redirect Packet's IP Packet

#define E_IP	((struct RFC791 *)(packet_buffer + ICMP_PLEN - ETHER_HLEN + 4))
#define E_TCP	((struct RFC793 *)(packet_buffer + ICMP_PLEN - ETHER_HLEN + 4))
#define G_IP	((struct RFC791 *)(packet_buffer + REDIRECT_PLEN - ETHER_HLEN))
#define G_TCP	((struct RFC793 *)(packet_buffer + REDIRECT_PLEN - ETHER_HLEN))

// ICMP Error Type

#define EICMP_SEQ	150		// ICMP Sequence NUmber Mismatch
#define EICMP_DATA	151		// ICMP ECHO DATA Mismatch

#define EUDP_4_ARP	152		// Block for ARP

//
// External Global Data Definitions
//

extern struct TCPIPCFG tcpipcfg;	// Kernel Configuration, KERNEL.C
extern struct snmp_object statistics;	// Kernel Statistics, KERNEL.C

extern struct TCB *tcb;

#define PACKET_BUFSIZE	1514
#define MAX_UDP_TXSIZE	(PACKET_BUFSIZE - UDP_PLEN)
#define MAX_RING_BUFFER	8192

extern unsigned char *packet_buffer;			// NIU Receive Buffer
extern unsigned char send_buffer[PACKET_BUFSIZE];	// NIU Send Buffer

extern int slip_driver;			// Media Type, PACKET.C
extern int hw_intr;			// Hardware INT no for PDI, PACKET.C
extern int MTU_size;			// Maximum Transfer Unit, PACKET.C
extern int signal_int;			// Signalling Interrupt Number
extern int num_socket;			// Number of Socket Used

//
// Function Prototypes
//

void event_report(struct TCB *tp, unsigned event, unsigned arg);

// Packet Driver Interface Functions

extern int driver_type;

// Driver Type Definition

#define ETHERNET_DRIVER	(1)	// Ethernet or Something like that
#define SLIP_DRIVER	(6)	// SLIP Driver
#define BUFFER_DRIVER	(7)	// Buffer Driver

int  pktisr(void);		// Far-Call Function from PDI, CZERO.ASM
int  init_driver(void);		// PDI initialize, PACKET.C
void close_driver(void);	// Close PDI, PACKET.C

void rcv_packet(void);		// Network Packet Receive, PACKET.C
int  snd_packet(void *packet, int size);	// Send a Packet, PACKET.C
extern int MTU_SIZE;

// ARP Functions

void rcv_arp(void);		// ARP Packet Receive, PACKET.C
int  start_arp(struct TCB *tp, unsigned long addr);
void timedout_arp(struct TCB *tp);

// IP functions

int samenet(unsigned long addr);	// if in the same net with me, return 1
void rcv_ip(void);			// IP Packet Receive, IP.C
void init_ip_stubs(void);
int  snd_ip(struct RFC791 *IP_PKT, int datasize);	// Send, IP.C
int  snd_raw_ip(struct TCB *tp, unsigned char far *data, int count);

// ICMP functions

void rcv_icmp(void);

// TCP functions

void rcv_tcp(void);
int  snd_tcp(struct TCB *tp, int seg_type, int next_state, int data_packet);
void check_tcp_send(struct TCB *tp);

// UDP functions

void rcv_udp(void);
void snd_udp(struct TCB *tp, unsigned char far *data, int count);
void check_udp_send(struct TCB *tp);

// Timer functions

void init_timer(void);
int  close_timer(void);
struct TIMER *get_timer(void);
#define free_timer(timer)	{ (timer)->tick = 0; }
extern unsigned long ticknow;
extern unsigned Tick_Count;
extern tcp_seq random_iss;
void init_s_iss(struct TCB *tp);
void check_tcp_timer(struct TCB *tp);
void clean_timer(struct TCB *tp);
void tmr_service(void);

// Protocol Support Functions

unsigned short checksum(void *packet, int length);
void clean_it(struct TCB *tp, int errno);

// Buffer Managing Function

void clean_rx_buffer(struct TCB *tp);
void clean_tx_buffer(struct TCB *tp);
void clean_udp_tx_buffer(struct TCB *tp);

unsigned init_buffer(unsigned char *start);
struct BUFFER *get_buffer(struct TCB *tp, int size, int type);
void free_buffer(struct BUFFER *buffer);
void free_list(struct BUFFER *buffer);
extern unsigned remained_bufsize;
unsigned data_avail(struct TCB *tp);	// Data maintained by a TCB

extern struct BUFFER *DEF_RX_POOL, *DEF_TX_POOL, *BUF_POOL;
#define IS_DEF_BUF(bp)		(((bp) - DEF_RX_POOL) < (2 * num_socket))
#define BUFFER_SIZE(bp)		(IS_DEF_BUF((bp)) ? DEF_BUFSIZE : MAX_BUFSIZE)
struct BUFFER *get_dgram_buf(struct TCB *tp, int length);

//=========================================================
//
// Support Function Prototypes

void copy_mem(unsigned d_s, unsigned d_o, unsigned s_s, unsigned s_o, int cnt);
void copy_hw_addr(void *dst, void *src);
void clear_mem(unsigned seg, unsigned ptr, int cnt);

unsigned long  lswap(unsigned long val);
unsigned short bswap(unsigned short val);

// bswap for CONSTANT
#define BSWAP(x)	(unsigned short)((x / 256) + (x & 0xFF) * 256)
#define HTONS(x)	BSWAP((x))
#define NTOHS(x)	BSWAP((x))

#define __highw(x)	(unsigned short)((unsigned long)(x) / 0x10000L)
#define __loww(x)	(unsigned short)((unsigned long)(x))

int  splnet(void);			// Disable Interrupt
void splx(int spl);			// Return to last state

void dos_print(char *str);	// Print strings with DOS Call
void printdec(unsigned value);
void printhex(unsigned char value);
void error_print(char *str);
unsigned short get_arg(int *argv, int *arglen, int base);
int  skip_blank(int *argv, int *arglen);

void rprintf(char *format, ...);		// non-bios printf for debugging
void __ring(void);
void print_ether_addr(unsigned char *eadr);	// print Ethernet Address
void print_packet(void *packet, int size);	// print a packet

// TSR Program Signature Search Function
int find_signature(char *sign_str, int sign_len, int v_start, int v_end);

// Interrupt Vector Table Access Macros

#define VEC_SEG(irq)	(*(unsigned short far *)MK_FP(0, (irq) * 4 + 2))
#define VEC_OFF(irq)	(*(unsigned short far *)MK_FP(0, ((irq) * 4)))
#define VEC_ISR(irq)	(*(unsigned long  far *)MK_FP(0, ((irq) * 4)))

// Socket Functions in Kernel

int  _init_kernel(int flag);
void _load_cfg(struct TCPCONFIG far *config);
void _get_stat(int clearflag, struct snmp_object far *object);
unsigned _sock_stat(int sd, struct TCB far *stat);
int  _socket(int protocol);
int  _connect(int sd, unsigned long destination, unsigned short port);
int  _bind(int sd, unsigned port);
void _listen(int sd, int backlog);
int  _accept(int sd);
void __close(int sd);
int  _recv(int sd, unsigned char far *data, int count, int flag);
int  _send(int sd, unsigned char far *data, int count, int flag);
int  _recvfrom(int sd, unsigned char far *data, int count, int flag);
int  _udp_dest(int sd, unsigned long destination, unsigned port);
int  _sendto(int sd, unsigned char far *data, int count);
void _shutdown(int sd, int how);
void _setoption(int sd, int option, int value);
void _select(struct SELECT far *fds, unsigned long bitmask);
int  _geterror(int sd);
int  _getpointer(struct POINTERS far *ptr);
void _set_async(int sd, void far (* async_func)());
int  _rbind(int sd);

#endif
