
;	Segment and Group declarations

_TEXT	SEGMENT BYTE PUBLIC 'CODE'
_TEXT	ENDS
_DATABEG	SEGMENT PARA PUBLIC 'DATA'
_DATABEG	ENDS
_DATA	SEGMENT PARA PUBLIC 'DATA'
_DATA	ENDS
_DATAEND SEGMENT PARA PUBLIC 'DATA'
_DATAEND ENDS
_BSS	SEGMENT PARA PUBLIC 'BSS'
_BSS	ENDS
_BSSEND SEGMENT BYTE PUBLIC 'BSS'
_BSSEND ENDS
_STACK	SEGMENT PARA STACK 'STACK'
_STACK	ENDS

DGROUP	GROUP	_DATABEG,_DATA, _DATAEND, _BSS, _BSSEND, _STACK
