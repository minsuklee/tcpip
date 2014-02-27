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

#include <sys\types.h>
#include <sys\time.h>
#include <sys\socket.h>
#include <sys\ioctl.h>
#include <netinet\in.h>
#include <arpa\inet.h>
#include <netdb.h>

#include <imigetcp\imigetcp.h>

#include <errno.h>
extern int errno;

#include "telnet.h"
#include "tn.h"

#define BLOCK_TRX

#define	SB_CLEAR(x)	(x)->optr = (x)->obuf;
#define	SB_TERM(x)	(x)->oend = (x)->optr;
#define	SB_ACCUM(x, c)	\
	if ((x)->optr < ((x)->obuf + SUBOPTSIZE)) { *((x)->optr)++ = (c); }

#define	NETADD(x, c)		{ char y = (c); send((x)->sd, &y, 1, 0); }
#define	NET2ADD(x, c1, c2)	{ NETADD((x), (c1)); NETADD((x), (c2)); }
#define NETWRITE(x, data, cnt)	{ send((x)->sd, (data), (cnt), 0); }

#define RSIZE(x)	(((x)->rfull) ? RBUFSIZE : \
			   (((x)->rhead - (x)->rtail) + \
			    (((x)->rtail > (x)->rhead) ? RBUFSIZE : 0)))
#define RPUSH(x, c)	{ (x)->recv_buf[(x)->rhead] = (c); \
			  (x)->rhead = ((x)->rhead + 1) % RBUFSIZE; \
			  if ((x)->rhead == (x)->rtail) (x)->rfull = 1; }

struct CONNECTION TELCON[MAXSCREEN];

#define	TS_DATA		0
#define	TS_IAC		1
#define	TS_WILL		2
#define	TS_WONT		3
#define	TS_DO		4
#define	TS_DONT		5
#define	TS_CR		6
#define	TS_SB		7		/* sub-option collection */
#define	TS_SE		8		/* looking for sub-option end */

#define SCR(x)	((int)((struct CONNECTION *)(x) - &TELCON[0]))
void init_chan(struct CONNECTION *t);
void tn_close(int ch);
static int chk_rcv(struct CONNECTION *t);
static void suboption(struct CONNECTION *t);

void
init_telnet()
{
	int i;
	struct CONNECTION *t;
	
	for (t = TELCON, i = 0; i < MAXSCREEN; i++, t++) {
		init_chan(t);
	}
}

void
init_chan(struct CONNECTION *t)
{
	memset(t, 0, sizeof(*t));		/* Fill ALL with 0 */
	SB_CLEAR(t);
	t->ospeed = t->ispeed = 38400L;
	strcpy(t->term, "vt100");
	t->sd = -1;
}

#ifdef BLOCK_TRX
unsigned char opttmp[3];
#endif

//static
void
send_do(struct CONNECTION *t, int c, int init)
{
	if (init) {
		if (((t->do_nt[c]==0) && my_st_do(t, c)) ||
		    my_want_st_do(t, c))
			return;
		set_my_want_state_do(t, c);
		t->do_nt[c]++;
	}
	//eprintf(SCR(t), "[DO %d]", c);
#ifdef BLOCK_TRX
	opttmp[0] = IAC; opttmp[1] = DO; opttmp[2] = c;
	NETWRITE(t, opttmp, 3);
#else
	NET2ADD(t, IAC, DO);
	NETADD(t, c);
#endif
}

//static
void
send_dont(struct CONNECTION *t, int c, int init)
{
	if (init) {
		if (((t->do_nt[c] == 0) && my_st_dont(t, c)) ||
		    my_want_st_dont(t, c))
			    return;
		set_my_want_state_dont(t, c);
		t->do_nt[c]++;
	}
	//eprintf(SCR(t), "[DONT %d]", c);
#ifdef BLOCK_TRX
	opttmp[0] = IAC; opttmp[1] = DONT; opttmp[2] = c;
	NETWRITE(t, opttmp, 3);
#else
	NET2ADD(t, IAC, DONT);
	NETADD(t, c);
#endif
}

//static
void
send_will(struct CONNECTION *t, int c, int init)
{
	if (init) {
		if (((t->wi_nt[c] == 0) && my_st_will(t, c)) ||
		    my_want_st_will(t, c))
			    return;
 		set_my_want_state_will(t, c);
		(t->wi_nt[c])++;
	}
	//eprintf(SCR(t), "[WILL %d]", c);
#ifdef BLOCK_TRX
	opttmp[0] = IAC; opttmp[1] = WILL; opttmp[2] = c;
	NETWRITE(t, opttmp, 3);
#else
	NET2ADD(t, IAC, WILL);
	NETADD(t, c);
#endif
}

