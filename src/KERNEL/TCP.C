/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

static struct TCB *cur_tp;
static tcp_seq cur_seq, cur_ack, my_want;
static int tcp_hdr_len, tcp_data_len, data_offset;
static int RSTed, ACKed, FACKed, SYNed, FINed, send_ACK;
static struct TCB *cur_send;

static int  quick_reply(int seg_type, int next_state);
static void rd_snd_mss(void);
static void copy_data(void);
static int  conn_check(void);
static void check_numbers(void);
static void check_timers(void);
static void check_buffers(void);
static void set_rcv_mss_opt(struct RFC793 *TCP_PKT, struct TCB *tp);

int
snd_tcp(struct TCB *tp, int seg_type, int next_state, int data_packet)
{
	int packet_len, data_len = 0, tmp, ret;
	struct TIMER *tmr = NULL;
	struct BUFFER *tbp;

	// start various timer to check peer's death

	if (data_packet && !(tmr = get_timer())) {
		//rprintf("TIMER BUFFER POOL\n");
		return(-1);
	}

	//rprintf("[S:%02X:", tp->status);
	if (seg_type & (TH_FIN | TH_RST))
		seg_type |= TH_ACK;
	// Copy IP Header
	copy_mem(_DS, (unsigned)IP_S, _DS, (unsigned)(IP_STUB(tp)), IP_PLEN);
	TCP_S->source = tp->local_port;
	TCP_S->destination = tp->remote_port;
	TCP_S->flags = seg_type;

	TCP_S->sequence = htonl(tp->snd_nxt);
	if (seg_type & TH_SYN) {
		clr_status(tp, TCB_TCP_SYN);
		packet_len = TCPO_HLEN;	  // SYN Packet includes MSS Option
		set_rcv_mss_opt(TCP_S, tp);
		//rprintf("[SYN SEQ=%ld]", tp->snd_nxt);
	} else {
		packet_len = TCP_HLEN;
		TCP_S->offset = 5;
	}
	if (seg_type & TH_FIN) {
		clr_status(tp, TCB_TCP_FIN);
		//rprintf("[FIN SEQ=%ld]", tp->snd_nxt);
	}
	// CHECK RST Bits
	if (seg_type & TH_RST) {
		statistics.tcpOutRsts++;
		clr_status(tp, TCB_TCP_RST);
	}

	if (seg_type & TH_ACK) {
		clr_status(tp, TCB_TCP_ACK);	// Clear ACK flag
		TCP_S->acknowledge = htonl(tp->rcv_nxt);
		//rprintf("{%ld}", tp->rcv_nxt);
		tp->del_ack_timer = 0;		     // Stop Delayed Ack Timer
		if (tp->status & TCB_TCP_POLL) {
			tp->persist_timer = ROUND_TRIP_TIME(tp);
			clr_status(tp, TCB_TCP_POLL);
			goto end_data_copy;
		}
		if (tp->status & TCB_TCP_KEEP) {
			TCP_S->sequence = htonl(tp->snd_nxt - 1);
			clr_status(tp, TCB_TCP_KEEP);
			data_len = 1;
			goto end_data_copy;
		}
		if (data_packet) {
			int dstart, dcount, tx_point;

			cur_send = tp;
			tbp = tp->nexttx;
			if (!tbp)
				goto no_more_data;
			while (tbp->r_count == 0) {	// Empty List ?
				// If this happen, this buffer will be freed
				//	at next ACK received
				tbp = tbp->next;
				if (!tbp) {		// No More Data
no_more_data:				clr_status(tp, TCB_TCP_DATA);
					goto end_data_copy;
				}
			}
check_again:		//rprintf("[tbp = %04x, BSEQ=%ld, N=%ld, U=%ld, C=%d, S=%d]", tbp, tbp->s_seq, tp->snd_nxt, tp->snd_una, tbp->r_count, tbp->r_start);
			if ((tbp->s_seq + tbp->r_count) < tp->snd_nxt) {
				dcount = 0;
			} else {
				tmp = tp->snd_nxt - tbp->s_seq;	// sent data count
				dstart = (tbp->r_start + tmp) % BUFFER_SIZE(tbp);
				dcount = tbp->r_count - tmp;
			}
			if (dcount == 0) {
				if (!(tbp->next)) {
					goto no_more_data;
				} else {
					tbp = tbp->next;
					tp->nexttx = tbp;
					goto check_again;
				}
			}
			tx_point = (unsigned)send_buffer + IP_PLEN + packet_len;
			while (1) {
				// snd_wnd may be changed while this loop, by ISR
				//rprintf("[dcount=%d, s_mss=%d, s_wnd=%d, sent=%d]",
				//	dcount, tp->snd_mss, tp->snd_wnd, data_len);
				if (!(tmp = min(dcount, min(tp->snd_mss, tp->snd_wnd) - data_len))) {
					//rprintf("Packet Full or no SND_WND");
					if (tp->snd_wnd == 0) {	// Start PERSIST to poll RX window
						//rprintf("[START PERSIST from send]");
						tp->persist_timer = ROUND_TRIP_TIME(tp);
					}
					goto end_data_copy;
				}
				if ((dstart + tmp) > BUFFER_SIZE(tbp)) // wrap
					tmp = BUFFER_SIZE(tbp) - dstart;
				copy_mem(_DS, tx_point, _DS, (unsigned)(tbp->data) + dstart, tmp);
				dcount -= tmp;
				data_len += tmp;
				tx_point += tmp;
				dstart = (dstart + tmp) % BUFFER_SIZE(tbp);
				if (dcount == 0) {
					// tbp->next is not race condition
					if (tbp->next) {
						tbp = tbp->next;
						tp->nexttx = tbp;
						dcount = tbp->r_count;
						dstart = tbp->r_start;
					} else {
						// May be Last Data
						TCP_S->flags |= TH_PUSH;
						break;
					}
				}
			}
		}
	}
end_data_copy:
	if (data_packet && !data_len) {
		if (tmr)
			free_timer(tmr);
		ret = 0;
		goto leave;
	}
	// Should Be tuned later for Excessive Traffic

	if (tp->t_state > TCPS_ESTABLISHED) {
		// Maximum window to extract all the peer's data
		TCP_S->window_adv = HTONS(31744);	// 32768 - 1024
	} else {
		//if (!(tp->rcv_adv) && (tp->rcv_wnd < tp->rcv_mss)) {
		//	TCP_S->window_adv = 0;
		//} else {
		//	TCP_S->window_adv = htons(tp->rcv_wnd);
			TCP_S->window_adv = HTONS(2048);
			tp->rcv_adv = tp->rcv_wnd;
		//}
	}
	//rprintf("[%d]", ntohs(TCP_S->window_adv));

	// should be refined for OOB data
	TCP_S->urgent_ptr = 0;

	tp->t_state = next_state;

	packet_len += data_len;
	IP_S->time_to_live = 0;
	IP_S->checksum = htons(packet_len);
	TCP_S->checksum = 0;
	TCP_S->checksum = checksum(&(IP_S->time_to_live), PSEUDO_HLEN + packet_len);
	statistics.tcpOutSegs++;

	if (data_len > 0) {		// no SYN, FIN set
		//rprintf("[%ld(%d)]", tp->snd_nxt, data_len);
		//rprintf("[%d]", tp->round_trip);
		tmr->tick = ticknow + tp->round_trip;
		tmr->seq = tp->snd_nxt + data_len;	// highest Seq No -1
	}
	//rprintf("%02X]", TCP_S->flags);
	if (snd_ip(IP_S, packet_len) < 0) {
		//rprintf("[!! SEND ERR]");
		if (tmr)
			free_timer(tmr);
		if (data_len) {
			set_status(tp, TCB_TCP_DATA);
		}
		ret = -1;
	} else {
		if (data_len > 0) {		// no SYN, FIN set
			tp->snd_wnd -= data_len;
			tp->snd_nxt += data_len;
			if (tp->ts) {
				(tp->te)->next = tmr;
			} else {
				tp->ts = tmr;
			}
			tp->te = tmr;
			tp->nexttx = tbp;
			if ((tbp == tp->txe) && ((tbp->r_count + tbp->s_seq) == tp->snd_nxt)) {
				// Last Data in Buffer
				clr_status(tp, TCB_TCP_DATA);
			}
		} else if (seg_type & (TH_SYN | TH_FIN)) {
			tp->snd_nxt++;
			tp->syn_fin_timer = ROUND_TRIP_TIME(tp);
		}
		statistics.TCP_snd_bytes += data_len;
		// if still have data to send and window available, loop
		ret = data_len;
	}
	//print_packet(IP_S, packet_len + IP_PLEN);
leave:	cur_send = NULL;
	return(ret);
}

