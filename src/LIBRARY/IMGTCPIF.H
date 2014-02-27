/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

//
// IMIGE/TCP 3.00   S/W Interrupt Function Definitions
//
//    WARNING : Assigned numbers are HARD-WIRED in KERNEL/ISRS.ASM
//

#ifndef _IMIGE_TCP_IF_H
#define _IMIGE_TCP_IF_H

#define IMG_RESERVED	(0x00)	// Reserved Function for possible ERROR
#define IMG_INITTCPIP	(0x01)	// Check What TCP/IP TSR is, Uninstall TSR
#define IMG_LOADCFG		(0x02)	// Tell the TSR about the Kernel Configuration
#define IMG_TCP_STAT	(0x03)	// Get Kernel Statistics
#define IMG_SOCK_STAT	(0x04)	// Get status of a socket
#define IMG_SOCKET		(0x05)	// Open a socket
#define IMG_CONNECT		(0x06)	// Connect (TCP, UDP)
#define IMG_BIND		(0x07)	// Bind local port to a socket
#define IMG_LISTEN		(0x08)	// Push a socket into TCPS_LISTEN state
#define IMG_ACCEPT		(0x09)	// Accept a passive-connected socket
#define IMG_CLOSE		(0x0A)	// Close a socket
#define IMG_RECV		(0x0B)	// Read data from TCP socket
#define IMG_SEND		(0x0C)	// Send data to TCP socket
#define IMG_RECVFROM	(0x0D)	// Recv data from UDP socket
#define IMG_SENDTO		(0x0E)	// Send data to UDP socket
#define IMG_SHUTDOWN	(0x0F)	// Block send/recv from socket
#define IMG_SETOPTION	(0x10)	// Set socket options
#define IMG_SELECT		(0x11)	// Poll sockets
#define IMG_GETERROR	(0x12)	// Get error number
#define IMG_GETPOINTER	(0x13)	// Get kernel pointers
#define IMG_UDP_DEST	(0x14)	// Set UDP Destination
#define IMG_ASYNC		(0x15)	// Set Socket Specific Event Function
#define IMG_RBIND		(0x16)	// Bind a Reserved Port

// Kernel Reports Following Events thru S/W Interrupt
// The S/W Interrupt Number is (_imige_vec + 1)
//
// Kernel Reports Event to registered(thru IMG_ASYNC) event function.
//
// in KERNEL/KERNEL.C event_report() function is defined as
//
//	 void event_report(struct TCB *tp, unsigned event, unsigned arg);
//
// Where
//   sd    (in AH) socket number for this event applies (actually tp - tcb)
//	 event (   AL) what happens
//	 arg   (   BX) further details for this event
//
// socket specific far call is done after push arg, event, sd (2 byte each).
//
// IMPORTANT WARNING to make the Event Interrupt Serice Routine EISR
//			     and Socket Specific Event Processing Function:
//
//  1. The Event Interrupt(Function) is called in Timer, Packet, User I/F ISR.
//  2. The Kernel Context must be saved and restored in EISR.
//  3. EISR cannot call the Kernel Itself, the Kernel is not reentrant
//  4. EISR must assign it's own stack, the Kernel leaves 200+ bytes of stack.
//  5. EISR must be finished as soon as possible. (in 1 msec or less)
//  X. ... and more

#define IEVENT_READ			(1)		// Received Data Available
#define IEVENT_OOB			(2)		// Out-of-Band Data Available
#define IEVENT_WRITE		(3)		// Write Buffer Available
#define IEVENT_CLOSED		(4)		// Connection is Closed in any reason
#define IEVENT_CLOSE_WAIT	(5)		// Remote Closed Connection
#define IEVENT_ACCEPT		(6)		// Passive-Open Connection Available
#define IEVENT_CONN_DONE	(7)		// Active Connection Made
#define IEVENT_CONN_FAILED	(8)		// Connection Failed
#define IEVENT_PING_DONE	(9)		// Ping Requested Replied
#define IEVENT_PING_FAILED	(10)	// Ping Requested Failed
#define IEVENT_ARP_FAILED	(11)	// ARP Failed
#define IEVENT_FATAL		(12)	// Kernel Meets Fatal Error

struct SELECT {
	fd_set closed;		// Closed
	fd_set connect;		// Connectivity
	fd_set readfds;		// Readability, Acceptability
	fd_set writefds;	// Writability
	fd_set exceptfds;	// OOB Data Availability
};

struct POINTERS {
	int    num_socket;			// number of possible socket
	struct TCB far *tcb;		// TCB base Pointer
	struct BUFFER far *buffer;	// Buffer Pool
	struct TIMER far *timer;	// Timer Pool
	int no_buffer;				// Number of buffer
	int no_timer;				// Number of timer
};

#endif
