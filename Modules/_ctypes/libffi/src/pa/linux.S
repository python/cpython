/* -----------------------------------------------------------------------
   linux.S - (c) 2003-2004 Randolph Chung <tausq@debian.org>

   HPPA Foreign Function Interface

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

	.text
	.level 1.1
	.align 4

	/* void ffi_call_LINUX(void (*)(char *, extended_cif *),
			       extended_cif *ecif,
			       unsigned bytes,
			       unsigned flags,
			       unsigned *rvalue,
			       void (*fn)());
	 */

	.export ffi_call_LINUX,code
	.import ffi_prep_args_LINUX,code

	.type ffi_call_LINUX, @function
.LFB1:
ffi_call_LINUX:
	.proc
	.callinfo FRAME=64,CALLS,SAVE_RP,SAVE_SP,ENTRY_GR=4
	.entry
	stw %rp, -20(%sp)
	copy %r3, %r1
.LCFI11:

	copy %sp, %r3
.LCFI12:

	/* Setup the stack for calling prep_args...
	   We want the stack to look like this:

	   [ Previous stack                            ] <- %r3

	   [ 64-bytes register save area               ] <- %r4

	   [ Stack space for actual call, passed as    ] <- %arg0
	   [     arg0 to ffi_prep_args_LINUX           ]

	   [ Stack for calling prep_args               ] <- %sp
	 */

	stwm %r1, 64(%sp)
	stw %r4, 12(%r3)
.LCFI13:
	copy %sp, %r4

	addl %arg2, %r4, %arg0                  /* arg stack */
	stw %arg3, -48(%r3)                     /* save flags; we need it later */

	/* Call prep_args:
	   %arg0(stack) -- set up above
	   %arg1(ecif) -- same as incoming param
	   %arg2(bytes) -- same as incoming param */
	bl ffi_prep_args_LINUX,%r2
	ldo 64(%arg0), %sp
	ldo -64(%sp), %sp

	/* now %sp should point where %arg0 was pointing.  */

	/* Load the arguments that should be passed in registers
	   The fp args were loaded by the prep_args function.  */
	ldw -36(%sp), %arg0
	ldw -40(%sp), %arg1
	ldw -44(%sp), %arg2
	ldw -48(%sp), %arg3

	/* in case the function is going to return a structure
	   we need to give it a place to put the result.  */
	ldw -52(%r3), %ret0                     /* %ret0 <- rvalue */
	ldw -56(%r3), %r22                      /* %r22 <- function to call */
	bl $$dyncall, %r31                      /* Call the user function */
	copy %r31, %rp

	/* Prepare to store the result; we need to recover flags and rvalue.  */
	ldw -48(%r3), %r21                      /* r21 <- flags */
	ldw -52(%r3), %r20                      /* r20 <- rvalue */

	/* Store the result according to the return type.  */

checksmst3:
	comib,<>,n FFI_TYPE_SMALL_STRUCT3, %r21, checksmst567
	/* 3-byte structs are returned in ret0 as ??xxyyzz.  Shift
	   left 8 bits to write to the result structure.  */
	zdep %ret0, 23, 24, %r22
	b done
	stw %r22, 0(%r20)

checksmst567:
	/* 5-7 byte values are returned right justified:
	      ret0     ret1
	   5: ??????aa bbccddee
	   6: ????aabb ccddeeff
	   7: ??aabbcc ddeeffgg

	   To store this in the result, write the first 4 bytes into a temp
	   register using shrpw (t1 = aabbccdd), followed by a rotation of
	   ret1:

	      ret0     ret1	   ret1
	   5: ??????aa bbccddee -> eebbccdd (rotate 8)
	   6: ????aabb ccddeeff -> eeffccdd (rotate 16)
	   7: ??aabbcc ddeeffgg -> eeffggdd (rotate 24)

	   then we write (t1, ret1) into the result.  */

	addi,<> -FFI_TYPE_SMALL_STRUCT5,%r21,%r0
	ldi 8, %r22
	addi,<> -FFI_TYPE_SMALL_STRUCT6,%r21,%r0
	ldi 16, %r22
	addi,<> -FFI_TYPE_SMALL_STRUCT7,%r21,%r0
	ldi 24, %r22

	/* This relies on all the FFI_TYPE_*_STRUCT* defines being <0 */
	cmpib,<=,n 0, %r21, checkint8
	mtsar %r22

	shrpw %ret0, %ret1, %sar, %ret0  /* ret0 = aabbccdd */
	shrpw %ret1, %ret1, %sar, %ret1  /* rotate ret1 */
	
	stw %ret0, 0(%r20)
	b done
	stw %ret1, 4(%r20)

checkint8:
	comib,<>,n FFI_TYPE_UINT8, %r21, checkint16
	b done
	stb %ret0, 0(%r20)

checkint16:
	comib,<>,n FFI_TYPE_UINT16, %r21, checkint32
	b done
	sth %ret0, 0(%r20)

checkint32:
	comib,<>,n FFI_TYPE_UINT32, %r21, checkint
	b done
	stw %ret0, 0(%r20)