//static
void
send_wont(struct CONNECTION *t, int c, int init)
{
	if (init) {
		if (((t->wi_nt[c] == 0) && my_st_wont(t, c)) ||
		    my_want_st_wont(t, c))
			    return;
		set_my_want_state_wont(t, c);
		t->wi_nt[c]++;
	}
	//eprintf(SCR(t), "[WONT %d]", c);
	
#ifdef BLOCK_TRX
	opttmp[0] = IAC; opttmp[1] = WONT; opttmp[2] = c;
	NETWRITE(t, opttmp, 3);
#else
	NET2ADD(t, IAC, WONT);
	NETADD(t, c);
#endif
}

static
void
willoption(struct CONNECTION *t, int option)
{
	int new_state_ok = 0;

	//eprintf(SCR(t), "WILL : %d\n", option);
	if (t->do_nt[option]) {
		--(t->do_nt[option]);
		if (t->do_nt[option] && my_st_do(t, option))
			--(t->do_nt[option]);
	}

	if ((t->do_nt[option] == 0) && my_want_st_dont(t, option)) {
		switch (option) {
		    case TELOPT_ECHO:
		    case TELOPT_BINARY:
		    case TELOPT_SGA:
		    case TELOPT_STATUS:
			new_state_ok = 1;
			break;
		    case TELOPT_TM:
			set_my_want_state_dont(t, option);
			set_my_state_dont(t, option);
			return;
		    case TELOPT_LINEMODE:
		    default:
			break;
		}

		if (new_state_ok) {
			set_my_want_state_do(t, option);
			send_do(t, option, 0);
		} else {
			t->do_nt[option]++;
			send_dont(t, option, 0);
		}
	}
	set_my_state_do(t, option);
}

static
void
wontoption(struct CONNECTION *t, int option)
{
	//eprintf(SCR(t), "WONT : %d\n", option);
	if (t->do_nt[option]) {
		--(t->do_nt[option]);
		if (t->do_nt[option] && my_st_dont(t, option))
			--(t->do_nt[option]);
	}

	if ((t->do_nt[option] == 0) && my_want_st_do(t, option)) {
		switch (option) {
		    case TELOPT_SGA:
		    case TELOPT_ECHO:
			break;
		    case TELOPT_TM:
			set_my_want_state_dont(t, option);
			set_my_state_dont(t, option);
			return;
		    default:
			break;
		}
		set_my_want_state_dont(t, option);
		if (my_st_do(t, option))
		    	send_dont(t, option, 0);
	} else if (option == TELOPT_TM) {
		set_my_want_state_dont(t, option);
	}
	set_my_state_dont(t, option);
}

static
void
dooption(struct CONNECTION *t, int option)
{
	int new_state_ok = 0;

	//eprintf(SCR(t), "DO : %d\n", option);
	if (t->wi_nt[option]) {
		--(t->wi_nt[option]);
		if (t->wi_nt[option] && my_st_will(t, option))
			--(t->wi_nt[option]);
	}

	if (t->wi_nt[option] == 0) {
		if (my_want_st_wont(t, option)) {
			switch (option) {
			    case TELOPT_TM:
				send_will(t, option, 0);
				set_my_want_state_wont(t, TELOPT_TM);
				set_my_state_wont(t, TELOPT_TM);
				return;
			    case TELOPT_BINARY:		/* binary mode */
			    case TELOPT_NAWS:		/* window size */
			    case TELOPT_TSPEED:		/* terminal speed */
			    case TELOPT_LFLOW:		/* local flow control */
			    case TELOPT_TTYPE:		/* terminal type option */
			    case TELOPT_SGA:		/* no big deal */
			    //case TELOPT_ENVIRON:
				new_state_ok = 1;
				break;
			    case TELOPT_LINEMODE:
				send_do(t, TELOPT_SGA, 1);
				set_my_want_state_will(t, TELOPT_LINEMODE);
				send_will(t, option, 0);
				set_my_state_will(t, TELOPT_LINEMODE);
				return;
			    case TELOPT_ECHO:		/* We're never going to echo... */
			    default:
				break;
	    		}
			if (new_state_ok) {
				set_my_want_state_will(t, option);
				send_will(t, option, 0);
			} else {
				t->wi_nt[option]++;
				send_wont(t, option, 0);
			}
	  	} else {
			switch (option) {
			    case TELOPT_LINEMODE:
				send_do(t, TELOPT_SGA, 1);
				set_my_state_will(t, option);
				send_do(t, TELOPT_SGA, 0);
				return;
			}
		}
	}
	set_my_state_will(t, option);
}

