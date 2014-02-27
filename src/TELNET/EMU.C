/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <bios.h>

#include "telnet.h"
#include "tn.h"

#define isdel(x)	((isalnum((x)) || ((unsigned char)(x) > 0xA0)))

/* VT100 Control State */

#define ST_NORMAL	0
#define ST_ESC		1	/* ESC */
#define ST_SESC		2	/* SESC */
#define ST_LBRACKET	3	/* ESC [ */
#define ST_LPAREN	4	/* ESC ( */
#define ST_QUEST	5	/* ESC [ ? */
#define ST_SHARP	6	/* ESC # */
#define ST_RPAREN	7	/* ESC ) */

#define MAXARG	8		/* Maximum Escape Sequence Arguments */
#define MAXTAB	20		/* Maximum Tab Positions */

struct SCREEN_STRUCT {
	char text_x, text_y;	/* Text Mode Cursor Position */
	char save_x, save_y;	/* Escaped Position */
	char s_mode;		/* Escaped Text Attribute */
	char c_mode;		/* Current Text Attribute */
	char t_scr_t, t_scr_b;	/* Text Mode Scroll Region */
	char tab_cnt;
	char tab[MAXTAB];
	short tbuf[MAXROW*MAXCOL];

	char state;		/* VT100 Emulator State */
	char wrap_mode;
	char arg_cnt;
	unsigned short arg[MAXARG];	/* Arguement */

#ifdef GRAPHIC_SCREEN
	char far *gr_screen;	/* Graphic Screen Start Addres */
	int gr_x, gr_y;		/* Graphic Mode Cursor Position */
	char c_color;		/* Current Drawing Color */
	char g_scr_t, g_scr_b;	/* Graphic Mode Scroll Region */
#endif
} SCREEN[MAXSCREEN];

static struct SCREEN_STRUCT *cs;
int c_screen;		/* Current Display Screen */

static char escape_screen[MAXROW*MAXCOL*2], escape_x, escape_y, escaped;

static char def_tab[10] = { 8, 16, 24, 32, 40, 48, 56, 64, 72, 80 };

//#define ERRTRACE
#ifdef ERRTRACE
#define MONITOR
FILE *fdebug;
#endif
int monitor = 0;

/* CUT BUFFER MANAGEMENT */

static int oscreen = -1;
static int osrow, oscol, oerow, oecol;

static void advance(int scr);
static void set_normal_state(struct SCREEN_STRUCT *sp, int ch);
static void linefeed(int scr);
static void captureline(int scr);
static void t_scroll_up(int scr, int top, int bottom, int n);
static void t_scroll_down(int scr, int top, int bottom, int n);
static void blank_line(int scr, int line, int start, int end);
static void put_a_char(int scr, int x, int y, unsigned char ch, int mode);
static void unimplemented(struct SCREEN_STRUCT *sp, int ch);

void hw_init(void);
void hw_close(void);
void set_cursor(int x, int y);
void hide_cursor(void);
void display_screen(char *base);
void hw_put_char(int x, int y, int ch, int mode);
void hw_scroll_up(int top, int bottom, int n);
void hw_scroll_down(int top, int bottom, int n);
void hw_blank_line(int line, int start, int end);

extern FILE *capturefile;
extern int capturescr;

void
init_screen()
{
	int i;

	memset(SCREEN, 0, sizeof(struct SCREEN_STRUCT) * MAXSCREEN);
	for (i = 0; i < MAXSCREEN; i++) {
		emu_makedefault(i, 1);
	}
	hw_init();
#ifdef ERRTRACE
	fdebug = fopen("DEBUG", "w");
	fclose(fdebug);
#endif
}

/* Flag = 1 : clear screen and cursor home */
void
emu_makedefault(int screen, int flag)
{
	int j;
	struct SCREEN_STRUCT *sp = SCREEN + screen;

	if (flag) {
		for (j = 0; j < (MAXROW*MAXCOL); j++)
			sp->tbuf[j] = A_NORMAL * 256 + C_BLANK;
		sp->text_x = sp->text_y = 0;
	}
	sp->c_mode = A_NORMAL;
	sp->t_scr_t = 0;
	sp->t_scr_b = MAXROW - 1;
	set_normal_state(sp, ' ');
	sp->tab_cnt = 10;
	memcpy(sp->tab, def_tab, 10);
	for (j = 10; j < MAXTAB; j++) {
		sp->tab[j] = MAXCOL;
	}
	sp->wrap_mode = 0;
}

