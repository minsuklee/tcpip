/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

//
// IMIGE/TCP Support Routines
//
// Warning : This Code is Highly Memory Model, Compiler Option Dependent Code
//
//   Please DO USE option "-c -mt -O2 -G -f-",  check MAKEFILE

// Borland-C Manual instructs how the compiler use the registers.
// To ignore the manual to get babo's FM code, turn-on BABO_CODE switch below.
// #define BABO_CODE

#include "imigetsr.h"

// memory copy
void
copy_mem(unsigned d_s, unsigned d_o, unsigned s_s, unsigned s_o, int cnt)
{
#ifdef BABO_CODE
	asm	push	es	// We dont need to push ES, CX
	asm	push	cx	//    because ES,CX are temporal themselves.
	asm	push	si	// We dont need to push SI, DI
	asm	push	di	//    because BCC saves SI,DI automatically
#endif
	asm	push	ds
	asm	pushf
	asm	cld		// We dont need push flags before Kernel always
				// run in Interrupt Service Routine

	_ES = d_s;		// Destination in ES:DI
	_DI = d_o;
	_DS = s_s;		// Source in DS:SI
	_SI = s_o;

	_CX = cnt;
	asm	shr	cx, 1		// CX = CNT / 2;
	if (_CX)
		asm	rep	movsw	// 16 bit copy as many as possible

	if (cnt & 1) {			// last byte, if the count is odd
		asm	lodsb
		asm	stosb
	}

	asm	popf
	asm	pop	ds
#ifdef BABO_CODE
	asm	pop	di
	asm	pop	si
	asm	pop	cx
	asm	pop	es
#endif
}

// Ethernet Address Copy (6 byte)
void
copy_hw_addr(void *dst, void *src)
{
#ifdef BABO_CODE
	asm	push	es	// We dont need to push ES, CX
	asm	push	cx	//    because ES,CX are temporal themselves.
	asm	push	si	// We dont need to push SI, DI
	asm	push	di	//    because BCC saves SI,DI automatically
#endif
	asm	pushf
	asm	cld
	_AX = _DS;
	_ES = _AX;

	_SI = (unsigned)src;
	_DI = (unsigned)dst;
	_CX = 3;		// 6 byte

	asm	rep	movsw

	asm	sti
	asm	popf
#ifdef BABO_CODE
	asm	pop	di
	asm	pop	si
	asm	pop	cx
	asm	pop	es
#endif
}

// fill it zero
void
clear_mem(unsigned seg, unsigned ptr, int cnt)
{
#ifdef BABO_CODE
	asm	push	es	// We dont need to push ES, CX, DI
	asm	push	cx	//    because ES,CX are temporal themselves.
	asm	push	di	//    because BCC saves DI automatically
#endif
	asm	pushf
	_ES = seg;
	_DI = ptr;	// Destination in ES:DI
	_CX = cnt / 2;

	asm	cld
	asm	xor	ax, ax
	asm	rep	stosw	// set 16 bit zero as many as possible

	if (cnt & 1) {		// last byte, if the count is odd
		asm	stosb
	}

	asm	popf
#ifdef BABO_CODE
	asm	pop	di
	asm	pop	cx
	asm	pop	es
#endif
}

void
dos_print(char *str)
{
	asm	mov	dx, str
	asm	mov	ah,9
	asm	int	21h
}

static
void
prc(char c)
{
	asm	mov	dl, c
	asm	mov	ah, 2
	asm	int	21h
}

static char digit[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

void
printhex(unsigned char value)
{
	prc(digit[(value & 0xF0) >> 4]);
	prc(digit[value & 0x0F]);
}

void
printdec(unsigned value)
{
	unsigned tmp, base = 10000, on = 0;

	while (base) {
		tmp = value / base;
		if (tmp) {
			on = 1;
			prc(digit[tmp]);
		} else if (on) {
			prc('0');
		}
		value = value % base;
		base /= 10;
	}
}

void
error_print(char *str)
{
	dos_print("Error : $");
	dos_print(str);			// str must end with '$'
}

unsigned short
get_arg(int *argv, int *arglen, int base)
{
	char *argp = (char *)*argv;
 	int value = 0;

	while (*arglen) {
		if ((*argp >= '0') && (*argp <= '9')) {
			value = value * base + (*argp - '0');
			goto next_char;
		} else if ((base == 16) && (*argp >= 'a') && (*argp <= 'f')) {
			value = value * 16 + (*argp - 'a' + 10);
			goto next_char;
		} else if ((base == 16) && (*argp >= 'A') && (*argp <= 'F')) {
			value = value * 16 + (*argp - 'A' + 10);
next_char:		argp++;
			(*arglen)--;
		} else {
			break;
		}
	}
	*argv = (int)argp;
	return(value);
}

int
skip_blank(int *argv, int *arglen)
{
	char *argp = (char *)*argv;

	while (*arglen) {
		if (*argp == ' ') {
			argp++;
			(*arglen)--;
		} else {
			break;
		}
	}
	*argv = (int)argp;
	if (*arglen == 0)
		return(-1);
	return(0);
}

int
splnet()
{
	#define	INTR_FLAG	0x0200

	asm	pushf
	disable_interrupt();
	asm	pop	ax
	return(_AX & INTR_FLAG);	// Return Non Zero if Enabled
}

void
splx(int spl)
{
	if (spl)			// Former state is "Enable"
		enable_interrupt();
}

int
find_signature(char *sign_str, int sign_len, int v_start, int v_end)
{
	int i, j;
	char far *tsr_str;
	char *mysign;

	for (i = v_start; i <= v_end; i++) {
		tsr_str = (char far *)(*(unsigned long far *)(i * 4) + 3);
		mysign = sign_str;
		for (j = 0; j < sign_len; j++)
			if (*mysign++ != *tsr_str++)
				goto next_vec;
		return(i);
next_vec:	;
	}
	return(0);
}

// Checksum Packet (length must higher that 1)

unsigned short
checksum(void *packet, int length)
{
#ifdef BABO_CODE
	asm	push	cx
	asm	push	si
#endif
	asm	pushf

	_SI = (unsigned)packet;		// Start with Header
	asm	mov	cx, length
	asm	xor	bx, bx		// Clear SUM
	asm	shr	cx, 1		// Adjust to word count
	asm	clc
	asm	cld
chksum:
	asm	lodsw			// Read next word
	asm	adc	bx, ax		// Add to CHKSUM
	asm	loop	chksum		// while CX > 0
	asm	adc	bx,0		// Add the last carry

last_byte:
	asm	mov	cx, length	// Check id dctr is even number
	asm	and	cx, 1
	asm	jz	negate

	asm	mov	al, [si]	// Read last byte
	asm	xor	ah, ah
	asm	add	bx, ax		// Add it
	asm	adc	bx, 0		// Add possible Carry

negate:
	asm	not	bx		// Take the 1's-complement

	asm	popf
#ifdef BABO_CODE
	asm	pop	si
	asm	pop	cx
#endif
	return(_BX);
}

#ifdef NO_OPTIMIZE
// Swap Routine should optimized into assembly code
//
unsigned short
bswap(unsigned short input)
{
	return((input / 256) + (input % 256) * 256);
}

unsigned long
lswap(unsigned long input)
{
	return( (input / 0x1000000L) +
		((input / 0x10000L) % 0x100L) * 0x100L +
		((input / 0x100L) % 0x100L) * 0x10000L +
		((input % 0x100) * 0x1000000L));
}
#endif
