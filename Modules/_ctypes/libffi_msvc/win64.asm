PUBLIC	ffi_call_AMD64

EXTRN	__chkstk:NEAR
EXTRN	ffi_closure_SYSV:NEAR

_TEXT	SEGMENT

;;; ffi_closure_OUTER will be called with these registers set:
;;;    rax points to 'closure'
;;;    r11 contains a bit mask that specifies which of the
;;;    first four parameters are float or double
;;;
;;; It must move the parameters passed in registers to their stack location,
;;; call ffi_closure_SYSV for the actual work, then return the result.
;;; 
ffi_closure_OUTER PROC FRAME
	;; save actual arguments to their stack space.
	test	r11, 1
	jne	first_is_float	
	mov	QWORD PTR [rsp+8], rcx
	jmp	second
first_is_float:
	movlpd	QWORD PTR [rsp+8], xmm0

second:
	test	r11, 2
	jne	second_is_float	
	mov	QWORD PTR [rsp+16], rdx
	jmp	third
second_is_float:
	movlpd	QWORD PTR [rsp+16], xmm1

third:
	test	r11, 4
	jne	third_is_float	
	mov	QWORD PTR [rsp+24], r8
	jmp	forth
third_is_float:
	movlpd	QWORD PTR [rsp+24], xmm2

forth:
	test	r11, 8
	jne	forth_is_float	
	mov	QWORD PTR [rsp+32], r9
	jmp	done
forth_is_float:
	movlpd	QWORD PTR [rsp+32], xmm3

done:
.ALLOCSTACK 40
	sub	rsp, 40
.ENDPROLOG
	mov	rcx, rax	; context is first parameter
	mov	rdx, rsp	; stack is second parameter
	add	rdx, 40		; correct our own area
	mov	rax, ffi_closure_SYSV
	call	rax		; call the real closure function
	;; Here, code is missing that handles float return values
	add	rsp, 40
	movd	xmm0, rax	; In case the closure returned a float.
	ret	0
ffi_closure_OUTER ENDP


;;; ffi_call_AMD64

stack$ = 0
prepfunc$ = 32
ecif$ = 40
bytes$ = 48
flags$ = 56
rvalue$ = 64
fn$ = 72

ffi_call_AMD64 PROC FRAME

	mov	QWORD PTR [rsp+32], r9
	mov	QWORD PTR [rsp+24], r8
	mov	QWORD PTR [rsp+16], rdx
	mov	QWORD PTR [rsp+8], rcx
.PUSHREG rbp
	push	rbp
.ALLOCSTACK 48
	sub	rsp, 48					; 00000030H
.SETFRAME rbp, 32
	lea	rbp, QWORD PTR [rsp+32]
.ENDPROLOG

	mov	eax, DWORD PTR bytes$[rbp]
	add	rax, 15
	and	rax, -16
	call	__chkstk
	sub	rsp, rax
	lea	rax, QWORD PTR [rsp+32]
	mov	QWORD PTR stack$[rbp], rax

	mov	rdx, QWORD PTR ecif$[rbp]
	mov	rcx, QWORD PTR stack$[rbp]
	call	QWORD PTR prepfunc$[rbp]

	mov	rsp, QWORD PTR stack$[rbp]

	movlpd	xmm3, QWORD PTR [rsp+24]
	movd	r9, xmm3

	movlpd	xmm2, QWORD PTR [rsp+16]
	movd	r8, xmm2

	movlpd	xmm1, QWORD PTR [rsp+8]
	movd	rdx, xmm1

	movlpd	xmm0, QWORD PTR [rsp]
	movd	rcx, xmm0

	call	QWORD PTR fn$[rbp]
ret_int$:
 	cmp	DWORD PTR flags$[rbp], 1 ; FFI_TYPE_INT
 	jne	ret_float$

	mov	rcx, QWORD PTR rvalue$[rbp]
	mov	DWORD PTR [rcx], eax
	jmp	SHORT ret_nothing$

ret_float$:
 	cmp	DWORD PTR flags$[rbp], 2 ; FFI_TYPE_FLOAT
 	jne	SHORT ret_double$

 	mov	rax, QWORD PTR rvalue$[rbp]
 	movlpd	QWORD PTR [rax], xmm0
 	jmp	SHORT ret_nothing$

ret_double$:
 	cmp	DWORD PTR flags$[rbp], 3 ; FFI_TYPE_DOUBLE
 	jne	SHORT ret_int64$

 	mov	rax, QWORD PTR rvalue$[rbp]
 	movlpd	QWORD PTR [rax], xmm0
 	jmp	SHORT ret_nothing$

ret_int64$:
  	cmp	DWORD PTR flags$[rbp], 12 ; FFI_TYPE_SINT64
  	jne	ret_nothing$

 	mov	rcx, QWORD PTR rvalue$[rbp]
 	mov	QWORD PTR [rcx], rax
 	jmp	SHORT ret_nothing$
	
ret_nothing$:
	xor	eax, eax

	lea	rsp, QWORD PTR [rbp+16]
	pop	rbp
	ret	0
ffi_call_AMD64 ENDP
_TEXT	ENDS
END
