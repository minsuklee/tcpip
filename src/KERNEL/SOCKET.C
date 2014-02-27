/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

static unsigned short localport = IPPORT_RESERVED;
static unsigned short resv_port = IPPORT_RESERVED;

int
_socket(int protocol)
{
	int i;
	struct TCB *tp;

	if (tcpipcfg.c_myip == 0)	// Before Load CFG
		return(-1);

	for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
	    if (!(IP_STUB(tp)->transport)) {
		IP_STUB(tp)->transport = protocol;

		// Allocate a Default TRX Buffers for each opened tcb

		tp->rx = tp->rxe = NULL;
		tp->nexttx = tp->tx = tp->txe = NULL;
		tp->tx_len = tp->rx_len = 0;

		tp->remote_port = tp->last_error = 0;

		if (protocol == IPPROTO_TCP) {
			// Do some initialize steps for TCP
			tp->round_trip = MIN_RETX_TIME;
			tp->rcv_wnd = MAX_RX_ALLOCATE * MAX_BUFSIZE;
			tp->snd_mss = TCP_MSS;
		}

		// Assign a Random Port to new socket

loop_here:	if (localport >= IPPORT_USERRESERVED)
			localport = IPPORT_RESERVED;
		if (_bind(i, htons(localport++)) < 0)
			goto loop_here;

		//rprintf("[SOCKET %d:%d]", i, ntohs(tp->local_port));
		return(i);
	    }
	}
	// No Empty Socket Now, socket() library will make ENODEV
oerror:	return(-1);
}

#define IPPORT_KERNELRESERVED	900

int
_rbind(int sd)
{
	unsigned short tmp;

loop:	if (resv_port <= IPPORT_KERNELRESERVED)
		resv_port = IPPORT_RESERVED;
	tmp = --resv_port;
	if (_bind(sd, htons(tmp)) < 0)
		goto loop;
	return(tmp);
}

int
_connect(int sd, unsigned long destination, unsigned short port)
{
	struct TCB *tp = tcb + sd;
	int ret = -1;

	// No loopback connection, now
	if (destination == tcpipcfg.c_myip) {
		tp->last_error = ECONNREFUSED;
		goto leave;
	}
	if (!samenet(destination) && (tcpipcfg.c_defgw == 0L)) {
		tp->last_error = ENETUNREACH;
		goto leave;
	}
	if (IP_STUB(tp)->transport != IPPROTO_TCP) {
		clr_status(tp, TCB_UDP_CONNECTED);	// Clear
		if (destination && port &&
		    ((ret = _udp_dest(sd, destination, port)) == 0))
			set_status(tp, TCB_UDP_CONNECTED);
		goto leave;
	} else {
		// TCP Connection Processing

		tp->remote_port = port;
		init_s_iss(tp);
		tp->timeout_count = 1;
		set_status(tp, TCB_TCP_SYN);	// Have to send SYN
		if (start_arp(tp, destination)) {
			check_tcp_send(tp);
		} // else, ARP or Timer can send SYN
		ret = 0;
	}
leave:	if ((IP_STUB(tp)->transport == IPPROTO_TCP) && (ret == -1))
		statistics.tcpAttemptFails++;
	return(ret);
}

int
_bind(int sd, unsigned port)	// port should be in network order
{
	struct TCB *pp, *tp = tcb + sd;
	int i;

	if (port == 0)
		goto leave;
	// check same PROTOCOL/PORT pair
	for (pp = tcb, i = 0; i < num_socket; i++, pp++) {
		if (IP_STUB(pp)->transport != IP_STUB(tp)->transport)
			continue;
		if (pp->local_port == port) {	// JJONG !!
			// But we support multiple server for the same port,
			// one in listen state and the others in connected
			if (IP_STUB(pp)->destination && pp->remote_port)
				continue;
			return(-1);
		}
	}
	tp->local_port = port;
leave:	return(0);
}

