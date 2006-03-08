; -----------------------------------------------------------------------
;  sysv.S - Copyright (c) 1998 Red Hat, Inc.
;  
;  ARM Foreign Function Interface 
;
;  Permission is hereby granted, free of charge, to any person obtaining
;  a copy of this software and associated documentation files (the
;  ``Software''), to deal in the Software without restriction, including
;  without limitation the rights to use, copy, modify, merge, publish,
;  distribute, sublicense, and/or sell copies of the Software, and to
;  permit persons to whom the Software is furnished to do so, subject to
;  the following conditions:
;
;  The above copyright notice and this permission notice shall be included
;  in all copies or substantial portions of the Software.
;
;  THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
;  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;  IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
;  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
;  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
;  OTHER DEALINGS IN THE SOFTWARE.
;  ----------------------------------------------------------------------- */

;#define LIBFFI_ASM     
;#include <fficonfig.h>
;#include <ffi.h>
;#ifdef HAVE_MACHINE_ASM_H
;#include <machine/asm.h>
;#else
;#ifdef __USER_LABEL_PREFIX__
;#define CONCAT1(a, b) CONCAT2(a, b)
;#define CONCAT2(a, b) a ## b

;/* Use the right prefix for global labels.  */
;#define CNAME(x) CONCAT1 (__USER_LABEL_PREFIX__, x)
;#else
;#define CNAME(x) x
;#endif
;#define ENTRY(x) .globl CNAME(x); .type CNAME(x),%function; CNAME(x):
;#endif


FFI_TYPE_VOID       EQU 0
FFI_TYPE_INT        EQU 1
FFI_TYPE_FLOAT      EQU 2
FFI_TYPE_DOUBLE     EQU 3
;FFI_TYPE_LONGDOUBLE EQU 4
FFI_TYPE_UINT8      EQU 5
FFI_TYPE_SINT8      EQU 6
FFI_TYPE_UINT16     EQU 7
FFI_TYPE_SINT16     EQU 8
FFI_TYPE_UINT32     EQU 9
FFI_TYPE_SINT32     EQU 10
FFI_TYPE_UINT64     EQU 11
FFI_TYPE_SINT64     EQU 12
FFI_TYPE_STRUCT     EQU 13
FFI_TYPE_POINTER    EQU 14

; WinCE always uses software floating point (I think)
__SOFTFP__ EQU {TRUE}


    AREA |.text|, CODE, ARM     ; .text


    ; a1:   ffi_prep_args
    ; a2:   &ecif
    ; a3:   cif->bytes
    ; a4:   fig->flags
    ; sp+0: ecif.rvalue
    ; sp+4: fn

    ; This assumes we are using gas.
;ENTRY(ffi_call_SYSV)

    EXPORT |ffi_call_SYSV|

|ffi_call_SYSV| PROC

    ; Save registers
    stmfd sp!, {a1-a4, fp, lr}
    mov   fp, sp

    ; Make room for all of the new args.
    sub   sp, fp, a3

    ; Place all of the ffi_prep_args in position
    mov   ip, a1
    mov   a1, sp
    ;     a2 already set

    ; And call
    mov   lr, pc
    mov   pc, ip

    ; move first 4 parameters in registers
    ldr   a1, [sp, #0]
    ldr   a2, [sp, #4]
    ldr   a3, [sp, #8]
    ldr   a4, [sp, #12]

    ; and adjust stack
    ldr   ip, [fp, #8]
    cmp   ip, #16
    movge ip, #16
    add   sp, sp, ip

    ; call function
    mov   lr, pc
    ldr   pc, [fp, #28]

    ; Remove the space we pushed for the args
    mov   sp, fp

    ; Load a3 with the pointer to storage for the return value
    ldr   a3, [sp, #24]

    ; Load a4 with the return type code 
    ldr   a4, [sp, #12]

    ; If the return value pointer is NULL, assume no return value.
    cmp   a3, #0
    beq   call_epilogue

; return INT
    cmp   a4, #FFI_TYPE_INT
    streq a1, [a3]
    beq   call_epilogue

; return FLOAT
    cmp     a4, #FFI_TYPE_FLOAT
    [ __SOFTFP__                    ;ifdef __SOFTFP__
    streq   a1, [a3]
    |                               ;else
    stfeqs  f0, [a3]
    ]                               ;endif
    beq     call_epilogue

; return DOUBLE or LONGDOUBLE
    cmp     a4, #FFI_TYPE_DOUBLE
    [ __SOFTFP__                    ;ifdef __SOFTFP__
    stmeqia a3, {a1, a2}
    |                               ;else
    stfeqd  f0, [a3]
    ]                               ;endif
    beq     call_epilogue

; return SINT64 or UINT64
    cmp     a4, #FFI_TYPE_SINT64
    stmeqia a3, {a1, a2}

call_epilogue
    ldmfd   sp!, {a1-a4, fp, pc}

;.ffi_call_SYSV_end:
    ;.size    CNAME(ffi_call_SYSV),.ffi_call_SYSV_end-CNAME(ffi_call_SYSV)
    ENDP


RESERVE_RETURN EQU 16

    ; This function is called by the trampoline
    ; It is NOT callable from C
    ; ip = pointer to struct ffi_closure

    IMPORT |ffi_closure_SYSV_inner|

    EXPORT |ffi_closure_SYSV|
|ffi_closure_SYSV| PROC

    ; Store the argument registers on the stack
    stmfd   sp!, {a1-a4}
    ; Push the return address onto the stack
    stmfd   sp!, {lr}

    mov     a1, ip            ; first arg = address of ffi_closure
    add     a2, sp, #4        ; second arg = sp+4 (points to saved a1)

    ; Allocate space for a non-struct return value
    sub     sp, sp, #RESERVE_RETURN
    mov     a3, sp            ; third arg = return value address

    ; static unsigned int
    ; ffi_closure_SYSV_inner (ffi_closure *closure, char *in_args, void *rvalue)
    bl      ffi_closure_SYSV_inner
    ; a1 now contains the return type code 

    ; At this point the return value is on the stack
    ; Transfer it to the correct registers if necessary

; return INT
    cmp     a1, #FFI_TYPE_INT
    ldreq   a1, [sp]
    beq     closure_epilogue

; return FLOAT
    cmp     a1, #FFI_TYPE_FLOAT
    [ __SOFTFP__                    ;ifdef __SOFTFP__
    ldreq   a1, [sp]
    |                               ;else
    stfeqs  f0, [sp]
    ]                               ;endif
    beq     closure_epilogue

; return DOUBLE or LONGDOUBLE
    cmp     a1, #FFI_TYPE_DOUBLE
    [ __SOFTFP__                    ;ifdef __SOFTFP__
    ldmeqia sp, {a1, a2}
    |                               ;else
    stfeqd  f0, [sp]
    ]                               ;endif
    beq     closure_epilogue

; return SINT64 or UINT64
    cmp     a1, #FFI_TYPE_SINT64
    ldmeqia sp, {a1, a2}

closure_epilogue
    add     sp, sp, #RESERVE_RETURN   ; remove return value buffer
    ldmfd   sp!, {ip}         ; ip = pop return address
    add     sp, sp, #16       ; remove saved argument registers {a1-a4} from the stack
    mov     pc, ip            ; return

    ENDP    ; ffi_closure_SYSV

    END
