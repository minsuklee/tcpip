/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdio.h>
#include <io.h>
#include <string.h>

#include "telnet.h"
#include "tn.h"

#define K_FILEOPEN	1	/* Error in fopen() */
#define K_STATE		2	/* State Error */
#define K_TIMEOUT	3	/* Aborted by Timeout */
#define K_RERROR	4	/* Receive Error Packet */
#define K_SEQUENCE	5	/* Sequence Number Error */
#define K_CHKSUM	6	/* Checksum Error */

#define MAXPACKSIZ	94
#define SOH		1
#define CR		13
#define	SP		32
#define DEL		127

#define MAXTRY		10
#define	MYQUOTE		'#'
#define	MYPAD		0
#define MYCHAR		0
#define MYEOL		'~'

#define MYTIME		10
#define	MAXTIM		60
#define	MINTIM		2

#define TRUE		1
#define FALSE		0

#define	tochar(ch)	((ch) + ' ')
#define unchar(ch)	((ch) - ' ')
#define ctl(ch)		((ch) ^ 64)

static int	size,		/* Sizeof Present Data */
		rpsiz,		/* Maximum receive Packet Size */
		spsiz,		/* Maximum receive Packet Size */
		pad = 0,	/* How much padding to send */
		timint,		/* Timeout for foreign host to send */
		n,		/* Packet Number */
		numtry,		/* Times this packet tried */
		oldtry,		/* Times previous packet tried */
		eol = CR,
		quote = '#',
		padchar = NULL;

static char	state,		/* Current kermit protocol state */
		recpkt[MAXPACKSIZ],	/* Receive packet buffer */
		packet[MAXPACKSIZ];	/* Packet Buffer */

static FILE	*fp;
char filnam1[50], *newfilnam;
int ker_scr;
static long byte_count;

static void spack(char type, int num, int len, char *data);
static int  rpack(int *len, int *num, char *data);
static void bufemp(char *buffer, int len);
static void spar(char *data);
static void rpar(char *data);
static void prerrpkt(char *msg);
static unsigned char get_byte(void);
static void flushinput(void);
static int bufill(char *buffer);

static char sinit(void), sfile(void), sdata(void), seof(void), sbreak(void);
static char rinit(void), rfile(void), rdata(void);

#define CHKTOUT { if (numtry++ > MAXTRY) { k_error = K_TIMEOUT; return('A'); }}
#define CHKSEQN { if (num != n) { k_error = K_SEQUENCE; return('A'); }}

static int k_error;
int
get_kerror()
{
	return(k_error);
}

void
set_kerm_port(int scr)
{
	ker_scr = scr;
}

int
recvsw()
{

	state = 'R';
	n = 0;
	numtry = 0;
	k_error = 0;

	eprintf(ker_scr, "KERMIT RCV : SCREEN : %d\n", ker_scr);
	while (TRUE) {
		eprintf(ker_scr, "%c", state);
		switch (state) {
			case 'R' : state = rinit(); break;
			case 'F' : state = rfile(); break;
			case 'D' : state = rdata(); break;
			case 'C' : return(TRUE);
			case 'A' : return(FALSE);
			default  : k_error = K_STATE; return(FALSE);
		}
	}
}

static char
rinit()
{
	int num, len;

	eprintf(ker_scr, "rinit()\n");
	CHKTOUT;
	switch (rpack(&len, &num, packet)) {
		case 'S' :
			rpar(packet);
			spar(packet);
			spack('Y', n, 6, packet);
			oldtry = numtry;
			numtry = 0;
			n = (n + 1) % 64;
			return('F');
		case 'E' :
			k_error = K_RERROR; 
			prerrpkt(recpkt);
			return('A');
		case FALSE :
			spack('N', n, 0, NULL);
			return(state);
		default :
			k_error = K_STATE;
			return('A');
	}
}