void
_listen(int sd, int backlog)
{
	struct TCB *tp = tcb + sd;
	int i;

	tp->logcount = backlog;
	for (i = 0; i < SOMAXCONN; i++)
		tp->backlog[i] = -1;	// Not connected Yet
	IP_STUB(tp)->destination = 0;
	tp->remote_port = 0;
	clr_status(tp,TCB_ARP_ED);
	tp->t_state = TCPS_LISTEN;
	clean_rx_buffer(tp);		// We dont need TRX Buffer Any more
	clean_tx_buffer(tp);
}

int
_accept(int sd)
{
	struct TCB *tp = tcb + sd;
	int i, ret;

	for (i = 0; i < SOMAXCONN; i++) {
		if (tp->backlog[i] >= 0) {
			ret = tp->backlog[i];
			if ((tcb[ret].t_state != TCPS_CLOSED) &&
			    (tcb[ret].t_state < TCPS_ESTABLISHED))
				continue;
			tp->backlog[i] = -1;
			tp->logcount++;
			clr_status(tcb + ret, TCB_TCP_WACC); // Now Accepted
			return(ret);
		}
	}
	return(-1);
}

void
__close(int sd)
{
	struct TCB *tp = tcb + sd;

	// If the socket is a TCP Listen Socket
	// Kill child sockets first recursively

	if (tp->t_state == TCPS_LISTEN) {
		int i;
		for (i = 0; i < SOMAXCONN; i++) {
			if (tp->backlog[i] >= 0) {
				__close(tp->backlog[i]);
				tp->backlog[i] = -1;
			}
		}
	}

	// If the socket is not connected or non_TCP socket just clear it

	if ((IP_STUB(tp)->transport != IPPROTO_TCP) || (tp->t_state < TCPS_ESTABLISHED)) {
		IP_STUB(tp)->transport = 0;
		clean_it(tp, 0);
	} else {
		// tp->t_state must be TCPS_ESTABLISHED or TCPS_CLOSE_WAIT here

		// Deallocate Receive Buffer
		clean_rx_buffer(tp);	// We dont need RX Buffer any more

		// Send FIN segment
		//rprintf("(%ld == %ld)", tp->snd_nxt, tp->snd_una);
		set_status(tp, TCB_TCP_FIN);	// Have to send FIN
		check_tcp_send(tp);
	}
	tp->async_func = (void far *)NULL;
}

// UDP Receive may suffer from NO-Large Buffer symptom
// This can be cured by "setsockopt(SO_RCVBUF)" ----------- need revise

int
_recvfrom(int sd, unsigned char far *data, int count, int flag)
{
	struct BUFFER *tbp;
	struct TCB *tp = tcb + sd;
	unsigned long  from_address;
	unsigned short from_port;

	if (flag)		// We dont support Any flag for Datagram Socket
		return(0);

	// We should Support Fragmented UDP packet here
	// for fragmented buffer we must apply multiple copy_mem()
	// Check MORE_FRAGMENT bit of buffer->buffer_type

	tbp = tp->rx;

	if (!tbp || tbp->r_count == 0)
		return(0);

	if (count > tbp->r_count)
		count = tbp->r_count;

	//rprintf("[recvfrom %d : %04X]", sd, tbp);
	copy_mem(FP_SEG(data), FP_OFF(data), _DS, (unsigned)(tbp->data), count);

	from_address = tbp->r_ip;
	from_port = tbp->r_port;

	if (tbp->next) {
		tp->rx = tbp->next;
	} else {
		tp->rx = tp->rxe = NULL;
	}
	free_buffer(tbp);
	//rprintf("[:%04X]", tp->rx);

	// ----------   WARNNING   ---------
	// This is not Portable Code to make Making return value
	// Do not Insert any "C" Statement
	_DX = __highw(from_address); _BX = __loww(from_address);
	_CX = from_port;
	return(count);
}

