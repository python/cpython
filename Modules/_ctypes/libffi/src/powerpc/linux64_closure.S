/* -----------------------------------------------------------------------
   sysv.h - Copyright (c) 2003 Jakub Jelinek <jakub@redhat.com>
	    Copyright (c) 2008 Red Hat, Inc.

   PowerPC64 Assembly glue.

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

	.file	"linux64_closure.S"

#ifdef POWERPC64
	FFI_HIDDEN (ffi_closure_LINUX64)
	.globl  ffi_closure_LINUX64
# if _CALL_ELF == 2
	.text
ffi_closure_LINUX64:
	addis	%r2, %r12, .TOC.-ffi_closure_LINUX64@ha
	addi	%r2, %r2, .TOC.-ffi_closure_LINUX64@l
	.localentry ffi_closure_LINUX64, . - ffi_closure_LINUX64
# else
	.section        ".opd","aw"
	.align  3
ffi_closure_LINUX64:
#  ifdef _CALL_LINUX
	.quad   .L.ffi_closure_LINUX64,.TOC.@tocbase,0
	.type   ffi_closure_LINUX64,@function
	.text
.L.ffi_closure_LINUX64:
#  else
	FFI_HIDDEN (.ffi_closure_LINUX64)
	.globl  .ffi_closure_LINUX64
	.quad   .ffi_closure_LINUX64,.TOC.@tocbase,0
	.size   ffi_closure_LINUX64,24
	.type   .ffi_closure_LINUX64,@function
	.text
.ffi_closure_LINUX64:
#  endif
# endif

# if _CALL_ELF == 2
#  32 byte special reg save area + 64 byte parm save area
#  + 64 byte retval area + 13*8 fpr save area + round to 16
#  define STACKFRAME 272
#  define PARMSAVE 32
#  define RETVAL PARMSAVE+64
# else
#  48 bytes special reg save area + 64 bytes parm save area
#  + 16 bytes retval area + 13*8 bytes fpr save area + round to 16
#  define STACKFRAME 240
#  define PARMSAVE 48
#  define RETVAL PARMSAVE+64
# endif

.LFB1:
# if _CALL_ELF == 2
	ld	%r12, FFI_TRAMPOLINE_SIZE(%r11)		# closure->cif
	mflr	%r0
	lwz	%r12, 28(%r12)				# cif->flags
	mtcrf	0x40, %r12
	addi	%r12, %r1, PARMSAVE
	bt	7, .Lparmsave
	# Our caller has not allocated a parameter save area.
	# We need to allocate one here and use it to pass gprs to
	# ffi_closure_helper_LINUX64.
	addi	%r12, %r1, -STACKFRAME+PARMSAVE
.Lparmsave:
	std	%r0, 16(%r1)
	# Save general regs into parm save area
	std	%r3, 0(%r12)
	std	%r4, 8(%r12)
	std	%r5, 16(%r12)
	std	%r6, 24(%r12)
	std	%r7, 32(%r12)
	std	%r8, 40(%r12)
	std	%r9, 48(%r12)
	std	%r10, 56(%r12)

	# load up the pointer to the parm save area
	mr	%r5, %r12
# else
	# copy r2 to r11 and load TOC into r2
	mr	%r11, %r2
	ld	%r2, 16(%r11)

	mflr	%r0
	# Save general regs into parm save area
	# This is the parameter save area set up by our caller.
	std	%r3, PARMSAVE+0(%r1)
	std	%r4, PARMSAVE+8(%r1)
	std	%r5, PARMSAVE+16(%r1)
	std	%r6, PARMSAVE+24(%r1)
	std	%r7, PARMSAVE+32(%r1)
	std	%r8, PARMSAVE+40(%r1)
	std	%r9, PARMSAVE+48(%r1)
	std	%r10, PARMSAVE+56(%r1)

	std	%r0, 16(%r1)

	# load up the pointer to the parm save area
	addi	%r5, %r1, PARMSAVE
# endif

	# next save fpr 1 to fpr 13
	stfd	%f1, -104+(0*8)(%r1)
	stfd	%f2, -104+(1*8)(%r1)
	stfd	%f3, -104+(2*8)(%r1)
	stfd	%f4, -104+(3*8)(%r1)
	stfd	%f5, -104+(4*8)(%r1)
	stfd	%f6, -104+(5*8)(%r1)
	stfd	%f7, -104+(6*8)(%r1)
	stfd	%f8, -104+(7*8)(%r1)
	stfd	%f9, -104+(8*8)(%r1)
	stfd	%f10, -104+(9*8)(%r1)
	stfd	%f11, -104+(10*8)(%r1)
	stfd	%f12, -104+(11*8)(%r1)
	stfd	%f13, -104+(12*8)(%r1)

	# load up the pointer to the saved fpr registers */
	addi	%r6, %r1, -104

	# load up the pointer to the result storage
	addi	%r4, %r1, -STACKFRAME+RETVAL

	stdu	%r1, -STACKFRAME(%r1)
