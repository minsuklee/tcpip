/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

#include "imigetsr.h"

#define BUFFER_PER_SOCKET	2

#define MIN_BUFCOUNT	8
//#define MIN_BUFCOUNT	15

struct TCB *tcb;	// tcb Structure

struct BUFFER *DEF_RX_POOL;	// Default DEF_BUFISZE Bytes Buffer Pool for RX
struct BUFFER *DEF_TX_POOL;	// Default DEF_BUFSIZE Bytes Buffer Pool for TX
struct BUFFER *BUF_POOL;	// MAX_BUFSIZE buffer pool to share

int MAX_RX_ALLOCATE, MAX_TX_ALLOCATE;	// Max buffer count for each socket
int num_buffer;				// Total Shared Buffer count
static unsigned buf_count;		// Remained Empty Buffer
unsigned remained_bufsize;

unsigned
init_buffer(unsigned char *start)
{
	unsigned i, allocsize, tmp, MAX_BUFCOUNT;
	struct BUFFER *bp;

	tcb = (struct TCB *)start;
	allocsize = sizeof(struct TCB) * num_socket;
	//rprintf("TCB=0x%04X(%d x %d => 0x%04X bytes)\n", tcb, sizeof(struct TCB), num_socket, allocsize);

	// WARNING DO NOT REORDER NEXT LINES !!!

	DEF_RX_POOL = (struct BUFFER *)(start + allocsize);
	DEF_TX_POOL = DEF_RX_POOL + num_socket;
	BUF_POOL = DEF_TX_POOL + num_socket;

	// Decide Shared Buffer counter

	buf_count = BUFFER_PER_SOCKET * num_socket;
	if (buf_count < MIN_BUFCOUNT) {
		buf_count = MIN_BUFCOUNT;
	} else {
		tmp = (unsigned)start + allocsize +
		      2 * num_socket * (DEF_BUFSIZE + sizeof(struct BUFFER));
		MAX_BUFCOUNT = (65535 - tmp) / (sizeof(struct BUFFER) + MAX_BUFSIZE);
		if (buf_count > MAX_BUFCOUNT)
			buf_count = MAX_BUFCOUNT;
	}
	num_buffer = buf_count;
	remained_bufsize = MAX_BUFSIZE * buf_count;

	tmp = 2 * num_socket + num_buffer;
	allocsize += sizeof(struct BUFFER) * tmp;	// Buffer Header

	clear_mem(_DS, (unsigned)start, allocsize);	// TCB, BUF_HEADER, BUF
	start += allocsize;		// Data Buffer Start Address
	
	for (bp = DEF_RX_POOL, i = 0; i < tmp; i++, bp++) {
		bp->data = start;
		if (IS_DEF_BUF(bp)) {
			start += DEF_BUFSIZE;
		} else {
			start += MAX_BUFSIZE;
		}
	}
#ifdef PERMIT_MONOPOLY
	MAX_RX_ALLOCATE = MAX_TX_ALLOCATE = buf_count;
#else
	//MAX_RX_ALLOCATE = MAX_TX_ALLOCATE = buf_count / 2;
	MAX_RX_ALLOCATE = MAX_TX_ALLOCATE = 5;
#endif

//#define BUF_DEBUG
#ifdef BUF_DEBUG
	rprintf("Total Buffer Allocated = %d(def) + %d(max) = %d\n", 2 * num_socket, num_buffer, tmp);
	rprintf("DS = %04X, TCB = %04X, DEF_RX = %04X, DEF_TX = %04X, POOL = %04X\n", _DS, tcb, DEF_RX_POOL, DEF_TX_POOL, BUF_POOL);
	for (bp = DEF_RX_POOL, i = 0; i < tmp; i++, bp++) {
		rprintf("%d:%04X(%04X) ", IS_DEF_BUF(bp), bp, bp->data);
		if ((i == num_socket - 1) || (i == 2 * num_socket - 1)) rprintf("\n");
	}
	rprintf("\n END_OF_DATA = %04X:%04X(%u bytes)", _DS, start, start);
#endif
	return((unsigned)start);
}

unsigned
data_avail(struct TCB *tp)
{
	struct BUFFER *rbp;
	unsigned count = 0;

	if (IP_STUB(tp)->transport) {
		rbp = tp->rx;
		while (rbp) {
			count += rbp->r_count;
			rbp = rbp->next;
		}
	}
	return(count);
}

#ifdef TXLENCHECK
unsigned
txlength(struct TCB *tp)
{
	struct BUFFER *rbp;
	unsigned count = 0;

	if (IP_STUB(tp)->transport) {
		rbp = tp->tx;
		while (rbp) {
			count += rbp->r_count;
			rbp = rbp->next;
		}
	}
	return(count);
}
#endif

static int lastsearch = 0;

struct BUFFER *
get_buffer(struct TCB *tp, int size, int type)
{
	int i = 0;
	struct BUFFER *nbp;

	if (size <= DEF_BUFSIZE) {
get_small:	if (type == RX_BUFFER) {
			nbp = DEF_RX_POOL + (tp - tcb);
		} else {
			nbp = DEF_TX_POOL + (tp - tcb);
		}
		if (nbp->buffer_type == FREE_BUFFER) {
			nbp->buffer_type = type;
			//rprintf("[g");
			goto allocated;
		}
	}
	if (buf_count == 0) {
no_buffer:	//rprintf("[NO BUFFER]");
		if (i)
			return(NULL);
		i++;				// Assign Small Buffer
		goto get_small;
	}
	for (i = 0; i < num_buffer; i++) {
		lastsearch = (lastsearch + 1) % num_buffer;
		nbp = BUF_POOL + lastsearch;
		if (nbp->buffer_type == FREE_BUFFER) {
			nbp->buffer_type = type;
			buf_count--;
			remained_bufsize -= MAX_BUFSIZE;
			//rprintf("[G");
allocated:		nbp->r_start = nbp->r_count = 0;
			nbp->next = NULL;
			//rprintf(":%04X(%d):%04X]", nbp, size, nbp->data);
			return(nbp);
		}
	}
	//rprintf("[FATAL ERROR IN BUFFER MANAGE]");
	return(NULL);
}

void
free_buffer(struct BUFFER *buffer)
{
	buffer->buffer_type = FREE_BUFFER;
	if (!IS_DEF_BUF(buffer)) {
		buf_count++;
		remained_bufsize += MAX_BUFSIZE;
		//rprintf("[F");
	} else {
		//rprintf("[f");
	}
	//rprintf("(%d):%04X:%04X]", buf_count, buffer, buffer->data);
}

void
free_list(struct BUFFER *buffer)
{
	struct BUFFER    *tmp;

	while (buffer) {
		tmp = buffer->next;
		free_buffer(buffer);
		buffer = tmp;
	}
}
