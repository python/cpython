/* -----------------------------------------------------------------------
   n32.S - Copyright (c) 1996, 1998, 2005  Red Hat, Inc.
   
   MIPS Foreign Function Interface 

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#define LIBFFI_ASM	
#include <fficonfig.h>
#include <ffi.h>

/* Only build this code if we are compiling for n32 */	

#if defined(FFI_MIPS_N32)

#define callback a0
#define bytes	 a2
#define flags	 a3
#define raddr    a4
#define fn       a5

#define SIZEOF_FRAME	( 8 * FFI_SIZEOF_ARG )

	.abicalls
	.text
	.align	2
	.globl	ffi_call_N32
	.ent	ffi_call_N32
ffi_call_N32:	

	# Prologue
	SUBU	$sp, SIZEOF_FRAME			# Frame size
	REG_S	$fp, SIZEOF_FRAME - 2*FFI_SIZEOF_ARG($sp)	# Save frame pointer
	REG_S	ra, SIZEOF_FRAME - 1*FFI_SIZEOF_ARG($sp)	# Save return address
	move	$fp, $sp

	move	t9, callback	# callback function pointer
	REG_S	bytes, 2*FFI_SIZEOF_ARG($fp) # bytes
	REG_S	flags, 3*FFI_SIZEOF_ARG($fp) # flags
	REG_S	raddr, 4*FFI_SIZEOF_ARG($fp) # raddr
	REG_S	fn,    5*FFI_SIZEOF_ARG($fp) # fn

	# Allocate at least 4 words in the argstack
	move	v0, bytes
	bge	bytes, 4 * FFI_SIZEOF_ARG, bigger	
	LI	v0, 4 * FFI_SIZEOF_ARG
	b	sixteen

	bigger:	
	ADDU	t4, v0, 2 * FFI_SIZEOF_ARG -1	# make sure it is aligned 
	and	v0, t4, -2 * FFI_SIZEOF_ARG		# to a proper boundry.

sixteen:
	SUBU	$sp, $sp, v0	# move the stack pointer to reflect the
				# arg space

	ADDU	a0, $sp, 0      # 4 * FFI_SIZEOF_ARG
	ADDU	a3, $fp, 3 * FFI_SIZEOF_ARG

	# Call ffi_prep_args
	jal	t9
	
	#	ADDU	$sp, $sp, 4 * FFI_SIZEOF_ARG	# adjust $sp to new args

	# Copy the stack pointer to t9
	move	t9, $sp
	
	# Fix the stack if there are more than 8 64bit slots worth
	# of arguments.

	# Load the number of bytes
	REG_L	t6, 2*FFI_SIZEOF_ARG($fp)

	# Is it bigger than 8 * FFI_SIZEOF_ARG?
	dadd	t7, $0, 8 * FFI_SIZEOF_ARG
	dsub	t8, t6, t7
	bltz	t8, loadregs

	add	t9, t9, t8
	
loadregs:	

	REG_L	t4, 3*FFI_SIZEOF_ARG($fp)  # load the flags word
	add	t6, t4, 0			      # and copy it into t6

	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg1_floatp
	REG_L	a0, 0*FFI_SIZEOF_ARG(t9)
	b	arg1_next
arg1_floatp:	
	bne	t4, FFI_TYPE_FLOAT, arg1_doublep
	l.s	$f12, 0*FFI_SIZEOF_ARG(t9)
	b	arg1_next
arg1_doublep:	
	l.d	$f12, 0*FFI_SIZEOF_ARG(t9)
arg1_next:	
	
	add	t4, t6, 0
	SRL	t4, 1*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg2_floatp
	REG_L	a1, 1*FFI_SIZEOF_ARG(t9)
	b	arg2_next
arg2_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg2_doublep
	l.s	$f13, 1*FFI_SIZEOF_ARG(t9)	
	b	arg2_next
arg2_doublep:	
	l.d	$f13, 1*FFI_SIZEOF_ARG(t9)	
arg2_next:	
	
	add	t4, t6, 0
	SRL	t4, 2*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg3_floatp
	REG_L	a2, 2*FFI_SIZEOF_ARG(t9)
	b	arg3_next
arg3_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg3_doublep
	l.s	$f14, 2*FFI_SIZEOF_ARG(t9)	
	b	arg3_next
arg3_doublep:	
	l.d	$f14, 2*FFI_SIZEOF_ARG(t9)	
arg3_next:	
	
	add	t4, t6, 0
	SRL	t4, 3*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg4_floatp
	REG_L	a3, 3*FFI_SIZEOF_ARG(t9)
	b	arg4_next
arg4_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg4_doublep
	l.s	$f15, 3*FFI_SIZEOF_ARG(t9)	
	b	arg4_next
arg4_doublep:	
	l.d	$f15, 3*FFI_SIZEOF_ARG(t9)	
