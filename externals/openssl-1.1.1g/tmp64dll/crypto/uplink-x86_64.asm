OPTION	DOTNAME
.text$	SEGMENT ALIGN(256) 'CODE'
EXTERN	OPENSSL_Uplink:NEAR
PUBLIC	OPENSSL_UplinkTable

ALIGN	16
_lazy1	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,1
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[8+rax]
_lazy1_end::
_lazy1	ENDP

ALIGN	16
_lazy2	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,2
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[16+rax]
_lazy2_end::
_lazy2	ENDP

ALIGN	16
_lazy3	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,3
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[24+rax]
_lazy3_end::
_lazy3	ENDP

ALIGN	16
_lazy4	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,4
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[32+rax]
_lazy4_end::
_lazy4	ENDP

ALIGN	16
_lazy5	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,5
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[40+rax]
_lazy5_end::
_lazy5	ENDP

ALIGN	16
_lazy6	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,6
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[48+rax]
_lazy6_end::
_lazy6	ENDP

ALIGN	16
_lazy7	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,7
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[56+rax]
_lazy7_end::
_lazy7	ENDP

ALIGN	16
_lazy8	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,8
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[64+rax]
_lazy8_end::
_lazy8	ENDP

ALIGN	16
_lazy9	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,9
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[72+rax]
_lazy9_end::
_lazy9	ENDP

ALIGN	16
_lazy10	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,10
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[80+rax]
_lazy10_end::
_lazy10	ENDP

ALIGN	16
_lazy11	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,11
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[88+rax]
_lazy11_end::
_lazy11	ENDP

ALIGN	16
_lazy12	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,12
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[96+rax]
_lazy12_end::
_lazy12	ENDP

ALIGN	16
_lazy13	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,13
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[104+rax]
_lazy13_end::
_lazy13	ENDP

ALIGN	16
_lazy14	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,14
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[112+rax]
_lazy14_end::
_lazy14	ENDP

ALIGN	16
_lazy15	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,15
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[120+rax]
_lazy15_end::
_lazy15	ENDP

ALIGN	16
_lazy16	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,16
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[128+rax]
_lazy16_end::
_lazy16	ENDP

ALIGN	16
_lazy17	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,17
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[136+rax]
_lazy17_end::
_lazy17	ENDP

ALIGN	16
_lazy18	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,18
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[144+rax]
_lazy18_end::
_lazy18	ENDP

ALIGN	16
_lazy19	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,19
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[152+rax]
_lazy19_end::
_lazy19	ENDP

ALIGN	16
_lazy20	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,20
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[160+rax]
_lazy20_end::
_lazy20	ENDP

ALIGN	16
_lazy21	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,21
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[168+rax]
_lazy21_end::
_lazy21	ENDP

ALIGN	16
_lazy22	PROC PRIVATE
DB	048h,083h,0ECh,028h
	mov	QWORD PTR[48+rsp],rcx
	mov	QWORD PTR[56+rsp],rdx
	mov	QWORD PTR[64+rsp],r8
	mov	QWORD PTR[72+rsp],r9
	lea	rcx,QWORD PTR[OPENSSL_UplinkTable]
	mov	rdx,22
	call	OPENSSL_Uplink
	mov	rcx,QWORD PTR[48+rsp]
	mov	rdx,QWORD PTR[56+rsp]
	mov	r8,QWORD PTR[64+rsp]
	mov	r9,QWORD PTR[72+rsp]
	lea	rax,QWORD PTR[OPENSSL_UplinkTable]
	add	rsp,40
	jmp	QWORD PTR[176+rax]
_lazy22_end::
_lazy22	ENDP
.text$	ENDS
_DATA	SEGMENT
OPENSSL_UplinkTable::
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
_DATA	ENDS
.pdata	SEGMENT READONLY ALIGN(4)
ALIGN	4
	DD	imagerel _lazy1,imagerel _lazy1_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy2,imagerel _lazy2_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy3,imagerel _lazy3_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy4,imagerel _lazy4_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy5,imagerel _lazy5_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy6,imagerel _lazy6_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy7,imagerel _lazy7_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy8,imagerel _lazy8_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy9,imagerel _lazy9_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy10,imagerel _lazy10_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy11,imagerel _lazy11_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy12,imagerel _lazy12_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy13,imagerel _lazy13_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy14,imagerel _lazy14_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy15,imagerel _lazy15_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy16,imagerel _lazy16_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy17,imagerel _lazy17_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy18,imagerel _lazy18_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy19,imagerel _lazy19_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy20,imagerel _lazy20_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy21,imagerel _lazy21_end,imagerel _lazy_unwind_info
	DD	imagerel _lazy22,imagerel _lazy22_end,imagerel _lazy_unwind_info
.pdata	ENDS
.xdata	SEGMENT READONLY ALIGN(8)
ALIGN	8
_lazy_unwind_info::
DB	001h,004h,001h,000h
DB	004h,042h,000h,000h

.xdata	ENDS
END
