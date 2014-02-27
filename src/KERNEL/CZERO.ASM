;//  IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
;
;//=======================================================================
;//
;//	Start Up Code for TCP/IP Kernel
;//
;//		1. Initialize Stack Registers (SS, SP)
;//		2. Zero BSS Area
;//		3. Call main()
;//
;//		4. Optimized Swap Routines
;//
;//			1994. 6. 14	Minsuk Lee
;//

_TEXT	SEGMENT BYTE PUBLIC 'CODE'
	ENDS
_DATA	SEGMENT PARA PUBLIC 'DATA'
	ENDS
_BSS	SEGMENT WORD PUBLIC 'BSS'
	ENDS
_BSSEND	SEGMENT WORD PUBLIC 'BSSEND'
	ENDS

	DGROUP GROUP _TEXT,_DATA,_BSS,_BSSEND
        ASSUME  CS:_TEXT, DS:DGROUP

_TEXT	SEGMENT
	ASSUME	CS:_TEXT, DS:DGROUP

extrn	_main		: near		; in KERNEL.C

;//========================================================
;// Program Starts Here
;//
	org	100h
tcpip	proc	near

	mov	ax, cs		;
	mov	ds, ax		; DS = CS
	mov	cs:DGROUP@, ax	; set DGROUP Variable

	; Initialized BSS Area to Zero (ANSI-C)

	xor	ax, ax
	mov	es, cs:DGROUP@
	mov	di, offset DGROUP: ___bob
	mov	cx, offset DGROUP: ___eop
	sub	cx, di				; CX = BSS Size
	cld
	rep	stosb

	call	near ptr _main	; Call main() routine in KERNEL.C

	; If _main processing is successful, it NEVER return to this point

	int	20h		; Just Quit

tcpip	endp

	public	_lswap		; Swap unsigned long
	public	_bswap		; Swap unsigned short

_lswap	proc	near
	push	bp
	mov	bp,sp
	mov	ax, word ptr [bp + 6]	; High word
	mov	dx, word ptr [bp + 4]	; Low  word	-- word swap
	xchg	ah, al			; byte swap
	xchg	dh, dl			; byte swap
	pop	bp
	ret	
_lswap	endp

_bswap	proc	near
	push	bp
	mov	bp, sp
	mov	ax, [bp + 4]		; unsigned short
	xchg	ah, al			; byte swap
	pop	bp
	ret	
_bswap	endp

	public	__IOerror	; Dummy function for int86(), int86x()

__IOerror	proc	near
		ret
__IOerror	endp

	public	DGROUP@
DGROUP@	dw	?

_TEXT	ENDS

;//========================================================
;//	Data Area

_DATA	SEGMENT
	public	___bod		; Begin of Data Segment
___bod	label   byte
_DATA	ENDS

;//========================================================
;//	BSS Area (used by C-functions)

_BSS	SEGMENT
	public	___bob		; Just for Reference in MAP file
___bob	label   byte		; Begin of BSS Segment
	ENDS

;//========================================================
;//	BSSEND Area (used by IMIGE/TCP-WIN Kernel ASM)

_BSSEND	SEGMENT
	public	___eop
___eop	label   byte		; This is the end of Program
	ENDS

	end	tcpip