void
close_screen()
{
	hw_close();
}

void
set_screen(int scr)
{
	cs = SCREEN + scr;
	display_screen((char *)(cs->tbuf));
	set_cursor(cs->text_x, cs->text_y);
	c_screen = scr;
}

void
em_puts(int scr, char *s)
{
	while (*s)
		em_put(scr, *s++);
}

void
em_put(int scr, char ch)
{
	int i, j;
	struct SCREEN_STRUCT *sp = SCREEN + scr;
	switch (sp->state) {
	   case ST_NORMAL :
		if (ch == C_ESC) {
			sp->state = ST_ESC;
			goto leave;
		}
#ifdef PROC_SESC
		else if (ch == C_SESC) {
			sp->state = ST_SESC;
			goto leave;
		}
#endif
		break;
	   case ST_ESC :
		switch (ch) {
		   case '[' :
			sp->state = ST_LBRACKET;
			for (i = 0; i < MAXARG; i++)
				sp->arg[i] = 0;
			break;
		   case '(' :
			sp->state = ST_LPAREN;
			break;
		   case ')' :
			sp->state = ST_RPAREN;
			break;
		   case '?' :
			sp->state = ST_QUEST;
			break;
		   case '#' :
			sp->state = ST_SHARP;
			break;
		   case '7' :
			sp->save_x = sp->text_x;
			sp->save_y = sp->text_y;
			sp->s_mode = sp->c_mode;
			set_normal_state(sp, ch);
			break;
		   case '8' :
			sp->text_x = sp->save_x;
			sp->text_y = sp->save_y;
			sp->c_mode = sp->s_mode;
			set_normal_state(sp, ch);
			break;
		   case 'D' :
			linefeed(scr);
			set_normal_state(sp, ch);
			break;
		   case 'M' :
			sp->text_y--;
			captureline(scr);
			if (sp->text_y < sp->t_scr_t) {
				t_scroll_down(scr, sp->t_scr_t, sp->t_scr_b, 1);
				sp->text_y = sp->t_scr_t;
			}
			set_normal_state(sp, ch);
			break;
		   case 'E' :
			linefeed(scr);
			sp->text_x = 0;
			set_normal_state(sp, ch);
			break;
		   case 'c' : /* RESET terminal */
			emu_makedefault(scr, 1);
			break;
		   case 'H' : /* Set TAB */
			for (i = 0; i < sp->tab_cnt; i++) {
				if (sp->tab[i] == sp->text_x)
					break;
				if (sp->tab[i] > sp->text_x) {
					for (j = sp->tab_cnt-1; j >= i; j--)
						sp->tab[j+1] = sp->tab[j];
					sp->tab[i] = sp->text_x;
					if (sp->tab_cnt < MAXTAB)
						sp->tab_cnt++;
					break;
				}
			}
			set_normal_state(sp, ch);
			break;
		   default :
			unimplemented(sp, ch);
			set_normal_state(sp, ch);
			break;
		}
		goto leave;
	   case ST_LBRACKET :
		if (ch >= '0' && ch <= '9') {
			sp->arg[sp->arg_cnt] *= 10;
			sp->arg[sp->arg_cnt] += ch - '0';
			goto leave;
		}
		sp->arg_cnt++;
		if (sp->arg_cnt == MAXARG)
			sp->arg_cnt = MAXARG - 1;
		switch (ch) {
		   case ';' :
			goto leave;
		   case 'A' :	/* CURSOR UP */
			if (sp->arg[0] == 0)
				sp->arg[0] = 1;
			sp->text_y -= sp->arg[0];
			captureline(scr);
			if (sp->text_y < 0)
				sp->text_y = 0;
			break;
		   case 'B' :	/* CURSOR DOWN */
			if (sp->arg[0] == 0)
				sp->arg[0] = 1;
			sp->text_y += sp->arg[0];
			captureline(scr);
			if (sp->text_y >= MAXROW)
				sp->text_y = MAXROW - 1;
			break;
		   case 'C' :	/* CURSOR RIGHT */
			if (sp->arg[0] == 0)
				sp->arg[0] = 1;
			sp->text_x += sp->arg[0];
			if (sp->text_x >= MAXCOL )
				sp->text_x = MAXCOL - 1;
			break;
		   case 'D' :	/* CURSOR LEFT */
			if (sp->arg[0] == 0)
				sp->arg[0] = 1;
			sp->text_x -= sp->arg[0];
			if (sp->text_x < 0)
				sp->text_x = 0;
			break;
		   case 'G' :
			sp->text_x = sp->arg[0] - 1;
			if (sp->text_x >= MAXCOL)
				sp->text_x = MAXCOL - 1;
			if (sp->text_x < 0)
				sp->text_x = 0;
			break;
		   case 'd' :
			captureline(scr);
			sp->text_y = sp->arg[0] - 1;
			if (sp->text_y >= MAXROW)
				sp->text_y = MAXROW - 1;
			if (sp->text_y < 0)
				sp->text_y = 0;
			break;
		   case 'E' :
			captureline(scr);
			sp->text_y += (sp->arg[0] - 1);
			if (sp->text_y >= MAXROW)
				sp->text_y = MAXROW - 1;
			sp->text_x = 0;
			break;
		   case 'F' :
			captureline(scr);
			sp->text_y -= (sp->arg[0] - 1);
			if (sp->text_y < 0)
				sp->text_y = 0;
			sp->text_x = 0;
			break;
		   case 'H' :
		   case 'f' :
			if (sp->arg[0] <= 0) sp->arg[0] = 1;
			if (sp->arg[0] > MAXROW) sp->arg[0] = MAXROW;
			if (sp->arg_cnt == 1) {
				sp->arg[1] = 1;
			} else {
				if (sp->arg[1] <= 0) sp->arg[1] = 1;
				if (sp->arg[1] > MAXCOL) sp->arg[1] = MAXCOL;
			}
			if (sp->text_y != (sp->arg[0] - 1))
				captureline(scr);
			sp->text_y = sp->arg[0] - 1;
			sp->text_x = sp->arg[1] - 1;
			break;
		   case 'K' :
			switch (sp->arg[0]) {
			   case 0 :	/* ESC [ 0 K */
				blank_line(scr, sp->text_y, sp->text_x, MAXCOL - 1);
				break;
			   case 1 :	/* ESC [ 1 K */
				blank_line(scr, sp->text_y, 0, sp->text_x - 1);
				break;
			   case 2 :	/* ESC [ 2 K */
				blank_line(scr, sp->text_y, 0, MAXCOL - 1);
				break;
			}
			break;
		   case 'J' :
			switch (sp->arg[0]) {
			   case 0 :	/* ESC [ 0 J */
				blank_line(scr, sp->text_y, sp->text_x, MAXCOL - 1);
				for (i = sp->text_y + 1; i < MAXROW; i++)
					blank_line(scr, i, 0, MAXCOL - 1);
				break;
			   case 1 :	/* ESC [ 1 J */
				for (i = 0; i < sp->text_y; i++)
					blank_line(scr, i, 0, MAXCOL - 1);
				blank_line(scr, sp->text_y, 0, sp->text_x - 1);
				break;
			   case 2 :	/* ESC [ 2 J */
				for (i = 0; i < MAXROW; i++)
					blank_line(scr, i, 0, MAXCOL - 1);
				break;
			}
			break;
		   case 'r' :
			if (sp->arg_cnt) {
				sp->t_scr_t = sp->arg[0] - 1;
				if (sp->t_scr_t < 0) sp->t_scr_t = 0;
			}
			if (sp->arg_cnt > 1) {
				sp->t_scr_b = sp->arg[1] - 1;
				if (sp->t_scr_b >= MAXROW) sp->t_scr_b = MAXROW - 1;
			}
			break;
		   case 'm' :
			if (sp->arg_cnt == 0) {
				sp->c_mode = A_NORMAL;
			} else for (i = 0; i < sp->arg_cnt; i++) {
				switch (sp->arg[i]) {
				   case 0 :
					sp->c_mode = A_NORMAL;
					break;
				   case 1 :		/* BOLD FACE */
					sp->c_mode |= A_ON_BOLD;
					break;
				   case 4:		/* UNDERSCORE */
					sp->c_mode |= A_ON_BOLD;
					break;
				   case 5 :		/* BLINKING */
					sp->c_mode |= A_ON_BLINK;
					break;
				   case 7 :		/* REVERSE */
					sp->c_mode &= 0x80;
					sp->c_mode |= A_INVERSE;
					break;
				   case 23 :		/* BOLD FACE */
					sp->c_mode &= ~A_ON_BOLD;
					break;
				   case 24:		/* UNDERSCORE */
					sp->c_mode &= ~A_ON_BOLD;
					break;
				   case 25 :		/* BLINKING */
					sp->c_mode &= ~A_ON_BLINK;
					break;
				   case 27 :		/* REVERSE */
					sp->c_mode &= 0x80;
					sp->c_mode |= A_NORMAL;
					break;
				   default :
					if (sp->arg[i] >= 30 && sp->arg[i] <= 37) {
						sp->c_mode &= 0xF8;
						sp->c_mode |= sp->arg[i] - 30;
					} else if (sp->arg[i] >= 40 && sp->arg[i] <= 47) {
						sp->c_mode &= 0x8F;
						sp->c_mode |= (sp->arg[i] - 40) * 16;
					} else
						unimplemented(sp, ch);
					break;
				}
			}
			break;
		   case '?' :
			sp->state = ST_QUEST;
			sp->arg_cnt = 0;
			goto leave;
		   case 'n' :
			if (sp->arg_cnt == 1) {
				char tmp[20];
				switch (sp->arg[0]) {
				   case 6 :
					sprintf(tmp, "%c[%d;%dR", 0x1B, sp->text_y+1, sp->text_x+1);
					break;
				   case 5 :
					sprintf(tmp, "%c[0n", 0x1B);
					break;
				   default :
					tmp[0] = 0;
					unimplemented(sp, ch);
				}
				tn_send(scr, tmp, strlen(tmp));
			} else
				unimplemented(sp, ch);
			break;
		   case 'g' :
			if (sp->arg[0] == 0) {
				for (i = 0; i < sp->tab_cnt; i++) {
					if (sp->tab[i] == sp->text_x) {
						for (j = i; j < sp->tab_cnt; j++)
							sp->tab[j] = sp->tab[j+1];
						sp->tab_cnt--;
						break;
					}
				}
			} else if (sp->arg[0] == 3) {
				for (i = 0; i < MAXTAB; i++)
					sp->tab[i] = MAXCOL;
				sp->tab_cnt = 0;
			}
			break;
		   case 'q' :
		   case 'h' :
		   case 'L' :
		   case 'c' :
		   default :
			unimplemented(sp, ch);
			break;
		}
		set_normal_state(sp, ch);
		goto leave;
	   case ST_QUEST :
		if (ch >= '0' && ch <= '9') {
			sp->arg[sp->arg_cnt] *= 10;
			sp->arg[sp->arg_cnt] += ch - '0';
			goto leave;
		}
		sp->arg_cnt++;
		if (sp->arg_cnt == MAXARG)
			sp->arg_cnt = MAXARG - 1;
		switch (sp->arg[0]) {
		   case 7 :
			sp->wrap_mode = (ch == 'h');
			break;
		   case 1 :	/* NO IMPLEMENT */
		   case 2 : 
		   case 3 : 
		   case 4 : 
		   case 5 : 
		   case 6 :
		   case 8 :
		   case 9 :
		   default :
			break;
		}
		unimplemented(sp, ch);
		set_normal_state(sp, ch);
		goto leave;
	   case ST_SHARP :
	   case ST_LPAREN :
	   case ST_RPAREN :
	   default :
		unimplemented(sp, ch);
		set_normal_state(sp, ch);
		break;
	}
#ifdef MONITOR
	if (monitor) {
		fdebug = fopen("DEBUG", "a+");
		fprintf(fdebug, "[%02X", ch);
		if ((ch >= 0x20) && (ch < 0x7f))
			fprintf(fdebug, " %c", ch);
		fprintf(fdebug, "]");
		fclose(fdebug);
	}
#endif
	switch (ch) {
	   case C_NULL :
		goto leave;
	   case C_CR :
		sp->text_x = 0;
		break;
	   case C_LF :
		linefeed(scr);
		break;
	   case C_BELL :
		ring();
		break;
	   case C_BSPACE :
		if (sp->text_x)
			sp->text_x--;
		break;
	   case C_TAB :
		for (i = 0; i < sp->tab_cnt; i++) {
			if (sp->text_x >= sp->tab[i])
				continue;
			if (sp->tab[i] < MAXCOL)
				sp->text_x = sp->tab[i];
			else
				advance(scr);
			break;
		}
		break;
	   default:
		put_a_char(scr, sp->text_x, sp->text_y, ch, sp->c_mode);
		advance(scr);
		break;
	}
leave:	if (scr == c_screen)
		set_cursor(sp->text_x, sp->text_y);
}

