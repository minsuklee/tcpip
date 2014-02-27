;//  IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
;
;//=======================================================================
;//
;//	Interrupt Service Routines
;//
;//		1. Intercept and Do Environmental processing for ISR's
;//		   (Packet Driver, Timer, Kernel Interface)
;//		2. Include Stub S/W ISR for Application wakeup signalling
;//
;//			1994. 6. 14	mslee
;//
_TEXT	SEGMENT BYTE PUBLIC 'CODE'
		ENDS
_DATA	SEGMENT PARA PUBLIC 'DATA'
		ENDS
_BSS	SEGMENT WORD PUBLIC 'BSS'
		ENDS


B_TSR	macro	BUSY			; Begin Critical Section
	mov	bx, cs: in_tsr
	or	bx, bx			; Check Reentrancy
	jne	BUSY			; if busy, jump to BUSY
	inc	bx
	mov	cs: in_tsr, bx		; I'm starting Kernel Service
	endm

S_STK	macro				; Save application's Stack Pointer
	mov	word ptr cs:tsr_ss, ss
	mov	word ptr cs:tsr_sp, sp
	mov	bx, cs
	mov	ss, bx				; Change to Internal Stack
	mov	sp, offset DGROUP: k_stack	;
	endm

R_STK	macro				; Restore Stack
	mov	ss, word ptr cs: tsr_ss
	mov	sp, word ptr cs: tsr_sp
	endm

E_TSR	macro				; End Critical Section
	xor	bx, bx
	mov	cs: in_tsr, bx		; Clear re-entrant bit
	endm

	DGROUP GROUP _TEXT,_DATA,_BSS
        ASSUME  CS:_TEXT, DS:DGROUP

_TEXT	SEGMENT
	ASSUME	CS:_TEXT, DS:DGROUP

in_tsr	dw	0		; Kernel Re-Entrancy Flag
tsr_ss	dw	0		; Application SS escape
tsr_sp	dw	0		; Application SP escape

;//========================================================
;//  FTP PD's Packet ISR far-calls Here
;//
	extrn	_check_buffer : near	; in PACKET.C
	extrn	_original_tmr
	public	_tmrisr			; Entry Point for Timer Interrupt 0x1C

_tmrisr	proc	far
	push	bx

	pushf				; Say "I'm Interrupt"
	mov	bx, offset _original_tmr
	call	cs: dword ptr [bx]	; call chained Timer Interrupt

	cli
	B_TSR	t_ret			; Begin Kernel, if fail, t_ret
	mov	cs:word ptr _Tick_Count,bx	; Increment Tick UnServiced Counter
	S_STK				; setup kernel stack
	sti

	push	ax			; push all registers
	push	cx			;
	push	dx			;
	push	di			;
	push	si			;
	push	ds			;
	push	es			;
	push	bp			;

	mov	ds, bx

	call	near ptr _check_buffer	; Call Buffer_Check/Timer Function

	pop	bp			; pop all registers
	pop	es			;
	pop	ds			;
	pop	si			;
	pop	di			;
	pop	dx			;
	pop	cx			;
	pop 	ax			;

	cli
	R_STK
	E_TSR
t_ret:	pop	bx
	sti
	iret
_tmrisr	endp

;//========================================================
;//  FTP PD's Packet ISR far-calls Here
;//
;//	Not used for SLIP Packet Driver, see check_buffer() in PACKET.C
;//
	extrn	_first_call  : near	; in PACKET.C
	extrn	_second_call : near	; in PACKET.C
	public	_pktisr			; Entry Point for PDI far-call

_pktisr	proc	far
	push	bx			; Temporal Register for Code Seg. Reg.
	mov	bx, cs
	push	dx
	push	ds
	push	si
	push	bp			;------------------------------------
	push	cx			; Size Argument for first_call()

	mov	ds, bx			; DS = CS
	
	or	ax, ax
	jne	short second_call	; if (AX != 0), Second Call

	call	near ptr _first_call
	mov	di, ax			; returned buffer address
	mov	es, dx
	jmp	return

