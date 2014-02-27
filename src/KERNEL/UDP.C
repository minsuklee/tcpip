/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

// If we support IP Fragmentation,
// another checksum() must be used to accumulate the sum thru multiple buffers

void
rcv_udp()
{
	struct TCB *tp;
	struct BUFFER *rbp;
	int i;

	statistics.udpInDatagrams++;

	//rprintf("[UDP %08lX(%04X)->%04X]", IP_P->source, ntohs(UDP_P->source), ntohs(UDP_P->destination));
	if (UDP_P->checksum) {
		//rprintf("[U_CHK]");
		// Setup Pseudo Header
		IP_P->time_to_live = 0; IP_P->checksum = UDP_P->length;
		if (checksum(&(IP_P->time_to_live), PSEUDO_HLEN + ntohs(UDP_P->length))) {
			statistics.udpInErrors++;
			goto leave;
		}
	} // if (UDP_P->checksum == 0), Ignore UDP checksum (BSD Convention)

	UDP_P->length = ntohs(UDP_P->length) - UDP_HLEN;
	for (tp = tcb, i = 0; i < num_socket ; i++, tp++) {
		if ((IP_STUB(tp)->transport != IPPROTO_UDP) ||
		    (tp->local_port != UDP_P->destination))
			continue;
		if (!(tp->status & TCB_UDP_CONNECTED) ||
		    ((tp->remote_port == UDP_P->source) &&
		     (IP_STUB(tp)->destination == IP_P->source)))
			goto udp_catch;
	}
	statistics.udpNoPorts++;
	goto leave;

udp_catch:

	// If we Allow IP Fragmentation following Comparison make no sense

	if (UDP_P->length > MAX_BUFSIZE) {
		UDP_P->length = MAX_BUFSIZE;
	}

	if (!(rbp = get_dgram_buf(tp, UDP_P->length))) {
		//rprintf("[UDP no BUFFER]");
		statistics.UDP_no_buffer++;
		goto leave;
	}

	rbp->r_count = UDP_P->length;
	rbp->r_ip = IP_P->source;
	rbp->r_port = UDP_P->source;
	copy_mem(_DS, (unsigned)(rbp->data), _DS, (unsigned)UDP_P + UDP_PLEN, UDP_P->length);
	statistics.UDP_rcv_bytes += UDP_P->length;
	event_report(tp, IEVENT_READ, data_avail(tp));
	//rprintf("[strored in B=%04x(%d)]", rbp, rbp->r_count);
leave:	
	;
}

void
snd_udp(struct TCB *tp, unsigned char far *data, int count)
{
	if (IP_STUB(tp)->transport != IPPROTO_UDP) {
		snd_raw_ip(tp, data, count);	// RAW_IP_Packet
	} else {
		statistics.udpOutDatagrams++;

		//rprintf("[udp send %d, %d byte]", tp -tcb, count);
		copy_mem(_DS, (unsigned)IP_S, _DS, (unsigned)IP_STUB(tp), IP_PLEN);
		UDP_S->source = tp->local_port;
		UDP_S->length = htons(count + UDP_HLEN);
		UDP_S->destination = tp->remote_port;

		copy_mem(_DS, (unsigned)UDP_S + UDP_PLEN, FP_SEG(data), FP_OFF(data), count);
		statistics.UDP_snd_bytes += count;
		IP_S->time_to_live = 0;
		IP_S->checksum = UDP_S->length;
		UDP_S->checksum = 0;
		UDP_S->checksum = checksum(&(IP_S->time_to_live), PSEUDO_HLEN + UDP_HLEN + count);
		snd_ip(IP_S, UDP_HLEN + count);
		//print_packet(IP_S, UDP_HLEN + count);
	}
	clean_udp_tx_buffer(tp);
}

void
clean_udp_tx_buffer(struct TCB *tp)		// Discard UDP TX Data
{
	if (tp->tx) {
		free_buffer(tp->tx);
		if (!IS_DEF_BUF(tp->tx)) {
			event_report(tp, IEVENT_WRITE, remained_bufsize);
		}
		tp->tx = NULL;
	}
}

void
check_udp_send(struct TCB *tp)
{
	if (tp->status & TCB_SENDTO_WARP) {
		clr_status(tp, TCB_SENDTO_WARP);
		snd_udp(tp, MK_FP(_DS, (tp->tx)->data), (tp->tx)->r_count);
	}
}

struct BUFFER *
get_dgram_buf(struct TCB *tp, int length)
{
	struct BUFFER *tbp;

	if (!(tbp = get_buffer(tp, length, RX_BUFFER)))
		return(NULL);
	if (tp->rxe) {
		(tp->rxe)->next = tbp;
		tp->rxe = tbp;
	} else
		tp->rx = tp->rxe = tbp;
	return(tbp);
}
