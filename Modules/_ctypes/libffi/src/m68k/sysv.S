/* -----------------------------------------------------------------------
   sysv.S - Copyright (c) 1998 Andreas Schwab
	    Copyright (c) 2008 Red Hat, Inc. 
   
   m68k Foreign Function Interface 

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#define LIBFFI_ASM	
#include <fficonfig.h>
#include <ffi.h>

#ifdef HAVE_AS_CFI_PSEUDO_OP
#define CFI_STARTPROC()		.cfi_startproc
#define CFI_OFFSET(reg,off)	.cfi_offset	reg,off
#define CFI_DEF_CFA(reg,off)	.cfi_def_cfa	reg,off
#define CFI_ENDPROC()		.cfi_endproc
#else
#define CFI_STARTPROC()
#define CFI_OFFSET(reg,off)
#define CFI_DEF_CFA(reg,off)
#define CFI_ENDPROC()
#endif

	.text

	.globl	ffi_call_SYSV
	.type	ffi_call_SYSV,@function
	.align	4

ffi_call_SYSV:
	CFI_STARTPROC()
	link	%fp,#0
	CFI_OFFSET(14,-8)
	CFI_DEF_CFA(14,8)
	move.l	%d2,-(%sp)
	CFI_OFFSET(2,-12)

	| Make room for all of the new args.
	sub.l	12(%fp),%sp

	| Call ffi_prep_args
	move.l	8(%fp),-(%sp)
	pea	4(%sp)
#if !defined __PIC__
	jsr	ffi_prep_args
#else
	bsr.l	ffi_prep_args@PLTPC
#endif
	addq.l	#8,%sp	

	| Pass pointer to struct value, if any
	move.l	%a0,%a1

	| Call the function
	move.l	24(%fp),%a0
	jsr	(%a0)

	| Remove the space we pushed for the args
	add.l	12(%fp),%sp

	| Load the pointer to storage for the return value
	move.l	20(%fp),%a1

	| Load the return type code 
	move.l	16(%fp),%d2

	| If the return value pointer is NULL, assume no return value.
	tst.l	%a1
	jbeq	noretval

	btst	#0,%d2
	jbeq	retlongint
	move.l	%d0,(%a1)
	jbra	epilogue

retlongint:
	btst	#1,%d2
	jbeq	retfloat
	move.l	%d0,(%a1)
	move.l	%d1,4(%a1)
	jbra	epilogue

retfloat:
	btst	#2,%d2
	jbeq	retdouble
	fmove.s	%fp0,(%a1)
	jbra	epilogue

retdouble:
	btst	#3,%d2
	jbeq	retlongdouble
	fmove.d	%fp0,(%a1)
	jbra	epilogue

retlongdouble:
	btst	#4,%d2
	jbeq	retpointer
	fmove.x	%fp0,(%a1)
	jbra	epilogue

retpointer:
	btst	#5,%d2
	jbeq	retstruct1
	move.l	%a0,(%a1)
	jbra	epilogue

retstruct1:
	btst	#6,%d2
	jbeq	retstruct2
	move.b	%d0,(%a1)
	jbra	epilogue

retstruct2:
	btst	#7,%d2
	jbeq	noretval
	move.w	%d0,(%a1)

noretval:
epilogue:
	move.l	(%sp)+,%d2
	unlk	%fp
	rts
	CFI_ENDPROC()
	.size	ffi_call_SYSV,.-ffi_call_SYSV

	.globl	ffi_closure_SYSV
	.type	ffi_closure_SYSV, @function
	.align	4

ffi_closure_SYSV:
	CFI_STARTPROC()
	link	%fp,#-12
	CFI_OFFSET(14,-8)
	CFI_DEF_CFA(14,8)
	move.l	%sp,-12(%fp)
	pea	8(%fp)
	pea	-12(%fp)
	move.l	%a0,-(%sp)
#if !defined __PIC__
	jsr	ffi_closure_SYSV_inner
#else
	bsr.l	ffi_closure_SYSV_inner@PLTPC
#endif

	lsr.l	#1,%d0
	jne	1f
	jcc	.Lcls_epilogue
	move.l	-12(%fp),%d0
.Lcls_epilogue:
	unlk	%fp
	rts
1:
	lea	-12(%fp),%a0
	lsr.l	#2,%d0
	jne	1f
	jcs	.Lcls_ret_float
	move.l	(%a0)+,%d0
	move.l	(%a0),%d1
	jra	.Lcls_epilogue
.Lcls_ret_float:
	fmove.s	(%a0),%fp0
	jra	.Lcls_epilogue
1:
	lsr.l	#2,%d0
	jne	1f
	jcs	.Lcls_ret_ldouble
	fmove.d	(%a0),%fp0
	jra	.Lcls_epilogue
.Lcls_ret_ldouble:
	fmove.x	(%a0),%fp0
	jra	.Lcls_epilogue
1:
	lsr.l	#2,%d0
	jne	.Lcls_ret_struct2
	jcs	.Lcls_ret_struct1
	move.l	(%a0),%a0
	move.l	%a0,%d0
	jra	.Lcls_epilogue
.Lcls_ret_struct1:
	move.b	(%a0),%d0
	jra	.Lcls_epilogue
.Lcls_ret_struct2:
	move.w	(%a0),%d0
	jra	.Lcls_epilogue
	CFI_ENDPROC()

	.size	ffi_closure_SYSV,.-ffi_closure_SYSV

	.globl	ffi_closure_struct_SYSV
	.type	ffi_closure_struct_SYSV, @function
	.align	4

ffi_closure_struct_SYSV:
	CFI_STARTPROC()
	link	%fp,#0
	CFI_OFFSET(14,-8)
	CFI_DEF_CFA(14,8)
	move.l	%sp,-12(%fp)
	pea	8(%fp)
	move.l	%a1,-(%sp)
	move.l	%a0,-(%sp)
#if !defined __PIC__
	jsr	ffi_closure_SYSV_inner
#else
	bsr.l	ffi_closure_SYSV_inner@PLTPC
#endif
	unlk	%fp
	rts
	CFI_ENDPROC()
	.size	ffi_closure_struct_SYSV,.-ffi_closure_struct_SYSV

#if defined __ELF__ && defined __linux__
	.section	.note.GNU-stack,"",@progbits
#endif