.LCFI0:

	# get the context pointer from the trampoline
	mr	%r3, %r11

	# make the call
# if defined _CALL_LINUX || _CALL_ELF == 2
	bl ffi_closure_helper_LINUX64
# else
	bl .ffi_closure_helper_LINUX64
# endif
.Lret:

	# now r3 contains the return type
	# so use it to look up in a table
	# so we know how to deal with each type

	# look up the proper starting point in table
	# by using return type as offset
	ld %r0, STACKFRAME+16(%r1)
	cmpldi %r3, FFI_V2_TYPE_SMALL_STRUCT
	bge .Lsmall
	mflr %r4		# move address of .Lret to r4
	sldi %r3, %r3, 4	# now multiply return type by 16
	addi %r4, %r4, .Lret_type0 - .Lret
	add %r3, %r3, %r4	# add contents of table to table address
	mtctr %r3
	bctr			# jump to it

# Each of the ret_typeX code fragments has to be exactly 16 bytes long
# (4 instructions). For cache effectiveness we align to a 16 byte boundary
# first.
	.align 4

.Lret_type0:
# case FFI_TYPE_VOID
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
	nop
# case FFI_TYPE_INT
# ifdef __LITTLE_ENDIAN__
	lwa %r3, RETVAL+0(%r1)
# else
	lwa %r3, RETVAL+4(%r1)
# endif
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_FLOAT
	lfs %f1, RETVAL+0(%r1)
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_DOUBLE
	lfd %f1, RETVAL+0(%r1)
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_LONGDOUBLE
	lfd %f1, RETVAL+0(%r1)
	mtlr %r0
	lfd %f2, RETVAL+8(%r1)
	b .Lfinish
# case FFI_TYPE_UINT8
# ifdef __LITTLE_ENDIAN__
	lbz %r3, RETVAL+0(%r1)
# else
	lbz %r3, RETVAL+7(%r1)
# endif
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_SINT8
# ifdef __LITTLE_ENDIAN__
	lbz %r3, RETVAL+0(%r1)
# else
	lbz %r3, RETVAL+7(%r1)
# endif
	extsb %r3,%r3
	mtlr %r0
	b .Lfinish
# case FFI_TYPE_UINT16
# ifdef __LITTLE_ENDIAN__
	lhz %r3, RETVAL+0(%r1)
# else
	lhz %r3, RETVAL+6(%r1)
# endif
	mtlr %r0
.Lfinish:
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_SINT16
# ifdef __LITTLE_ENDIAN__
	lha %r3, RETVAL+0(%r1)
# else
	lha %r3, RETVAL+6(%r1)
# endif
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_UINT32
# ifdef __LITTLE_ENDIAN__
	lwz %r3, RETVAL+0(%r1)
# else
	lwz %r3, RETVAL+4(%r1)
# endif
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_SINT32
# ifdef __LITTLE_ENDIAN__
	lwa %r3, RETVAL+0(%r1)
# else
	lwa %r3, RETVAL+4(%r1)
# endif
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_UINT64
	ld %r3, RETVAL+0(%r1)
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_SINT64
	ld %r3, RETVAL+0(%r1)
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_TYPE_STRUCT
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
	nop
# case FFI_TYPE_POINTER
	ld %r3, RETVAL+0(%r1)
	mtlr %r0
	addi %r1, %r1, STACKFRAME
	blr
# case FFI_V2_TYPE_FLOAT_HOMOG
	lfs %f1, RETVAL+0(%r1)
	lfs %f2, RETVAL+4(%r1)
	lfs %f3, RETVAL+8(%r1)
	b .Lmorefloat