static
void
set_normal_state(struct SCREEN_STRUCT *sp, int ch)
{
	int i;

#ifdef MONITOR

	if (monitor == 0)
		goto doit;

	fdebug = fopen("DEBUG", "a+");
	switch (sp->state) {
		case ST_ESC :
			fprintf(fdebug, "ESC"); break;
		case ST_SESC :
			fprintf(fdebug, "SESC"); break;
		case ST_LBRACKET :
			fprintf(fdebug, "ESC ["); break;
		case ST_LPAREN :
			fprintf(fdebug, "ESC ("); break;
		case ST_QUEST :
			fprintf(fdebug, "ESC [ ?"); break;
		case ST_SHARP :
			fprintf(fdebug, "ESC #"); break;
		case ST_NORMAL :
			fprintf(fdebug, "NORMAL"); break;
		default :
			fprintf(fdebug, "ERROR"); break;
	}
	for (i = 0; i < sp->arg_cnt; i++)
		fprintf(fdebug, " %d", sp->arg[i]);
	fprintf(fdebug, " %c(%02X)\n", ch, ch);
	fclose(fdebug);
doit:
#endif
	sp->state = ST_NORMAL;
	sp->arg_cnt = 0;
	for (i = 0; i < MAXARG; i++)
		sp->arg[i] = 0;
}

