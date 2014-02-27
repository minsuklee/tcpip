/******************************************************
 * IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
 *
 *  All rights reserved.
 *  This software is under BSD license. see LICENSE.txt
 *
 *  Author : Minsuk Lee (ykhl1itj@gmail.com)
 ******************************************************/

 #define GRAPHIC_SCREEN		/* Turnon Graphic Mode */

#define MAXSCREEN	4	/* Maximum Connection */

#define GR_MAXCOL	640	/* x-axis */
#define GR_MAXROW	480	/* y-axis */

#define MAXCOL		80	/* x-axis */
#define MAXROW		24	/* y-axis */

#define SBUFSIZE	1024
#define RBUFSIZE	2048

/* one struct per screen */

struct CONNECTION {
	char	obuf[SUBOPTSIZE], *optr, *oend;
	char	opts[256], do_nt[256], wi_nt[256];

	int	sd;			/* Socket Number */
	int	constate;		/* Connection State */
	char	term[10];		/* Terminal Type */
	long	ospeed, ispeed;		/* Terminal Speed */
	int	telrcv_state;

	/* char	send_buf[SBUFSIZE];
	int	shead, stail; no send buffer */
	char	recv_buf[RBUFSIZE];
	int	rhead, rtail;	/* recv buffer */
	int	rfull;
	char	hname[30];	/* Host Name for this host */
	int	hname_cnt;

//	char	monitor;	/* Monitoring Mode beep when data arrive */
};

#define UNCONNECTED	0
#define CONNECTING	1
#define CONNECTED	2

extern struct CONNECTION TELCON[MAXSCREEN];
extern int c_screen;
extern int mouse;

/* Special Characters */

#define C_BLANK		0x20	/* SPACE */
#define C_FF		0x0C	/* FORM FEED */
#define	C_XON		0x11	/* XON */
#define	C_XOFF		0x13	/* XOFF */
#define	C_CR		0x0D	/* CARRIAGE RETURN */
#define	C_LF		0x0A	/* LINE FEED */
#define	C_ESC		0x1B	/* ESCAPE */
#define C_SESC		0x9B	/* MSB SET ESCAPE */
#define	C_BSPACE	0x08	/* BACK SPACE */
#define	C_DELETE	0x7F	/* DELETE */
#define	C_TAB		0x09	/* HTAB */
#define	C_BELL		0x07	/* BELL */
#define	C_NULL		0x00	/* NULL CHAR */

/* Attribute */

#define A_NORMAL	0x07
#define A_BOLD		0x0F
#define A_INVERSE	0x70
#define A_BLINKING	0x87

#define A_ON_BOLD		0x08
#define A_ON_BLINK		0x80

/* Functions */
void leave_pgm(void);
void closed(int screen);
int no_of_con(void);
void init_telnet(void);
void init_chan(struct CONNECTION *t);
int telrcv(int ch);
void make_con(int scr, char *s);
void close_all(void);
void tn_close(int ch);
int tn_send(int ch, unsigned char *data, int count);
int islocalecho(int ch);
int tn_recv(int ch, unsigned char *data, int count);
void copyhname(int scr, char *s, int len);
void current_status(int scr);
int ctrlc(void);
void init_key(void);
void key_input(int scr);
void next_screen(int scr);
void init_stat(void);
void hide_stat_line(void);
void clear_stat(void);
void stat_hostname(int chan, char *name, int len, int attr);
void focus(int chan);
void capturesign(int chan);
void refresh_stat(void);
void ring(void);
void eprintf(int ch, char *format, ...);
void cut_it(void);
int check_click(int but);
int mouse_init(int *buttons);
int get_kerror(void);
void set_kerm_port(int scr);
int recvsw(void);
int sendsw(void);
char *getfile(char *s);
void init_screen(void);
void emu_makedefault(int screen, int flag);
void close_screen(void);
void set_screen(int scr);
void em_puts(int scr, char *s);
void em_put(int scr, char ch);
void save_screen(int scr);
void load_screen(int scr);
unsigned _getch(void);
int _kbhit(void);
void emu_cut(int srow, int scol, int erow, int ecol, int mode);
void draw_cut(int first, int srow, int scol, int erow, int ecol);
void put_it(int sendcr);
void undraw_cut(void);
void emu_stat(int scr);
void set_cursor(int x, int y);
void eperror(int scr, char *s, int error);