static
void
dontoption(struct CONNECTION *t, int option)
{
	//eprintf(SCR(t), "DONT : %d\n", option);
	if (t->wi_nt[option]) {
		--(t->wi_nt[option]);
		if (t->wi_nt[option] && my_st_wont(t, option))
			--(t->wi_nt[option]);
	}

	if ((t->wi_nt[option] == 0) && my_want_st_will(t, option)) {
		set_my_want_state_wont(t, option); /* always accept a DONT */
		if (my_st_will(t, option))
	    		send_wont(t, option, 0);
	}
	set_my_state_wont(t, option);
}

static
void
suboption(struct CONNECTION *t)
{
	switch (t->obuf[0] & 0xff) {
	    case TELOPT_TTYPE:
		if (my_want_st_wont(t, TELOPT_TTYPE))
			return;
		if ((t->obuf[1] & 0xff) == TELQUAL_SEND) {
			char tmp[50], len;
			len = strlen(t->term) + 4 + 2;
			sprintf(tmp, "%c%c%c%c%s%c%c", IAC, SB,
				TELOPT_TTYPE, TELQUAL_IS, t->term, IAC, SE);
			NETWRITE(t, tmp, len);
		}
		break;
	    case TELOPT_TSPEED:
		if (my_want_st_wont(t, TELOPT_TSPEED))
			return;
		if ((t->obuf[1] & 0xff) == TELQUAL_SEND) {
			char tmp[50];
			int len;

			sprintf(tmp, "%c%c%c%c%ld,%ld%c%c", IAC, SB,
				TELOPT_TSPEED, TELQUAL_IS,
				t->ospeed, t->ispeed, IAC, SE);
			len = strlen(tmp+4) + 4;  /* tmp[3] = 0 */
			NETWRITE(t, tmp, len);
		}
		break;
	    default:
		break;
	}
}

#define	PUTSHORT(cp, x) { \
		if ((*cp++ = ((x) >> 8) & 0xff) == IAC) *cp++ = IAC;	\
		if ((*cp++ = (x) & 0xff) == IAC) *cp++ = IAC;		\
}

static
void
sendnaws(struct CONNECTION *t)
{
	unsigned char tmp[16], *cp = tmp;

	if (my_st_wont(t, TELOPT_NAWS))
		return;

	*cp++ = IAC; *cp++ = SB; *cp++ = TELOPT_NAWS;
	PUTSHORT(cp, MAXCOL);
	PUTSHORT(cp, MAXROW);
	*cp++ = IAC; *cp++ = SE;
	NETWRITE(t, tmp, cp - tmp);
}

#ifdef ENV_SEND
static
void
sendenv(struct CONNECTION *t)
{
	unsigned char tmp[16], *cp = tmp;

	if (my_st_wont(t, TELOPT_ENVIRON))
		return;
	//eprintf(SCR(t), "SEND ENV");
	*cp++ = IAC; *cp++ = SB; *cp++ = TELOPT_ENVIRON; *cp++ = TELQUAL_INFO;
//	PUTSHORT(cp, MAXCOL);
//	PUTSHORT(cp, MAXROW);
	*cp++ = IAC; *cp++ = SE;
	NETWRITE(t, tmp, cp - tmp);
}
#endif

static unsigned char tmp_buf[RBUFSIZE];

