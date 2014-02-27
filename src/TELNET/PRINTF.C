/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #include <stdarg.h>
#include <stdio.h>
#include <dos.h>

#include "telnet.h"
#include "tn.h"

#define FRMWRI_BUFSIZE 134
#define put_one_char(ch, x)	em_put((ch), (x))

static char buf[FRMWRI_BUFSIZE];
static char __hex[] = "0123456789ABCDEF";

static void _formatted_write (int ch, char *format, va_list ap);

void
eprintf(int ch, char *format, ...)		/* Our main entry */
{
	va_list ap;

	va_start (ap, format);	/* Variable argument begin */
	_formatted_write (ch, format, ap);
	va_end (ap);		/* Variable argument end */
}

static
void
_formatted_write (int ch, char *format, va_list ap)
{
	char format_flag;
	int precision;
	int length, n;
	int field_width;
	char flag_char, left_adjust;
	unsigned long ulong;

	long num;
	char *buf_pointer;
	char *ptr;
	char zeropad;

	for (;;) {
		while ((format_flag = *format++) != '%') {
			if (!format_flag)
				return; /* End of Format String */
			put_one_char (ch, format_flag);
			if (format_flag == '\n')
				put_one_char (ch, '\r');
		}
		if (*format == '%') {	/* %% prints as % */
			format++;
			put_one_char(ch, '%');
			continue;
		}

		flag_char = left_adjust = 0;
		for (;;) {	/* check for leading -, + or ' 'flags  */
			if (*format == '+' || *format == ' ') {
				flag_char = *format++;
			} else if (*format == '-') {
				left_adjust++;
				format++;
			} else {
				break;
			}
		}

		if (*format == '0') {	/* this is the requested fill character */
			zeropad = 1;
			format++;
		} else {
			zeropad = 0;
		}
    
		field_width = 0;
		while (*format >= '0' && *format <= '9')
			field_width = field_width * 10 + (*format++ - '0');
    
		precision = -1;
		if (*format == 'l') {	/* Optional "l" modifier? */
			length = 1;
			format++;
		} else {
			length = 0;
		}
    
		switch (format_flag = *format++) {
		   case 'c':
			buf[0] = va_arg(ap, int);
			ptr = buf_pointer = &buf[0];
			if (buf[0])
				ptr++;
			break;

		   case 's':
			if ( !(buf_pointer = va_arg(ap,char *)) )
				buf_pointer = "(null)";
			precision = 10000;
			for (n=0; *buf_pointer++ && n < precision; n++)
				;
			ptr = --buf_pointer;
			buf_pointer -= n;
			break;

		   case 'p':
		   case 'X':
		   case 'x':
			if (format_flag == 'p')
				ulong = (long)va_arg(ap,char *);
			else if (length)
				ulong = va_arg(ap,unsigned long);
			else
				ulong = (unsigned)va_arg(ap,int);
			ptr = buf_pointer = &buf[FRMWRI_BUFSIZE - 1];
			do {
				*--buf_pointer = *(__hex + ((int) ulong & 0x0F));
			} while (ulong >>= 4);
			if (zeropad)
				precision = field_width;
loop0:			if ((int)(ptr - buf_pointer) < precision) {
				*--buf_pointer = '0';
				goto loop0;
			}
			break;

		   case 'd':
		   case 'D':
		   case 'u':
			if (length)
				num = va_arg(ap, long);
			else {
				n = va_arg(ap, int);
				if (format_flag == 'u')
					num = (unsigned) n;
				else
					num = (long) n;
			}
			if ((n = (format_flag != 'u' && num < 0)) != 0) {
				flag_char = '-';
				ulong = -num;
			} else {
				n = flag_char != 0;
				ulong = num;
			}

		        /* now convert to digits */

			ptr = buf_pointer = &buf[FRMWRI_BUFSIZE - 1];
			do {
				*--buf_pointer = (ulong % 10) + '0';
			} while (ulong /= 10);
			if (zeropad)
				precision = field_width - n;
loop1:			if ((int)(ptr - buf_pointer) < precision) {
				*--buf_pointer = '0';
				goto loop1;
			}
			break;

		   default:	/* Undefined conversion! */
			ptr = buf_pointer = "???";
			ptr += 4;
			break;
		}

		/* emittes the formatted string to "put_one_char". */
		if ( (length = (unsigned)ptr - (unsigned)buf_pointer) > field_width) {
			n = 0;
		} else {
			n = field_width - length - (flag_char != 0);
		}
  
		/* emit any leading pad characters */
		if (!left_adjust) {
			while (--n >= 0) {
				put_one_char(ch, ' ');
			}
		}
      
		/* emit flag characters (if any) */
		if (flag_char) {
			put_one_char(ch, flag_char);
		}

		/* emit the string itself */
		while (--length >= 0) {
			put_one_char(ch, *buf_pointer++);
		}
    
		/* emit trailing space characters */
		if (left_adjust)
			while (--n >= 0) {
				put_one_char(ch, ' ');
			}
	}
}
