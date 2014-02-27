/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

unsigned char *packet_buffer;
static unsigned char RING_BUFFER[MAX_RING_BUFFER];
unsigned char send_buffer[PACKET_BUFSIZE];

unsigned short Rhead = 0, Rtail = 0;		// Rx buffer Pointers
unsigned Frame_Count = 0;			// Frame Received count

#define PACKET_SIZE(p)	(*(unsigned *)(RING_BUFFER + (p)))

static int packet_int;		// Packet Driver Interface S/W Interrupt
static int handle[2];

int MTU_SIZE;			// MTU Size
int driver_type;

struct RFC826 ARP_STUB = {
	{ { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF },		// Always Broadcast
	  { 0x00,0x00,0x00,0x00,0x00,0x00 }, ETADR },	// My Ethernet Address
	ARETH, ARIP, 6, 4, ARREQ,		// Ethernet/IP (6,4)
	{ 0x00,0x00,0x00,0x00,0x00,0x00 }, 0L,	// My  Ethernet/IP Address
	{ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, 0L	// His Ethernet/IP Address
};

int
init_driver()
{
	union REGS ireg, oreg;
	struct SREGS sreg;
	unsigned short tmp, i;
	extern int kernel_int;
	struct TCB *tp;

	// Find Packet Driver TSR

	packet_int = find_signature("PKT DRVR", 8, 0x60, 0x7f);
	if (!packet_int) {
		error_print("No Packet Driver.\r$");
		return(-1);
	}
	if ((packet_int == kernel_int) || (packet_int == (kernel_int + 1))) {
		error_print("Bad Interrupt Number.\r$");
		return(-1);
	}
	//dos_print("Packet Driver is found at 0x$");
	//printhex(packet_int);
	//dos_print(".\r\n$");

	ireg.x.ax = 0x01FF;	// Get Driver Info
	ireg.x.bx = 0xFFFF;
	int86(packet_int, &ireg, &oreg);
	driver_type = oreg.x.cflag ? ETHERNET_DRIVER : oreg.h.ch;

	// MTU_SIZE Adjustment be done for Specific Driver
	// MTU size includes TCP/IP header (excludes Datalink header)
	switch (driver_type) {
	//	case  BUFFER_DRIVER   :
		case  ETHERNET_DRIVER : MTU_SIZE = MAX_BUFSIZE + TCP_HLEN; break;
	//	case  SLIP_DRIVER     : MTU_SIZE =  296; break;
		default		      : MTU_SIZE =  576; break;
	}

	ireg.h.ah = 0x02;
	sreg.es = _CS;
	if (driver_type == SLIP_DRIVER) {
		ireg.x.di = (unsigned)RING_BUFFER + MAX_RING_BUFFER / 2;
	} else {
		ireg.h.al = driver_type;
		ireg.x.cx = 0x2;
		ireg.h.dl = 0;
		ireg.x.di = (unsigned)pktisr;	// RX Routine
		tmp = ETIP;
		sreg.ds = _SS; ireg.x.si = (unsigned)&tmp;	// for IP
	}
	int86x(packet_int, &ireg, &oreg, &sreg);
	if (driver_type == SLIP_DRIVER) {
		packet_buffer = RING_BUFFER;
		return(0);
	}
	if (oreg.x.cflag) {
		goto usedbyother;
	} else {
		handle[0] = oreg.x.ax;
	}

	tmp = ETADR; sreg.es = _CS;			// di still alive
	sreg.ds = _SS; ireg.x.si = (unsigned)&tmp;	// for ARP
	int86x(packet_int, &ireg, &oreg, &sreg);
	if (oreg.x.cflag) {
		ireg.h.ah = 0x03;			// release handle[0]
		ireg.x.bx = handle[0];
		int86(packet_int, &ireg, &oreg);
usedbyother:	error_print("Packet Driver in USE.\r$");
		return(-1);
	}
	handle[1] = oreg.x.ax;

get_addr:
	// Read my Ethernet Address
	//dos_print("Read Ethernet Address\n\r$");

	ireg.h.ah = 0x06;
	ireg.x.bx = handle[0];
	ireg.x.cx = 6;					// 6 byte address
	sreg.es = _DS; ireg.x.di = (unsigned)(ARP_STUB.my_hw_addr);
	int86x(packet_int, &ireg, &oreg, &sreg);
	if (oreg.x.cflag) {
		error_print("in Reading H/W Address.\r$");
		close_driver();
		return(-1);
	}

	copy_hw_addr(ARP_STUB.ether_pkt.source, ARP_STUB.my_hw_addr);
	for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
		copy_hw_addr(ETH_STUB(tp)->source, ARP_STUB.my_hw_addr);
	}