int
telrcv(int ch)
{
	struct CONNECTION *t = &TELCON[ch];
	unsigned char *tmp0, *tmp;
	int cnt, i, j;

	if (t->rfull)
		return(RBUFSIZE);

	if (chk_rcv(t) <= 0) {
		goto chk_data;
	}
	if ((cnt = recv(t->sd, tmp_buf, RBUFSIZE - RSIZE(t), 0)) < 0) {
		closesocket(t->sd);
		t->sd = -1;
		goto chk_data;
	}
	//printf("[%d]", cnt);
	//if (cnt > 0)
	//	getch();
	//for (i = 0; i < cnt; i++)
	//	eprintf(ch, "[%02X]", tmp_buf[i]);

	for (tmp0 = tmp = tmp_buf, i = 0; i < cnt; i++, tmp++, tmp0++) {
		if (cnt == 1)
			goto state_p;
		if (t->telrcv_state == TS_DATA) {
		   int tcnt;
		   for (; i < cnt; i++, tmp++) {
			if (*tmp == IAC)
				break;
		   }
		   tcnt = tmp - tmp0;
		   if (tcnt) {
			if ((t->rhead + tcnt) > RBUFSIZE) {  /* wrap */
				int rc = RBUFSIZE - t->rhead;
				memcpy(t->recv_buf + t->rhead, tmp0, rc);
				memcpy(t->recv_buf, tmp0 + rc, tcnt - rc);
			} else
				memcpy(t->recv_buf + t->rhead, tmp0, tcnt);
			t->rhead = (t->rhead + tcnt) % RBUFSIZE;
			if (t->rhead == t->rtail)
				t->rfull = 1;
			tmp0 = tmp;
		   }
		   if (tmp == (tmp_buf + cnt))	/* End of data */
			break;
		}
state_p:	switch (t->telrcv_state) {
		    case TS_DATA:
			if (*tmp == IAC) {
				t->telrcv_state = TS_IAC;
				break;
			} else
				RPUSH(t, *tmp);
			break;
		    case TS_IAC:
process_iac:		switch (*tmp) {
	    		    case WILL:
				t->telrcv_state = TS_WILL;
				continue;
			    case WONT:
				t->telrcv_state = TS_WONT;
				continue;
			    case DO:
				t->telrcv_state = TS_DO;
				continue;
			    case DONT:
				t->telrcv_state = TS_DONT;
				continue;
			    case SB:
				SB_CLEAR(t);
				t->telrcv_state = TS_SB;
				continue;
			    case IAC:
				RPUSH(t, IAC);
				break;
			    case NOP:
		            case GA:
			    case DM:
			    default:
				break;
	    		}
			t->telrcv_state = TS_DATA;
			break;
		    case TS_WILL:
			willoption(t, *tmp);
			t->telrcv_state = TS_DATA;
			continue;
		    case TS_WONT:
			wontoption(t, *tmp);
			t->telrcv_state = TS_DATA;
			continue;
		    case TS_DO:
			dooption(t, *tmp);
			if (*tmp == TELOPT_NAWS)
				sendnaws(t);
			//else if (*tmp == TELOPT_ENVIRON)
			//	sendenv(t);
			t->telrcv_state = TS_DATA;
			continue;
		    case TS_DONT:
			dontoption(t, *tmp);
			t->telrcv_state = TS_DATA;
			continue;
		    case TS_SB:
			if (*tmp == IAC)
				t->telrcv_state = TS_SE;
			else
				SB_ACCUM(t, *tmp);
			continue;
		    case TS_SE:
			if (*tmp != SE) {
				if (*tmp != IAC) {
					SB_TERM(t);
					SB_ACCUM(t, IAC);
					SB_ACCUM(t, *tmp);
					suboption(t);	/* handle sub-option */
					t->telrcv_state = TS_IAC;
					goto process_iac;
				}
				SB_ACCUM(t, *tmp);
				t->telrcv_state = TS_SB;
			} else {
				SB_TERM(t);
				SB_ACCUM(t, IAC);
				SB_ACCUM(t, SE);
				suboption(t);	/* handle sub-option */
				t->telrcv_state = TS_DATA;
			}
		}	 /* switch */
	}
chk_data:
	if ((RSIZE(t) == 0) && (t->sd < 0)) {
		return(-1);
	}
	return(RSIZE(t));
}