static
void
advance(int scr)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	if (sp->text_x == (MAXCOL - 1)) {
		if (sp->wrap_mode) {
			sp->text_x = 0;
			linefeed(scr);
		}
	} else
		sp->text_x++;
}

void
linefeed(int scr)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	sp->text_y++;
	if (sp->text_y > sp->t_scr_b) {
		t_scroll_up(scr, sp->t_scr_t, sp->t_scr_b, 1);
		sp->text_y = sp->t_scr_b;
	}
	captureline(scr);
}

void
captureline(int scr)
{
	if (capturefile && (scr == capturescr)) {
		fputc('\n', capturefile);
	}
}

void
t_scroll_up(int scr, int top, int bottom, int n)
{
	int i;
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	if (bottom < top)	/* ERROR CONDITION */
		return;
	if ((bottom - top) < n) {
		for (i = top; i <= bottom; i++)
			blank_line(scr, i, 0, MAXCOL - 1);
		return;
	}
	memmove(sp->tbuf + top * MAXCOL,
		sp->tbuf + (top + n) * MAXCOL,
		(bottom - top - n + 1) * MAXCOL * 2);
	if (scr == oscreen) {
		osrow -= n; oerow -= n;
		if ((osrow < 0) && (oerow < 0))
			oscreen = -1;
		else {
			if (osrow < 0) {
				osrow = 0; oscol = 0;
			}
			if (oerow < 0)
				oerow = 0;
		}			
	}
	if (scr == c_screen)
		hw_scroll_up(top, bottom, n);
	for (i = (bottom - n + 1); i <= bottom; i++)
		blank_line(scr, i, 0, MAXCOL - 1);
} 

