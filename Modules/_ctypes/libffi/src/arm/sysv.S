/* -----------------------------------------------------------------------
   sysv.S - Copyright (c) 1998, 2008 Red Hat, Inc.
   
   ARM Foreign Function Interface 

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
#ifdef HAVE_MACHINE_ASM_H
#include <machine/asm.h>
#else
#ifdef __USER_LABEL_PREFIX__
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

/* Use the right prefix for global labels.  */
#define CNAME(x) CONCAT1 (__USER_LABEL_PREFIX__, x)
#else
#define CNAME(x) x
#endif
#define ENTRY(x) .globl CNAME(x); .type CNAME(x),%function; CNAME(x):
#endif

#ifdef __ELF__
#define LSYM(x) .x
#else
#define LSYM(x) x
#endif

/* We need a better way of testing for this, but for now, this is all 
   we can do.  */
@ This selects the minimum architecture level required.
#define __ARM_ARCH__ 3

#if defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__)
# undef __ARM_ARCH__
# define __ARM_ARCH__ 4
#endif
        
#if defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5T__) \
	|| defined(__ARM_ARCH_5E__) || defined(__ARM_ARCH_5TE__) \
	|| defined(__ARM_ARCH_5TEJ__)
# undef __ARM_ARCH__
# define __ARM_ARCH__ 5
#endif

#if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) \
        || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) \
        || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) \
	|| defined(__ARM_ARCH_6M__)
# undef __ARM_ARCH__
# define __ARM_ARCH__ 6
#endif

#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \
        || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) \
	|| defined(__ARM_ARCH_7EM__)
# undef __ARM_ARCH__
# define __ARM_ARCH__ 7
#endif

#if __ARM_ARCH__ >= 5
# define call_reg(x)	blx	x
#elif defined (__ARM_ARCH_4T__)
# define call_reg(x)	mov	lr, pc ; bx	x
# if defined(__thumb__) || defined(__THUMB_INTERWORK__)
#  define __INTERWORKING__
# endif
#else
# define call_reg(x)	mov	lr, pc ; mov	pc, x
#endif

/* Conditionally compile unwinder directives.  */
#ifdef __ARM_EABI__
#define UNWIND
#else
#define UNWIND @
#endif	

	
#if defined(__thumb__) && !defined(__THUMB_INTERWORK__)
.macro	ARM_FUNC_START name
	.text
	.align 0
	.thumb
	.thumb_func
	ENTRY(\name)
	bx	pc
	nop
	.arm
	UNWIND .fnstart
/* A hook to tell gdb that we've switched to ARM mode.  Also used to call
   directly from other local arm routines.  */
_L__\name:		
.endm
#else
.macro	ARM_FUNC_START name
	.text
	.align 0
	.arm
	ENTRY(\name)
	UNWIND .fnstart
.endm
#endif

.macro	RETLDM	regs=, cond=, dirn=ia
#if defined (__INTERWORKING__)
	.ifc "\regs",""
	ldr\cond	lr, [sp], #4
	.else
	ldm\cond\dirn	sp!, {\regs, lr}
	.endif
	bx\cond	lr
#else
	.ifc "\regs",""
	ldr\cond	pc, [sp], #4
	.else
	ldm\cond\dirn	sp!, {\regs, pc}
	.endif
#endif
.endm


	@ r0:   ffi_prep_args
	@ r1:   &ecif
	@ r2:   cif->bytes
	@ r3:   fig->flags
	@ sp+0: ecif.rvalue
	@ sp+4: fn

	@ This assumes we are using gas.
