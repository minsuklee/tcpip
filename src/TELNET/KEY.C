/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <ctype.h>
#include <string.h>
#include <bios.h>

#include "telnet.h"
#include "tn.h"

char cpre = 0;
int
ctrlc()
{
	cpre = 3;
	return(0);
}

static char _cmd[100];
FILE *capturefile = NULL;
int capturescr;

static char *process_fkey(int key);

void
key_input(int scr)
{
	struct CONNECTION *t = &TELCON[scr];
	int local, i;
	unsigned screen, input;
	char *tmp, ch, *filename;

	local = islocalecho(scr);
	if (cpre != 0) {
		if (t->sd >= 0)
			tn_send(scr, &cpre, 1);
		cpre = 0;
	}
	if (bioskey(1) == 0) {
		return;
	}
	input = bioskey(0);
	//eprintf(scr, "[%x]", input);
	if ((input & 0xff) == 0) {
		char *kseq, *process_fkey();
		extern char keyfile[];

		input /= 256;
		switch (input) {
		//  case 36  :  /* ALT-J */
		//	{ extern int monitor;
		//		monitor = !monitor;
		//	save_screen(scr);
		//	printf("\nCURRENT MONITOR STATE = %d : \n", monitor);
		//	_getch();
		//	load_screen(scr);
		//	}
		//	break;
		//  case 31  :  /* ALT-S */
		//	save_screen(scr);
		//	current_status(scr);
		//	_getch();
		//	load_screen(scr);
		//	break;
		    case 35  :  /* ALT-H */
			save_screen(scr);
			eprintf(scr, "\n\nHELP : \n");
			eprintf(scr, "  ALT F1-F4 : Select SCREEN\n");
			eprintf(scr, "  ALT F5    : ASCII file Send\n");
			eprintf(scr, "  ALT F6    : Load KEYMAP File\n");
			eprintf(scr, "  ALT F7    : Kermit Receive\n");
			eprintf(scr, "  ALT F8    : Kermit Send\n");
			eprintf(scr, "  ALT F9    : Direct RUN F9 Command\n");
			eprintf(scr, "  ALT F10   : Push DOS\n");
			eprintf(scr, "  ALT X     : Close All session and Exit\n");
			eprintf(scr, "  ALT C     : Capture\n");
			eprintf(scr, "  ALT N     : Next open session\n");
			eprintf(scr, "  ALT M     : Set Monitor Mode\n");
		//	eprintf(scr, "  ALT S     : Show Status\n");
			eprintf(scr, "  ALT H     : This HELP Screen\n");
			eprintf(scr, "\nPrint Any Key to return TELNET ");
			_getch();
			load_screen(scr);
			break;
		    case 45  :  /* ALT-X */
			if (!no_of_con())
				leave_pgm();
			save_screen(scr);
			eprintf(scr, "\nClose all Connection and exit (y/N) ?");
			ch = _getch();
			if (ch == 'y' || ch == 'Y') {
				close_all();
				leave_pgm();
			}
			load_screen(scr);
			break;
		    case 46  :  /* ALT-C */
			if (capturefile) {
				if (capturescr != scr)
					break;
				fclose(capturefile);
				capturefile = NULL;
				capturesign(100);
			} else {
				save_screen(scr);
				capturescr = scr;
				eprintf(scr, "\nFilename to Capture: ");
				filename = getfile(_cmd);
				if ((capturefile = fopen(_cmd, "a+w")) == NULL) {
					eprintf(scr, "Open Error: '%s' capture ABORTED\n", filename);
					eprintf(scr, "Print Any Key to return TELNET ");
					_getch();
				} else {
					//eprintf(scr, "Capture Started\n");
					fseek(capturefile, 1L, SEEK_END);
					capturesign(scr);
				}
				load_screen(scr);
			}
			break;
		    case 104 :	/* ALT-F1 */
		    case 105 :
		    case 106 :
		    case 107 :
			screen = input - 104;
			set_screen(screen);
			focus(screen);
			break;
		    case 49 :	/* ALT-N */
			next_screen(scr);
			break;
		    //case 50 :   /* ALT-M */
		    //	TELCON[scr].monitor = 1;
		    //	break;
		    case 108 :	/* ALT-F5 */
			save_screen(scr);
			eprintf(scr, "\nASCII FIlename to Send :");
			_getch();
			load_screen(scr);
			break;
		    case 109 :	/* ALT_F6 */
			init_key();
			break;
		    case 110 :	/* ALT_F7 */
			set_kerm_port(scr);
			recvsw();
			break;
		    case 111 :	/* ALT_F8 */
			set_kerm_port(scr);
			sendsw();
			break;
		    case 112 :	/* ALT_F9 */
			tmp = process_fkey(input);
			strcpy(_cmd, tmp);
			if (strlen(_cmd) > 1) {
				char q;
				q = _cmd[strlen(_cmd)-1];
				if (q == '&')
					_cmd[strlen(_cmd)-1] = 0;
				save_screen(scr);
				system(_cmd);
				if (q == '&') {
					system("pause");
				}
				load_screen(scr);
			}
			break;
		    case 113 :	/* ALT_F10 */
			save_screen(scr);
			hide_stat_line();
			putenv("PROMPT=TELNET-$p$g");
			if (spawnvp(P_WAIT, getenv("COMSPEC"), NULL) < 0)
				eperror(scr, "DOS EXIT", errno);
			load_screen(scr);
			break;
		    default :	/* Other ALT, CTRL Key is Mapped */
			kseq = process_fkey(input);
			if (t->sd >= 0) {
				tn_send(scr, kseq, strlen(kseq));
			} else {
				copyhname(scr, kseq, strlen(kseq));
				em_puts(scr, kseq);
			}
			if (local) {
				em_puts(scr, kseq);
				//printf("LOCAL ECHO ON");
			}
		}
	} else {
		input &= 0xff;	/* Lower byte only */
		//eprintf(3, "[%02X]\n", input);
		if (t->sd < 0) {
			if (input == '\r') {
				make_con(scr, NULL);
			} else if (input == 0x08) {
				if (t->hname_cnt > 0) {
					t->hname_cnt--;
					em_put(scr, input);
					em_put(scr, ' ');
					em_put(scr, input);
				}
			} else if ((input >= ' ') && (input <= 0x7f)) {
				copyhname(scr, (char *)&input, 1);
				em_put(scr, input);
			}
		} else {
			tn_send(scr, (unsigned char *)&input, 1);
			if (local) {
				em_put(scr, input);
				//printf("LOCAL ECHO ON");
			}
			if (input == '\r') {
				input = 0;
				tn_send(scr, (unsigned char *)&input, 1);
			}
		}
	}
}

