/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include <imigelib.h>
#include <dos.h>

unsigned char sping[2000];
unsigned char rping[2000];
struct ICMP_ECHO *s_echop = (struct ICMP_ECHO *)sping;
struct ICMP_ECHO *r_echop = (struct ICMP_ECHO *)rping;
void check_end(void);

unsigned char *sdata = sping + ICMP_PLEN;
unsigned char *rdata = rping + ICMP_PLEN;
struct sockaddr_in sin, rin;

unsigned short ping_checksum(char *basep, int len);
char *hostname;

long t_snd = 0, t_rcv = 0;
long min_time = 0x7fffffff, max_time = 0, t_time = 0;
int sd = -1;
int ctrlc(void);
char cpre = 0;

main(argc, argv)
int argc;
char *argv[];
{
	struct hostent *host;
	unsigned long addr;
	int icmp_size, ping_len, i;
	struct timeval time = {1L, 0L};
	int sequence = 0, sel_time;
	fd_set rfds;
	long old_time, new_time, tmp_time; // in milli sec order

	printf("Ping,  Version 1.00,\n");
	printf("(C) Copyright 1998, IMIGE Systems Inc.,  All Rights Reserved.\n");

	if( argc < 2 ) {
		printf("usages: ping hostname [len]\n");
		return(0);
	}

	if( argc > 2 ) {
		ping_len = atoi(argv[2]);
		if (ping_len < 64)
			ping_len = 64;
		if (ping_len > 512)
			ping_len = 512;
	} else
		ping_len = 100;

	i = _init_imigetcp(0);
	if (i != IMIGETCP_VER) {
		fprintf(stderr, "It's Not the TCP/IP Kernel.\n");
		exit(1);
	}

	ctrlbrk(ctrlc);
	hostname = argv[1];
	if ((addr = inet_addr(hostname)) == 0xFFFFFFFF) {
		host = gethostbyname(hostname);
		if (!host) {
			printf("%s: Unknown host\n", hostname);
			exit(1);
		}
		addr = *(unsigned long *)host->h_addr;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = addr;
	sin.sin_port = 0;


	for (i = 0; i < ping_len; i++)
		sdata[i] = i % 256;

	icmp_size = ping_len + ECHO_HLEN;	// + ECHO Header size 

	s_echop->icmp_pkt.packet_type = ECHO_REQUEST;
	s_echop->icmp_pkt.opcode = 0;
	s_echop->identification = *(unsigned short far *)MK_FP(0, 0x46C);
	s_echop->sequence = 0;

	printf("\nPing %s    ...  type any key to stop.\n\n",
		inet_ntoa(sin.sin_addr));


loop_here:

	if (sd >= 0)
		closesocket(sd);
	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("Socket");
		exit(1);
	}
	old_time = biostime(0,0L);
	s_echop->sequence = sequence;

	s_echop->icmp_pkt.checksum = 0;
	s_echop->icmp_pkt.checksum =
		ping_checksum(&(s_echop->icmp_pkt.packet_type), icmp_size);

	if (sendto(sd, (char *)&(s_echop->icmp_pkt.packet_type), icmp_size,
			0, (struct sockaddr *)&sin, sizeof(sin)) != icmp_size) {
		perror("Send");
error:		closesocket(sd);
		exit(1);
	}
	t_snd++;

	sel_time = 5;

sel_again:
	if (sel_time-- < 0) {
		printf("no answer from %s: ", inet_ntoa(sin.sin_addr));
		printf("icmp_seq=%d\n", sequence);
		goto next_seq;
	}
	FD_ZERO(&rfds);
	FD_SET(sd, &rfds);
	if (select(32, &rfds, NULL, NULL, &time) < 0) {
		perror("Select");
		goto error;
	}

	if (FD_ISSET(sd, &rfds)) {
		goto read_it;
	} else {
		if ((i = _get_errno(sd)) != 0) {
			//printf("=%d", i);
			if (i == ENETUNREACH)
				printf("Network");
			else
				printf("Host");
			printf(" Unreachable to %s\n", inet_ntoa(sin.sin_addr));
			goto next_seq;
		} else
			check_end();
	}
	goto sel_again;

read_it:

	i = sizeof(sin);

	new_time = biostime(0,0L);
	if (recvfrom(sd, (char *)&(r_echop->icmp_pkt.packet_type), icmp_size,
			0, (struct sockaddr *)&rin, &i) != icmp_size) {
		perror("Recv");
		goto error;
	}
	if ((sin.sin_addr.s_addr != rin.sin_addr.s_addr) ||
	    (r_echop->icmp_pkt.opcode) ||
	    (r_echop->sequence != sequence) ||
	    (s_echop->identification != r_echop->identification)) {
		goto sel_again;
	}

	t_rcv++;
	printf("%d bytes from %s: ", ping_len, inet_ntoa(rin.sin_addr));
	printf("icmp_seq=%d ", r_echop->sequence);
	tmp_time = (new_time - old_time) * 54;
	printf("time=%ldms", tmp_time);
	t_time += tmp_time;
	if (tmp_time > max_time)
		max_time = tmp_time;
	if (tmp_time < min_time)
		min_time = tmp_time;
	if (bcmp(sdata, rdata, ping_len)) {
		printf(" Data Error.");
	}
	printf("\n");
next_seq:
	sequence++;
	delay(1000);
	check_end();
	goto loop_here;
}

unsigned short
ping_checksum(char *basep, int len)
{
	int i;
	unsigned long sum = 0;

	unsigned short *p = (unsigned short *)basep;
	for (i = 0; i < (len / 2); i++, p++) {
		sum += *p;
		if (sum & 0xffff0000L) {
			sum &= 0xFFFFL;
			sum++;
		}
	}
	if (len & 1) {
		sum += *(unsigned char *)p;
		if (sum & 0xffff0000L) {
			sum &= 0xFFFFL;
			sum++;
		}
	}
	return((unsigned short)(~sum));
}

void
check_end()
{
	if (bioskey(1) || cpre) {
		bioskey(0);
		printf("----- %s PING Statistics -----\n", hostname);
		printf("%ld packet(s) transmitted, %ld packet(s) received, %ld%% packet loss\n",
			t_snd, t_rcv, (t_snd - t_rcv) * 100 / t_snd);
		if (t_rcv) {
			printf("round-trip (ms) min/avr/max = %ld/%ld/%ld\n",
				min_time, t_time / t_rcv, max_time);
		}
		if (sd >= 0)
			closesocket(sd);
		exit(1);
	}
}

int
ctrlc()
{
	cpre = 3;
	return(0);
}
