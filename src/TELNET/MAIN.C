/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>	
#include <dos.h>
#include <time.h>

#include <sys\types.h>
#include <sys\time.h>
#include <sys\socket.h>
#include <sys\ioctl.h>
#include <netinet\in.h>
#include <arpa\inet.h>
#include <netdb.h>

#include <errno.h>
#include <imigetcp/tcpcfg.h>
#include <imigetcp/imigetcp.h>

#include "telnet.h"
#include "tn.h"

void new_time(time_t *now);

static unsigned char buf[RBUFSIZE];

char keyfile[80];

int mouse = 0;

//#define CAPTURE
#ifdef CAPTURE
extern int monitor;
FILE *fcap;
#endif

void
main(argc, argv)
int argc;
char *argv[];
{
	struct CONNECTION *t;
	int cnt, i, j;
	char *str;
	time_t t_now, t_old = 0;

	_get_errno(0);
	init_telnet();
	init_screen();
	for (i = 0; i < MAXSCREEN; i++) {
		eprintf(i, "Multi Session TELNET vt100 emulator.\n");
		//eprintf(i, "(C) Copyright in 1995, Minsuk Lee, mslee@sparc.snu.ac.kr. %s.\n\n", __DATE__);
	}
	mouse = mouse_init(NULL);
	ctrlbrk(ctrlc);
	//install_cbrk(ctrlc);
	//set_cntrlbrk(0);
	if ((str = getenv(IMIGETCP_ENV)) == NULL)
		keyfile[0] = 0;
	else
		strcpy(keyfile, str);
	strcat(keyfile, "\\");
	strcat(keyfile, "KEY.DEF");
	init_key();
	for (i = 1; i < MAXSCREEN; i++) {
		eprintf(i, "hostname : ");
	}
	init_stat();
	set_screen(c_screen);
	focus(c_screen);
	time(&t_old);
	new_time(&t_old);
	if (argc == 2) {
		make_con(c_screen, argv[1]);
	} else {
		struct TCPCONFIG conf;
		if (_read_config(&conf) < 0) {
			clear_stat();
			exit(1);
		}
		make_con(c_screen, conf.c_defhost);
	}
#ifdef CAPTURE
	fcap = fopen("CAP", "w");
	fclose(fcap);
#endif
	while (1) {
		time(&t_now);
		if (t_now != t_old) {
			new_time(&t_now);
			t_old = t_now;
		}
		key_input(c_screen);
		if (mouse) {
			if (check_click(0x01)) {
				cut_it();
			}
			if (check_click(0x02)) {	/* just Put it */
				put_it(0);
			}
			if (check_click(0x04)) {	/* put it with CR */
				put_it(1);
			}
		}
		for (t = TELCON, i = 0; i < MAXSCREEN; i++, t++) {
			if (t->sd >= 0) {
				if (t->constate == CONNECTING) {
					unsigned long cbits = 0;
					struct timeval TimeValue = { 0L, 0L };

					FD_SET(t->sd, &cbits);
					if ((j = select(t->sd + 1, NULL, &cbits, NULL, &TimeValue)) < 0) {
						eperror(i, "connect", _get_errno(t->sd));
						tn_close(i);
						eprintf(i, "\nhostname : ");
						stat_hostname(i, NULL, 0, 0);
						continue;
					}
					if ((j == 1) && FD_ISSET(t->sd, &cbits)) {
						int off = 0;
						ioctlsocket(t->sd, FIONBIO, (char *)&off);
						t->constate = CONNECTED;
						stat_hostname(i, t->hname, t->hname_cnt, 2);
						//send_do(t, TELOPT_SGA, 1);
						//send_will(t, TELOPT_TTYPE, 1);
					} else
						continue;
				}
				//if (i != c_screen) continue;
				cnt = telrcv(i);
				//if (cnt > 0 && t->monitor) {
				//	t->monitor = 0;
				//	ring(); ring(); ring();
				//	ring(); ring(); ring(); 
				//}
				switch (cnt) {
					case 0  :
						continue;
					case -1 :
						//eprintf(i, " telrcv return -1 ");
						closed(i);
						if (no_of_con()) {
							eprintf(i, "\nhostname : ");
							next_screen(i);
							continue;
						}
						goto leave;
				}
			}
			if (i != c_screen)
				continue;
			cnt = tn_recv(i, buf, RBUFSIZE);
			if (cnt && (t->sd < 0)) {
				//eprintf(i, " tn_recv return -1 ");
				closed(i);	// passive closed in backgound
			}
			//printf("<%d>", cnt);
			//if (cnt > 0)
			//	getch();
#ifdef CAPTURE
			if (i == c_screen && monitor) {
				fcap = fopen("CAP", "a+");
				for (j = 0; j < cnt; j++) {
					if (buf[j] < 0x20) {
						fputc('^', fcap);
						fputc(buf[j] + 0x40, fcap);
					} else
						fputc(buf[j], fcap);
				}
				fclose(fcap);
			}
#endif
			if ((cnt > 0) && mouse)
				undraw_cut();
			for (j = 0; j < cnt; j++) {
				/* eprintf(i, "[%02x]", buf[j]); */
				em_put(i, buf[j]);
			}
		}
	}
leave:
	leave_pgm();
}

void
leave_pgm()
{
	//uninstall_cbrk();
	clear_stat();
	close_screen();
	exit(0);
}

void
closed(int screen)
{
	tn_close(screen);
	stat_hostname(screen, NULL, 0, 0);
	//eprintf(screen, "connection closed by foreign host.\n");
	emu_makedefault(screen, 0);
}

int
no_of_con()
{
	int i, cnt = 0;
	for (i = 0; i < MAXSCREEN; i++)
		if (TELCON[i].sd >= 0)
			cnt++;
	return(cnt);
}

struct ERR {
	int no;
	char *msg;
};

void
eperror(int scr, char *s, int error)
{
	extern struct ERR ___nerror[];
	struct ERR *ep;

	if (error < EWOULDBLOCK) {
		eprintf(scr, "%s: %s\n", s, sys_errlist[error]);
		return;
	}
	ep = ___nerror;
	while (ep->no != EREMOTE) {
		if (ep->no == error) {
			eprintf(scr, "%s: %s\n", s, ep->msg);
			return;
		}
		ep++;
	}
	eprintf(scr, "%s: unknown error (%d)\n", s, error);
}