static unsigned char conv2hex(char *s);

struct keymap {
	int extend;	/* Extended Key Number */
	char *value;	/* Symbolic Name for Definition */
	char *map;	/* What to Send for the Extended Key */
};

static struct keymap ktable[] = {
	{ 15,	"S_TAB",	NULL },
	{ 16,	"A_Q",		NULL },
	{ 17,	"A_W",		NULL },
	{ 18,	"A_E",		NULL },
	{ 19,	"A_R",		NULL },
	{ 20,	"A_T",		NULL },
	{ 21,	"A_Y",		NULL },
	{ 22,	"A_U",		NULL },
	{ 23,	"A_I",		NULL },
	{ 24,	"A_O",		NULL },
	{ 25,	"A_P",		NULL },
	{ 30,	"A_A",		NULL },
	{ 31,	"A_S",		NULL },
	{ 32,	"A_D",		NULL },
	{ 33,	"A_F",		NULL },
	{ 34,	"A_G",		NULL },
/*	{ 35,	"A_H",		NULL },  RESERVED FOR HELP */
	{ 36,	"A_J",		NULL },
	{ 37,	"A_K",		NULL },
	{ 38,	"A_L",		NULL },
	{ 44,	"A_Z",		NULL },
/*	{ 45,	"A_X",		NULL },  RESERVED BY EXIT */
	{ 46,	"A_C",		NULL },
	{ 47,	"A_V",		NULL },
	{ 48,	"A_B",		NULL },
/*	{ 49,	"A_N",		NULL },  RESERVED BY NEXT OPENED SESSION */
	{ 50,	"A_M",		NULL },
	{ 59,	"F1",		NULL },
	{ 60,	"F2",		NULL },
	{ 61,	"F3",		NULL },
	{ 62,	"F4",		NULL },
	{ 63,	"F5",		NULL },
	{ 64,	"F6",		NULL },
	{ 65,	"F7",		NULL },
	{ 66,	"F8",		NULL },
	{ 67,	"F9",		NULL },
	{ 68,	"F10",		NULL },
	{ 71,	"HOME",		NULL },
	{ 72,	"UP",		NULL },
	{ 73,	"PGUP",		NULL },
	{ 75,	"LEFT",		NULL },
	{ 77,	"RIGHT",	NULL },
	{ 79,	"END",		NULL },
	{ 80,	"DOWN",		NULL },
	{ 81,	"PGDN",		NULL },
	{ 82,	"INS",		NULL },
	{ 83,	"DEL",		NULL },
	{ 84,	"S_F1",		NULL },
	{ 85,	"S_F2",		NULL },
	{ 86,	"S_F3",		NULL },
	{ 87,	"S_F4",		NULL },
	{ 88,	"S_F5",		NULL },
	{ 89,	"S_F6",		NULL },
	{ 90,	"S_F7",		NULL },
	{ 91,	"S_F8",		NULL },
	{ 92,	"S_F9",		NULL },
	{ 93,	"S_F10",	NULL },
	{ 94,	"C_F1",		NULL },
	{ 95,	"C_F2",		NULL },
	{ 96,	"C_F3",		NULL },
	{ 97,	"C_F4",		NULL },
	{ 98,	"C_F5",		NULL },
	{ 99,	"C_F6",		NULL },
	{ 100,	"C_F7",		NULL },
	{ 101,	"C_F8",		NULL },
	{ 102,	"C_F9",		NULL },
	{ 103,	"C_F10",	NULL },
/*	{ 104,	"A_F1",		NULL },
	{ 105,	"A_F2",		NULL },
	{ 106,	"A_F3",		NULL },
	{ 107,	"A_F4",		NULL },
	{ 108,	"A_F5",		NULL },
	{ 109,	"A_F6",		NULL },
	{ 110,	"A_F7",		NULL }, : Resreved For Internal Use */
/*	{ 111,	"A_F8",		NULL }, */
	{ 112,	"A_F9",		NULL },
/*	{ 113,	"A_F10",	NULL }, : Reserved For Internal Use */
	{ 115,	"C_LEFT",	NULL },
	{ 116,	"C_RIGHT",	NULL },
	{ 117,	"C_END",	NULL },
	{ 118,	"C_PGDN",	NULL },
	{ 119,	"C_HOME",	NULL },
	{ 120,	"A_1",		NULL },
	{ 121,	"A_2",		NULL },
	{ 122,	"A_3",		NULL },
	{ 123,	"A_4",		NULL },
	{ 124,	"A_5",		NULL },
	{ 125,	"A_6",		NULL },
	{ 126,	"A_7",		NULL },
	{ 127,	"A_8",		NULL },
	{ 128,	"A_9",		NULL },
	{ 129,	"A_0",		NULL },
	{ 132,	"C_PGUP",	NULL },
#ifdef MOREKEY
	{ 133,	"F11",		NULL },
	{ 134,	"F12",		NULL },
	{ 135,	"S_F11",	NULL },
	{ 136,	"S_F12",	NULL },
	{ 137,	"C_F11",	NULL },
	{ 138,	"C_F12",	NULL },
	{ 139,	"A_F11",	NULL },
	{ 140,	"A_F12",	NULL },
	{ 141,	"C_UP",		NULL },
	{ 145,	"C_DOWN",	NULL },
	{ 146,	"C_INS",	NULL },
	{ 147,	"C_DEL",	NULL },
	{ 151,	"A_HOME",	NULL },
	{ 152,	"A_UP",		NULL },
	{ 153,	"A_PGUP",	NULL },
	{ 155,	"A_LEFT",	NULL },
	{ 156,	"A_RIGHT",	NULL },
	{ 159,	"A_END",	NULL },
	{ 160,	"A_DOWN",	NULL },
	{ 161,	"A_PGDN",	NULL },
	{ 162,	"A_INS",	NULL },
	{ 163,	"A_DEL",	NULL },
#endif
	{ 0,	NULL,		NULL }
};

static char mapbuf[4000];

static
char *
process_fkey(int key)
{
	int i;
	register struct keymap *kp = ktable;
	while (kp->extend) {
		if (key == kp->extend)
			if (kp->map)
				return(kp->map);
			else
				break;
		kp++;
	}
	return("");
}

void
init_key()
{
	FILE *keydef;
	char *key, *map, *tmp;
	int count = 0, i;
	register struct keymap *kp;
	int mindex = 0;
	char line[100];
	extern char keyfile[80];

	kp = ktable;
	while (kp->extend) {
		kp->map = NULL;
		kp++;
	}
	if ((keydef = fopen("KEY.DEF", "r")) == NULL) {
		if ((keydef = fopen(keyfile, "r")) == NULL)
			return;
	}
	while (!feof(keydef)) {
		fgets(line, 99, keydef);  /* read one line */
		count++;
		if (line[0]=='#') {		/* Comment Line */
			if (line[1] == '$')
				break;
			continue;
		}
		key = strchr(line, '"');
		key++;
		tmp = strchr(key, '"');
		*tmp++ = 0;
		map = strchr(tmp, '"');
		map++;
		tmp = strchr(map, '"');
		*tmp = 0;
		if (strlen(map) <= 0)
			continue;
		/* printf("'%s' -> '%s'\n", key, map); */
		kp = ktable;
		while (kp->extend) {
			if (strcmpi(kp->value, key) == 0) {
				kp->map = mapbuf + mindex;
				while (*map) {
				   if (*map == '^') {
				      map++;
				      if (*map != '^') {
					 if (*map != 'x') {
						*map -= 64;
					 } else {
						unsigned char t;
						map++;
						t = conv2hex(map);
						map++;
						*map = t;
					 }
				      }
				   }
				   mapbuf[mindex++] = *map++;
				}
				mapbuf[mindex++] = 0;
				/* printf("'%s' -> '%s'\n", kp->value, kp->map); */
				break;
			}
			kp++;
		}
	}
	fclose(keydef);
}

static
unsigned char
conv2hex(s)
char *s;
{
	unsigned char tmp = 0;
	if (*s >= '0' && *s <= '9')
		tmp = *s - '0';
	else if (*s >= 'a' && *s <= 'f')
		tmp = *s - 'a' + 10;
	else if (*s >= 'A' && *s <= 'F')
		tmp = *s - 'A' + 10;
	s++;
	tmp *= 16;
	if (*s >= '0' && *s <= '9')
		tmp += (*s - '0');
	else if (*s >= 'a' && *s <= 'f')
		tmp += (*s - 'a' + 10);
	else if (*s >= 'A' && *s <= 'F')
		tmp += (*s - 'A' + 10);
	return(tmp);
}

void
next_screen(int scr)
{
	int screen = scr;
	int i;
	for (i = 1; i <= MAXSCREEN; i++) {
		if (TELCON[(i + scr) % MAXSCREEN].sd >= 0) {
			screen = (i + scr) % MAXSCREEN;
			break;
		}
	}
	set_screen(screen);
	focus(screen);
}