int
_udp_dest(int sd, unsigned long destination, unsigned port)
{
	struct TCB *tp = tcb + sd;

	if (!samenet(destination) && (tcpipcfg.c_defgw == 0L)) {
		// This implies we dont support DONTROUTE option
		tp->last_error = ENETUNREACH;
		return(-1);
	}

	if (tp->status & TCB_SENDTO_WARP) {	// Have data in Queue
		tp->last_error = EWOULDBLOCK;
		return(-1);
	}

	tp->remote_port = port;
	if (IP_STUB(tp)->destination != destination) {
		clr_status(tp, TCB_ARP_ED);
	} else if ((destination == 0x100007fL) || (destination == tcpipcfg.c_myip)) {
		set_status(tp, TCB_ARP_ED);
	} IP_STUB(tp)->destination = destination;
	return(0);
}

int
_sendto(int sd, unsigned char far *data, int count)
{
	struct TCB *tp = tcb + sd;

	if (tp->status & TCB_SENDTO_WARP)	// Have data in Queue
		return(0);

	//rprintf("[sendto %d, %lx, %d]", sd, data, count);
	// We dont support IP fragmentation so
	// We cannot send a packet that is too long

	if (start_arp(tp, IP_STUB(tp)->destination)) {		// ARP DONE
send_it:	if (count > MAX_UDP_TXSIZE)
			count = MAX_UDP_TXSIZE;
		snd_udp(tp, data, count);
	} else {				// ARP PENDING, wait for reply
		// Buffer Type should be checked after
		if (!(tp->tx = get_buffer(tp, count, TX_BUFFER)))
			return(0);
		if (count > BUFFER_SIZE(tp->tx)) {
			count = BUFFER_SIZE(tp->tx);
		}
		// Think again for fragmentation
		copy_mem(_DS, (unsigned)((tp->tx)->data),
			 FP_SEG(data), FP_OFF(data), count);
		(tp->tx)->r_count = count;
		// Check ARP is done before set WARP flag
		set_status(tp, TCB_SENDTO_WARP);// Have data in Queue
	}
	return(count);
}

int
_recv(int sd, unsigned char far *data, int count, int flag)
{
	register struct BUFFER *tbp;
	struct TCB *tp = tcb + sd;
	int total_read = 0, tmp;

	if (flag & MSG_OOB) {
		// We dont support Out-Of-Band data Now
		return(0);
	}
again:	tbp = tp->rx;
	while (count) {
		//rprintf("<%04X:%d>", tbp, tbp->r_count);
		if (!tbp) {
			//if (!tbp) rprintf("[NO RXB]");
			break;
		}
		if (!(tbp->r_count))
			goto check_empty;

		tmp = min(count, tbp->r_count);
		if ((tmp + tbp->r_start) > BUFFER_SIZE(tbp))	// Wrapped
			tmp = BUFFER_SIZE(tbp) - tbp->r_start;
		copy_mem(FP_SEG(data), FP_OFF(data),
			_DS, (unsigned)(tbp->data) + tbp->r_start, tmp);
		count -= tmp;
		data += tmp;
		total_read += tmp;

		tbp->r_start = (tbp->r_start + tmp) % BUFFER_SIZE(tbp);
		tbp->r_count -= tmp;

check_empty:	if (!tbp->r_count) {
			struct BUFFER *tmp = tbp;
			if (tbp->next) {
				tp->rx = tbp->next;
				tbp = tp->rx;
				tp->rx_len--;
				free_buffer(tmp);
			} else {
				// No more received Data
				tp->rx = tp->rxe = NULL;
				tp->rx_len = 0;
				free_buffer(tbp);
				break;
			}
		}
	}
	// Wait more time because application still wanna do somthing
	//  for this socket
	if (tp->t_state >= TCPS_CLOSE_WAIT)
		tp->cleanup_timer = CLOSE_WAIT_TIME;

	if ((total_read == 0) &&
	    ((tp->t_state >= TCPS_CLOSE_WAIT) || (tp->t_state == TCPS_CLOSED))) {
		// Packet Interrupt Service Routine Can fill the buffer
		// between while loop and TCPS_CLOSE_WAIT check
		//rprintf("[NO DATA & CLOSED %d]", data_avail(tp));
		if (tp->rx && (tp->rx)->r_count)
			goto again;
		tp->last_error = ENOTCONN;
		event_report(tp, IEVENT_CLOSE_WAIT, 0);
		return(-1);
	}
	// Receive Window Adjustment & Advertizement
	if (total_read > 0) {
		tp->rcv_wnd += total_read;
#define ALWAYS_ADVERTIZE
#ifdef ALWAYS_ADVERTIZE
		snd_tcp(tp, TH_ACK, tp->t_state, 0);
#else
		//rprintf("-%d-", total_read);
		if (!(tp->rcv_adv)) {		// dont adv. small window
			if (tp->rcv_wnd > tp->rcv_mss)
				goto advertize_window;
		}
		// Next scheme should be checked for performance improvement
		else if ((tp->rcv_wnd - tp->rcv_adv) > (MAX_BUFSIZE * (MAX_RX_ALLOCATE / 2))) {
advertize_window:		snd_tcp(tp, TH_ACK, tp->t_state, 0);
		}
#endif
	}
	//if (total_read) rprintf("[%d]", total_read);
	return(total_read);
}

