#if defined(__ppc64__)

/* -----------------------------------------------------------------------
   ppc64-darwin_closure.S - Copyright (c) 2002, 2003, 2004, Free Software Foundation,
   Inc. based on ppc_closure.S

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

#include <ffi.h>
#include <ppc-ffitarget.h>	// for FFI_TRAMPOLINE_SIZE
#include <ppc-darwin.h>
#include <architecture/ppc/mode_independent_asm.h>

	.file	"ppc64-darwin_closure.S"
.text
	.align	LOG2_GPR_BYTES
	.globl	_ffi_closure_ASM

.text
	.align	LOG2_GPR_BYTES

_ffi_closure_ASM:
LFB1:
	mflr	r0
	stg		r0,SF_RETURN(r1)	// save return address

	// Save GPRs 3 - 10 (aligned to 8) in the parents outgoing area.
	stg		r3,SF_ARG1(r1)
	stg		r4,SF_ARG2(r1)
	stg		r5,SF_ARG3(r1)
	stg		r6,SF_ARG4(r1)
	stg		r7,SF_ARG5(r1)
	stg		r8,SF_ARG6(r1)
	stg		r9,SF_ARG7(r1)
	stg		r10,SF_ARG8(r1)

LCFI0:
/*	48 bytes (Linkage Area)
	64 bytes (outgoing parameter area, always reserved)
	112 bytes (14*8 for incoming FPR)
	? bytes (result)
	112 bytes (14*8 for outgoing FPR)
	16 bytes (2 saved registers)
	352 + ? total bytes
*/

	std		r31,-8(r1)	// Save registers we use.
	std		r30,-16(r1)
	mr		r30,r1		// Save the old SP.
	mr		r31,r11		// Save the ffi_closure around ffi64_data_size.

	// Calculate the space we need.
	stdu	r1,-SF_MINSIZE(r1)
	ld		r3,FFI_TRAMPOLINE_SIZE(r31)	// ffi_closure->cif*
	ld		r3,16(r3)					// ffi_cif->rtype*
	bl		Lffi64_data_size$stub
	ld		r1,0(r1)

	addi	r3,r3,352	// Add our overhead.
	neg		r3,r3
	li		r0,-32		// Align to 32 bytes.
	and		r3,r3,r0
	stdux	r1,r1,r3	// Grow the stack.

	mr		r11,r31		// Copy the ffi_closure back.

LCFI1:
	// We want to build up an area for the parameters passed
	// in registers. (both floating point and integer)

/*	320 bytes (callee stack frame aligned to 32)
	48 bytes (caller linkage area)
	368 (start of caller parameter area aligned to 8)
*/

	// Save FPRs 1 - 14. (aligned to 8)
	stfd	f1,112(r1)
	stfd	f2,120(r1)
	stfd	f3,128(r1)
	stfd	f4,136(r1)
	stfd	f5,144(r1)
	stfd	f6,152(r1)
	stfd	f7,160(r1)
	stfd	f8,168(r1)
	stfd	f9,176(r1)
	stfd	f10,184(r1)
	stfd	f11,192(r1)
	stfd	f12,200(r1)
	stfd	f13,208(r1)
	stfd	f14,216(r1)

	// Set up registers for the routine that actually does the work.
	mr		r3,r11			// context pointer from the trampoline
	addi	r4,r1,224		// result storage
	addi	r5,r30,SF_ARG1	// saved GPRs
	addi	r6,r1,112		// saved FPRs
	bl		Lffi_closure_helper_DARWIN$stub

	// Look the proper starting point in table
	// by using return type as an offset.
	addi	r5,r1,224				// Get pointer to results area.
	bl		Lget_ret_type0_addr		// Get pointer to Lret_type0 into LR.
	mflr	r4						// Move to r4.
	slwi	r3,r3,4					// Now multiply return type by 16.
	add		r3,r3,r4				// Add contents of table to table address.
	mtctr	r3
	bctr

LFE1:
	//	Each of the ret_typeX code fragments has to be exactly 16 bytes long
	//	(4 instructions). For cache effectiveness we align to a 16 byte
	//	boundary first.
	.align 4
	nop
	nop
	nop