// Be sure if seg_type has TH_SYN, TH_ACK, cur_tp should point the current tcb
// for received packet.
//
static
int
quick_reply(int seg_type, int next_state)
{
	int packet_len;
	unsigned short tmp;

	// Swap Ethernet Address
	copy_hw_addr(ETH_P->destination, ETH_P->source);
	copy_hw_addr(ETH_P->source, ETH_STUB(tcb)->source);

	IP_P->destination = IP_P->source;		// Swap IP Address
	IP_P->source = tcpipcfg.c_myip;

	tmp = TCP_P->source;			// Swap Port Number
	TCP_P->source = TCP_P->destination;
	TCP_P->destination = tmp;

	if (seg_type & TH_SYN) {
		packet_len = TCPO_HLEN;	  // SYN Packet includes MSS Option
		set_rcv_mss_opt(TCP_P, cur_tp);
		TCP_P->sequence = htonl(cur_tp->snd_nxt);
		//rprintf("[SYN SEQ=%ld]", cur_tp->snd_nxt);
		cur_tp->snd_nxt++;
	} else {
		packet_len = TCP_HLEN;
		TCP_P->offset = 5;
		TCP_P->sequence = htonl(cur_ack);
	}

	// Should Be tuned later for Excessive Traffic

	if (seg_type & TH_ACK) {
		TCP_P->acknowledge = htonl(cur_tp->rcv_nxt);
		if (!(cur_tp->rcv_adv) && (cur_tp->rcv_wnd < cur_tp->rcv_mss)) {
			TCP_P->window_adv = 0;
		} else {
			TCP_P->window_adv = htons(cur_tp->rcv_wnd);
			cur_tp->rcv_adv = cur_tp->rcv_wnd;
		}
	} else {
		TCP_S->window_adv = HTONS(31744);	// 32768 - 1024
		TCP_P->acknowledge = htonl(cur_seq + !((TCP_P->flags & (TH_FIN | TH_SYN)) == 0));
	}

	TCP_P->flags = seg_type;
	if (seg_type & TH_RST) {
		TCP_P->flags |= TH_ACK;
		statistics.tcpOutRsts++;
	}

	TCP_P->urgent_ptr = 0;
	if (next_state >= 0)
		cur_tp->t_state = next_state;

	TCP_P->xxxxxx = 0;
	TCP_P->window_adv = HTONS(2048);	// Window Size = 512

	IP_P->time_to_live = 0;			// Pseudo Header
	IP_P->checksum = htons(packet_len);

	TCP_P->checksum = 0;
	TCP_P->checksum = checksum(&(IP_P->time_to_live), PSEUDO_HLEN + packet_len);
	statistics.tcpOutSegs++;
	//rprintf("[S_R]");
	snd_ip(IP_P, packet_len);
	//print_packet(TCP_P, 0);
	return(0);
}

