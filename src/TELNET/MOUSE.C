/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdio.h>
#include <dos.h>
#include <bios.h>

#include "telnet.h"
#include "tn.h"

static void read_mouse(int *row, int *col, int *button);
static void mouse_cursor_on(), mouse_cursor_off();
static void mouse_hrange(int min, int max), mouse_vrange(int min, int max);

#define WORDTICK 3

/*
	return	mode == 0 : cancel cut operation
			1 : Normal Cut from (srow, scol) thru (erow, ecol)
			2 : Word Cut at srow, scol
*/
void
cut_it()
{
	int srow, scol, erow, ecol;
	int button;
	unsigned long ontime, offtime;
	int first = 1;

	undraw_cut();
	while (1) {
		if (_kbhit())
			goto failed;
		read_mouse(&srow, &scol, &button);
		srow /= 8; scol /= 8;
		if (srow >= MAXROW)
			srow = MAXROW-1;
		if (scol >= MAXCOL)
			scol = MAXCOL-1;
		erow = srow; ecol = scol;
		draw_cut(first, srow, scol, erow, ecol);
		first = 0;
		ontime = biostime(0, 0L);
		if (button & 0x01) {
			//printf("ON  row:%d col:%d T:%ld\n", srow, scol, ontime);
			while (1) {
				if (_kbhit())
					goto failed;
				read_mouse(&erow, &ecol, &button);
				offtime = biostime(0,0L) - ontime;
				erow /= 8; ecol /= 8;
				if (erow >= MAXROW)
					erow = MAXROW-1;
				if (ecol >= MAXCOL)
					ecol = MAXCOL-1;
				draw_cut(first, srow, scol, erow, ecol);
				if (!(button & 0x01)) {
					//printf("OFF row:%d col:%d time:%ld\n", erow, ecol, offtime);
					emu_cut(srow, scol, erow, ecol,	offtime > WORDTICK);
					goto leave;
				}
				if (button & 0x6)
					goto failed;
			}
		} else if (button & 0x6) {
			goto failed;
		}
	}
failed: undraw_cut();
leave:	;
}

int
check_click(int but)
{
	int row, col, button;

	read_mouse(&row, &col, &button);
	if (button & but) {
		while (1) {
			read_mouse(&row, &col, &button);
			if (!(button & but)) {
				break;
			}
		}
		return(1);
	} else
		return(0);
}

/*                           mouse_status()
      This function determines whether the mouse driver is installed
   and how many buttons the mouse has. The ax is loaded with the
   function number (0x00), and interrupt 0x33 is called.

   Argument list:    int buttons      pointer to the number of mouse
                                      buttons available
   Return value:     int              nonzero if the mouse is installed;
                                      0 if not. Note that after the call
                                      to int86(), the bx holds the number
                                      of mouse buttons.			*/
int
mouse_init(int *buttons)
{
   union REGS ireg;

   ireg.x.ax = 0x00;                 /* Function 0x00--mouse status   */
   int86(0x33, &ireg, &ireg);        /* bx is number of mouse buttons */
   if (buttons)
	*buttons = ireg.x.bx;
   if (ireg.x.ax) {
	mouse_hrange(0, 639);
	mouse_vrange(0, 191);
	return(1);
   } else
	return(0);
}

/*                          mouse_cursor_on()
      This function enables you to turn the mouse cursor on. The
   function maintains an internal counter that determines whether the
   cursor is on; if the counter is 0, the cursor is on. By calling this
   function and function 0x02, you can turn the mouse cursor on and off. */

void
mouse_cursor_on(void)
{
   union REGS ireg;

   ireg.x.ax = 0x01;                     /* Toggle cursor on          */
   int86(0x33, &ireg, &ireg);
}

/*                         mouse_cursor_off()
      This function enables you to turn the mouse cursor off. The
   function decrements the internal cursor counter. You should alternate
   calls to mouse_cursor_on() and mouse_cursor_off() for things to work
   properly.								*/

void
mouse_cursor_off(void)
{
   union REGS ireg;

   ireg.x.ax = 0x02;                     /* Toggle cursor off         */
   int86(0x33, &ireg, &ireg);
}

/*                            read_mouse()
      This function enables you to read the mouse's position and status.
   On return from the interrupt, the following information is available:

         ireg.x.bx  =  button status, where bits 0 and 1 are for the
                       left and right buttons. If the button is pressed,
                       the bit is 1. If there is a middle button, it
                       uses bit 2.
         ireg.x.cx  =  horizontal cursor position
         ireg.x.dx  =  vertical cursor position

      Therefore, the return values in ireg provide the necessary
   information.

   Argument list:    int *row      pointer to the row position of
                                   mouse cursor
                     int *col      pointer to the column position of
                                   mouse cursor
                     int *button   pointer to return the button status	*/
void
read_mouse(int *row, int *col, int *button)
{
   union REGS ireg;

   ireg.x.ax = 0x03;                /* Read mouse position and status */
   int86(0x33, &ireg, &ireg);
   *button = ireg.x.bx;
   *col = ireg.x.cx;
   *row = ireg.x.dx;
}

/*                            mouse_press()
      This function enables you to check which mouse button was pressed
   and the mouse's row-column position when the press occurred. The
   button passed in is the one that is checked.

   Argument list:    int row      the new row position
                     int col      the new column position
                     int button   button to check
   Return value:     int          the button status			*/

int
mouse_press(int *row, int *col, int button)
{
   union REGS ireg;

   ireg.x.ax = 0x05;              /* Function to get mouse data       */
   ireg.x.bx = button;            /* Update only on this button press */
   int86(0x33, &ireg, &ireg);
   *col = ireg.x.cx;
   *row = ireg.x.dx;
   return (ireg.x.ax);            /* Return button status             */
}

