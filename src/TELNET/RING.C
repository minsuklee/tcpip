/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 /*
 *	ring: program to make sound for ASCII BEL code
 */
#include <dos.h>

#define	PORTB	0x61	/* PC address for port that gates speaker */
#define	TIMER2	0x42	/* PC address for timer that drives speaker */
#define	TIMCTL	0x43	/* PC address for timer load control register */
#define	TIMCW	0xB6	/* Control byte for: timer 2, load lsb and msb, */
			/* output square wave, count in binary. */
short chirpf = 533;	/* Start chirp at 1487 Hz (F'') */
short chirps = 25;	/* Use 25 different pitches */
short chirpl = 1000;	/* Length of any one pitch, 10 ms */
short chirpd = -24;	/* Decrement for each pitch */

void
ring()
{
	short	divisor;	/* Current divisor */
	char	portcb;		/* Place to hold old port control byte. */
	char	oldval;
	char	temp;
	int	count;
	int	chirps_t;

	divisor = chirpf; 	/* initialize clock stuffer */

	outp(TIMCTL, TIMCW); 	/* set timer control register. */

	/* Make sure timer running when we first start the loudspeaker */
	outp(TIMER2, divisor & 0xff);
	outp(TIMER2, (divisor>>8) & 0xff);

	oldval = inp(PORTB);
	outp(PORTB, oldval | 3 );	/* turn bell on */

	chirps_t = chirps;
	while( chirps_t ) {
		count = chirpl;
		if( count ) while( count-- );
#ifdef	XXX

		temp = (char)divisor;
		temp += chirpd;
		outp(TIMER2, temp & 0xff);
		outp(TIMER2, (temp>>8) & 0xff);
#endif
		chirps_t--;
	}
	outp( PORTB, oldval);
}