void
make_con(int scr, char *s)
{
	struct hostent *hp;
	struct CONNECTION *t = &TELCON[scr];
	unsigned long haddr;
	struct sockaddr_in sin;
	int on = 1;

	if (s) {
		strcpy(t->hname, s);
		t->hname_cnt = strlen(s);
	} else if (t->hname_cnt == 0)
		goto error;
	else
		eprintf(scr, "\n");
	//eprintf(scr, "Searching: %s\n", t->hname);
	stat_hostname(scr, t->hname, t->hname_cnt, 1);
	if ((haddr = inet_addr(t->hname)) == 0xffffffffL) {
		struct hostent *hp;
		if ((hp = gethostbyname(t->hname)) == NULL) {
			eprintf(scr, "Host '%s' Unknown.\n", t->hname);
error:			eprintf(scr, "\nhostname : ");
			stat_hostname(scr, NULL, 0, 0);
			init_chan(t);
			return;
		}
		haddr= *(unsigned long *)(hp->h_addr);
	}
    	sin.sin_family = AF_INET;
    	sin.sin_port = 5888;	/* well-known telnet port */
	sin.sin_addr = *(struct in_addr *)&haddr;
	if ((t->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		eprintf(scr, "Can't Open Socket\n");
		goto error;
	}
	ioctlsocket(t->sd, FIONBIO, (char *)&on);
	eprintf(scr, "Trying %s...\n", inet_ntoa(sin.sin_addr));
	if (connect(t->sd, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
		if (errno != EINPROGRESS) {
			eperror(scr, "connect", errno);
			tn_close(t->sd);
			goto error;
		}
		t->constate = CONNECTING;
		return;
	} else {
		t->constate = CONNECTED;
		stat_hostname(scr, t->hname, t->hname_cnt, 2);
	}

	//send_do(t, TELOPT_SGA, 1);
	//send_will(t, TELOPT_TTYPE, 1);
	//send_will(t, TELOPT_NAWS, 1);
	//send_will(t, TELOPT_TSPEED, 1);
	//send_will(t, TELOPT_LFLOW, 1);
	//send_will(t, TELOPT_LINEMODE, 1);
	//send_do(t, TELOPT_STATUS, 1);
	return;
}

void
close_all()
{
	struct CONNECTION *t;
	int i;

	for (t = TELCON, i = 0; i < MAXSCREEN; i++, t++) {
		if (t->sd >=  0) {
			//printf("CLOSE scr=%d(sd = %d)\n", i, t->sd);
			closesocket(t->sd);
			init_chan(t);
		}
	}
}

void
tn_close(int ch)
{
	struct CONNECTION *t = &TELCON[ch];
	if (t->sd >= 0) {
		closesocket(t->sd);
	}
	init_chan(t);
}

int
tn_send(int ch, unsigned char *data, int count)
{
	struct CONNECTION *t = &TELCON[ch];
	unsigned char *tmp = data;
	int i;

	for (i = 0; i < count; i++) {
		if (*tmp++ == IAC) {
			NETWRITE(t, data, tmp - data);
			NETADD(t, IAC);
			data = tmp;
		}
	}
	if (tmp - data)
		NETWRITE(t, data, tmp - data);
	return(count);
}

int
islocalecho(int ch)
{
	struct CONNECTION *t = &TELCON[ch];
	return(my_want_st_dont(t, TELOPT_ECHO) != 0);
}

int
tn_recv(int ch, unsigned char *data, int count)
{
	struct CONNECTION *t = &TELCON[ch];
	int cnt;

	if (count == 0)
		return(0);
	cnt = (RSIZE(t) > count) ? count : RSIZE(t);
	if ((t->rtail + cnt) > RBUFSIZE) {	/* wrap */
		int tmp = RBUFSIZE - t->rtail;
		memcpy(data, t->recv_buf + t->rtail, tmp);
		memcpy(data + tmp, t->recv_buf, cnt - tmp);
	} else
		memcpy(data, t->recv_buf + t->rtail, cnt);
	t->rtail = (t->rtail + cnt) % RBUFSIZE;
	if (cnt)
		t->rfull = 0;
	return(cnt);
}

static
int
chk_rcv(struct CONNECTION *t)
{
	unsigned long ibits;
	struct timeval TimeValue = { 0L, 0L };

	if (t->sd < 0)
		return(-1);
retry:	ibits = 0L;
	FD_SET(t->sd, &ibits);

	if (select(t->sd + 1, &ibits, NULL, NULL, &TimeValue) < 0) {
		closesocket(t->sd);
		t->sd = -1;
		return(-1);
	}
	if (FD_ISSET(t->sd, &ibits))
		return(1);
	return(0);
}

void
copyhname(int scr, char *s, int len)
{
	struct CONNECTION *t = &TELCON[scr];

	memcpy(&(t->hname[t->hname_cnt]), s, len);
	t->hname_cnt += len;
	t->hname[t->hname_cnt] = 0;
}

void
current_status(int scr)
{
	int i, j;
	struct CONNECTION *t;

	eprintf(scr, "\n CURRENT SCREEN : %d(%d)\n", c_screen, scr);
	for (t = TELCON, i = 0; i < MAXSCREEN; i++, t++) {
		eprintf(scr, "CH:%d SD:%d (%d-%d) : %d : ",
			i, t->sd, t->rhead, t->rtail, t->telrcv_state);
		for (j = 0; j < t->hname_cnt; j++)
			eprintf(scr, "%c", t->hname[j]);
		eprintf(scr, "              \n");
	}
	emu_stat(scr);
}