int
_send(int sd, unsigned char far *data, int count, int flag)
{
	struct TCB *tp = tcb + sd;
	unsigned total_copied = 0, poke_start, tmp;
	struct BUFFER *tbp;

	// Connection is closed or is closing, BUT
	// To be sorry, application may not know this important information

	if ((tp->t_state == TCPS_CLOSED) || (tp->t_state > TCPS_CLOSE_WAIT)) {
		tp->last_error = ENOTCONN;
		return(-1);
	}

	if (flag & MSG_OOB) {
		// We should process OOB in differrent way
	}

	//rprintf("[#");
	if (tp->tx) {
		tbp = tp->txe;
	} else {
		if (!(tbp = tp->tx = tp->txe = tp->nexttx = get_buffer(tp, count, TX_BUFFER))) {
			return(0);
		}
		tbp->s_seq = tp->snd_nxt;
		tp->tx_len = 1;
	}
	while (total_copied < count) {
		if (tbp->r_count == BUFFER_SIZE(tbp)) {  // Buffer Full
			struct BUFFER *tbuf;

			// Allocate Additional Buffer
			if (tp->tx_len >= MAX_TX_ALLOCATE) {
				//rprintf("M");
				break;
			}
			if (!(tbuf = get_buffer(tp, (count - total_copied), TX_BUFFER))) {
				//rprintf("[BUFFER FULL]");
				break;
			}
			tbuf->s_seq = tbp->s_seq + tbp->r_count;
			tbp->next = tp->txe = tbuf;
			tp->tx_len++;
			tbp = tbuf;
		}
start_copy:	poke_start = (tbp->r_start + tbp->r_count) % BUFFER_SIZE(tbp);
		tmp = min(BUFFER_SIZE(tbp) - tbp->r_count, count - total_copied);
		if ((poke_start + tmp) > BUFFER_SIZE(tbp)) 	// wrap
			tmp = BUFFER_SIZE(tbp) - poke_start;
		copy_mem(_DS, (unsigned)tbp->data + poke_start,	FP_SEG(data), FP_OFF(data + total_copied), tmp);
		//rprintf("{S:%ld,%d(%d)}", tbp->s_seq, poke_start, tmp);
		tbp->r_count += tmp;
		total_copied += tmp;
	}
	if (total_copied) {
		//rprintf("{%d}", total_copied);
		set_status(tp, TCB_TCP_DATA);
		check_tcp_send(tp);
	}
	//rprintf("#]");
	return(total_copied);
}

