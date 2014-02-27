/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>

struct snmp_object STAT;
struct TCPCONFIG config;

int clear_stat = 0;

void print_iaddr(unsigned long ipadr);

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
	
	int i, count;
	unsigned long addr;

	//clrscr();
	if ((argc > 1) && (argv[1][0] == '0'))
		clear_stat++;
	if (_read_config(&config) < 0)
		exit(1);
	printf("IP:"); print_iaddr(config.c_kernel.c_myip);
	printf(" (NETMASK: 0x%08lX), GW:", lswap(config.c_kernel.c_subnet));
	if ((addr = inet_addr(config.gate_way)) == 0xffffffffL) {
		struct hostent *hp;
		if (strlen(config.gate_way)) {
			if ((hp = gethostbyname(config.gate_way)) == NULL) {
				printf("Gateway '%s' unknown.\n", config.gate_way);
				addr = 0L;
			} else
				addr= *(unsigned long *)(hp->h_addr);
		} else
			addr = 0L;
	}
	config.c_kernel.c_defgw = addr;
	print_iaddr(config.c_kernel.c_defgw);
	printf("\n\n");

	_net_stat(&STAT, clear_stat);
	_imige_reg.h.ah = IMG_GETPOINTER;
	_imige_reg.x.bx = FP_SEG(&pointer);
	_imige_reg.x.cx = FP_OFF(&pointer);
	int86(_imige_vec, &_imige_reg, &_imige_reg);