static
char
rfile()
{
	int num, len;

	eprintf(ker_scr, "rfile()\n");
	CHKTOUT;
	switch (rpack(&len, &num, packet)) {
		case 'S' :
			CHKTOUT;
			if (num == ((n == 0) ? 63 : (n - 1))) {
				spar(packet);
				spack('Y', num, 6, packet);
				numtry = 0;
				return(state);
			} else {
				k_error = K_SEQUENCE;
				return('A');
			}
		case 'Z' :
			CHKTOUT;
			if (num == ((n == 0) ? 63 : (n - 1))) {
				spack('Y', num, 0, NULL);
				numtry = 0;
				return(state);
			} else {
				k_error = K_SEQUENCE;
				return('A');
			}
		case 'F' :
			CHKSEQN;
			strncpy(filnam1, packet, len);
			filnam1[len] = 0;
			if ((fp = fopen(filnam1, "wb")) == NULL) {
				k_error = K_FILEOPEN;
				return('A');
			} else
				eprintf(ker_scr, "Receiveing %s Started\n", filnam1);
			byte_count = 0L;
			spack('Y', n, 0, NULL);
			oldtry = numtry;
			numtry = 0;
			n = (n + 1) % 64;
			return('D');
		case 'B' :
			CHKSEQN;
			spack('Y', n, 0, NULL);
			return('C');
		case 'E' :
			prerrpkt(recpkt);
			k_error = K_RERROR; 
			return('A');
		case FALSE :
			spack('N', n, 0, NULL);
			return(state);
		default :
			k_error = K_STATE;
			return('A');
	}
}

static
char
rdata()
{
	int num, len;

	CHKTOUT;
	switch (rpack(&len, &num, packet)) {
		case 'D' :
			if (num != n) {
				if (oldtry++ > MAXTRY) {
					k_error = K_TIMEOUT;
					return('A');
				}
				if (num == ((n == 0) ? 63 : (n - 1))) {
					spack('Y', num, 6, packet);
					numtry = 0;
					return(state);
				} else {
					k_error = K_SEQUENCE;
					return('A');
				}
			}
			bufemp(packet, len);
			spack('Y', n, 0, NULL);
			oldtry = numtry;
			numtry = 0;
			n = (n + 1) % 64;
			return('D');
		case 'F' :
			if (oldtry++ > MAXTRY) {
				k_error = K_TIMEOUT;
				return('A');
			}
			if (num == ((n == 0) ? 63 : (n - 1))) {
				spack('Y', num, 0, NULL);
				numtry = 0;
				return(state);
			} else {
				k_error = K_SEQUENCE;
				return('A');
			}
		case 'Z' :
			CHKSEQN;
			spack('Y', n, 0, NULL);
			fclose(fp);
			n = (n + 1) % 64;
			return('F');
		case 'E' :
			prerrpkt(recpkt);
			k_error = K_RERROR; 
			return('A');
		case FALSE :
			spack('N', n, 0, NULL);
			return(state);
		default :
			k_error = K_STATE;
			return('A');
	}
}

static
void
spack(char type, int num, int len, char *data)
{
	int i;
	char chksum, buffer[100], *bufp;

	bufp = buffer;
	for (i = 1; i <= pad; i++)
		tn_send(ker_scr, (unsigned char *)&padchar, 1);
	*bufp++ = SOH;
	*bufp++ = tochar(len + 3);
	chksum  = tochar(len + 3);
	*bufp++ = tochar(num);
	chksum += tochar(num);
	*bufp++ = type;
	chksum += type;
	for (i = 0; i < len; i++, data++) {
		*bufp++ = *data;
		chksum += *data;
	}
	chksum = (((chksum & 0300) >> 6) + chksum) & 077;
	*bufp++ = tochar(chksum);
	*bufp++ = eol;
	/* tn_send(ker_scr, buffer, bufp - buffer); */
	for (i = 0; i < (bufp - buffer); i++) {
		/* em_put(ker_scr, buffer[i]);
		eprintf(ker_scr, "[%02X]", buffer[i]); */
		tn_send(ker_scr, &buffer[i], 1);
	}
	/* eprintf(ker_scr, " SEND:%c(%d-%d) ", type, num, len); */
}

#define GETBYTE	{ lerror = 0; t = get_byte(); if (lerror) return(0); }
int lerror;