Lget_ret_type0_addr:
	blrl

// case FFI_TYPE_VOID
Lret_type0:
	b		Lfinish
	nop
	nop
	nop

// case FFI_TYPE_INT
Lret_type1:
	lwz		r3,4(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_FLOAT
Lret_type2:
	lfs		f1,0(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_DOUBLE
Lret_type3:
	lfd		f1,0(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_LONGDOUBLE
Lret_type4:
	lfd		f1,0(r5)
	lfd		f2,8(r5)
	b		Lfinish
	nop

// case FFI_TYPE_UINT8
Lret_type5:
	lbz		r3,7(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_SINT8
Lret_type6:
	lbz		r3,7(r5)
	extsb	r3,r3
	b		Lfinish
	nop

// case FFI_TYPE_UINT16
Lret_type7:
	lhz		r3,6(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_SINT16
Lret_type8:
	lha		r3,6(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_UINT32
Lret_type9:		// same as Lret_type1
	lwz		r3,4(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_SINT32
Lret_type10:	// same as Lret_type1
	lwz		r3,4(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_UINT64
Lret_type11:
	ld		r3,0(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_SINT64
Lret_type12:	// same as Lret_type11
	ld		r3,0(r5)
	b		Lfinish
	nop
	nop

// case FFI_TYPE_STRUCT
Lret_type13:
	b		Lret_struct
	nop
	nop
	nop

// ** End 16-byte aligned cases **
// case FFI_TYPE_POINTER
// This case assumes that FFI_TYPE_POINTER == FFI_TYPE_LAST. If more types
// are added in future, the following code will need to be updated and
// padded to 16 bytes.
Lret_type14:
	lg		r3,0(r5)
	b		Lfinish

// copy struct into registers
Lret_struct:
	ld		r31,FFI_TRAMPOLINE_SIZE(r31)	// ffi_closure->cif*
	ld		r3,16(r31)						// ffi_cif->rtype*
	ld		r31,24(r31)						// ffi_cif->flags
	mr		r4,r5							// copy struct* to 2nd arg
	addi	r7,r1,SF_ARG9					// GPR return area
	addi	r9,r30,-16-(14*8)				// FPR return area
	li		r5,0							// struct offset ptr (NULL)
	li		r6,0							// FPR used count ptr (NULL)
	li		r8,0							// GPR return area size ptr (NULL)
	li		r10,0							// FPR return area size ptr (NULL)
	bl		Lffi64_struct_to_reg_form$stub

	// Load GPRs
	ld		r3,SF_ARG9(r1)
	ld		r4,SF_ARG10(r1)
	ld		r5,SF_ARG11(r1)
	ld		r6,SF_ARG12(r1)
	nop
	ld		r7,SF_ARG13(r1)
	ld		r8,SF_ARG14(r1)
	ld		r9,SF_ARG15(r1)
	ld		r10,SF_ARG16(r1)
	nop

	// Load FPRs
	mtcrf	0x2,r31
	bf		26,Lfinish
	lfd		f1,-16-(14*8)(r30)
	lfd		f2,-16-(13*8)(r30)
	lfd		f3,-16-(12*8)(r30)
	lfd		f4,-16-(11*8)(r30)
	nop
	lfd		f5,-16-(10*8)(r30)
	lfd		f6,-16-(9*8)(r30)
	lfd		f7,-16-(8*8)(r30)
	lfd		f8,-16-(7*8)(r30)
	nop
	lfd		f9,-16-(6*8)(r30)
	lfd		f10,-16-(5*8)(r30)
	lfd		f11,-16-(4*8)(r30)
	lfd		f12,-16-(3*8)(r30)
	nop
	lfd		f13,-16-(2*8)(r30)
	lfd		f14,-16-(1*8)(r30)
	// Fall through

// case done
Lfinish:
	lg		r1,0(r1)			// Restore stack pointer.
	ld		r31,-8(r1)			// Restore registers we used.
	ld		r30,-16(r1)
	lg		r0,SF_RETURN(r1)	// Get return address.
	mtlr	r0					// Reset link register.
	blr

// END(ffi_closure_ASM)

.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
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
	.byte	0x10		; FDE Encoding (pcrel)
	.byte	0xc			; DW_CFA_def_cfa
	.byte	0x1			; uleb128 0x1
	.byte	0x0			; uleb128 0x0
	.align	LOG2_GPR_BYTES
LECIE1:
.globl _ffi_closure_ASM.eh
_ffi_closure_ASM.eh:
LSFDE1:
	.set	L$set$1,LEFDE1-LASFDE1
	.long	L$set$1		; FDE Length

LASFDE1:
	.long	LASFDE1-EH_frame1		; FDE CIE offset
	.g_long	LFB1-.					; FDE initial location
	.set	L$set$3,LFE1-LFB1
	.g_long	L$set$3					; FDE address range
	.byte   0x0						; uleb128 0x0; Augmentation size
	.byte	0x4						; DW_CFA_advance_loc4
	.set	L$set$3,LCFI1-LCFI0
	.long	L$set$3
	.byte	0xe						; DW_CFA_def_cfa_offset
	.byte	176,1					; uleb128 176
	.byte	0x4						; DW_CFA_advance_loc4
	.set	L$set$4,LCFI0-LFB1
	.long	L$set$4
	.byte   0x11					; DW_CFA_offset_extended_sf
	.byte	0x41					; uleb128 0x41
	.byte   0x7e					; sleb128 -2
	.align	LOG2_GPR_BYTES

LEFDE1:
.data
	.align	LOG2_GPR_BYTES
LDFCM0:
.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	LOG2_GPR_BYTES

Lffi_closure_helper_DARWIN$stub:
	.indirect_symbol _ffi_closure_helper_DARWIN
	mflr	r0
	bcl		20,31,LO$ffi_closure_helper_DARWIN

LO$ffi_closure_helper_DARWIN:
	mflr	r11
	addis	r11,r11,ha16(L_ffi_closure_helper_DARWIN$lazy_ptr - LO$ffi_closure_helper_DARWIN)
	mtlr	r0
	lgu		r12,lo16(L_ffi_closure_helper_DARWIN$lazy_ptr - LO$ffi_closure_helper_DARWIN)(r11)
	mtctr	r12
	bctr

.lazy_symbol_pointer
L_ffi_closure_helper_DARWIN$lazy_ptr:
	.indirect_symbol _ffi_closure_helper_DARWIN
	.g_long dyld_stub_binding_helper

.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	LOG2_GPR_BYTES

Lffi64_struct_to_reg_form$stub:
	.indirect_symbol _ffi64_struct_to_reg_form
	mflr	r0
	bcl		20,31,LO$ffi64_struct_to_reg_form

LO$ffi64_struct_to_reg_form:
	mflr	r11
	addis	r11,r11,ha16(L_ffi64_struct_to_reg_form$lazy_ptr - LO$ffi64_struct_to_reg_form)
	mtlr	r0
	lgu		r12,lo16(L_ffi64_struct_to_reg_form$lazy_ptr - LO$ffi64_struct_to_reg_form)(r11)
	mtctr	r12
	bctr

.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	LOG2_GPR_BYTES

Lffi64_data_size$stub:
	.indirect_symbol _ffi64_data_size
	mflr	r0
	bcl		20,31,LO$ffi64_data_size

LO$ffi64_data_size:
	mflr	r11
	addis	r11,r11,ha16(L_ffi64_data_size$lazy_ptr - LO$ffi64_data_size)
	mtlr	r0
	lgu		r12,lo16(L_ffi64_data_size$lazy_ptr - LO$ffi64_data_size)(r11)
	mtctr	r12
	bctr

.lazy_symbol_pointer
L_ffi64_struct_to_reg_form$lazy_ptr:
	.indirect_symbol _ffi64_struct_to_reg_form
	.g_long dyld_stub_binding_helper

L_ffi64_data_size$lazy_ptr:
	.indirect_symbol _ffi64_data_size
	.g_long dyld_stub_binding_helper

#endif // __ppc64__
