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
#include <time.h>

#include "telnet.h"
#include "tn.h"

#define HLENGTH 8
#define S_LENGTH (HLENGTH + 3)

static char status[MAXCOL];
static char mode[MAXCOL];

void hw_put_stat(char *str, char *mode);

void
init_stat()
{
	int i;

	for (i = 0; i < MAXCOL; i++) {
		status[i] = C_BLANK;
		mode[i] = A_NORMAL;
	}
	for (i = 0; i < MAXSCREEN; i++) {
		stat_hostname(i, NULL, 0, 0);
	}
	hw_put_stat(status, mode);
}

void
hide_stat_line()
{
	char tmp1[MAXCOL];
	char tmp2[MAXCOL];
	int i;

	for (i = 0; i < MAXCOL; i++) {
		tmp1[i] = C_BLANK;
		tmp2[i] = A_NORMAL;
	}
	hw_put_stat(tmp1, tmp2);
}

void
clear_stat()
{
	int i;

	for (i = 0; i < MAXCOL; i++) {
		status[i] = C_BLANK;
		mode[i] = A_NORMAL;
	}
	hw_put_stat(status, mode);
}

/* attr 0 : CLOSED, 1 : ON CONNECTING, 2 : CONNECTED */
void
stat_hostname(int chan, char *name, int len, int attr)
{
	int i, index;

	index = chan * S_LENGTH;
	status[index++] = '1' + chan;
	status[index++] = ':';

	if (len > HLENGTH)
		len = HLENGTH;
	for (i = 0; i < len; i++) {
		status[index] = *name++;
		switch(attr) {
			case 0 : mode[index++] = A_NORMAL; break;
			case 1 : mode[index++] = A_BLINKING; break;
			case 2 : mode[index++] = A_INVERSE; break;
		}
	}
	for (i = index; i < (chan + 1) * S_LENGTH; i++) {
		status[i] = C_BLANK;
		mode[i] = A_NORMAL;
	}
	hw_put_stat(status, mode);
}

void
focus(int chan)
{
	int index, i;

	for (index = i = 0; i < MAXSCREEN; i++, index += S_LENGTH) {
		if (i == chan)
			mode[index] = A_BOLD;
		else
			mode[index] = A_NORMAL;
	}
	hw_put_stat(status, mode);
}

void
capturesign(int chan)
{
	int index, i;

	for (index = i = 0; i < MAXSCREEN; i++, index += S_LENGTH) {
		if (i == chan)
			status[index] = 'C';
		else
			status[index] = i + '0';
	}
	hw_put_stat(status, mode);
}

void
refresh_stat()
{
	hw_put_stat(status, mode);
}

void
new_time(time_t *now)
{
	struct tm *tm_now;

	tm_now = localtime(now);
	sprintf(status + 74, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	refresh_stat();
}