static
void
rd_snd_mss()
{
	if (SYNed && (TCP_P->offset > 5) && (TCP_P->option_code == TCPOPT_MAXSEG)) {
		switch (TCP_P->option_len - 2) {
			case  sizeof(char) :
				cur_tp->snd_mss = *(char *)&(TCP_P->max_segment);
				break;
			case  sizeof(short) :
				cur_tp->snd_mss = ntohs(TCP_P->max_segment);
				break;
			case  sizeof(long) :
				cur_tp->snd_mss = ntohl(*(unsigned long *)&(TCP_P->max_segment));
				break;
		}
		if (cur_tp->snd_mss > (MTU_SIZE - (TCP_HLEN + IP_HLEN)))
			cur_tp->snd_mss = MTU_SIZE - (TCP_HLEN + IP_HLEN);
		if (!samenet(IP_STUB(cur_tp)->destination) && (cur_tp->snd_mss > TCP_MSS))
			cur_tp->snd_mss = TCP_MSS;
	}
}

void
rcv_tcp()
{
	int i, sd, newsd;
	struct TCB *tpp;

	statistics.tcpInSegs++;

	// Setup Pseudo Header
	IP_P->time_to_live = 0;
	IP_P->total_len = ntohs(IP_P->total_len) - IP_HLEN;
	IP_P->checksum = htons(IP_P->total_len);

	if (checksum(&(IP_P->time_to_live), PSEUDO_HLEN + IP_P->total_len)) {
		//rprintf("[TCP CHECKSUM %d]", PSEUDO_HLEN + IP_P->total_len);
		goto in_error;
	}

	tcp_hdr_len = (TCP_P->offset << 2);	// TCP Header Length
	tcp_data_len = IP_P->total_len - tcp_hdr_len;	// Data length, if ANY
	//print_packet(TCP_P, IP_P->total_len + IP_PLEN);
	if (tcp_data_len < 0) {
		//rprintf("[TCP SIZE %d]", PSEUDO_HLEN + IP_P->total_len);
in_error:	statistics.tcpInErrs++;
		goto leave;
	}

	SYNed = TCP_P->flags & TH_SYN;		// For performance
	RSTed = TCP_P->flags & TH_RST;

	cur_ack = ntohl(TCP_P->acknowledge);
	cur_seq = ntohl(TCP_P->sequence);
	//rprintf("[T_R S:%ld A:%ld]", cur_seq, cur_ack);

	for (cur_tp = tcb, sd = 0; sd < num_socket ; sd++, cur_tp++) {
		if ((IP_STUB(cur_tp)->transport != IPPROTO_TCP) ||
		    (cur_tp->t_state == TCPS_CLOSED) ||
		    (cur_tp->local_port != TCP_P->destination))
			continue;
		if (((cur_tp->t_state == TCPS_LISTEN) && SYNed &&
		     (!(cur_tp->local_port) || (cur_tp->local_port == TCP_P->destination))) ||
		    ((cur_tp->remote_port == TCP_P->source) &&
		     (IP_STUB(cur_tp)->destination == IP_P->source))) {
			//rprintf("[TCP PACKET Cached by %d]", sd);
			goto tcp_catch;
		}
	}
	// This is TCPS_CLOSED State
	//rprintf("{DROP}");
	statistics.TCP_dropped++;
	if (!RSTed) {
tcp_proc_error:
		quick_reply(TH_RST, -1);
	}
	goto leave;

tcp_catch:

	send_ACK = 0;
	TCP_P->window_adv = ntohs(TCP_P->window_adv);
	//rprintf("<%ld(%d)>", cur_ack, TCP_P->window_adv);
	//rprintf("[T_R:%d,%d, w:%d]", sd, cur_tp->t_state, TCP_P->window_adv);
	//if (tcp_data_len) rprintf("[R%d-%d]", sd, tcp_data_len);
	my_want = cur_tp->rcv_nxt;
	// FINed is check in copy_data()
	ACKed = TCP_P->flags & TH_ACK;		// ACK flag set
	FACKed = ACKed && (cur_ack == cur_tp->snd_nxt);	// fully acked

	switch (cur_tp->t_state) {
	   case TCPS_LISTEN :
		//rprintf("{LISTEN}");
		if (RSTed)
			break;
		if (ACKed || !(SYNed) || (cur_tp->logcount <= 0) ||
		    ((newsd = _socket(IPPROTO_TCP)) < 0)) {
			//rprintf("[ERR:NEW SD = %d]", newsd);
			goto tcp_proc_error;
		}
		//rprintf("[NEW SD = %d]", newsd);
		for (i = 0; i < SOMAXCONN; i++) {	// Find Empty Slot
			if (cur_tp->backlog[i] == -1)
				break;
		}
		cur_tp->backlog[i] = newsd;
		cur_tp->logcount--;
		cur_tp = tcb + newsd;
		cur_tp->backlog[0] = sd;	// Save Parent Socket

		IP_STUB(cur_tp)->transport = IPPROTO_TCP;
		copy_hw_addr(ETH_STUB(cur_tp)->destination, ETH_P->source);
		set_status(cur_tp, TCB_ARP_ED | TCB_TCP_WACC);
		IP_STUB(cur_tp)->destination = IP_P->source;
		cur_tp->remote_port = TCP_P->source;
		cur_tp->local_port = TCP_P->destination;

		cur_tp->t_state = TCPS_LISTEN;
		init_s_iss(cur_tp);
		cur_tp->rcv_nxt = cur_seq + 1;
		rd_snd_mss();
		if (quick_reply(TH_SYN | TH_ACK, TCPS_SYN_RECVD) < 0) {
			cur_tp->t_state = TCPS_SYN_RECVD;
			set_status(cur_tp, TCB_TCP_SYN | TCB_TCP_ACK);
		}
		break;
	   case TCPS_SYN_SENT :		/* Active Connection */
		//rprintf("{SYN_SENT}");
		if (RSTed) {	// Connection Refused
con_refused:		//rprintf("[---REFUSED---]");
			if (FACKed) {
				statistics.tcpAttemptFails++;
				clean_it(cur_tp, ECONNREFUSED);
			}
			break;
		}
		if (SYNed) {
			cur_tp->snd_wnd = TCP_P->window_adv;
			//rprintf("[SND_WND = %d]", TCP_P->window_adv);
			cur_tp->rcv_nxt = cur_seq + 1;
			if (FACKed) {
				rd_snd_mss();
				check_numbers();
				copy_data();
				//rprintf("[---TCP Connected---]");
				statistics.tcpActiveOpens++;
				if (quick_reply(TH_ACK, TCPS_ESTABLISHED) < 0) {
					cur_tp->t_state = TCPS_ESTABLISHED;
					set_status(cur_tp, TCB_TCP_ACK);
				}
				event_report(cur_tp, IEVENT_CONN_DONE, 0);
			} else {
				cur_tp->t_state = TCPS_SYN_RECVD;
			}
			break;
		}

		// If remote host's state is FIN_WAIT_2
		// he returns ACK only, it will be Error Condition
		// so We Reset the Connection, and Timer will resend SYN

		if (ACKed /* && (cur_ack != cur_tp->snd_nxt)*/ ) {
			goto tcp_proc_error;	// False Ack, RESET
		}
		break;

	   case TCPS_SYN_RECVD :
		//rprintf("{SYN_RECVD}");
		if (conn_check()) {
			//rprintf("RST, SYN received\n");
			if (cur_tp->status & TCB_TCP_WACC) {	// Remove from
				IP_STUB(cur_tp)->transport = 0;		// Listen QUEUE
				tpp = tcb + cur_tp->backlog[0]; // Parent
				for (i = 0; i < SOMAXCONN; i++) {
					if (tpp->backlog[i] == sd) {
						tpp->backlog[sd] = -1;
						tpp->logcount++;
					}
				}
			} else {
				goto con_refused;
			}
			break;
		}
		if (!FACKed || (my_want != cur_seq)) {
			//rprintf("Passive Connection Acknowledge Failure ");
			break;
		}
		//rprintf("[-- Passive OPEN --%ld]", cur_ack);
		statistics.tcpPassiveOpens++;
		rd_snd_mss();
		if (cur_tp->backlog[0] >= 0) {
			tpp = tcb + cur_tp->backlog[0];	// Parent Socket
			event_report(tpp, IEVENT_ACCEPT, tpp->logcount);
		}
		cur_tp->t_state = TCPS_ESTABLISHED;
		// Fall Thru if any data or FIN
	   case TCPS_ESTABLISHED :
		//rprintf("{ESTABILSHED}");
		if (cur_tp->option & SO_KEEPALIVE)
			cur_tp->kpalive_timer = KEEPALIVE_TIME;
	   case TCPS_CLOSE_WAIT :	// Do nothing
		if (conn_check())
			break;
		check_numbers();
		copy_data();
		if ((cur_tp->t_state == TCPS_ESTABLISHED) && FINed) {
			if (!data_avail(cur_tp)) {
				event_report(cur_tp, IEVENT_CLOSE_WAIT, 0);
			}
			//rprintf("[CLOSE_WAIT %d:%04X:%d]", cur_tp - tcb, cur_tp->rxb, (cur_tp->rxb)->count);
			if (quick_reply(TH_ACK, TCPS_CLOSE_WAIT) < 0) {
				cur_tp->t_state = TCPS_CLOSE_WAIT;
				set_status(cur_tp, TCB_TCP_ACK);
			}
			cur_tp->cleanup_timer = CLOSE_WAIT_TIME * 2;
		} else if (send_ACK /* || (driver_type == SLIP_DRIVER) */) {
			if (quick_reply(TH_ACK, cur_tp->t_state) < 0) {
				set_status(cur_tp, TCB_TCP_ACK);
			}
		} else if ((tcp_data_len > 0) && !(cur_tp->del_ack_timer))
			cur_tp->del_ack_timer = DEL_ACK_TIME;
		// else, Delayed ACK must be Running Now
		break;
	   case TCPS_CLOSING :
	   case TCPS_LAST_ACK :
		if (conn_check())
			break;
		if (FACKed) {
clear_tcb:		clean_it(cur_tp, 0);
			IP_STUB(cur_tp)->transport = 0;
		}
		break;
	   case TCPS_FIN_WAIT_1 :
		if (conn_check())
			break;
		check_numbers();
		copy_data();
		//rprintf("[FIN ACK = %ld]", cur_ack);
		if (FINed) {
			if (FACKed) {
				goto timewait;
			} else {
				if (quick_reply(TH_ACK, TCPS_CLOSING) < 0) {
					cur_tp->t_state = TCPS_CLOSING;
					set_status(cur_tp, TCB_TCP_ACK);
				}
				cur_tp->cleanup_timer= CLOSE_WAIT_TIME;
			}
		} else if (FACKed) {
			if (quick_reply(TH_ACK, TCPS_FIN_WAIT_2) < 0) {
				cur_tp->t_state = TCPS_FIN_WAIT_2;
			}
			cur_tp->cleanup_timer = CLOSE_WAIT_TIME;
		} else {
			if (quick_reply(TH_ACK, cur_tp->t_state) < 0) {
				set_status(cur_tp, TCB_TCP_ACK);
			}
		}
		break;
	   case TCPS_FIN_WAIT_2 :
		if (conn_check())
			break;
		check_numbers();
		copy_data();
		if (FINed) {
timewait:		quick_reply(TH_ACK, TCPS_CLOSED);
			goto clear_tcb;
		}
#ifdef TESTRX
		if (send_ACK) {
			if (quick_reply(TH_ACK, cur_tp->t_state) < 0) {
				set_status(cur_tp, TCB_TCP_ACK);
			}
		} else if ((tcp_data_len > 0) && !(cur_tp->del_ack_timer))
			cur_tp->del_ack_timer = DEL_ACK_TIME;
#else
		if (tcp_data_len > 0) {
			clean_it(cur_tp, 0);
			IP_STUB(cur_tp)->transport = 0;
			goto tcp_proc_error;
		}
#endif
		break;
	   case TCPS_TIME_WAIT :
		// We dont support TIME_WAIT state
	   default:
		//rprintf("FATAL TCB %d STATE ERROR\n", cur_tp - tcb);
		IP_STUB(cur_tp)->transport = 0;
		clean_it(cur_tp, ECONNABORTED);
		break;
	}
leave:
	//rprintf("[STATE = %d]", cur_tp->t_state);
	cur_tp = NULL;
}