second_call:
	push	di			; second call don't use
	push	es			;  es:di as an argument

	call	near ptr _second_call	; Queuing the Packet

	pop	es			; pop es:di for second call
	pop	di

return:	pop	cx			; pop size argument for first_call()
	pop	bp			; pop registers
	pop	si			;
	pop	ds			;
	pop	dx			;
	pop	bx
	ret
_pktisr	endp

;//========================================================
;// Kernel Invokes Application by Signaling Interface
;// _sigisr is the STUB ISR for Signalling Interrupt
;//
	public	_if_isr
	public	_i_sign

kif_bx	dw	0		; BX is used as Temporal Variable

CALL_F	macro	FUNCTION, STACK_COUNT
	mov	ax, cs
	mov	ds, ax			; change to Kernel's Environment
	call	near ptr _check_buffer	; run buffer checker
	call	near ptr FUNCTION
	if STACK_COUNT			; We don't need to pop arguments here,
	   add	sp, STACK_COUNT		; because SS, SP will be changed
	endif				; just after CALL_F. see "done:"
	endm

_if_isr	proc	far
	jmp	isrmain
_i_sign	db	"TCP/IP"	; IMIGE/TCP Signature, defined in TCPCFG.H

isrmain:
	cli
;	push	ax
;	mov     ax, 1681h
;	int     2fh
;	pop	ax
	mov	word ptr cs:kif_bx, bx	; use BX temp buffer
	B_TSR	k_err			; Start Kernel Processing
	S_STK
	sti

	push	ds		; Save Application Context
	push	es		; Application must Backup AX, BX, CX, DX
	push	si		; because KERNEL save only DS,ES,SI,DI
	push	di		; 

	mov	bx, word ptr cs:kif_bx	; restore BX
	push	bx

	mov	bl, ah			; Function Number
	xor	bh, bh
	add	bx, bx			; Table
	jmp	word ptr CS:[bx + FTABLE]	; Jump to Function

k_err:	mov	ax, -1
	jmp	k_ret

; Do not Change the Order of Functions below
; It's Defined in ..\LIBRARY\IMGTCPIF.H

FTABLE:	dw	IMG_RESERVED	;  0x00 - Reserved for Unwanted S/W
	dw	IMG_INITTCPIP	;  0x01 - Get Kernel Type, unload
	dw	IMG_LOADCFG	;  0x02	- Config Kernel and Startup
	dw	IMG_TCP_STAT	;  0x03 - Get Kernel Stat
	dw	IMG_SOCK_STAT	;  0x04 - Get status of a socket
	dw	IMG_SOCKET	;  0x05 - Open a socket
	dw	IMG_CONNECT	;  0x06 - Connect (TCP, UDP)
	dw	IMG_BIND	;  0x07 - Bind local port to a socket
	dw	IMG_LISTEN	;  0x08 - Push a socket into TCPS_LISTEN state
	dw	IMG_ACCEPT	;  0x09 - Accept a passive-connected socket
	dw	IMG_CLOSE	;  0x0A - Close a socket
	dw	IMG_RECV	;  0x0B - Read data from TCP socket
	dw	IMG_SEND	;  0x0C - Send data to TCP socket
	dw	IMG_RECVFROM	;  0x0D - Recv data from UDP socket
	dw	IMG_SENDTO	;  0x0E - Send data to UDP socket
	dw	IMG_SHUTDOWN	;  0x0F - Block send/recv from socket
	dw	IMG_SETOPTION	;  0x10 - Set socket options
	dw	IMG_SELECT	;  0x11 - Poll sockets
	dw	IMG_GETERROR	;  0x12 - Get error number
	dw	IMG_GETPOINTER	;  0x13 - Get Kernel Pointer for debug
	dw	IMG_UDP_DEST	;  0x14 - Set UDP sendto Destination
	dw	IMG_ASYNC	;  0x15 - Socket Specific Event Report
	dw	IMG_RBIND	;  0x16 - Bind a Reserved Port
	dw	IMG_POLL	;  0x17 - Just call check_buffer
	dw	UNWANTED	; --

