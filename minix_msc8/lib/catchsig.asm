title catchsig  -  C86 version
page,132

	INCLUDE ..\lib\prologue.h

	PUBLIC _begsig
	EXTRN  _vectab:WORD, _M:WORD


_TEXT	SEGMENT
	ASSUME	CS:_TEXT,DS:DGROUP

MTYPE = 2			; M+mtype =  &M.m_type
_begsig:
	push ax			; after interrupt, save all regs
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	push ds
	push es
	mov bp,sp
	mov bx,18[bp]		; bx = signal number
	dec bx			; vectab[0] is for sig 1
	add bx,bx		; pointers are two bytes on 8088
	mov bx,_vectab[bx]	; bx = address of routine to call
	push _M+mtype		; push status of last system call
	push ax			; func called with signal number as arg
	call bx
back:
	pop ax			; get signal number off stack
	pop _M+mtype		; restore status of previous system call
	pop es			; signal handling finished
	pop ds
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	pop dummy		; remove signal number from stack
	iret
_TEXT	ENDS

_DATA	SEGMENT
dummy	 DW	 0
_DATA	ENDS

	END	; end of assembly-file
