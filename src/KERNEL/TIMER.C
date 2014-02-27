/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

// If you want to turn off timer interrupt, comment out next switch
#define ENABLE_TIMER_SERVICE

#define TIMER_USED	0x08
#define PRE_SCALER	(TICK_TO_SEC / ONE_SECOND)	// Scaler for TICK

unsigned long original_tmr;	// Former Timer Service Routine
int tmrisr();			// Timer ISR in isrs.asm

struct TIMER TIMER_POOL[MAX_TIMER_COUNT];

static int prescaler = PRE_SCALER;
unsigned long ticknow = 0;
tcp_seq random_iss;
unsigned Tick_Count = 0;

void
init_timer()
{
	struct TIMER *tp;
	int i;

	// Timer Structure Initialize
	for (tp = TIMER_POOL, i = 0; i < MAX_TIMER_COUNT; i++, tp++)
		tp->tick = 0;			// 0 means Empty

	// Chain Timer Interrupt
#ifdef ENABLE_TIMER_SERVICE
	disable_interrupt();
	original_tmr = VEC_ISR(TIMER_USED);
	VEC_OFF(TIMER_USED) = (unsigned)tmrisr;
	VEC_SEG(TIMER_USED) = _CS;
	enable_interrupt();
#endif
	// Start Initial Sequence Number from System Time Tick
	random_iss = (tcp_seq)*(unsigned *)MK_FP(0, 0x46C);
}

// Restore Original Timer Interrupt Vector

int
close_timer()
{
#ifdef ENABLE_TIMER_SERVICE
	if (VEC_ISR(TIMER_USED) != (unsigned long)MK_FP(_CS, tmrisr))
		return(-1);
	disable_interrupt();
	VEC_ISR(TIMER_USED) = original_tmr;
	enable_interrupt();
#endif
	return(0);
}

void
tmr_service()
{
	extern unsigned long t_stk;
	struct TCB *tp;
	int i;

	//rprintf("+");
	if (--prescaler)
		goto leave;
	prescaler = PRE_SCALER;

	//rprintf("(%08lx:", t_stk);
	ticknow++;
	for (tp = tcb, i = 0; i < num_socket; i++, tp++) {
		if (!(IP_STUB(tp)->transport))
			continue;
		if ((tp->status & TCB_ARP_ING) && (--(tp->arp_retrytimer) == 0)) {
			timedout_arp(tp);
			continue;
		}

		if (IP_STUB(tp)->transport == IPPROTO_TCP) {
			// run TCP timer if TCP is not in TCPS_CLOSED state
			check_tcp_timer(tp);
		} else {
			// send UDP, RAW_IP Packet if any
			// there must be a chance that ARP routine cannot send
			check_udp_send(tp);
		}
	}
	//rprintf("%08lx)", t_stk);
leave:
	Tick_Count = 0;
	;
}

struct TIMER *
get_timer(void)
{
	struct TIMER *tp;
	int i;

	for (tp = TIMER_POOL, i = 0; i < MAX_TIMER_COUNT; i++, tp++) {
		if (tp->tick == 0) {
			tp->tick = TIME_FOREVER;
			tp->next = NULL;
			return(tp);
		}
	}
	//rprintf("[FATAL ERROR = TIMER POOL EMPTY]");
	return(NULL);
}

void
clean_timer(struct TCB *tp)
{
	struct TIMER *tmp, *tmr;

	tmr = tp->ts;
	tp->ts = tp->te = NULL;		// Remove Timer from TCB
	while (tmr) {
		tmp = tmr->next;
		free_timer(tmr);
		tmr = tmp;
	}
}