static
int
rpack(int *len, int *num, char *data)
{
	int i, done;
	char t, type, cchksum, rchksum;

	while (t != SOH) {
		GETBYTE;
		t &= 0177;
	}

	done = FALSE;
	while (!done) {
		GETBYTE;
		if (t == SOH)
			continue;
		cchksum = t;
		*len = unchar(t) - 3;

		GETBYTE;
		if (t == SOH)
			continue;
		cchksum += t;
		*num = unchar(t);

		GETBYTE;
		if (t == SOH)
			continue;
		cchksum += t;
		type = t;

		for (i = 0; i < *len; i++) {
			GETBYTE;
			if (t == SOH)
				continue;
			cchksum += t;
			data[i] = t;
		}
		data[*len] = 0;
		GETBYTE;
		rchksum = unchar(t);
		GETBYTE;
		if (t == SOH)
			continue;
		done = TRUE;
	}
	cchksum = (((cchksum & 0300) >> 6) + cchksum) & 077;
	if (cchksum != rchksum) {
		eprintf(ker_scr, " CHECKSUM_ERR:%c(%d-%d) ", type, *num, *len);
		return(FALSE);
	}
	/* eprintf(ker_scr, " RCV:%c(%d-%d) ", type, *num, *len); */
	return(type);
}

static
void
bufemp(char *buffer, int len)
{
	int i;
	char t;

	for (i = 0; i < len; i++) {
		t = buffer[i];
		if (t == MYQUOTE) {
			t = buffer[++i];
			if ((t & 0x7f) != MYQUOTE)
				t = ctl(t);
		}
		putc(t, fp);
		byte_count++;
	}
	eprintf(ker_scr, "\r%ld bytes received", byte_count);
}

static
void
spar(char *data)
{
	*data++ = tochar(MAXPACKSIZ);
	*data++ = tochar(MYTIME);
	*data++ = tochar(MYPAD);
	*data++ = ctl(MYCHAR);
	*data++ = tochar(MYEOL);
	*data++ = MYQUOTE;
}

void
rpar(data)
char *data;
{
	spsiz = unchar(*data++);
	timint = unchar(*data++);
	pad = unchar(*data++);
	padchar = unchar(*data++);
	eol = unchar(*data++);
	quote = unchar(*data++);
	eprintf(ker_scr, "spsiz:%d(%02x), timint:%d(%02x), pad:%d(%02x)",
			spsiz, spsiz, timint, timint, pad, pad);
	eprintf(ker_scr, "padch:%d(%02x), eol:%d(%02x), quote:%d(%02x)\n",
			padchar, padchar, eol, eol, quote, quote);
}

void
prerrpkt(char *msg)
{
	eprintf(ker_scr, "Kermit Aborted : %s\n", msg);
}

static
unsigned char
get_byte()
{
	unsigned char ch;
	int tmp;

loop:	tmp = telrcv(ker_scr);
	if (tmp < 0) {
		lerror = 1;
		return(0);
	} else if (tmp == 0) {
		if (_kbhit()) {
			_getch();
			lerror = 1;
			/* spack('E', n, 0, NULL); */
			return(0);
		}
		goto loop;
	}
	tn_recv(ker_scr, &ch, 1);
	return(ch);
}

int
sendsw()
{
	char *filename;

	state = 'S';
	n = 0;
	numtry = 0;
	k_error = 0;

	eprintf(ker_scr, "\nFilename to Send: ");
	filename = getfile(filnam1);
	if (fp == NULL) {
		if ((fp = fopen(filnam1, "rb")) == NULL) {
			eprintf(ker_scr, "Filenot FOUND or Open Error: kermit ABORTED\n");
			k_error = K_FILEOPEN;
			return(FALSE);
		}
	}
	eprintf(ker_scr, "KERMIT SEND : SCREEN : %d\n", ker_scr);
	eprintf(ker_scr, "Sending %s->%s (%ld bytes)\n",
			filnam1, filename, filelength(fileno(fp)));
	strcpy(filnam1, filename);
	while (TRUE) {
		switch (state) {
			case 'S' : state = sinit();  break;
			case 'F' : state = sfile();  break;
			case 'D' : state = sdata();  break;
			case 'Z' : state = seof();   break;
			case 'B' : state = sbreak(); break;
			case 'C' : return(TRUE);
			case 'A' : return(FALSE);
			default  : k_error = K_STATE; return(FALSE);
		}
	}
}