void
t_scroll_down(int scr, int top, int bottom, int n)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;
	int i;

	if (bottom < top)
		return;
	if ((bottom - top) < n) {
		for (i = top; i <= bottom; i++)
			blank_line(scr, i, 0, MAXCOL - 1);
		return;
	}
	memmove(sp->tbuf + (top + n) * MAXCOL,
		sp->tbuf + top * MAXCOL,
		(bottom - top - n + 1) * MAXCOL * 2);
	if (scr == oscreen) {
		osrow += n; oerow += n;
		if ((osrow >= MAXROW) && (oerow >= MAXROW))
			oscreen = -1;
		else {
			if (osrow >= MAXROW)
				osrow = MAXROW - 1;
			if (oerow >= MAXROW) {
				oerow = MAXROW - 1; oecol = MAXCOL - 1;
			}
		}			
	}
	if (scr == c_screen)
		hw_scroll_down(top, bottom, n);
	for (i = top; i <= (top + n - 1); i++)
		blank_line(scr, i, 0, MAXCOL - 1);
} 

void
blank_line(int scr, int line, int start, int end)
{
	int i;
	short *tmp;
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	tmp = sp->tbuf + line * MAXCOL + start;
	for (i = start; i <= end; i++)
		*tmp++ = A_NORMAL * 256 + C_BLANK;
	if (scr == c_screen)
		hw_blank_line(line, start, end);
}

