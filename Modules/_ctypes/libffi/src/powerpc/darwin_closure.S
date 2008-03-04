/* -----------------------------------------------------------------------
   darwin_closure.S - Copyright (c) 2002, 2003, 2004, Free Software Foundation,
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
#define L(x) x

#if defined(__ppc64__)
#define MODE_CHOICE(x, y) y
#else
#define MODE_CHOICE(x, y) x
#endif

#define lgu     MODE_CHOICE(lwzu, ldu)

#define g_long  MODE_CHOICE(long, quad)         /* usage is ".g_long" */

#define LOG2_GPR_BYTES  MODE_CHOICE(2,3)        /* log2(GPR_BYTES) */

	.file	"darwin_closure.S"
.text
	.align LOG2_GPR_BYTES
.globl _ffi_closure_ASM

.text
	.align LOG2_GPR_BYTES
_ffi_closure_ASM:
LFB1:
	mflr	r0		/* extract return address  */
	stw	r0,8(r1)	/* save the return address  */
LCFI0:
	/* 24 Bytes (Linkage Area)
	   32 Bytes (outgoing parameter area, always reserved)
	   104 Bytes (13*8 from FPR)
	   16 Bytes (result)
	   176 Bytes  */

	stwu	r1,-176(r1)	/* skip over caller save area
				keep stack aligned to 16.  */
LCFI1:
	/* We want to build up an area for the parameters passed
	   in registers. (both floating point and integer)  */

	/* We store gpr 3 to gpr 10 (aligned to 4)
	   in the parents outgoing area.  */
	stw   r3,200(r1)
	stw   r4,204(r1)
	stw   r5,208(r1)
	stw   r6,212(r1)
	stw   r7,216(r1)
	stw   r8,220(r1)
	stw   r9,224(r1)
	stw   r10,228(r1)

	/* We save fpr 1 to fpr 13. (aligned to 8)  */
	stfd  f1,56(r1)
	stfd  f2,64(r1)
	stfd  f3,72(r1)
	stfd  f4,80(r1)
	stfd  f5,88(r1)
	stfd  f6,96(r1)
	stfd  f7,104(r1)
	stfd  f8,112(r1)
	stfd  f9,120(r1)
	stfd  f10,128(r1)
	stfd  f11,136(r1)
	stfd  f12,144(r1)
	stfd  f13,152(r1)

	/* Set up registers for the routine that actually does the work
	   get the context pointer from the trampoline.  */
	mr r3,r11

	/* Now load up the pointer to the result storage.  */
	addi r4,r1,160

	/* Now load up the pointer to the saved gpr registers.  */
	addi r5,r1,200

	/* Now load up the pointer to the saved fpr registers.  */
	addi r6,r1,56

	/* Make the call.  */
	bl	Lffi_closure_helper_DARWIN$stub

	/* Now r3 contains the return type
	   so use it to look up in a table
	   so we know how to deal with each type.  */

	/* Look up the proper starting point in table
	   by using return type as offset.  */
	addi  r5,r1,160		  /* Get pointer to results area.  */
	bl    Lget_ret_type0_addr /* Get pointer to Lret_type0 into LR.  */
	mflr  r4		  /* Move to r4.  */
	slwi  r3,r3,4		  /* Now multiply return type by 16.  */
	add   r3,r3,r4		  /* Add contents of table to table address.  */
	mtctr r3
	bctr			  /* Jump to it.  */
LFE1:
/* Each of the ret_typeX code fragments has to be exactly 16 bytes long
   (4 instructions). For cache effectiveness we align to a 16 byte boundary
   first.  */

	.align 4

	nop
	nop
	nop
Lget_ret_type0_addr:
	blrl

/* case FFI_TYPE_VOID  */
Lret_type0:
	b	Lfinish
	nop
	nop
	nop

