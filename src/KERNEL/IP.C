/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

static unsigned short my_identification = 0;

int
samenet(unsigned long addr)
{
	// Return 1, if the addr_host is on the same network.
	return((addr & tcpipcfg.c_subnet) == (tcpipcfg.c_myip & tcpipcfg.c_subnet));
}

void
rcv_ip()
{
	statistics.ipInReceives++;

	if (IP_P->destination != tcpipcfg.c_myip) {
		// Must Check for Multicast Address to use IP MULTICASTING
		// And Broadcast
		statistics.ipInAddrErrors++;
		return;
	}

	// We reject IP Packet with IP options(hdr_len != 5), 
	//			    Other version (ver != 4)
	if (IP_P->hdr_len_ver != 0x45) {
		statistics.ipInUnknownProtos++;
		return;
	}

	if (checksum(&(IP_P->hdr_len_ver), IP_HLEN)) {
		//rprintf("[IP CHKSUM]");
		statistics.ipInHdrErrors++;
		return;
	}

//	There must be some measure against the fragmented Packet
//	We should do packet Reassembly
//
// We dont even check the frag-bits, some host send IP packet with
// MAY-FRAG bit set for small packets
//
//	if (IP_P->frag_offset) {
//		// (IP_P->frag_offset * 8) is the real offset
//		statistics.ipReasmReqds++;
//		statistics.ipReasmFails++;
//		// But now We don't
//		return;
//	}

	//rprintf("[PROTO = %d]", IP_P->transport);
	switch (IP_P->transport) {
	   case IPPROTO_ICMP : rcv_icmp(); break;
	   case IPPROTO_TCP  :
			//rprintf("[T");
			rcv_tcp();
			//rprintf("T]");
			break;
	   case IPPROTO_UDP  : rcv_udp();  break;
	   case IPPROTO_IP   : // rcv_raw_ip(); break
	   default :	// We support no other protocols
		statistics.ipInUnknownProtos++; break;
	}
}

void
init_ip_stubs()			// called from _load_cfg:kernel.c
{
	int i;
	struct TCB *tp;
	extern struct RFC826 ARP_STUB;

	ARP_STUB.my_proto_addr = tcpipcfg.c_myip;
	for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
		IP_STUB(tp)->source = tcpipcfg.c_myip;
		IP_STUB(tp)->hdr_len_ver = 0x45;
		ETH_STUB(tp)->protocol = ETIP;
	}
}

int
snd_ip(struct RFC791 *IP_PKT, int datasize)
{
	// some ICMP statistics here

	if (IP_PKT->transport == IPPROTO_ICMP) {
		statistics.icmpOutMsgs++;
		if (((struct RFC792 *)IP_PKT)->packet_type == ECHO_REQUEST)
			statistics.icmpOutEchos++;
	}

	// We must check the size with MTU size of the Network Interface
	// Then fragment the packet, if needed.
	IP_PKT->packet_id = htons(my_identification++);
	IP_PKT->total_len = htons(datasize + IP_HLEN);
	IP_PKT->time_to_live = IP_TTL;
	IP_PKT->checksum = 0;
	IP_PKT->checksum = checksum(&(IP_PKT->hdr_len_ver), IP_HLEN);
	statistics.ipOutSends++;
	//print_packet(IP_PKT, 0);
	return(snd_packet(IP_PKT, datasize + IP_HLEN + ETHER_HLEN));
}

int
snd_raw_ip(struct TCB *tp, unsigned char far *data, int count)
{
	int ret;

	if ((IP_STUB(tp)->destination == tcpipcfg.c_myip) ||
	    (IP_STUB(tp)->destination == 0x100007FL)) {
		// We support Loop back only for ICMP;
		struct BUFFER *rbp;
		if (!(rbp = get_dgram_buf(tp, count)))
			return(-1);
		//rprintf("[%d:%04X]", i, rbp);
		rbp->r_count = count;
		rbp->r_ip = IP_STUB(tp)->destination;
		copy_mem(_DS, (unsigned)(rbp->data), FP_SEG(data), FP_OFF(data), count);
		event_report(tp, IEVENT_READ, data_avail(tp));
		ret = 0;
	} else {
		copy_mem(_DS, (unsigned)IP_S, _DS, (unsigned)IP_STUB(tp), IP_PLEN);
		copy_mem(_DS, (unsigned)IP_S + IP_PLEN, FP_SEG(data), FP_OFF(data), count);
		ret = snd_ip(IP_S, count);
	}
	return(ret);
}

void
rcv_icmp()
{
	struct TCB *tp;
	struct BUFFER *rbp;
	unsigned short icmp_size, i;
	unsigned char *rd;

	//rprintf("[ICMP_R]");
	//print_packet(ICMP_P, 0);
	statistics.icmpInMsgs++;
	icmp_size = ntohs(IP_P->total_len) - IP_HLEN;	//  ICMP Packet size 
	if (checksum(&(ICMP_P->packet_type), icmp_size)) {
		//rprintf("[ICMP CHKSUM]");
		statistics.icmpInErrors++;
		return;
	}
	switch (ICMP_P->packet_type) {
	   case ECHO_REQUEST :	/* Incomming Request */
		statistics.icmpInEchos++;
		//rprintf("[ICMP ECHO_REQUEST]");
		copy_hw_addr(ETH_P->destination, ETH_P->source);
		copy_hw_addr(ETH_P->source, ETH_STUB(tcb)->source);
		IP_P->destination = IP_P->source;
		IP_P->source = tcpipcfg.c_myip;
		ICMP_P->packet_type = ECHO_REPLY;
		ICMP_P->checksum = 0;
		ICMP_P->checksum = checksum(&(ICMP_P->packet_type), icmp_size);
		//rprintf("ECHO REPLY");
		snd_ip(IP_P, icmp_size);
		break;
	   case ECHO_REPLY :		/* Echo Backed Data */
		//rprintf("ICMP REPLY");
		statistics.icmpInEchoReps++;
		for (tp = tcb, i = 0; i < num_socket ; i++, tp++) {
			if (IP_STUB(tp)->transport != IPPROTO_ICMP)
				continue;
			if (IP_STUB(tp)->destination == IP_P->source) {
				if (icmp_size > MAX_BUFSIZE)
					icmp_size = MAX_BUFSIZE;
				if (!(rbp = get_dgram_buf(tp, icmp_size)))
					break;
				//rprintf("[%d:%04X]", i, rbp);
				rbp->r_count = icmp_size;
				rbp->r_ip = IP_P->source;
				copy_mem(_DS, (unsigned)(rbp->data), _DS, (unsigned)ICMP_P + IP_PLEN, icmp_size);
				event_report(tp, IEVENT_READ, data_avail(tp));
				//rprintf("DATA COPYED");
			}
		}
		break;
	   case GATEWAY_REDIRECT :	/* ICMP Redirect */
		statistics.icmpInRedirects++;
		//rprintf("ICMP New Gateway : %08lX", GATE_P->new_gateway);
		// Find channels that uses the former
		// check open, TCB_ARP_ED, TCB_ECHO_ING | SYN_SENT, destination, d_port
		for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
			if (!(IP_STUB(tp)->transport) || !(tp->status & TCB_ARP_ED))
				continue;
			if ((IP_STUB(tp)->destination == E_IP->destination) &&
			    (arp_target(tp) == IP_P->source)) {
				tcpipcfg.c_defgw = GATE_P->new_gateway;
				tp->timeout_count = 1;
				tp->syn_fin_timer = 0;
				clr_status(tp, TCB_ARP_ED);
				start_arp(tp, IP_STUB(tp)->destination);
			}
		}
		break;
	   case DEST_UNREACHABLE :	/* ICMP Destination Unreachable */
		statistics.icmpInDestUnreachs++;
		//rprintf("[Dest Unreach: %08lX(%d) from %08lX code:%d]", E_IP->destination, ntohs(E_TCP->destination), IP_P->source, ICMP_P->opcode);
		for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
			if (!(IP_STUB(tp)->transport) || !(tp->status & TCB_ARP_ED))
				continue;
			if (IP_STUB(tp)->destination == E_IP->destination) {
				if (IP_STUB(tp)->transport == IPPROTO_TCP) {
					statistics.tcpAttemptFails++;
					clean_it(tp, 0);
				}
				if (ICMP_P->opcode == ICMP_NETUNREACH) {
					tp->last_error = ENETUNREACH;
				} else {
					tp->last_error = EHOSTUNREACH;
				}
			}
		}
		break;
	   default :
		//rprintf("ICMP UNKNOWN");
		//rprintf(" ERROR Type:%02X OPcode:%02X 1st:%04X 2nd:%d ",
		//	ICMP_P->packet_type,ICMP_P->opcode,ECHO_P->identification,ECHO_P->sequence);
		//rprintf("from:%08lX\n", IP_P->source);
		statistics.icmpInErrors++;
	}
leave:
	;
}