ARM_FUNC_START ffi_call_SYSV
	@ Save registers
        stmfd	sp!, {r0-r3, fp, lr}
	UNWIND .save	{r0-r3, fp, lr}
	mov	fp, sp

	UNWIND .setfp	fp, sp

	@ Make room for all of the new args.
	sub	sp, fp, r2

	@ Place all of the ffi_prep_args in position
	mov	ip, r0
	mov	r0, sp
	@     r1 already set

	@ Call ffi_prep_args(stack, &ecif)
	call_reg(ip)

	@ move first 4 parameters in registers
	ldmia	sp, {r0-r3}

	@ and adjust stack
	ldr	ip, [fp, #8]
        cmp	ip, #16
	movhs	ip, #16
        add	sp, sp, ip

	@ call (fn) (...)
	ldr	ip, [fp, #28]
	call_reg(ip)
	
	@ Remove the space we pushed for the args
	mov	sp, fp

	@ Load r2 with the pointer to storage for the return value
	ldr	r2, [sp, #24]

	@ Load r3 with the return type code 
	ldr	r3, [sp, #12]

	@ If the return value pointer is NULL, assume no return value.
	cmp	r2, #0
	beq	LSYM(Lepilogue)

@ return INT
	cmp	r3, #FFI_TYPE_INT
#if defined(__SOFTFP__) || defined(__ARM_EABI__)
	cmpne	r3, #FFI_TYPE_FLOAT
#endif
	streq	r0, [r2]
	beq	LSYM(Lepilogue)

	@ return INT64
	cmp	r3, #FFI_TYPE_SINT64
#if defined(__SOFTFP__) || defined(__ARM_EABI__)
	cmpne	r3, #FFI_TYPE_DOUBLE
#endif
	stmeqia	r2, {r0, r1}

#if !defined(__SOFTFP__) && !defined(__ARM_EABI__)
	beq	LSYM(Lepilogue)

@ return FLOAT
	cmp	r3, #FFI_TYPE_FLOAT
	stfeqs	f0, [r2]
	beq	LSYM(Lepilogue)

@ return DOUBLE or LONGDOUBLE
	cmp	r3, #FFI_TYPE_DOUBLE
	stfeqd	f0, [r2]
#endif

LSYM(Lepilogue):
	RETLDM	"r0-r3,fp"

.ffi_call_SYSV_end:
	UNWIND .fnend
        .size    CNAME(ffi_call_SYSV),.ffi_call_SYSV_end-CNAME(ffi_call_SYSV)

/*
	unsigned int FFI_HIDDEN
	ffi_closure_SYSV_inner (closure, respp, args)
	     ffi_closure *closure;
	     void **respp;
  	     void *args;
*/

ARM_FUNC_START ffi_closure_SYSV
	UNWIND .pad #16
	add	ip, sp, #16
	stmfd	sp!, {ip, lr}
	UNWIND .save	{r0, lr}
	add	r2, sp, #8
	.pad #16
	sub	sp, sp, #16
	str	sp, [sp, #8]
	add	r1, sp, #8
	bl	ffi_closure_SYSV_inner
	cmp	r0, #FFI_TYPE_INT
	beq	.Lretint

	cmp	r0, #FFI_TYPE_FLOAT
#if defined(__SOFTFP__) || defined(__ARM_EABI__)
	beq	.Lretint
#else
	beq	.Lretfloat
#endif

	cmp	r0, #FFI_TYPE_DOUBLE
#if defined(__SOFTFP__) || defined(__ARM_EABI__)
	beq	.Lretlonglong
#else
	beq	.Lretdouble
#endif

	cmp	r0, #FFI_TYPE_LONGDOUBLE
#if defined(__SOFTFP__) || defined(__ARM_EABI__)
	beq	.Lretlonglong
#else
	beq	.Lretlongdouble
#endif

	cmp	r0, #FFI_TYPE_SINT64
	beq	.Lretlonglong
.Lclosure_epilogue:
	add	sp, sp, #16
	ldmfd	sp, {sp, pc}
.Lretint:
	ldr	r0, [sp]
	b	.Lclosure_epilogue
.Lretlonglong:
	ldr	r0, [sp]
	ldr	r1, [sp, #4]
	b	.Lclosure_epilogue

#if !defined(__SOFTFP__) && !defined(__ARM_EABI__)
.Lretfloat:
	ldfs	f0, [sp]
	b	.Lclosure_epilogue
.Lretdouble:
	ldfd	f0, [sp]
	b	.Lclosure_epilogue
.Lretlongdouble:
	ldfd	f0, [sp]
	b	.Lclosure_epilogue
#endif

.ffi_closure_SYSV_end:
	UNWIND .fnend
        .size    CNAME(ffi_closure_SYSV),.ffi_closure_SYSV_end-CNAME(ffi_closure_SYSV)

#if defined __ELF__ && defined __linux__
	.section	.note.GNU-stack,"",%progbits
#endif
