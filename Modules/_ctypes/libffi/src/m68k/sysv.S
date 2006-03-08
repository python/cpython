/* -----------------------------------------------------------------------
   sysv.S
   
   m68k Foreign Function Interface 
   ----------------------------------------------------------------------- */

#define LIBFFI_ASM	
#include <fficonfig.h>
#include <ffi.h>

	.text

	.globl	ffi_call_SYSV
	.type	ffi_call_SYSV,@function

ffi_call_SYSV:
	link	%fp,#0
	move.l	%d2,-(%sp)

	| Make room for all of the new args.
	sub.l	16(%fp),%sp

	| Call ffi_prep_args
	move.l	12(%fp),-(%sp)
	pea	4(%sp)
	move.l	8(%fp),%a0
	jsr	(%a0)
	addq.l	#8,%sp	

	| Pass pointer to struct value, if any
	move.l	%a0,%a1

	| Call the function
	move.l	32(%fp),%a0
	jsr	(%a0)

	| Remove the space we pushed for the args
	add.l	16(%fp),%sp

	| Load the pointer to storage for the return value
	move.l	28(%fp),%a1

	| Load the return type code 
	move.l	20(%fp),%d2

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
	jbeq	retstruct
	move.l	%a0,(%a1)
	jbra	epilogue

retstruct:
	btst	#6,%d2
	jbeq	noretval
	move.l	24(%fp),%d2
	bfins	%d0,(%a1){#0,%d2}

noretval:
epilogue:
	move.l	(%sp)+,%d2
	unlk	%a6
	rts
	.size	ffi_call_SYSV,.-ffi_call_SYSV