static
char
sinit()
{
	int num, len;

	CHKTOUT;
	spar(packet);
	flushinput();
	spack('S', n, 6, packet);
	switch (rpack(&len, &num, recpkt)) {
		case 'N' :
			return(state);
		case 'Y' :
			if (n != num)
				return(state);
			rpar(recpkt);
			if (eol == 0)
				eol = '\n';
			/* if (quote == 0) */
				quote = '#';
			numtry = 0;
			n = (n + 1) % 64;
			eprintf(ker_scr, "Send start\n");
			return('F');
		case 'E' :
			k_error = K_RERROR; 
			prerrpkt(recpkt);
			return('A');
		case FALSE :
			return(state);
		default :
			k_error = K_STATE;
			return('A');
	}
}

static
char
sfile()
{
	int num, len;

	CHKTOUT;
	eprintf(ker_scr, "sfile()\n");
	spack('F', n, strlen(filnam1), filnam1);
	switch (rpack(&len, &num, recpkt)) {
		case 'N' :
			num = ((--num < 0) ? 63 : num);
			if (n != num)
				return(state);
		case 'Y' :
			if (num != n)
				return(state);
			numtry = 0;
			n = (n + 1) % 64;
			byte_count = 0L;
			size = bufill(packet);
			return('D');
		case 'E' :
			prerrpkt(recpkt);
			return('A');
		case FALSE :
			return(state);
		default :
			k_error = K_STATE;
			return('A');
	}
}

static
char
sdata()
{
	int num, len;

	CHKTOUT;
	spack('D', n, size, packet);
	switch (rpack(&len, &num, recpkt)) {
		case 'N' :
			num = ((--num < 0) ? 63 : num);
			if (num != n)
				return(state);
		case 'Y' :
			if (num != n)
				return(state);
			numtry = 0;
			n = (n + 1) % 64;
			if ((size = bufill(packet)) == EOF)
				return('Z');
			return('D');
		case 'E' :
			prerrpkt(recpkt);
			return('A');
		default :
			return('A');
	}
}

static
char
seof()
{
	int num, len;

	CHKTOUT;
	eprintf(ker_scr, "seof()\n");
	spack('Z', n, 0, packet);
	switch (rpack(&len, &num, recpkt)) {
		case 'N' :
			num = ((--num < 0) ? 63 : num);
			if (num != n)
				return(state);
		case 'Y' :
			if (num != n)
				return(state);
			numtry = 0;
			n = (n + 1) % 64;
			eprintf(ker_scr, "CLose FILE\n");
			fclose(fp);
			fp = NULL;
			return('B');
		case 'E' :
			prerrpkt(recpkt);
			return('A');
		default :
			return('A');
	}
}

static
char
sbreak()
{
	int num, len;

	CHKTOUT;
	spack('B', n, 0, packet);
	switch (rpack(&len, &num, recpkt)) {
		case 'N' :
			num = ((--num < 0) ? 63 : num);
			if (num != n)
				return(state);
		case 'Y' :
			if (num != n)
				return(state);
			numtry = 0;
			n = (n + 1) % 64;
			return('C');
		case 'E' :
			prerrpkt(recpkt);
			return('A');
		case FALSE :
			return(state);
		default :
			return('A');
	}
}

static
void
flushinput()
{
	while (telrcv(ker_scr)) {
		tn_recv(ker_scr, recpkt, MAXPACKSIZ);
	}
}

static
int
bufill(char *buffer)
{
	int i, t;
	char t7;

	i = 0;
	eprintf(ker_scr, "\r%ld bytes sent", byte_count);
	while (!feof(fp)) {
		t = getc(fp);
		byte_count++;
		t7 = t & 0x7f;
		if ((t7 < SP) || (t7 == DEL) || (t7 == quote)) {
			buffer[i++] = quote;
			if (t7 != quote) {
				t = ctl(t);
			}
		}
		buffer[i++] = t;
		if (i >= (spsiz - 8))
			return(i);
	}
	if (i == 0)
		return(EOF);
	return(i);
}

char *
getfile(char *s)
{
	int i;

	gets(s);
	if (strlen(s) == 0)
		return(NULL);

	for (i = strlen(s) - 1; i >= 0; i--) {
		if (s[i] == '\\' || s[i] == ':')
			return(s + i + 1);
	}
	if (strlen(s) == 0)
		return(NULL);
	return(s);
}