void
_shutdown(int sd, int how)
{
	struct TCB *tp = tcb + sd;

	if (!(how & 1)) {
		clean_rx_buffer(tp);	// We dont need RX Buffer any more
	}
	// We should not clean the retransmit Buffer, we must wait the ACK
	if ((how > 0) && (tp->tx)) {	// Send FIN
		set_status(tp, TCB_TCP_FIN);
		check_tcp_send(tp);
	}
}

void
_setoption(int sd, int option, int value)
{
	struct TCB *tp = tcb + sd;

	if (value) {
		tp->option |= option;
	} else {
		tp->option &= ~option;
	}
	//if (option == SO_BROADCAST)
	//	clr_status(tp, TCB_ARP_ED);
}

void
_select(struct SELECT far *fds, unsigned long bitmask)
{
	int i, j;
	struct TCB *tp;
	unsigned long checkpoint = 1;

	clear_mem(FP_SEG(fds), FP_OFF(fds), sizeof(struct SELECT));
	for (tp = tcb, i = 0; i < num_socket; i++, tp++, checkpoint *= 2) {
		if (!(bitmask & checkpoint))
			continue;
		if (!(IP_STUB(tp)->transport) ||
		    ((IP_STUB(tp)->transport == IPPROTO_TCP) &&
		     !(tp->status & TCB_ARP_ING) &&
		     !(tp->status & TCB_TCP_SYN) &&
		     (tp->t_state == TCPS_CLOSED))) {
			//rprintf("--------- SOCKET %d CLOSED ---------", i);
			fds->closed |= checkpoint;
		}
		if ((IP_STUB(tp)->transport == IPPROTO_TCP) &&
		    (tp->t_state == TCPS_ESTABLISHED))
			fds->connect |= checkpoint;
		if (((IP_STUB(tp)->transport == IPPROTO_TCP) &&
		     ((tp->t_state == TCPS_CLOSED) ||
		      (tp->t_state == TCPS_CLOSE_WAIT))) ||
		    (tp->rx && (tp->rx)->r_count))
			fds->readfds |= checkpoint;
		if ((IP_STUB(tp)->transport == IPPROTO_TCP) &&
		    (tp->t_state == TCPS_LISTEN)) {
			for (j = 0; j < SOMAXCONN; j++) {
				if ((tp->backlog[j] >= 0) &&
				    (tcb[tp->backlog[j]].t_state >= TCPS_ESTABLISHED)) {
					fds->readfds |= checkpoint;
					break;
				}
			}
		}
		if (remained_bufsize ||
		    ((IP_STUB(tp)->transport == IPPROTO_TCP) &&
		     (tp->tx) && ((tp->txe)->r_count < BUFFER_SIZE(tp->txe))))
			fds->writefds = checkpoint;
		// OOB Data Check
	}
}

int
_geterror(int sd)
{
	return(tcb[sd].last_error);
}

void
clean_rx_buffer(struct TCB *tp)
{
	struct BUFFER *tmp;

	tmp = tp->rx;
	tp->rx = tp->rxe = NULL;	// Remove RX Buffer from TCB
	tp->rx_len = 0;
	free_list(tmp);
}

void
clean_tx_buffer(struct TCB *tp)
{
	struct BUFFER *tmp;

	tmp = tp->tx;
	tp->nexttx = tp->tx = tp->txe = NULL;	// Remove RX Buffer from TCB
	tp->tx_len = 0;
	free_list(tmp);
}

void
clean_it(struct TCB *tp, int errcode)
{
	struct BUFFER *tmp;

	tp->t_state = TCPS_CLOSED;	// TCP CLOSED
	clr_status(tp, TCB_ARP_ED);	// Keep the ARPDONE Status
	tp->last_error = errcode;

	tp->option = 0;
	tp->logcount = 0;

	clean_rx_buffer(tp);		// Free RX Buffer
	clean_tx_buffer(tp);		// Free TX Buffer

	clean_timer(tp);		// Free All Retransmittion Timer

	// Report This Socket is Closed in Any Reason
	event_report(tp, IEVENT_CLOSED, 0);
}