checkint:
	comib,<>,n FFI_TYPE_INT, %r21, checkll
	b done
	stw %ret0, 0(%r20)

checkll:
	comib,<>,n FFI_TYPE_UINT64, %r21, checkdbl
	stw %ret0, 0(%r20)
	b done
	stw %ret1, 4(%r20)

checkdbl:
	comib,<>,n FFI_TYPE_DOUBLE, %r21, checkfloat
	b done
	fstd %fr4,0(%r20)

checkfloat:
	comib,<>,n FFI_TYPE_FLOAT, %r21, done
	fstw %fr4L,0(%r20)

	/* structure returns are either handled by one of the
	   INT/UINT64 cases above, or, if passed by pointer,
	   is handled by the callee.  */

done:
	/* all done, return */
	copy %r4, %sp                           /* pop arg stack */
	ldw 12(%r3), %r4
	ldwm -64(%sp), %r3                      /* .. and pop stack */
	ldw -20(%sp), %rp
	bv %r0(%rp)
	nop
	.exit
	.procend
.LFE1:

	/* void ffi_closure_LINUX(void);
	   Called with closure argument in %r21 */
	.export ffi_closure_LINUX,code
	.import ffi_closure_inner_LINUX,code

	.type ffi_closure_LINUX, @function
.LFB2:
ffi_closure_LINUX:
	.proc
	.callinfo FRAME=64,CALLS,SAVE_RP,SAVE_SP,ENTRY_GR=3
	.entry

	stw %rp, -20(%sp)
.LCFI20:
	copy %r3, %r1
.LCFI21:
	copy %sp, %r3
.LCFI22:
	stwm %r1, 64(%sp)

	/* Put arguments onto the stack and call ffi_closure_inner.  */
	stw %arg0, -36(%r3)
	stw %arg1, -40(%r3)
	stw %arg2, -44(%r3)
	stw %arg3, -48(%r3)

	copy %r21, %arg0
	bl ffi_closure_inner_LINUX, %r2
	copy %r3, %arg1

	ldwm -64(%sp), %r3
	ldw -20(%sp), %rp
	ldw -36(%sp), %ret0
	bv %r0(%r2)
	ldw -40(%sp), %ret1

	.exit
	.procend
.LFE2:

	.section        ".eh_frame",EH_FRAME_FLAGS,@progbits
.Lframe1:
	.word   .LECIE1-.LSCIE1 ;# Length of Common Information Entry
.LSCIE1:
	.word   0x0     ;# CIE Identifier Tag
	.byte   0x1     ;# CIE Version
	.ascii "\0"     ;# CIE Augmentation
	.uleb128 0x1    ;# CIE Code Alignment Factor
	.sleb128 4      ;# CIE Data Alignment Factor
	.byte   0x2     ;# CIE RA Column
	.byte   0xc     ;# DW_CFA_def_cfa
	.uleb128 0x1e
	.uleb128 0x0
	.align 4
.LECIE1:
.LSFDE1:
	.word   .LEFDE1-.LASFDE1        ;# FDE Length
.LASFDE1:
	.word   .LASFDE1-.Lframe1       ;# FDE CIE offset
	.word   .LFB1   ;# FDE initial location
	.word   .LFE1-.LFB1     ;# FDE address range

	.byte   0x4     ;# DW_CFA_advance_loc4
	.word   .LCFI11-.LFB1
	.byte	0x83	;# DW_CFA_offset, column 0x3
	.uleb128 0x0
	.byte   0x11    ;# DW_CFA_offset_extended_sf; save r2 at [r30-20]
	.uleb128 0x2
	.sleb128 -5

	.byte   0x4     ;# DW_CFA_advance_loc4
	.word   .LCFI12-.LCFI11
	.byte   0xd     ;# DW_CFA_def_cfa_register = r3
	.uleb128 0x3

	.byte   0x4     ;# DW_CFA_advance_loc4
	.word   .LCFI13-.LCFI12
	.byte	0x84	;# DW_CFA_offset, column 0x4
	.uleb128 0x3

	.align 4
.LEFDE1:

.LSFDE2:
	.word   .LEFDE2-.LASFDE2        ;# FDE Length
.LASFDE2:
	.word   .LASFDE2-.Lframe1       ;# FDE CIE offset
	.word   .LFB2   ;# FDE initial location
	.word   .LFE2-.LFB2     ;# FDE address range
	.byte   0x4     ;# DW_CFA_advance_loc4
	.word   .LCFI21-.LFB2
	.byte   0x83    ;# DW_CFA_offset, column 0x3
	.uleb128 0x0
	.byte   0x11    ;# DW_CFA_offset_extended_sf
	.uleb128 0x2
	.sleb128 -5

	.byte   0x4     ;# DW_CFA_advance_loc4
	.word   .LCFI12-.LCFI11
	.byte   0xd     ;# DW_CFA_def_cfa_register = r3
	.uleb128 0x3

	.align 4
.LEFDE2:
