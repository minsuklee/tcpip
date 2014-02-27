/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/


#include "imigetsr.h"

struct TCPIPCFG tcpipcfg;	// Kernel Configuration, KERNEL.C
struct snmp_object statistics;	// Kernel Statistics, KERNEL.C

// Escape Area for the former Vectors
static unsigned long original_tsr;	// Kernel I/F INT
static unsigned long original_sig;	// Signal I/F
int signal_int;				// Signalling Interrupt
int kernel_int = 0x7E;			// Kernel I/F Interrupt

int num_socket = 4;			// Number of socket

int if_isr();	// Interface Interrupt Service Routine
int sigisr();	// Signalling Interrupt Stub Routine

int
main()
{
	extern unsigned char __eop;
	extern char i_sign;
	int argv, arglen, tmp;
	unsigned end_of_tsr;
	unsigned char *day = __DATE__;

	day[11] = '$';
	dos_print("TCP/IP Kernel,  Version 1.00,  $");
	dos_print(day);
	dos_print(".\n\r(C) Copyright 1998,  Imageland Inc.,  All Rights Reserved.\n\r$");

	tmp = find_signature(&i_sign, TSR_SIGLEN, 0x60, 0x7E);
	if (tmp) {
		error_print("Another TCP/IP is Running at 0x$");
		printhex(tmp);
		dos_print(".\r$");
		return;
	}

// Arguement Processing

	argv = 0x0080;		// DS:80h = Argument Length
	arglen = *(char *)argv++;
	if (skip_blank(&argv, &arglen) < 0) 	// Set as Default
		goto arg_ok;
	if ((*(char *)argv != '0') || ((*(char *)(argv + 1) != 'x') && (*(char *)(argv + 1) != 'X'))) {
		goto check_numsock;
	} else {
		argv += 2; arglen -= 2;
	}
	kernel_int = get_arg(&argv, &arglen, 16);
	if ((kernel_int < 0x60) || (kernel_int > 0x7e)) {
usage:		dos_print("Usage: TCPIP sw_int(0x60-0x7E) no_of_channel(1-20)\n\r$");
		return;
	}
	if (skip_blank(&argv, &arglen) < 0)	// Set as Default
		goto arg_ok;
check_numsock:
	num_socket = get_arg(&argv, &arglen, 10);
	if (!num_socket)
		goto usage;
	if (num_socket > MAX_TCB)
		num_socket = MAX_TCB;
	if (skip_blank(&argv, &arglen) >= 0)
		goto usage;

arg_ok:	// Initialize System Resources

	end_of_tsr = init_buffer(&__eop);

	if (init_driver() < 0)			// Init Packet Drivers
		return;

	init_timer();

	signal_int = kernel_int + 1;
	original_tsr = VEC_ISR(kernel_int);
	original_sig = VEC_ISR(signal_int);
	VEC_OFF(kernel_int) = (unsigned)if_isr;
	VEC_SEG(kernel_int) = _CS;
	VEC_OFF(signal_int) = (unsigned)sigisr;
	VEC_SEG(signal_int) = _CS;

	dos_print("Kernel is installed at 0x$");
	printhex(kernel_int); dos_print("-$");
	printhex(signal_int); dos_print(", $");
	printdec(num_socket); dos_print(" Channels.\r\n$");

	// Make it TSR, return to COMMAND.COM

	_DX = (end_of_tsr + 15) / 16;	// Number of Paragraphs
	asm	mov	ah, 31h		// Terminate and Stay Resident
	asm	int	21h		// DOS Function Call

	// CONTROL NEVER REACH THIS LINE
	
	return 0;
}

int
_init_kernel(int flag)
{
	if (flag) {			// Unload Kernel
		if (close_timer() < 0) {
			dos_print(".\n\rError : Unload other TSR first.\n\r$");
			return(-1);
		}
		close_driver();

		VEC_OFF(kernel_int) = (unsigned)original_tsr;
		VEC_SEG(kernel_int) = (original_tsr >> 16);
		VEC_OFF(signal_int) = (unsigned)original_sig;
		VEC_SEG(signal_int) = (original_sig >> 16);

		asm	mov	ax, word ptr [02ch]	// Environment
		asm	mov	es, ax
		asm	mov	ah, 49h			// Free
		asm	int	21h

		asm	push	cs			// Kernel Code and Data
		asm	pop	es
		asm	mov	ah,49h			// Free
		asm	int	21h
	}
	return(IMIGETCP_VER);
}

void
_load_cfg(struct TCPCONFIG far *config)
{
	copy_mem(_CS, (unsigned)(&tcpipcfg),
		 FP_SEG(config), FP_OFF(config), sizeof(struct TCPIPCFG));
	init_ip_stubs();
	//rprintf("\nLOAD CONFIGURATION : MYIP:%08lX, %08lx, %08lx\n",
	//	tcpipcfg.c_myip, tcpipcfg.c_subnet, tcpipcfg.c_defgw);
}

void
_set_async(int sd, void far (* async_func)())
{
	struct TCB *tp = tcb + sd;
	tp->async_func = async_func;
}

void
event_report(struct TCB *tp, unsigned event, unsigned arg)
{
	union   REGS _imige_reg;

	_imige_reg.h.ah = tp - tcb;
	_imige_reg.h.al = event;
	_imige_reg.x.bx = arg;
	int86(signal_int, &_imige_reg, &_imige_reg);

	if (tp->async_func)
		tp->async_func(tp - tcb, event, arg);
}

void
_get_stat(int clearflag, struct snmp_object far *object)
{
	copy_mem(FP_SEG(object), FP_OFF(object), _DS, (unsigned)&statistics, sizeof(struct snmp_object));
	if (clearflag)
		clear_mem(_DS, (unsigned)&statistics, sizeof(struct snmp_object));
}

unsigned
_sock_stat(int sd, struct TCB far *stat)
{
	struct TCB *tp = tcb + sd;

	if (stat) {
		copy_mem(FP_SEG(stat), FP_OFF(stat),
			_DS, (unsigned)tp, sizeof(struct TCB));
	}
	return(data_avail(tp));
}

int
_getpointer(struct POINTERS far *ptr)
{
	extern struct BUFFER *BUF_POOL;
	extern struct TIMER TIMER_POOL[MAX_TIMER_COUNT];
	extern int num_buffer;

	if (ptr) {
		ptr->num_socket = num_socket;		// Number of Socket
		ptr->tcb = MK_FP(_DS, tcb);		// TCB base Pointer
		ptr->buffer = MK_FP(_DS, BUF_POOL);	// Buffer Pool
		ptr->timer = MK_FP(_DS, TIMER_POOL);	// Timer Pool
		ptr->no_buffer = num_buffer;		// Number of buffer
		ptr->no_timer = MAX_TIMER_COUNT;	// Number of timer
	}
	return(num_socket);
}