/*                           mouse_release()
      This function enables you to see whether the mouse button has been
   released. The row and column values are those that existed when the
   button was released.

      Although it is not included in this function, ireg.x.bx holds the
   number of releases for the button after the interrupt. This counter
   is set to zero after the call.

   Argument list:    int row      the new row position
                     int col      the new column position
                     int button   the button to check
   Return value:     int          the button status: 1 if the button is
                                  pressed, 0 if not			*/
int
mouse_release(int *row, int *col, int button)
{
   union REGS ireg;

   ireg.x.ax = 0x06;                    /* Function to get mouse data */
   ireg.x.bx = button;                  /* The button to inspect      */
   int86(0x33, &ireg, &ireg);
   *col = ireg.x.cx;
   *row = ireg.x.dx;
   return (ireg.x.ax);                  /* Return button status       */
}

/*                           mouse_vrange_()
      This function enables you to set the mouse max-min values for the
   row coordinates. The range permitted falls within 0, 200.

   Argument list:    int min      the min row position
                     int max      the max row position			*/
void
mouse_vrange(int min, int max)
{
   union REGS ireg;

   ireg.x.ax = 0x08;                /* Function to set vertical range */
   ireg.x.cx = min;
   ireg.x.dx = max;
   int86(0x33, &ireg, &ireg);
}

/*                           mouse_hrange_()
      This function enables you to set the mouse max-min values for the
   column coordinates. The range permitted must fall within 0-640.

   Argument list:    int min      the min column position
                     int max      the max column position		*/
void
mouse_hrange(int min, int max)
{
   union REGS ireg;

   ireg.x.ax = 0x07;              /* Function to set horizontal range */
   ireg.x.cx = min;
   ireg.x.dx = max;
   int86(0x33, &ireg, &ireg);
}

/*                             set_mouse()
      This function enables you to set the mouse position. The new
   values must fall within the row-column ranges that have been set.
   When in the text mode, values are rounded to the nearest values
   permitted.

   Argument list:    int row      the new row position
                     int col      the new column position		*/
void
set_mouse(int row, int col)
{
   union REGS ireg;

   ireg.x.ax = 0x04;                /* Function to set mouse position */
   ireg.x.cx = col;
   ireg.x.dx = row;
   int86(0x33, &ireg, &ireg);
}

/*                          mouse_cursor_t()
      This function enables you to redefine the mouse cursor when in the
   text mode. Note that this function works only when in the text (not
   graphics) video mode, hence the 't' at the end of the function name.

      NOTE: If a hardware cursor is selected, the screen and mask
   arguments are interpreted to be the beginning and ending scan lines
   for the cursor.

   Argument list:    int type      the type of cursor:
                                      0 = software will define the cursor
                                      1 = hardware will define it
                     int screen    defines the cursor screen
                     int mask      defines the cursor mask		*/
void
mouse_cursor_t(int type, int screen, int mask)
{
   union REGS ireg;
   struct SREGS s;             /* CAUTION: MUST BE "SREG" FOR ECO-C88 */

   segread(&s);                          /* Read segment reg          */
   s.es = s.ds;                          /* Load es with dseg         */
   ireg.x.ax = 0x0a;                     /* Set text cursor           */
   ireg.x.bx = type;                     /* Type of cursor            */
   ireg.x.cx = (unsigned) screen;        /* Screen mask               */
   ireg.x.dx = (unsigned) mask;          /* Cursor mask               */
   int86x(0x33, &ireg, &ireg, &s);       /* Set all registers         */
}

/*                          mouse_cursor_g()
      This function enables you to redefine the mouse cursor. The
   code shown here defines an arrow to use as the mouse cursor. Note
   that this function works only when in the graphics (not text)
   video mode, hence the 'g' at the end of the function name.

      If you write out the binary bit positions for the mask[] array,
   you can see how the cursor is defined.				*/

void
mouse_cursor_g(void)
{
   static unsigned int mask[] = {
      0x3ff, 0x1ff, 0x0fff, 0x7ff, 0x3ff, 0x1ff, 0xff, 0x7f,
      0x3f, 0x1f, 0x1ff, 0x10ff, 0x30ff, 0xf87f, 0xf87f, 0xfc3f,
      0x00, 0x4000, 0x6000, 0x7000, 0x7800, 0x7c00, 0x7e00, 0x7f00,
      0x7f80, 0x78c0, 0x7c00, 0x4600, 0x0600, 0x0300, 0x300, 0x0180};
   union REGS ireg;
   struct SREGS s;             /* CAUTION: MUST BE "SREG" FOR ECO-C88 */

   segread(&s);                          /* Read segment reg          */
   s.es = s.ds;                          /* Load es with dseg         */
   ireg.x.ax = 0x09;                     /* Set graphics cursor       */
   ireg.x.bx = 0x01;                     /* Column hot spot           */
   ireg.x.cx = 0x01;                     /* Row hot spot              */
   ireg.x.dx = (unsigned) mask;          /* Pointer to mask           */
   int86x(0x33, &ireg, &ireg, &s);       /* Set all registers         */
}

/*                          mouse_counters()
      This function enables you to read the mouse row-col counters.
   Each unit represents about .01 inch and has the range of a signed
   int (about plus/minus 32000).

      Note that the values returned represent the changes in the
   counts since the last call.

   Argument list:    int *row      pointer to the row count
                     int *col      pointer to the column count		*/
void
mouse_counters(int *row, int *col)
{
   union REGS ireg;

   ireg.x.ax = 0x0b;                /* Function to read motion counts */
   int86(0x33, &ireg, &ireg);
   *col = ireg.x.cx;
   *row = ireg.x.dx;
}