arg4_next:	
	
	add	t4, t6, 0
	SRL	t4, 4*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg5_floatp
	REG_L	a4, 4*FFI_SIZEOF_ARG(t9)
	b	arg5_next
arg5_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg5_doublep
	l.s	$f16, 4*FFI_SIZEOF_ARG(t9)	
	b	arg5_next
arg5_doublep:	
	l.d	$f16, 4*FFI_SIZEOF_ARG(t9)	
arg5_next:	
	
	add	t4, t6, 0
	SRL	t4, 5*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg6_floatp
	REG_L	a5, 5*FFI_SIZEOF_ARG(t9)
	b	arg6_next
arg6_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg6_doublep
	l.s	$f17, 5*FFI_SIZEOF_ARG(t9)	
	b	arg6_next
arg6_doublep:	
	l.d	$f17, 5*FFI_SIZEOF_ARG(t9)	
arg6_next:	
	
	add	t4, t6, 0
	SRL	t4, 6*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg7_floatp
	REG_L	a6, 6*FFI_SIZEOF_ARG(t9)
	b	arg7_next
arg7_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg7_doublep
	l.s	$f18, 6*FFI_SIZEOF_ARG(t9)	
	b	arg7_next
arg7_doublep:	
	l.d	$f18, 6*FFI_SIZEOF_ARG(t9)	
arg7_next:	
	
	add	t4, t6, 0
	SRL	t4, 7*FFI_FLAG_BITS
	and	t4, ((1<<FFI_FLAG_BITS)-1)
	bnez	t4, arg8_floatp
	REG_L	a7, 7*FFI_SIZEOF_ARG(t9)
	b	arg8_next
arg8_floatp:
	bne	t4, FFI_TYPE_FLOAT, arg8_doublep
 	l.s	$f19, 7*FFI_SIZEOF_ARG(t9)	
	b	arg8_next
arg8_doublep:	
 	l.d	$f19, 7*FFI_SIZEOF_ARG(t9)	
arg8_next:	

callit:		
	# Load the function pointer
	REG_L	t9, 5*FFI_SIZEOF_ARG($fp)

	# If the return value pointer is NULL, assume no return value.
	REG_L	t5, 4*FFI_SIZEOF_ARG($fp)
	beqz	t5, noretval

	# Shift the return type flag over
	SRL	t6, 8*FFI_FLAG_BITS
	
	bne     t6, FFI_TYPE_INT, retfloat
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	REG_S	v0, 0(t4)
	b	epilogue

retfloat:
	bne     t6, FFI_TYPE_FLOAT, retdouble
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.s	$f0, 0(t4)
	b	epilogue

retdouble:	
	bne	t6, FFI_TYPE_DOUBLE, retstruct_d
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.d	$f0, 0(t4)
	b	epilogue

retstruct_d:	
	bne	t6, FFI_TYPE_STRUCT_D, retstruct_f
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.d	$f0, 0(t4)
	b	epilogue
	
retstruct_f:	
	bne	t6, FFI_TYPE_STRUCT_F, retstruct_d_d
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.s	$f0, 0(t4)
	b	epilogue
	
retstruct_d_d:	
	bne	t6, FFI_TYPE_STRUCT_DD, retstruct_f_f
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.d	$f0, 0(t4)
	s.d	$f2, 8(t4)
	b	epilogue
	
retstruct_f_f:	
	bne	t6, FFI_TYPE_STRUCT_FF, retstruct_d_f
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.s	$f0, 0(t4)
	s.s	$f2, 4(t4)
	b	epilogue
	
retstruct_d_f:	
	bne	t6, FFI_TYPE_STRUCT_DF, retstruct_f_d
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.d	$f0, 0(t4)
	s.s	$f2, 8(t4)
	b	epilogue
	
retstruct_f_d:	
	bne	t6, FFI_TYPE_STRUCT_FD, retstruct_small
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	s.s	$f0, 0(t4)
	s.d	$f2, 8(t4)
	b	epilogue
	
retstruct_small:	
	bne	t6, FFI_TYPE_STRUCT_SMALL, retstruct_small2
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	REG_S	v0, 0(t4)
	b	epilogue
	
retstruct_small2:	
	bne	t6, FFI_TYPE_STRUCT_SMALL2, retstruct
	jal	t9
	REG_L	t4, 4*FFI_SIZEOF_ARG($fp)
	REG_S	v0, 0(t4)
	REG_S	v1, 8(t4)
	b	epilogue
	
retstruct:	
noretval:	
	jal	t9
	
	# Epilogue
epilogue:	
	move	$sp, $fp	
	REG_L	$fp, SIZEOF_FRAME - 2*FFI_SIZEOF_ARG($sp) # Restore frame pointer
	REG_L	ra, SIZEOF_FRAME - 1*FFI_SIZEOF_ARG($sp)  # Restore return address
	ADDU	$sp, SIZEOF_FRAME		      # Fix stack pointer
	j	ra

	.end	ffi_call_N32
	
#endif