void
init_s_iss(struct TCB *tp)
{
	// Initialize Seq Number for New Connection
	random_iss += 904;
	tp->snd_una = tp->snd_nxt = random_iss;
}

static
void
copy_data()
{
	struct BUFFER *tbp;
	unsigned data_copied, start_ptr, tmp;
	char *data_start;

	if ((tcp_data_len == 0) || 			// No data or no Buffer
	    (cur_tp->t_state > TCPS_CLOSE_WAIT)) {	// Not in RX_state
		//rprintf("[D %d]", tcp_data_len);
		data_copied = tcp_data_len;	// Return As if All Data Copied
		goto just_ack;
	}

	tbp = cur_tp->rxe;
	if (tbp == NULL) {
		if (!(tbp = cur_tp->rx = cur_tp->rxe = get_buffer(cur_tp, tcp_data_len, RX_BUFFER)))
			return;
		cur_tp->rx_len = 1;
	}
	data_copied = 0;
	data_start = packet_buffer + IP_PLEN + tcp_hdr_len + data_offset;
	while (data_copied < tcp_data_len) {
		// To avoid wrapping copy, allocate new buffer.
		// if ((BUFFER_SIZE(tbp) - tbp->r_count) < (tcp_data_len - data_copied)) {
		if (BUFFER_SIZE(tbp) == tbp->r_count) {
			// Allocate Buffer, But avoid monopoly
			if (cur_tp->rx_len >= MAX_RX_ALLOCATE)
				break;
			if (!(tbp->next = get_buffer(cur_tp, (tcp_data_len - data_copied), RX_BUFFER))) {
				break;
			} else {
				cur_tp->rx_len++;
				cur_tp->rxe = tbp->next;
				tbp = tbp->next;
			}
		}
copy_it:	start_ptr = (tbp->r_start + tbp->r_count) % BUFFER_SIZE(tbp);
		tmp = min(BUFFER_SIZE(tbp) - tbp->r_count, tcp_data_len - data_copied);
		if ((start_ptr + tmp) > BUFFER_SIZE(tbp)) { 	// wrap
			tmp = BUFFER_SIZE(tbp) - start_ptr;
		}
		copy_mem(_DS, (unsigned)(tbp->data) + start_ptr,
			 _DS, (unsigned)data_start, tmp);
		tbp->r_count += tmp;
		data_copied += tmp;
		data_start += tmp;
	}
	if (data_copied) {
		event_report(cur_tp, IEVENT_READ, data_avail(cur_tp));
		cur_tp->rcv_wnd -= data_copied;
		statistics.TCP_rcv_bytes += data_copied;
	}
just_ack:
	cur_tp->rcv_nxt += data_copied;

	// FIN is assrted, when Sequence Number is OK (send_ACK == 0)
	// and current data is fully copied in.

	if ((send_ACK == 0) && (TCP_P->flags & TH_FIN) && (data_copied == tcp_data_len)) { // All data read
		//rprintf("[FIN RECEIVED]");
		FINed = 1;
		cur_tp->rcv_nxt++;
	} else
		FINed = 0;
}