;//========================================================
;// int _init_kernel(int flag);
;//
	extrn	__init_kernel : near
IMG_INITTCPIP:
;	pop	bx
;	push	bx	; proto
	CALL_F	__init_kernel, 2

done:	pop	di
	pop	si
	pop	es
	pop	ds
	cli
	mov	word ptr cs:kif_bx, bx	; use BX temp buffer
	R_STK					; Restore Stack
	E_TSR
	mov	bx, word ptr cs:kif_bx	; restore BX
k_ret:
;	push	ax
;	mov     ax, 1682h
;	int     2fh
;	pop	ax
	sti
	iret

;//========================================================
;// int _load_cfg(TCPCONFIG far *config)
;//
	extrn	__load_cfg : near
IMG_LOADCFG:
;	pop	bx
;	push	bx	; FP_SEG(config)
	push	cx	; FP_OFF(config)
	CALL_F	__load_cfg, 4
	jmp	done

;//========================================================
;// void _get_stat(int clearflag, struct snmp_object far *object)
;//
	extrn	__get_stat : near
IMG_TCP_STAT:
;	pop	bx
;	push	bx	; FP_SEG(object)
	push	cx	; FP_OFF(object)
	xor	ah, ah
	push	ax	; clearflag
	CALL_F	__get_stat, 6
	jmp	done

;//========================================================
;// unsigned _sock_stat(int sd, struct TCB far *stat)
;//
	extrn	__sock_stat : near
IMG_SOCK_STAT:
;	pop	bx
;	push	bx	; FP_SEG(stat)
	push	cx	; FP_OFF(stat)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__sock_stat, 6
	jmp	done

;//========================================================
;// int _socket(int protocol)
;//
	extrn	__socket : near
IMG_SOCKET:
;	pop	bx
;	push	bx	; protocol
	CALL_F	__socket, 2
	jmp	done

;//========================================================
;// int _connect(int sd, unsigned long destination, unsigned short port)
;//
	extrn	__connect : near
IMG_CONNECT:
;	pop	bx
;	push	bx	; Destination Port
	push	dx	; high(Destination IP Address)
	push	cx	; low (Destination IP Address)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__connect, 8
	jmp	done

;//========================================================
;// int _bind(int sd, unsigned port)
;//
	extrn	__bind : near
IMG_BIND:
;	pop	bx
;	push	bx	; port number
	xor	ah, ah
	push	ax	; sd
	CALL_F	__bind, 4
	jmp	done

;//========================================================
;// void _listen(int sd, int backlog)
;//
	extrn	__listen : near
IMG_LISTEN:
;	pop	bx
;	push	bx	; backlog
	xor	ah, ah
	push	ax	; sd
	CALL_F	__listen, 4
	jmp	done

;//========================================================
;// int _accept(int sd)
;//
	extrn	__accept : near
IMG_ACCEPT:
	pop	bx
	xor	ah, ah
	push	ax	; sd
	CALL_F	__accept, 2
	jmp	done

;//========================================================
;// void __close(int sd)
;//
	extrn	___close : near
IMG_CLOSE:
	pop	bx
	xor	ah, ah
	push	ax	; sd
	CALL_F	___close, 2
	jmp	done

;//========================================================
;// int _recv(int sd, unsigned char far *data, int count, int flag)
;//
	extrn	__recv : near
IMG_RECV:
;	pop	bx
;	push	bx	; flag
	push	cx	; count
	push	si	; FP_SEG(data)
	push	di	; FP_OFF(data)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__recv, 10
	jmp	done

;//========================================================
;// int _send(int sd, unsigned char far *data, int count, int flag)
;//
	extrn	__send : near
IMG_SEND:
;	pop	bx
;	push	bx	; flag
	push	cx	; count
	push	si	; FP_SEG(data)
	push	di	; FP_OFF(data)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__send, 10
	jmp	done

;//========================================================
;// int _recvfrom(int sd, unsigned char far *data, int count, int flag)
;//
	extrn	__recvfrom : near
