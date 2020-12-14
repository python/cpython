default	rel
%define XMMWORD
%define YMMWORD
%define ZMMWORD
section	.text code align=64

EXTERN	OPENSSL_Uplink
global	OPENSSL_UplinkTable

ALIGN	16
_lazy1:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,1
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[8+rax]
_lazy1_end:


ALIGN	16
_lazy2:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,2
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[16+rax]
_lazy2_end:


ALIGN	16
_lazy3:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,3
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[24+rax]
_lazy3_end:


ALIGN	16
_lazy4:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,4
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[32+rax]
_lazy4_end:


ALIGN	16
_lazy5:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,5
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[40+rax]
_lazy5_end:


ALIGN	16
_lazy6:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,6
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[48+rax]
_lazy6_end:


ALIGN	16
_lazy7:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,7
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[56+rax]
_lazy7_end:


ALIGN	16
_lazy8:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,8
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[64+rax]
_lazy8_end:


ALIGN	16
_lazy9:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,9
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[72+rax]
_lazy9_end:


ALIGN	16
_lazy10:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,10
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[80+rax]
_lazy10_end:


ALIGN	16
_lazy11:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,11
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[88+rax]
_lazy11_end:


ALIGN	16
_lazy12:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,12
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[96+rax]
_lazy12_end:


ALIGN	16
_lazy13:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,13
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[104+rax]
_lazy13_end:


ALIGN	16
_lazy14:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,14
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[112+rax]
_lazy14_end:


ALIGN	16
_lazy15:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,15
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[120+rax]
_lazy15_end:


ALIGN	16
_lazy16:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,16
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[128+rax]
_lazy16_end:


ALIGN	16
_lazy17:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,17
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[136+rax]
_lazy17_end:


ALIGN	16
_lazy18:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,18
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[144+rax]
_lazy18_end:


ALIGN	16
_lazy19:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,19
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[152+rax]
_lazy19_end:


ALIGN	16
_lazy20:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,20
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[160+rax]
_lazy20_end:


ALIGN	16
_lazy21:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,21
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[168+rax]
_lazy21_end:


ALIGN	16
_lazy22:
DB	0x48,0x83,0xEC,0x28
	mov	QWORD[48+rsp],rcx
	mov	QWORD[56+rsp],rdx
	mov	QWORD[64+rsp],r8
	mov	QWORD[72+rsp],r9
	lea	rcx,[OPENSSL_UplinkTable]
	mov	rdx,22
	call	OPENSSL_Uplink
	mov	rcx,QWORD[48+rsp]
	mov	rdx,QWORD[56+rsp]
	mov	r8,QWORD[64+rsp]
	mov	r9,QWORD[72+rsp]
	lea	rax,[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD[176+rax]
_lazy22_end:

section	.data data align=8

OPENSSL_UplinkTable:
	DQ	22
	DQ	_lazy1
	DQ	_lazy2
	DQ	_lazy3
	DQ	_lazy4
	DQ	_lazy5
	DQ	_lazy6
	DQ	_lazy7
	DQ	_lazy8
	DQ	_lazy9
	DQ	_lazy10
	DQ	_lazy11
	DQ	_lazy12
	DQ	_lazy13
	DQ	_lazy14
	DQ	_lazy15
	DQ	_lazy16
	DQ	_lazy17
	DQ	_lazy18
	DQ	_lazy19
	DQ	_lazy20
	DQ	_lazy21
	DQ	_lazy22
section	.pdata rdata align=4
ALIGN	4
	DD	_lazy1 wrt ..imagebase,_lazy1_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy2 wrt ..imagebase,_lazy2_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy3 wrt ..imagebase,_lazy3_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy4 wrt ..imagebase,_lazy4_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy5 wrt ..imagebase,_lazy5_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy6 wrt ..imagebase,_lazy6_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy7 wrt ..imagebase,_lazy7_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy8 wrt ..imagebase,_lazy8_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy9 wrt ..imagebase,_lazy9_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy10 wrt ..imagebase,_lazy10_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy11 wrt ..imagebase,_lazy11_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy12 wrt ..imagebase,_lazy12_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy13 wrt ..imagebase,_lazy13_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy14 wrt ..imagebase,_lazy14_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy15 wrt ..imagebase,_lazy15_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy16 wrt ..imagebase,_lazy16_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy17 wrt ..imagebase,_lazy17_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy18 wrt ..imagebase,_lazy18_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy19 wrt ..imagebase,_lazy19_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy20 wrt ..imagebase,_lazy20_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy21 wrt ..imagebase,_lazy21_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
	DD	_lazy22 wrt ..imagebase,_lazy22_end wrt ..imagebase,_lazy_unwind_info wrt ..imagebase
section	.xdata rdata align=8
ALIGN	8
_lazy_unwind_info:
DB	0x01,0x04,0x01,0x00
DB	0x04,0x42,0x00,0x00