static
int
conn_check()
{
	if ((cur_seq - my_want) < 0) {		// Out of Window
		send_ACK++;
		return(0);
	}
	if (RSTed) {
		//rprintf("[CHECK_RSTed]");
		//clean_it(cur_tp, ECONNRESET);
		//IP_STUB(cur_tp)->transport = 0;
		goto clear_connection;
	}
	if (SYNed) {
		quick_reply(TH_RST, -1);
		//rprintf("[CHECK_SYNed]");
		if (cur_tp->t_state > TCPS_CLOSE_WAIT) {
			clean_it(cur_tp, ECONNRESET);
			IP_STUB(cur_tp)->transport = 0;
		}
clear_connection:
		statistics.tcpEstabResets++;
		return(1);
	}
	return(0);
}

static
void
check_numbers()
{
	// Check Packet Misses or Duplicated Packet
	// long number arithmetic resolve wrapped sequence number

	//rprintf("[RD:%d]", tcp_data_len);
	//rprintf("W:%ld,S:%ld", my_want, cur_seq);

	if ((tcp_data_len > 0) && (cur_seq != my_want)) {
		data_offset = my_want - cur_seq;
		if ((data_offset > 0) && (data_offset < tcp_data_len)) {
			tcp_data_len -= data_offset;	// Duplicated
		} else {
			//rprintf("XX");
			send_ACK++;
			tcp_data_len = 0;		// Missed
		}
	} else
		data_offset = 0;

	//rprintf("(%d)", tcp_data_len);
	if (ACKed) {
		int old_snd_wnd = cur_tp->snd_wnd;		// Peer's Old Window

		//rprintf("[%d]", TCP_P->window_adv);
		check_buffers();
		check_timers();
		cur_tp->snd_wnd = TCP_P->window_adv;	// Peer's Window Advertizement
		if (cur_tp->snd_wnd < (cur_tp->snd_nxt - cur_tp->snd_una)) {
			cur_tp->snd_wnd = 0;
		} else {
			cur_tp->snd_wnd -= (cur_tp->snd_nxt - cur_tp->snd_una);
		}
		if (cur_tp->snd_wnd == 0) {
			if (old_snd_wnd) {	// Start PERSIST to poll RX window
				//rprintf("[START PERSIST]");
				cur_tp->persist_timer = ROUND_TRIP_TIME(cur_tp);
			}
		} else {
			//rprintf("[R_END PERSIST]");
			clr_status(cur_tp, TCB_TCP_POLL);
			cur_tp->persist_timer = 0;
		}
	}
}

static
void
check_buffers()
{
	struct BUFFER *tbp;
	int freed = 0, tmp;

	//rprintf("[CHK_BUF]");
	tbp = cur_tp->tx;
	if (tbp == NULL)	// No Retransmit Buffer in this channel
		goto leave;	// for SYN_RCVD

	while ((tbp != NULL) && ((cur_ack - tbp->s_seq) > 0)) {
		//rprintf("[CB]");
		tmp = (cur_ack - tbp->s_seq);
		if (tmp >= tbp->r_count) {	// Fully ACKed
			struct BUFFER *tbuf;
			if (!IS_DEF_BUF(tbp)) {
				freed++;
			}
			tbuf = tbp->next;
			if (tbuf) {
				cur_tp->tx = tbuf;
				cur_tp->tx_len--;
				free_buffer(tbp);
				tbp = tbuf;
				continue;
			} else {		// It's last Retx Buffer
				free_buffer(tbp);
				cur_tp->nexttx = cur_tp->tx = cur_tp->txe = NULL;
				cur_tp->tx_len = 0;
				freed = DEF_BUFSIZE;
				goto report_it;
			}
		} else if (tmp > 0) {			// Partially ACKed
			//rprintf("[PB]");
			tbp->s_seq = cur_ack;
partial_ack:		tbp->r_start = (tbp->r_start + tmp) % BUFFER_SIZE(tbp);
			tbp->r_count -= tmp;
			freed = BUFFER_SIZE(tbp) - tbp->r_count;
report_it:		freed += remained_bufsize;
			event_report(cur_tp, IEVENT_WRITE, freed);
			freed = 0;
			break;
		}
	}
leave:	if ((cur_ack - cur_tp->snd_nxt) > 0) {
		// This can be happen with time out condition
		cur_ack = cur_tp->snd_nxt;
	} else if ((cur_ack - cur_tp->snd_una) > 0)
		cur_tp->snd_una = cur_ack;
	if (freed)
		event_report(cur_tp, IEVENT_WRITE, remained_bufsize);
}

static
void
check_timers()
{
	struct TIMER *tptr;
	unsigned long expected_time = 0;

	tptr = cur_tp->ts;
	while (tptr && ((cur_ack - tptr->seq) >= 0)) {
		cur_tp->ts = tptr->next;
		expected_time = tptr->tick;
		free_timer(tptr);
		tptr = cur_tp->ts;
	}
	if (!tptr)
		cur_tp->te = NULL;
	if (expected_time) {
		expected_time -= ticknow;
		cur_tp->round_trip -= (expected_time / 4);
		if (cur_tp->round_trip < MIN_RETX_TIME)
			cur_tp->round_trip = MIN_RETX_TIME;
		//rprintf("[:%d]", cur_tp->round_trip);
	}
}

void
check_tcp_timer(struct TCB *tp)
{
	//rprintf("[@");
	// SYN Retransmission
	if (((tp->t_state == TCPS_SYN_SENT) ||
	     (tp->t_state == TCPS_SYN_RECVD)  ) && tp->syn_fin_timer) {
		//rprintf("*");
		if (!--(tp->syn_fin_timer)) {
			if (tp->timeout_count > SYN_RETRY) {
				statistics.tcpAttemptFails++;
				event_report(tp, IEVENT_CONN_FAILED, 0);
				clean_it(tp, ETIMEDOUT);
			} else {
				set_status(tp, TCB_TCP_SYN);
			}
		}
	}

	if ((tp->option & SO_KEEPALIVE) && tp->kpalive_timer) {
		if (!--(tp->kpalive_timer))
			set_status(tp, TCB_TCP_KEEP);
	}

	// Check Keep Alive, Delayed ACK, Poll Remote Buffer Size
	if (tp->t_state == TCPS_ESTABLISHED) {
		if ((tp->option & SO_KEEPALIVE) && tp->kpalive_timer) {
			if (!--(tp->kpalive_timer))
				set_status(tp, TCB_TCP_KEEP);
		}
		if (tp->del_ack_timer) {
			if (!--(tp->del_ack_timer))
				set_status(tp, TCB_TCP_ACK);
		}
		if (tp->persist_timer) {
			if (!--(tp->persist_timer))
				set_status(tp, TCB_TCP_POLL);
		}
	}

	if ((tp->t_state == TCPS_ESTABLISHED) || (tp->t_state == TCPS_CLOSE_WAIT)) {
		struct TIMER *ttp, *tip;
		int tmp;

		ttp = tp->ts;
		//if (ttp) rprintf("x");
		if (ttp && ((ttp->tick - ticknow) <= 0)) {
			tp->ts = tp->te = NULL;
			tmp = (tp->snd_nxt - tp->snd_una);
			if (tmp) {
				set_status(tp, TCB_TCP_DATA);
				tp->snd_wnd += tmp;
			}
			tp->snd_nxt = tp->snd_una;
			if (tmp)
				statistics.TCP_snd_bytes -= tmp;
			//rprintf("[TCP_RETX_OUT]");
			if (tp->snd_wnd) {		// Stop Persist Timer
				tp->persist_timer = 0;	// his window is enlarged
			}
			// How to calc tcpRetransSegs ?, we simplify
			statistics.tcpRetransSegs += (tmp / tp->snd_mss);
			tp->round_trip *= 2;
			if (tp->round_trip > MAX_RETX_TIME)
				tp->round_trip = MAX_RETX_TIME;
			while (ttp != NULL) {
				tip = ttp->next;
				free_timer(ttp);
				ttp = tip;
			}
			tp->nexttx = tp->tx;
		}
	}

	// FIN Retransmission
	if (((tp->t_state == TCPS_FIN_WAIT_1) ||
	     (tp->t_state == TCPS_LAST_ACK)  ) && tp->syn_fin_timer) {
		if (!--(tp->syn_fin_timer))
			set_status(tp, TCB_TCP_FIN);
	}

	// CLean-up Connection
	if ((tp->t_state >= TCPS_CLOSE_WAIT) && tp->cleanup_timer) {
		if (!--(tp->cleanup_timer))
			set_status(tp, TCB_TCP_RST);
	}

	if (cur_tp != tp)
		check_tcp_send(tp);
	//rprintf("@]");
}