IMG_RECVFROM:
;	pop	bx
;	push	bx	; flag
	push	cx	; count
	push	si	; FP_SEG(data)
	push	di	; FP_OFF(data)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__recvfrom, 10
	jmp	done

;//========================================================
;// int _sendto(int sd, unsigned char far *data, int count)
;//
;//	Destination Address should be set by IMG_UDP_DEST call
;//
	extrn	__sendto : near
IMG_SENDTO:
;	pop	bx
;	push	bx	; count
	push	si	; FP_SEG(data)
	push	di	; FP_OFF(data)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__sendto, 8
	jmp	done

;//========================================================
;// void _shutdown(int sd, int how)
;//
	extrn	__shutdown : near
IMG_SHUTDOWN:
;	pop	bx
;	push	bx	; how
	xor	ah, ah
	push	ax	; sd
	CALL_F	__shutdown, 4
	jmp	done

;//========================================================
;// void _setoption(int sd, int option, int value)
;//
	extrn	__setoption : near
IMG_SETOPTION:
;	pop	bx
;	push	bx	; value
	push	dx	; option
	xor	ah, ah
	push	ax	; sd
	CALL_F	__setoption, 6
	jmp	done

;//========================================================
;// void _select(struct SELECT far *fds, unsigned long bitmask)
;//
	extrn	__select : near
IMG_SELECT:
;	pop	bx
;	push	bx	; high(bitmask)
	push	cx	; low(bitmask)
	push	dx	; FP_SEG(fds)
	push	si	; FP_OFF(fds)
	CALL_F	__select, 8
	jmp	done

;//========================================================
;// int _geterror(int sd)
;//
	extrn	__geterror : near
IMG_GETERROR:
	pop	bx
	xor	ah, ah
	push	ax	; sd
	CALL_F	__geterror, 2
	jmp	done

;//========================================================
;// void _getpointer(struct POINTERS far *ptr)
;//
	extrn	__getpointer : near
IMG_GETPOINTER:
;	pop	bx
;	push	bx	; FP_SEG(ptr)
	push	cx	; FP_OFF(ptr)
	CALL_F	__getpointer, 4
	jmp	done

;//========================================================
;// int _udp_dest(int sd, unsigned long destination, unsigned port)
;//
	extrn	__udp_dest : near
IMG_UDP_DEST:
;	pop	bx
;	push	bx	; Destination Port
	push	dx	; high(Destination IP Address)
	push	cx	; high(Destination IP Address)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__udp_dest, 8
	jmp	done

;//========================================================
;// void _set_async(int sd, void far (* async_func)())
;//
	extrn	__set_async : near
IMG_ASYNC:
;	pop	bx
;	push	bx	; FP_SEG(async_func)
	push	cx	; FP_OFF(async_func)
	xor	ah, ah
	push	ax	; sd
	CALL_F	__set_async, 6
	jmp	done

;//========================================================
;// int _rbind(int sd);
;//
	extrn	__rbind : near
IMG_RBIND:
	pop	bx
	xor	ah, ah
	push	ax	; sd
	CALL_F	__rbind, 2
	jmp	done

;//========================================================
;// Trap Accidental S/W Interrupt
;//
UNWANTED:
IMG_POLL:
IMG_RESERVED:
	pop	bx	; Do nothing
	jmp	done

_if_isr	endp

;//========================================================
;// Kernel Invokes Application by Signaling Interface
;// _sigisr is the STUB ISR for Signalling Interrupt
;//
	public	_sigisr
_sigisr	proc	far
	iret
_sigisr	endp

_TEXT	ENDS

;//========================================================
;//	Data Area

_DATA	SEGMENT
	extrn	_packet_buffer	; Packet Receive Buffer
	extrn	_Tick_Count
_DATA	ENDS

;//========================================================
;//	BSS Area (used by C-functions)

_BSS	SEGMENT
	public	k_stack		; Just for Reference in MAP file
	db	1024 dup (?)	; Kernel Stack
k_stack	label	byte
	ENDS

	end