//	printf("IMIGE/TCP found at 0x%02X-%02X, %d Socket available.\n",
//		_imige_vec, _imige_vec + 1, _num_sock);

	printf("TCP RPKT:%ld, ", STAT.tcpInSegs);
	printf("SPKT:%ld, ", STAT.tcpOutSegs);
	printf("RBYTE:%ld  ", STAT.TCP_rcv_bytes);
	printf("SBYTE:%ld, ", STAT.TCP_snd_bytes);
	printf("CERR:%ld, ", STAT.tcpInErrs);
	printf("\n    CFAIL:%ld, ", STAT.tcpAttemptFails);
	printf("AOPEN:%ld, ", STAT.tcpActiveOpens);
	printf("POPEN:%ld, ", STAT.tcpPassiveOpens);
	printf("RETX:%ld, ", STAT.tcpRetransSegs);
	printf("ORST:%ld, ", STAT.tcpOutRsts);
	printf("IRST:%ld, ", STAT.tcpEstabResets);
	printf("DROP:%ld\n", STAT.TCP_dropped);

	printf("UDP RPKT:%ld, ", STAT.udpInDatagrams);
	printf("SPKT:%ld, ", STAT.udpOutDatagrams);
	printf("RBYTE:%ld  ", STAT.UDP_rcv_bytes);
	printf("SBYTE:%ld, ", STAT.UDP_snd_bytes);
	printf("CERR:%ld, ", STAT.udpInErrors);
	printf("DROP:%ld\n", STAT.UDP_no_buffer + STAT.udpNoPorts);

	printf("ICMP RPKT:%ld, ", STAT.icmpInMsgs);
	printf("SPKT=%ld, ", STAT.icmpOutMsgs);
	printf("OECHO=%ld(", STAT.icmpOutEchos);
	printf("REP=%ld), ", STAT.icmpInEchoReps);
	printf("IECHO=%ld, ", STAT.icmpInEchos);
	printf("ERROR=%ld\n", STAT.icmpInErrors);

	printf("IP RPKT:%ld, ", STAT.ipInReceives);
	printf("SPKT=%ld, ", STAT.ipOutSends);
	printf("CERR=%ld, ", STAT.ipInHdrErrors);
	printf("DROP=%ld\n", STAT.ipInUnknownProtos);

	printf("ARP SREQ=%ld(", STAT.arpOutRequests);
	printf("REP=%ld), ", STAT.arpInReplys);
	printf("SREP=%ld\n", STAT.arpInRequests);

	printf("LAN RPKT=%ld, ", STAT.lanInPackets);
	printf("SPKT=%ld, ", STAT.lanOutPackets);
	printf("SERR=%ld, ", STAT.lanOutErrors);
	printf("DROP=%ld\n", STAT.lanInUnknownProtos);

	printf("\nTCB %04X:%04X(%d x %d), BUFFER %04X:%04X(%d), TIMER %04X:%04X(%d)\n",
		FP_SEG(pointer.tcb), FP_OFF(pointer.tcb), sizeof(struct TCB), pointer.num_socket,
		FP_SEG(pointer.buffer), FP_OFF(pointer.buffer), pointer.no_buffer,
		FP_SEG(pointer.timer), FP_OFF(pointer.timer), pointer.no_timer);

	count = 0;
	for (tp = pointer.tcb, i = 0; i < _num_sock; i++, tp++) {
		if (!(tp->IP_HDR.transport))
			count++;
	}
	printf("%d TCB, ", count);
	count = 0;
	for (tbp = pointer.buffer, i = 0; i < pointer.no_buffer; i++, tbp++) {
		if (tbp->buffer_type == FREE_BUFFER)
			count++;
	}
	printf("%d Buffer, ", count);
	count = 0;
	for (ttp = pointer.timer, i = 0; i < pointer.no_timer; i++, ttp++)
		if (ttp->tick == 0)
			count++;
	printf("%d TIMER   Available.\n\n", count);

	for (tp = pointer.tcb, i = 0; i < _num_sock; i++, tp++) {
		switch (tp->IP_HDR.transport) {
			case IPPROTO_TCP  :	printf("T"); break;
			case IPPROTO_UDP  :	printf("U"); break;
			case IPPROTO_ICMP :	printf("I"); break;
			case 0 :		printf("-"); break;
			default :
				printf("X%2X", tp->IP_HDR.transport);
				break;
		}
		printf(" ");
		// TCP Flags
		switch (tp->status & TCB_TCP_SYN) {
			case TCB_TCP_SYN	: printf("W"); break;
			case 0			: printf("-"); break;
		}
		// ICMP Flags
		switch (tp->status & (TCB_ECHO_ING | TCB_ECHO_ED)) {
			case TCB_ECHO_ING		  : printf("P"); break;
			case TCB_ECHO_ED		  : printf("A"); break;
			case (TCB_ECHO_ING | TCB_ECHO_ED) : printf("E"); break;
			case 0				  : printf("-"); break;
		}
		// ARP Flags
		switch (tp->status & (TCB_ARP_ING | TCB_ARP_ED)) {
			case TCB_ARP_ING		: printf("G"); break;
			case TCB_ARP_ED			: printf("A"); break;
			case (TCB_ARP_ING | TCB_ARP_ED)	: printf("E"); break;
			case 0				: printf("-"); break;
		}

		printf(" %04d-%08lX:%05d ",
			bswap(tp->local_port), tp->IP_HDR.destination, bswap(tp->remote_port));
		switch (tp->t_state) {
			case TCPS_CLOSED :	printf("CLOSED     "); break;
			case TCPS_LISTEN :	printf("LISTEN     "); break;
			case TCPS_SYN_SENT :	printf("SYN_SENT   "); break;
			case TCPS_SYN_RECVD :	printf("SYN_RCVD   "); break;
			case TCPS_ESTABLISHED :	printf("ESTABLISHED"); break;
			case TCPS_CLOSE_WAIT :	printf("CLOSE_WAIT "); break;
			case TCPS_FIN_WAIT_1 :	printf("FIN_WAIT-1 "); break;
			case TCPS_CLOSING :	printf("CLOSING    "); break;
			case TCPS_LAST_ACK :	printf("LAST_ACK   "); break;
			case TCPS_FIN_WAIT_2 :	printf("FIN_WAIT-2 "); break;
			case TCPS_TIME_WAIT :	printf("TIME_WAIT  "); break;
			default :		printf("   %3d     ", tp->t_state); break;
		}
		printf(" R:%d K:%d SF:%d C:%d",
			tp->round_trip, tp->kpalive_timer, tp->syn_fin_timer, tp->cleanup_timer);
		printf("\n");
		if (i == 10)
			getch();
	}
}

void
print_iaddr(unsigned long ipadr)
{
	unsigned char *iadr = (unsigned char *)(&ipadr);

	printf("%d.%d.%d.%d", iadr[0], iadr[1], iadr[2], iadr[3]);
}