/* case FFI_TYPE_INT  */
Lret_type1:
	lwz	r3,0(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_FLOAT  */
Lret_type2:
	lfs	f1,0(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_DOUBLE  */
Lret_type3:
	lfd	f1,0(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_LONGDOUBLE  */
Lret_type4:
	lfd	f1,0(r5)
	lfd	f2,8(r5)
	b	Lfinish
	nop

/* case FFI_TYPE_UINT8  */
Lret_type5:
	lbz	r3,3(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_SINT8  */
Lret_type6:
	lbz	r3,3(r5)
	extsb	r3,r3
	b	Lfinish
	nop

/* case FFI_TYPE_UINT16  */
Lret_type7:
	lhz	r3,2(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_SINT16  */
Lret_type8:
	lha	r3,2(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_UINT32  */
Lret_type9:
	lwz	r3,0(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_SINT32  */
Lret_type10:
	lwz	r3,0(r5)
	b	Lfinish
	nop
	nop

/* case FFI_TYPE_UINT64  */
Lret_type11:
	lwz	r3,0(r5)
	lwz	r4,4(r5)
	b	Lfinish
	nop

/* case FFI_TYPE_SINT64  */
Lret_type12:
	lwz	r3,0(r5)
	lwz	r4,4(r5)
	b	Lfinish
	nop

/* case FFI_TYPE_STRUCT  */
Lret_type13:
	b	Lfinish
	nop
	nop
	nop

/* case FFI_TYPE_POINTER  */
Lret_type14:
	lwz	r3,0(r5)
	b	Lfinish
	nop
	nop

/* case done  */
Lfinish:
	addi	r1,r1,176	/* Restore stack pointer.  */
	lwz	r0,8(r1)	/* Get return address.  */
	mtlr	r0		/* Reset link register.  */
	blr

/* END(ffi_closure_ASM)  */

.data
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
EH_frame1:
	.set	L$set$0,LECIE1-LSCIE1
	.long	L$set$0	; Length of Common Information Entry
LSCIE1:
	.long	0x0	; CIE Identifier Tag
	.byte	0x1	; CIE Version
	.ascii	"zR\0"	; CIE Augmentation
	.byte	0x1	; uleb128 0x1; CIE Code Alignment Factor
	.byte	0x7c	; sleb128 -4; CIE Data Alignment Factor
	.byte	0x41	; CIE RA Column
	.byte	0x1	; uleb128 0x1; Augmentation size
	.byte	0x90	; FDE Encoding (indirect pcrel)
	.byte	0xc	; DW_CFA_def_cfa
	.byte	0x1	; uleb128 0x1
	.byte	0x0	; uleb128 0x0
	.align	LOG2_GPR_BYTES
LECIE1:
.globl _ffi_closure_ASM.eh
_ffi_closure_ASM.eh:
LSFDE1:
	.set	L$set$1,LEFDE1-LASFDE1
	.long	L$set$1	; FDE Length

LASFDE1:
	.long	LASFDE1-EH_frame1	; FDE CIE offset
	.g_long	LLFB1$non_lazy_ptr-.	; FDE initial location
	.set	L$set$3,LFE1-LFB1
	.g_long	L$set$3	; FDE address range
	.byte   0x0     ; uleb128 0x0; Augmentation size
	.byte	0x4	; DW_CFA_advance_loc4
	.set	L$set$3,LCFI1-LCFI0
	.long	L$set$3
	.byte	0xe	; DW_CFA_def_cfa_offset
	.byte	176,1	; uleb128 176
	.byte	0x4	; DW_CFA_advance_loc4
	.set	L$set$4,LCFI0-LFB1
	.long	L$set$4
	.byte   0x11    ; DW_CFA_offset_extended_sf
	.byte	0x41	; uleb128 0x41
	.byte   0x7e    ; sleb128 -2
	.align	LOG2_GPR_BYTES
LEFDE1:
.data
	.align	LOG2_GPR_BYTES
LDFCM0:
.section __TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	LOG2_GPR_BYTES
Lffi_closure_helper_DARWIN$stub:
#if 1
	.indirect_symbol _ffi_closure_helper_DARWIN
	mflr	r0
	bcl	20,31,LO$ffi_closure_helper_DARWIN
LO$ffi_closure_helper_DARWIN:
	mflr	r11
	addis	r11,r11,ha16(L_ffi_closure_helper_DARWIN$lazy_ptr - LO$ffi_closure_helper_DARWIN)
	mtlr	r0
	lgu	r12,lo16(L_ffi_closure_helper_DARWIN$lazy_ptr - LO$ffi_closure_helper_DARWIN)(r11)
	mtctr	r12
	bctr
.lazy_symbol_pointer
L_ffi_closure_helper_DARWIN$lazy_ptr:
	.indirect_symbol _ffi_closure_helper_DARWIN
	.g_long dyld_stub_binding_helper
#endif
.data
	.align LOG2_GPR_BYTES
LLFB1$non_lazy_ptr:
	.g_long LFB1
