#if defined(__ppc__) || defined(__ppc64__) 

/* -----------------------------------------------------------------------
   darwin.S - Copyright (c) 2000 John Hornkvist
	      Copyright (c) 2004 Free Software Foundation, Inc.

   PowerPC Assembly glue.

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
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#define LIBFFI_ASM

#include <fficonfig.h>
#include <ffi.h>
#include <ppc-darwin.h>
#include <architecture/ppc/mode_independent_asm.h>

.text
	.align 2
.globl _ffi_prep_args

.text
	.align 2
.globl _ffi_call_DARWIN

.text
	.align 2
_ffi_call_DARWIN:
LFB0:
	mr		r12,r8	/* We only need r12 until the call,
					so it doesn't have to be saved.  */

LFB1:
	/* Save the old stack pointer as AP.  */
	mr		r8,r1

LCFI0:
#if defined(__ppc64__)
	/*	Allocate the stack space we need.
		r4 (size of input data)
		48 bytes (linkage area)
		40 bytes (saved registers)
		8 bytes (extra FPR)
		r4 + 96 bytes total
	*/

	addi	r4,r4,-96	// Add our overhead.
	li		r0,-32		// Align to 32 bytes.
	and		r4,r4,r0
#endif
	stgux	r1,r1,r4	// Grow the stack.
	mflr	r9

	/* Save registers we use.  */
#if defined(__ppc64__)
	std		r27,-40(r8)
#endif
	stg		r28,MODE_CHOICE(-16,-32)(r8)
	stg		r29,MODE_CHOICE(-12,-24)(r8)
	stg		r30,MODE_CHOICE(-8,-16)(r8)
	stg		r31,MODE_CHOICE(-4,-8)(r8)
	stg		r9,SF_RETURN(r8)			/* return address */
#if !defined(POWERPC_DARWIN)			/* TOC unused in OS X */
	stg		r2,MODE_CHOICE(20,40)(r1)
#endif

LCFI1:
#if defined(__ppc64__)
	mr		r27,r3	// our extended_cif
#endif
	/* Save arguments over call.  */
	mr		r31,r5	/* flags,  */
	mr		r30,r6	/* rvalue,  */
	mr		r29,r7	/* function address,  */
	mr		r28,r8	/* our AP.  */

LCFI2:
	/* Call ffi_prep_args.  */
	mr		r4,r1
	li		r9,0
	mtctr	r12 /* r12 holds address of _ffi_prep_args.  */
	bctrl
#if !defined(POWERPC_DARWIN)			/* TOC unused in OS X */
	lg		r2,MODE_CHOICE(20,40)(r1)
#endif

	/* Now do the call.
	   Set up cr1 with bits 4-7 of the flags.  */
	mtcrf	0x40,r31

	/* Load all those argument registers.
	   We have set up a nice stack frame, just load it into registers.  */
	lg		r3,SF_ARG1(r1)
	lg		r4,SF_ARG2(r1)
	lg		r5,SF_ARG3(r1)
	lg		r6,SF_ARG4(r1)
	nop
	lg		r7,SF_ARG5(r1)
	lg		r8,SF_ARG6(r1)
	lg		r9,SF_ARG7(r1)
	lg		r10,SF_ARG8(r1)

	/* Load all the FP registers.  */
	bf		6,L2	/* No floats to load.  */
#if defined(__ppc64__)
	lfd		f1,MODE_CHOICE(-16,-40)-(14*8)(r28)
	lfd		f2,MODE_CHOICE(-16,-40)-(13*8)(r28)
	lfd		f3,MODE_CHOICE(-16,-40)-(12*8)(r28)
	lfd		f4,MODE_CHOICE(-16,-40)-(11*8)(r28)
	nop
	lfd		f5,MODE_CHOICE(-16,-40)-(10*8)(r28)
	lfd		f6,MODE_CHOICE(-16,-40)-(9*8)(r28)
	lfd		f7,MODE_CHOICE(-16,-40)-(8*8)(r28)
	lfd		f8,MODE_CHOICE(-16,-40)-(7*8)(r28)
	nop
	lfd		f9,MODE_CHOICE(-16,-40)-(6*8)(r28)
	lfd		f10,MODE_CHOICE(-16,-40)-(5*8)(r28)
	lfd		f11,MODE_CHOICE(-16,-40)-(4*8)(r28)
	lfd		f12,MODE_CHOICE(-16,-40)-(3*8)(r28)
	nop
	lfd		f13,MODE_CHOICE(-16,-40)-(2*8)(r28)
	lfd		f14,MODE_CHOICE(-16,-40)-(1*8)(r28)