# case FFI_V2_TYPE_DOUBLE_HOMOG
	lfd %f1, RETVAL+0(%r1)
	lfd %f2, RETVAL+8(%r1)
	lfd %f3, RETVAL+16(%r1)
	lfd %f4, RETVAL+24(%r1)
	mtlr %r0
	lfd %f5, RETVAL+32(%r1)
	lfd %f6, RETVAL+40(%r1)
	lfd %f7, RETVAL+48(%r1)
	lfd %f8, RETVAL+56(%r1)
	addi %r1, %r1, STACKFRAME
	blr
.Lmorefloat:
	lfs %f4, RETVAL+12(%r1)
	mtlr %r0
	lfs %f5, RETVAL+16(%r1)
	lfs %f6, RETVAL+20(%r1)
	lfs %f7, RETVAL+24(%r1)
	lfs %f8, RETVAL+28(%r1)
	addi %r1, %r1, STACKFRAME
	blr
.Lsmall:
# ifdef __LITTLE_ENDIAN__
	ld %r3,RETVAL+0(%r1)
	mtlr %r0
	ld %r4,RETVAL+8(%r1)
	addi %r1, %r1, STACKFRAME
	blr
# else
	# A struct smaller than a dword is returned in the low bits of r3
	# ie. right justified.  Larger structs are passed left justified
	# in r3 and r4.  The return value area on the stack will have
	# the structs as they are usually stored in memory.
	cmpldi %r3, FFI_V2_TYPE_SMALL_STRUCT + 7 # size 8 bytes?
	neg %r5, %r3
	ld %r3,RETVAL+0(%r1)
	blt .Lsmalldown
	mtlr %r0
	ld %r4,RETVAL+8(%r1)
	addi %r1, %r1, STACKFRAME
	blr
.Lsmalldown:
	addi %r5, %r5, FFI_V2_TYPE_SMALL_STRUCT + 7
	mtlr %r0
	sldi %r5, %r5, 3
	addi %r1, %r1, STACKFRAME
	srd %r3, %r3, %r5
	blr
# endif

.LFE1:
	.long	0
	.byte	0,12,0,1,128,0,0,0
# if _CALL_ELF == 2
	.size	ffi_closure_LINUX64,.-ffi_closure_LINUX64
# else
#  ifdef _CALL_LINUX
	.size	ffi_closure_LINUX64,.-.L.ffi_closure_LINUX64
#  else
	.size	.ffi_closure_LINUX64,.-.ffi_closure_LINUX64
#  endif
# endif

	.section	.eh_frame,EH_FRAME_FLAGS,@progbits
.Lframe1:
	.4byte	.LECIE1-.LSCIE1	 # Length of Common Information Entry
.LSCIE1:
	.4byte	0x0	 # CIE Identifier Tag
	.byte	0x1	 # CIE Version
	.ascii "zR\0"	 # CIE Augmentation
	.uleb128 0x1	 # CIE Code Alignment Factor
	.sleb128 -8	 # CIE Data Alignment Factor
	.byte	0x41	 # CIE RA Column
	.uleb128 0x1	 # Augmentation size
	.byte	0x14	 # FDE Encoding (pcrel udata8)
	.byte	0xc	 # DW_CFA_def_cfa
	.uleb128 0x1
	.uleb128 0x0
	.align 3
.LECIE1:
.LSFDE1:
	.4byte	.LEFDE1-.LASFDE1	 # FDE Length
.LASFDE1:
	.4byte	.LASFDE1-.Lframe1	 # FDE CIE offset
	.8byte	.LFB1-.	 # FDE initial location
	.8byte	.LFE1-.LFB1	 # FDE address range
	.uleb128 0x0	 # Augmentation size
	.byte	0x2	 # DW_CFA_advance_loc1
	.byte	.LCFI0-.LFB1
	.byte	0xe	 # DW_CFA_def_cfa_offset
	.uleb128 STACKFRAME
	.byte	0x11	 # DW_CFA_offset_extended_sf
	.uleb128 0x41
	.sleb128 -2
	.align 3
.LEFDE1:

# if defined __ELF__ && defined __linux__
	.section	.note.GNU-stack,"",@progbits
# endif
#endif
