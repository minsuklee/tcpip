/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>

//#define MK_FP(x, y) ((unsigned long)x * 10000L + (unsigned long)y)

void
main(argc, argv)
int argc;
char *argv[];
{
	struct POINTERS pointer;
	union  REGS _imige_reg;
	struct TCB far *tp;
	struct BUFFER far *tbp;
	struct TIMER  far *ttp;
	unsigned segment;
	int i, sd, count;

	//clrscr();

	if (_init_imigetcp(0) < 0)
		exit(1);

	_imige_reg.h.ah = IMG_GETPOINTER;
	_imige_reg.x.bx = FP_SEG(&pointer);
	_imige_reg.x.cx = FP_OFF(&pointer);
	int86(_imige_vec, &_imige_reg, &_imige_reg);

	if (argc != 2) {
		printf("Usage: stat socket\n");
		goto pointer;
	}
	if (argv[1][0] == '*') {
		sd = -1;
	} else {
		sd = atoi(argv[1]);
	}
	if (sd >= _num_sock) {
		printf("Available Number of Socket is %d.\n", _num_sock);
		exit(1);
	}

	if (sd < 0) {
		sd = 0;
		for (tbp = pointer.buffer, i = 0; i < pointer.no_buffer; i++, tbp++) {
			if (tbp->buffer_type != FREE_BUFFER) {
				sd++;
				printf("%2d : %04X : ", i, FP_OFF(tbp));
				switch (tbp->buffer_type) {
					case FREE_BUFFER : printf("F"); sd++; break;
					case RX_BUFFER : printf("R"); break;
					case TX_BUFFER : printf("T"); break;
					default: printf("X"); break;
				}
				printf(" ->%04X, Count=%d, Start=%d, Seq=%ld\n",
					tbp->next, tbp->r_count, tbp->r_start, tbp->s_seq);
			} else if (tbp->buffer_type & LOCK_BUFFER) {
				printf("%2d : %04X : L\n", i, tbp->buffer_type);
			}
		}
		printf("\n%d Buffers are in use out of %d buffers\n", sd, pointer.no_buffer);
		exit(0);
	}
	tp = pointer.tcb + sd;
	segment = FP_SEG(tp);
	//printf("[%04X:%04X] ", segment, (unsigned)(tp));
	
	switch (tp->IP_HDR.transport) {
		case IPPROTO_TCP  :	printf("TCP"); break;
		case IPPROTO_UDP  :	printf("UDP"); break;
		case IPPROTO_ICMP :	printf("RAW"); break;
		case 0 :		printf("---"); break;
		default :
			printf("%2X:", tp->IP_HDR.transport);
			break;
	}
	printf(" %04X ", tp->status);
	// TCP Flags
	switch (tp->status & TCB_TCP_SYN) {
		case TCB_TCP_SYN	: printf("W"); break;
		case 0			: printf("-"); break;
	}
	// ARP Flags
	switch (tp->status & (TCB_ARP_ING | TCB_ARP_ED)) {
		case TCB_ARP_ING		: printf("G"); break;
		case TCB_ARP_ED			: printf("A"); break;
		case (TCB_ARP_ING | TCB_ARP_ED)	: printf("E"); break;
		case 0				: printf("-"); break;
	}
	if (tp->status & TCB_TCP_DATA)
		printf("D");
	else
		printf("-");
	if (tp->status & TCB_TCP_FIN)
		printf("F");
	else
		printf("-");
	printf(" %04d-%08lX:%05d ",
		bswap(tp->local_port), tp->IP_HDR.destination, bswap(tp->remote_port));
	switch (tp->t_state) {
		case TCPS_CLOSED :	printf("TCLSD"); break;
		case TCPS_LISTEN :	printf("TLIST"); break;
		case TCPS_SYN_SENT :	printf("SYNSE"); break;
		case TCPS_SYN_RECVD :	printf("SYNRC"); break;
		case TCPS_ESTABLISHED :	printf("ESTAB"); break;
		case TCPS_CLOSE_WAIT :	printf("CL_WA"); break;
		case TCPS_FIN_WAIT_1 :	printf("FIWT1"); break;
		case TCPS_CLOSING :	printf("CLSNG"); break;
		case TCPS_LAST_ACK :	printf("LSTAC"); break;
		case TCPS_FIN_WAIT_2 :	printf("FIWT2"); break;
		case TCPS_TIME_WAIT :	printf("TIMWT"); break;
		default :		printf("?????"); break;
	}
	printf(" ERR=%d, OPT:%04X, ACC:%d", tp->last_error, tp->option, tp->logcount);
	if (tp->logcount) {
		printf("[");
		for (i = 0; i < SOMAXCONN; i++) {
			if (tp->backlog[i] >= 0)
				printf("%d ", tp->backlog[i]);
		}
		printf("]");
	}
	printf("\n");

	printf("  SND: una:%ld, nxt:%ld, wnd:%u, mss:%d\n",
		tp->snd_una, tp->snd_nxt, tp->snd_wnd, tp->snd_mss);
	printf("  RCV: nxt:%ld, wnd:%u, adv:%u mss:%d\n",
		tp->rcv_nxt, tp->rcv_wnd, tp->rcv_adv, tp->rcv_mss);
	printf("  TMR: A:%d K:%d RC:%d S:%d P:%d\n",
		tp->del_ack_timer, tp->kpalive_timer, tp->cleanup_timer,
		tp->syn_fin_timer, tp->persist_timer);

	printf("  RX(%d): ", tp->rx_len);
	count = 0;
	tbp = (struct BUFFER far *)MK_FP(segment, (unsigned)(tp->rx));
	while (FP_OFF(tbp)) {
		count += tbp->r_count;
		printf("%04X(%d,%d) ", FP_OFF(tbp), tbp->r_count, tbp->r_start);
		tbp = (struct BUFFER far *)MK_FP(segment, (unsigned)(tbp->next));
	}
	printf(": rxe=%04X : %d Bytes\n", tp->rxe, count);

	printf("  TX(%d): ", tp->tx_len);
	count = 0;
	tbp = (struct BUFFER far *)MK_FP(segment, (unsigned)(tp->tx));
	while (FP_OFF(tbp)) {
		count += tbp->r_count;
		printf("%04X(S:%ld,%d,%d) ", FP_OFF(tbp), tbp->s_seq, tbp->r_count, tbp->r_start);
		tbp = (struct BUFFER far *)MK_FP(segment, (unsigned)(tbp->next));
	}
	printf(": txe=%04X N=%04X (%d bytes (%d una))\n", tp->txe, tp->nexttx, count,
		(tp->snd_nxt - tp->snd_una));

	printf("  TIMER: ");
	ttp = (struct TIMER far *)MK_FP(segment, (unsigned)(tp->ts));
	while (FP_OFF(ttp)) {
		printf("%04X(S:%ld,%d) ", FP_OFF(ttp), ttp->seq, ttp->tick);
		ttp = (struct TIMER far *)MK_FP(segment, (unsigned)(ttp->next));
	}
	printf(": te=%04X\n\n", tp->te);

pointer:
	sd = 0;
	for (tp = pointer.tcb, i = 0; i < _num_sock; i++, tp++) {
		if (!(tp->IP_HDR.transport))
			sd++;
	}
	printf("%d TCB(%d), ", sd, _num_sock);
	sd = 0;
	for (tbp = pointer.buffer, i = 0; i < pointer.no_buffer; i++, tbp++) {
		if (tbp->buffer_type == FREE_BUFFER)
			sd++;
	}
	printf("%d Buffer(%d), ", sd, pointer.no_buffer);
	sd = 0;
	for (ttp = pointer.timer, i = 0; i < pointer.no_timer; i++, ttp++) {
		if (ttp->tick == 0)
			sd++;
	}
	printf("%d TIMER(%d)s are Available.\n", sd, pointer.no_timer);
}