#elif defined(__ppc__)
	lfd		f1,MODE_CHOICE(-16,-40)-(13*8)(r28)
	lfd		f2,MODE_CHOICE(-16,-40)-(12*8)(r28)
	lfd		f3,MODE_CHOICE(-16,-40)-(11*8)(r28)
	lfd		f4,MODE_CHOICE(-16,-40)-(10*8)(r28)
	nop
	lfd		f5,MODE_CHOICE(-16,-40)-(9*8)(r28)
	lfd		f6,MODE_CHOICE(-16,-40)-(8*8)(r28)
	lfd		f7,MODE_CHOICE(-16,-40)-(7*8)(r28)
	lfd		f8,MODE_CHOICE(-16,-40)-(6*8)(r28)
	nop
	lfd		f9,MODE_CHOICE(-16,-40)-(5*8)(r28)
	lfd		f10,MODE_CHOICE(-16,-40)-(4*8)(r28)
	lfd		f11,MODE_CHOICE(-16,-40)-(3*8)(r28)
	lfd		f12,MODE_CHOICE(-16,-40)-(2*8)(r28)
	nop
	lfd		f13,MODE_CHOICE(-16,-40)-(1*8)(r28)
#else
#error undefined architecture
#endif

L2:
	mr		r12,r29	// Put the target address in r12 as specified.
	mtctr  	r12		// Get the address to call into CTR.
	nop
	nop
	bctrl	// Make the call.

	// Deal with the return value.
#if defined(__ppc64__)
	mtcrf	0x3,r31		// flags in cr6 and cr7
	bt		27,L(st_return_value)
#elif defined(__ppc__)
	mtcrf	0x1,r31		// flags in cr7
#else
#error undefined architecture
#endif

	bt		30,L(done_return_value)
	bt		29,L(fp_return_value)
	stg		r3,0(r30)
#if defined(__ppc__)
	bf		28,L(done_return_value)	// Store the second long if necessary.
	stg		r4,4(r30)
#endif
	// Fall through

L(done_return_value):
	lg		r1,0(r1)			// Restore stack pointer.
	// Restore the registers we used.
	lg		r9,SF_RETURN(r1)	// return address
	lg		r31,MODE_CHOICE(-4,-8)(r1)
	mtlr	r9
	lg		r30,MODE_CHOICE(-8,-16)(r1)
	lg		r29,MODE_CHOICE(-12,-24)(r1)
	lg		r28,MODE_CHOICE(-16,-32)(r1)
#if defined(__ppc64__)
	ld		r27,-40(r1)
#endif
	blr	

#if defined(__ppc64__)
L(st_return_value):
	// Grow the stack enough to fit the registers. Leave room for 8 args
	// to trample the 1st 8 slots in param area.
	stgu	r1,-SF_ROUND(280)(r1)	// 64 + 104 + 48 + 64

	// Store GPRs
	std		r3,SF_ARG9(r1)
	std		r4,SF_ARG10(r1)
	std		r5,SF_ARG11(r1)
	std		r6,SF_ARG12(r1)
	nop
	std		r7,SF_ARG13(r1)
	std		r8,SF_ARG14(r1)
	std		r9,SF_ARG15(r1)
	std		r10,SF_ARG16(r1)

	// Store FPRs
	nop
	bf		26,L(call_struct_to_ram_form)
	stfd	f1,SF_ARG17(r1)
	stfd	f2,SF_ARG18(r1)
	stfd	f3,SF_ARG19(r1)
	stfd	f4,SF_ARG20(r1)
	nop
	stfd	f5,SF_ARG21(r1)
	stfd	f6,SF_ARG22(r1)
	stfd	f7,SF_ARG23(r1)
	stfd	f8,SF_ARG24(r1)
	nop
	stfd	f9,SF_ARG25(r1)
	stfd	f10,SF_ARG26(r1)
	stfd	f11,SF_ARG27(r1)
	stfd	f12,SF_ARG28(r1)
	nop
	stfd	f13,SF_ARG29(r1)

L(call_struct_to_ram_form):
	ld		r3,0(r27)			// extended_cif->cif*
	ld		r3,16(r3)			// ffi_cif->rtype*
	addi	r4,r1,SF_ARG9		// stored GPRs
	addi	r6,r1,SF_ARG17		// stored FPRs
	li		r5,0				// GPR size ptr (NULL)
	li		r7,0				// FPR size ptr (NULL)
	li		r8,0				// FPR count ptr (NULL)
	li		r10,0				// struct offset (NULL)
	mr		r9,r30				// return area
	bl		Lffi64_struct_to_ram_form$stub
	lg		r1,0(r1)			// Restore stack pointer.
	b		L(done_return_value)