void
save_screen(int scr)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	escaped = scr;
	escape_x = sp->text_x;
	escape_y = sp->text_y;

	memcpy(escape_screen, sp->tbuf, MAXROW * MAXCOL * 2);
}

void
load_screen(int scr)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	sp->text_x = escape_x;
	sp->text_y = escape_y;

	memcpy(sp->tbuf, escape_screen, MAXROW * MAXCOL * 2);
	display_screen((char *)(sp->tbuf));
	if (scr == c_screen)
		set_cursor(sp->text_x, sp->text_y);
	refresh_stat();
}

void
put_a_char(int scr, int x, int y, unsigned char ch, int mode)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	*(sp->tbuf + y * MAXCOL + x) = mode * 256 + ch;
	if (scr == c_screen)
		hw_put_char(x, y, ch, mode);
	if (capturefile && (scr == capturescr)) {
		fputc(ch, capturefile);
	}
}

char cut_buffer[MAXROW * (MAXCOL + 1)];
int cutcnt = 0;

/* CUT ALWAYS DONE on the CURRENT SCREEN */
void
emu_cut(int srow, int scol, int erow, int ecol, int mode)
{
	int i;

	if ((srow * MAXCOL + scol) > (erow * MAXCOL + ecol)) {	/* SWAP */
		i = erow; erow = srow; srow = i;
		i = ecol; ecol = scol; scol = i;
	}
	if (!mode) {	/* CUT WORD */
		erow = srow;
		if (!isdel(*(cs->tbuf + srow * MAXCOL + scol) & 0xff)) {
			ecol = scol;
			goto cutit;
		}
		for (i = (scol-1); i >= 0; i--)
			if (!isdel(*(cs->tbuf + srow * MAXCOL + i) & 0xff))
				break;
		scol = i + 1;
		for (i = scol; i < MAXCOL; i++)
			if (!isdel(*(cs->tbuf + srow * MAXCOL + i) & 0xff))
				break;
		ecol = i - 1;
	}
cutit:	draw_cut(0, srow, scol, erow, ecol);
	cutcnt = 0;
loop:	cut_buffer[cutcnt++] = *(cs->tbuf + srow * MAXCOL + scol) & 0xff;
	if ((srow != erow) || (scol != ecol)) {
		scol++;
		if (scol == MAXCOL) {
			for (i = cutcnt - 1; i >= 0; i--)
				if (cut_buffer[i] != C_BLANK)
					break;
			cutcnt = i + 1;
			cut_buffer[cutcnt++] = 0x0d;
			scol = 0;
			srow++;
		}
		goto loop;
	}
	// SHOW CUT DATA
	//printf("[");
	//for (i = 0; i < cutcnt; i++) {
	//	printf("%c", cut_buffer[i]);
	//}
	//printf("]");
}

void
draw_cut(int first, int srow, int scol, int erow, int ecol)
{
	int i;
	unsigned ch;

	if ((srow * MAXCOL + scol) > (erow * MAXCOL + ecol)) {	/* SWAP */
		i = erow; erow = srow; srow = i;
		i = ecol; ecol = scol; scol = i;
	}

	if (!first) {
		if ((osrow == srow) && (oscol == scol) &&
		    (oerow == erow) && (oecol == ecol))
			return;
	}

	undraw_cut();
	osrow = srow;
	oscol = scol;
	oerow = erow;
	oecol = ecol;
	oscreen = c_screen;

	//printf("(%d,%d - %d,%d)", srow, scol, erow, ecol);
loop:	ch = *(cs->tbuf + srow * MAXCOL + scol);
	put_a_char(c_screen, scol, srow, ch & 0xff, (ch / 256) ^ 0x77);
	if ((srow != erow) || (scol != ecol)) {
		scol++;
		if (scol == MAXCOL) {
			scol = 0;
			srow++;
		}
		goto loop;
	}
}

void
put_it(int sendcr)
{
	int sent = 0, tmp;

	//printf("[put_it called %d %d]", TELCON[c_screen].sd, cutcnt);
	if ((TELCON[c_screen].sd >= 0) && (cutcnt > 0)) {
		while (sent < cutcnt) {
			tmp = tn_send(c_screen, cut_buffer + sent, cutcnt - sent);
			if (tmp < 0)
				return;
			sent += tmp;
		}
		if (sendcr) {
			tmp = 0x0d;
			tn_send(c_screen, (unsigned char *)&tmp, 1);
		}
	}
}

void
undraw_cut()
{
	struct SCREEN_STRUCT *sp;
	unsigned ch;

	if (oscreen < 0)
		return;

	sp = SCREEN + oscreen;

	//printf("[%d,%d - %d,%d]", osrow, oscol, oerow, oecol);
eloop:	ch = *(sp->tbuf + osrow * MAXCOL + oscol);
	put_a_char(oscreen, oscol, osrow, ch & 0xff, (ch / 256) ^ 0x77);
	if ((osrow != oerow) || (oscol != oecol)) {
		oscol++;
		if (oscol == MAXCOL) {
			oscol = 0;
			osrow++;
		}
		goto eloop;
	}
	oscreen = -1;
}

unsigned
_getch()
{
	return(bioskey(0));
}

int
_kbhit()
{
	return(bioskey(1));
}

void
unimplemented(struct SCREEN_STRUCT *sp, int ch)
{
#ifdef ERRTRACE
	int i;

	fdebug = fopen("DEBUG", "a+");
	fprintf(fdebug, "UNIMPLENTED:  ");
	switch (sp->state) {
		case ST_ESC :
			fprintf(fdebug, "ESC"); break;
		case ST_SESC :
			fprintf(fdebug, "SESC"); break;
		case ST_LBRACKET :
			fprintf(fdebug, "ESC ["); break;
		case ST_LPAREN :
			fprintf(fdebug, "ESC ("); break;
		case ST_RPAREN :
			fprintf(fdebug, "ESC )"); break;
		case ST_QUEST :
			fprintf(fdebug, "ESC [ ?"); break;
		case ST_SHARP :
			fprintf(fdebug, "ESC #"); break;
		case ST_NORMAL :
			fprintf(fdebug, "NORMAL"); break;
		default :
			fprintf(fdebug, "ERROR"); break;
	}
	for (i = 0; i < sp->arg_cnt; i++)
		fprintf(fdebug, " %d", sp->arg[i]);
	fprintf(fdebug, " %c(%02X)\n", ch, ch);
	fclose(fdebug);
#endif
}

void
emu_stat(int scr)
{
	struct SCREEN_STRUCT *sp = SCREEN + scr;

	eprintf(scr, "EMU:");
	switch (sp->state) {
		case ST_ESC :
			eprintf(scr,"ST_ESC"); break;
		case ST_SESC :
			eprintf(scr,"ST_SESC"); break;
		case ST_LBRACKET :
			eprintf(scr,"ST_LBRACKET"); break;
		case ST_LPAREN :
			eprintf(scr,"ST_LPAREN"); break;
		case ST_RPAREN :
			eprintf(scr,"ST_RPAREN"); break;
		case ST_QUEST :
			eprintf(scr,"ST_QUEST"); break;
		case ST_SHARP :
			eprintf(scr,"ST_SHARP"); break;
		case ST_NORMAL :
			eprintf(scr,"ST_NORMAL"); break;
		default :
			eprintf(scr,"ST_ERROR"); break;
	}
	eprintf(scr," POS:%d,%d, S_POS:%d:%d\n",
		sp->text_x, sp->text_y, sp->save_x, sp->save_y);
	eprintf(scr,"SCRO:%d,%d", sp->t_scr_t, sp->t_scr_b);
	eprintf(scr,"  ARG:%d(%d,%d,%d,...)\n",
		sp->arg_cnt, sp->arg[0], sp->arg[1], sp->arg[2]);
}
