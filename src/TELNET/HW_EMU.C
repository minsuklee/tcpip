/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <dos.h>
#include <conio.h>
#include <mem.h>

#include "telnet.h"
#include "tn.h"

#define LINESIZE	(MAXCOL * 2)

static short screen;
union REGS	Register;

void
hw_init()
{

	Register.h.ah = 0x0f;
	int86(0x10, &Register, &Register);

	if (Register.h.al == 7) {
		screen = 0xb000;	/* Mono */
	} else {
		screen = 0xb800;	/* Color */
	}

	Register.h.ah = 0x0;	/* Func. 0 : Video SetMode */
	int86(0x10, &Register, &Register);
}

void
hw_close()
{
	set_cursor(0, MAXROW);
}

void
set_cursor(int x, int y)
{
	Register.h.bh = 0;	/* Display Page */
	Register.h.dh = y;	/* Row # */
	Register.h.dl = x;	/* Column */
	Register.h.ah = 0x2;	/* Func. 2 : Set Cursor Position */
	int86(0x10, &Register, &Register);
}

void
hide_cursor()
{
	Register.h.bh = 0;	/* Display Page */
	Register.h.dh = 25;	/* Row # */
	Register.h.dl = 80;	/* Column */
	Register.h.ah = 0x2;	/* Func. 2 : Set Cursor Position */
	int86(0x10, &Register, &Register);
}

void
display_screen(char *base)
{
	movedata(FP_SEG(base), FP_OFF(base), screen, 0, MAXROW * MAXCOL * 2);
}

void
hw_put_char(int x, int y, int ch, int mode)
{
	poke(screen, y * LINESIZE + x * 2, mode * 256 + ch);
}

void
hw_scroll_up(int top, int bottom, int n)
{
	char *src, *dst;

	src = MK_FP(screen, (top + n) * MAXCOL * 2);
	dst = MK_FP(screen, top * MAXCOL * 2);
	movmem(src, dst, (bottom - top - n + 1) * MAXCOL * 2);
}

void
hw_scroll_down(int top, int bottom, int n)
{
	char *src, *dst;

	src = MK_FP(screen, top * MAXCOL * 2);
	dst = MK_FP(screen, (top + n) * MAXCOL * 2);
	movmem(src, dst, (bottom - top - n + 1) * MAXCOL * 2);
} 

void
hw_blank_line(int line, int start, int end)
{
	int i;
	short far *tmp;

	tmp = MK_FP(screen, line * MAXCOL * 2 + start * 2);
	for (i = start; i <= end; i++)
		*tmp++ = 0x0720;
}

void
hw_put_stat(char *str, char *mode)
{
	int i;
	char far *tmp = MK_FP(screen, MAXROW * MAXCOL *2);

	for (i = 0; i < MAXCOL; i++) {
		*tmp++ = *str++;
		*tmp++ = *mode++;
	}
}