#endif

L(fp_return_value):
	/* Do we have long double to store?  */
	bf		31,L(fd_return_value)
	stfd	f1,0(r30)
	stfd	f2,8(r30)
	b		L(done_return_value)

L(fd_return_value):
	/* Do we have double to store?  */
	bf		28,L(float_return_value)
	stfd	f1,0(r30)
	b		L(done_return_value)

L(float_return_value):
	/* We only have a float to store.  */
	stfs	f1,0(r30)
	b		L(done_return_value)

LFE1:
/* END(_ffi_call_DARWIN)  */

/* Provide a null definition of _ffi_call_AIX.  */
.text
	.align 2
.globl _ffi_call_AIX
.text
	.align 2
_ffi_call_AIX:
	blr
/* END(_ffi_call_AIX)  */

.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms
EH_frame1:
	.set	L$set$0,LECIE1-LSCIE1
	.long	L$set$0		; Length of Common Information Entry
LSCIE1:
	.long	0x0			; CIE Identifier Tag
	.byte	0x1			; CIE Version
	.ascii	"zR\0"		; CIE Augmentation
	.byte	0x1			; uleb128 0x1; CIE Code Alignment Factor
	.byte	0x7c		; sleb128 -4; CIE Data Alignment Factor
	.byte	0x41		; CIE RA Column
	.byte	0x1			; uleb128 0x1; Augmentation size
	.byte	0x90		; FDE Encoding (indirect pcrel)
	.byte	0xc			; DW_CFA_def_cfa
	.byte	0x1			; uleb128 0x1
	.byte	0x0			; uleb128 0x0
	.align	LOG2_GPR_BYTES
LECIE1:
.globl _ffi_call_DARWIN.eh
_ffi_call_DARWIN.eh:
LSFDE1:
	.set	L$set$1,LEFDE1-LASFDE1
	.long	L$set$1		; FDE Length

LASFDE1:
	.long	LASFDE1-EH_frame1		; FDE CIE offset
	.g_long	LLFB0$non_lazy_ptr-.	; FDE initial location
	.set	L$set$3,LFE1-LFB0
	.g_long	L$set$3					; FDE address range
	.byte   0x0     				; uleb128 0x0; Augmentation size
	.byte	0x4						; DW_CFA_advance_loc4
	.set	L$set$4,LCFI0-LFB1
	.long	L$set$4
	.byte	0xd						; DW_CFA_def_cfa_register
	.byte	0x08					; uleb128 0x08
	.byte	0x4						; DW_CFA_advance_loc4
	.set	L$set$5,LCFI1-LCFI0
	.long	L$set$5
	.byte   0x11					; DW_CFA_offset_extended_sf
	.byte	0x41					; uleb128 0x41
	.byte   0x7e					; sleb128 -2
	.byte	0x9f					; DW_CFA_offset, column 0x1f
	.byte	0x1						; uleb128 0x1
	.byte	0x9e					; DW_CFA_offset, column 0x1e
	.byte	0x2						; uleb128 0x2
	.byte	0x9d					; DW_CFA_offset, column 0x1d
	.byte	0x3						; uleb128 0x3
	.byte	0x9c					; DW_CFA_offset, column 0x1c
	.byte	0x4						; uleb128 0x4
	.byte	0x4						; DW_CFA_advance_loc4
	.set	L$set$6,LCFI2-LCFI1
	.long	L$set$6
	.byte	0xd						; DW_CFA_def_cfa_register
	.byte	0x1c					; uleb128 0x1c
	.align LOG2_GPR_BYTES
LEFDE1:
.data
	.align LOG2_GPR_BYTES
LLFB0$non_lazy_ptr:
	.g_long LFB0

#if defined(__ppc64__)
.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	LOG2_GPR_BYTES

Lffi64_struct_to_ram_form$stub:
	.indirect_symbol _ffi64_struct_to_ram_form
	mflr	r0
	bcl		20,31,LO$ffi64_struct_to_ram_form

LO$ffi64_struct_to_ram_form:
	mflr	r11
	addis	r11,r11,ha16(L_ffi64_struct_to_ram_form$lazy_ptr - LO$ffi64_struct_to_ram_form)
	mtlr	r0
	lgu		r12,lo16(L_ffi64_struct_to_ram_form$lazy_ptr - LO$ffi64_struct_to_ram_form)(r11)
	mtctr	r12
	bctr

.lazy_symbol_pointer
L_ffi64_struct_to_ram_form$lazy_ptr:
	.indirect_symbol _ffi64_struct_to_ram_form
	.g_long dyld_stub_binding_helper

#endif // __ppc64__
#endif // __ppc__ || __ppc64__