	//dos_print("Packet Driver is initialized\n\r$");

	//rprintf("H(%d,%d) ", handle[0], handle[1]);
	//rprintf("ADR:"); print_ether_addr(ARP_STUB.my_hw_addr);
	//rprintf("\n");
	return(0);
}

void
close_driver()
{
	union REGS ireg, oreg;

	ireg.h.ah = 0x03;
	ireg.x.bx = handle[0];
	int86(packet_int, &ireg, &oreg);

	if (driver_type != SLIP_DRIVER) {
		ireg.x.bx = handle[1];
		int86(packet_int, &ireg, &oreg);
	}
}

int
snd_packet(void *packet, int size)
{
	union REGS ireg, oreg;
	struct SREGS sreg;

	//rprintf("<%d", size);
	//print_packet(packet, size);
	statistics.lanOutPackets++;
	ireg.h.ah = 4;
	sreg.ds  = _DS;
	ireg.x.si = (unsigned)packet;
	ireg.x.cx = size;
	int86x(packet_int, &ireg, &oreg, &sreg);
	if (oreg.x.cflag) {		// Packet Driver Failed to send
		statistics.lanOutErrors++;
		//rprintf("[SEND ERR]");
		return(-1);
	}
	//rprintf(">");
	return(0);
}

void
rcv_packet(void)
{
	//rprintf("{", ETH_P->protocol);
	statistics.lanInPackets++;
	switch (ETH_P->protocol) {
	   case ETIP  :
		//rprintf("I");
		rcv_ip();
		break;
	   case ETADR :
		//rprintf("A");
		rcv_arp();
		break;
	   default :	// Unknown Packet : just Discard
		//rprintf("X");
		statistics.lanInUnknownProtos++;
		break;
	}
	//rprintf("}");
}

// START ARP : check TCB tables
//
//	return   1 : ARP is Done
//		 0 : ARP_Request is Sent

int
start_arp(struct TCB *tp, unsigned long addr)
{
	struct TCB *sp;
	int i;

	if ((addr == IP_STUB(tp)->destination) && (tp->status & TCB_ARP_ED))
		return(1);
	if (driver_type == SLIP_DRIVER)		// We dont need ARP
		goto arp_ed;
	if ((tp->option & SO_BROADCAST) && (addr == INADDR_BROADCAST)) {
		// Copy Broadcast Address
		copy_hw_addr(ETH_STUB(tp)->destination, ARP_STUB.ether_pkt.destination);
		goto arp_ed;
	};
	for (sp = tcb, i = 0; i < num_socket; i++, sp++) {
		if ((sp->status & TCB_ARP_ED) &&
		    (IP_STUB(sp)->destination == addr)) {
			copy_hw_addr(ETH_STUB(tp)->destination, ETH_STUB(sp)->destination);
arp_ed:			IP_STUB(tp)->destination = addr;
			set_status(tp, TCB_ARP_ED);
			return(1);
		}
	}
	IP_STUB(tp)->destination = addr;
	arp_target(tp) = samenet(addr) ? addr : tcpipcfg.c_defgw;

	// Failed to find Matched Address, Now do the ARP
	ARP_STUB.his_proto_addr = arp_target(tp);
	clr_status(tp, TCB_ARP_ED);
	set_status(tp, TCB_ARP_ING);
	statistics.arpOutRequests++;
	tp->arp_retrytimer = ONE_SECOND;
	tp->timeout_count = 1;
	//rprintf("[ARP_S]");
	//print_packet(&ARP_STUB, 0);
	snd_packet(&ARP_STUB, ARP_PLEN);
	return(0);
}

void
rcv_arp(void)
{
	if ((ARP_P->hw_type != ARETH) || (ARP_P->protocol != ARIP))
		goto leave;
	if (ARP_P->his_proto_addr != tcpipcfg.c_myip) {
		//rprintf("[ARP:NOT FOR ME]");
		goto leave;
	}
	//rprintf("[ARP]");
	//print_packet(ARP_P, 0);
	switch (ARP_P->opcode) {
	   case ARREQ :				// Incomming Request
		statistics.arpInRequests++;
		copy_hw_addr(ETH_P->destination, ARP_P->my_hw_addr);
		ARP_P->his_proto_addr = ARP_P->my_proto_addr;
		copy_hw_addr(ARP_P->his_hw_addr, ARP_P->my_hw_addr);
		copy_hw_addr(ARP_P->my_hw_addr, ARP_STUB.my_hw_addr);
		ARP_P->my_proto_addr = tcpipcfg.c_myip;
		ARP_P->opcode = ARREP;
		snd_packet(ARP_P, ARP_PLEN);
		break;

	   case	ARREP : {			// Response for my ARP Request
		struct TCB *tp;
		int i;

		//rprintf("[R_A-REP]");
		for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
			if (!(IP_STUB(tp)->transport))
				continue;

			// Update All TCB's with matching IP address
			if ((tp->status & TCB_ARP_ING) && (arp_target(tp) == ARP_P->my_proto_addr)) {
				//rprintf("[ARP-OK %d]", i);
				copy_hw_addr(ETH_STUB(tp)->destination, ARP_P->my_hw_addr);
				if (tp->status & TCB_ARP_ING) {
					tp->arp_retrytimer = 0;	// Stop timer
					tp->timeout_count = 0;
					clr_status(tp, TCB_ARP_ING);
					set_status(tp, TCB_ARP_ED);
					statistics.arpInReplys++;
				}
				check_tcp_send(tp);
				check_udp_send(tp);
			}
		}
		break;
	   }
	   default :
		break;
	}
leave:
	;
}

void
timedout_arp(struct TCB *tp)
{
	if (tp->timeout_count >= ARP_RETRY) {
		tp->last_error = ETIMEDOUT;
		clr_status(tp, TCB_ARP_ING | TCB_ARP_ED);	// Failed
		event_report(tp, IEVENT_ARP_FAILED, 0);
		if (tp->status & TCB_TCP_SYN) {
			clr_status(tp, TCB_TCP_SYN);
		} else if (tp->status & TCB_SENDTO_WARP) {
			clr_status(tp, TCB_SENDTO_WARP);
			clean_udp_tx_buffer(tp);		// Discard Data
		}
		return;
	}
	ARP_STUB.his_proto_addr = arp_target(tp);
	statistics.arpOutRequests++;
	tp->arp_retrytimer = ONE_SECOND * tp->timeout_count++;
	//rprintf("[A_TS]");
	snd_packet(&ARP_STUB, ARP_PLEN);
}

unsigned char far *
first_call(unsigned size)
{
retry:	if ((Rtail == Rhead) && Frame_Count) {
		goto no_buffer;
	}
	if (Rhead < Rtail) {
		if ((Rtail - Rhead) > (size + 2)) {
buffer_ok:		PACKET_SIZE(Rhead) = size;
			return(RING_BUFFER + Rhead + 2);
		} else {
			goto no_buffer;
		}
	} else if ((MAX_RING_BUFFER - Rhead) > (size + 2)) {
		goto buffer_ok;
	} else {
		PACKET_SIZE(Rhead) = 0;
		Rhead = 0;
		goto retry;
	}
no_buffer:
	return(NULL);			// Bufer Full
}

void
second_call(void)
{
	int spl;
	Rhead += (PACKET_SIZE(Rhead) + 2);

	spl = splnet();
	Frame_Count++;
	splx(spl);
}


void
check_buffer()
{
	if (driver_type == SLIP_DRIVER) {
		union REGS ireg, oreg;

		ireg.h.ah = 6;
		ireg.x.cx = _DS;
		ireg.x.dx = (unsigned)RING_BUFFER;
loop:		//rprintf("BEFORE SLIP");
		int86(packet_int, &ireg, &oreg);
		//rprintf("AFTER SLIP");
		if (oreg.x.ax) {
			statistics.lanInPackets++;
			rcv_ip();
			goto loop;
		}
	} else {
		while (Frame_Count || Tick_Count) {
			if (Frame_Count) {
				if (!PACKET_SIZE(Rtail))
					Rtail = 0;
				packet_buffer = RING_BUFFER + Rtail + 2;
				rcv_packet();
				Rtail += (PACKET_SIZE(Rtail) + 2);
				disable();
				Frame_Count--;
				enable();
			}
			if (Tick_Count) {
				tmr_service();
			}
		}
	}
}