void
check_tcp_send(struct TCB *tp)
{
	if (tp->status & TCB_TCP_RST) {
		snd_tcp(tp, TH_RST, TCPS_CLOSED, 0);
		clean_it(tp, 0);
		IP_STUB(tp)->transport = 0;
		return;
	}

	// SYN can be sent at anytime
	if (((tp->t_state == TCPS_SYN_SENT) || (tp->t_state == TCPS_CLOSED)) &&
	    (tp->status & TCB_TCP_SYN) && (tp->status & TCB_ARP_ED)) {
		//rprintf("[SEND SYN %d]", tp->timeout_count);
		snd_tcp(tp, TH_SYN, TCPS_SYN_SENT, 0);
		tp->syn_fin_timer = ROUND_TRIP_TIME(tp) * 2 * ++(tp->timeout_count);
		return;
	}

	if (tp->status & (TCB_TCP_POLL | TCB_TCP_KEEP | TCB_TCP_ACK)) {
		snd_tcp(tp, TH_ACK, tp->t_state, 0);
		return;
	}

	//if ((tp->snd_una == tp->snd_nxt) && (tp->tx)->r_count) {
	//	if (!(tp->status & TCB_TCP_DATA))
	//		rprintf("[WHY THIS?]");
	//	set_status(tp, TCB_TCP_DATA);
	//}

	while ((tp->status & TCB_TCP_DATA) && tp->snd_wnd) {
		if (snd_tcp(tp, TH_ACK, tp->t_state, 1) <= 0) {
			return;
		}
	}

	// When no data and all sent data has been acked, we can send FIN
	if (!(tp->status & TCB_TCP_DATA) && (tp->status & TCB_TCP_FIN) &&
	    (tp->snd_una == tp->snd_nxt) ) {
		if (tp->tx && ((tp->tx)->r_count)) {
			//rprintf("[ACK FAILURE]");
			set_status(tp, TCB_TCP_DATA);
			return;
		}
		//rprintf("[SEND FIN]");
		if (snd_tcp(tp, TH_FIN,
			((tp->t_state == TCPS_ESTABLISHED) ?
				TCPS_FIN_WAIT_1:TCPS_LAST_ACK), 0) < 0) {
			// Network Error, send again later
			set_status(tp, TCB_TCP_FIN);
		} else
			tp->syn_fin_timer = ROUND_TRIP_TIME(tp);
	}
}

static
void
set_rcv_mss_opt(struct RFC793 *TCP_PKT, struct TCB *tp)
{
	unsigned short tmp;

	tmp = MTU_SIZE - (TCP_HLEN + IP_HLEN);
	if (!samenet(IP_STUB(tp)->destination))
		tmp = min(tmp, TCP_MSS);
	tp->rcv_mss = tmp;
	TCP_PKT->offset = 6;
	TCP_PKT->option_code = TCPOPT_MAXSEG;
	TCP_PKT->option_len = 0x04;
	TCP_PKT->max_segment = htons(tmp);
}
