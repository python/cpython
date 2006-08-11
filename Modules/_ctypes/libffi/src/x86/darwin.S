#ifdef __i386__
/* -----------------------------------------------------------------------
   darwin.S - Copyright (c) 1996, 1998, 2001, 2002, 2003  Red Hat, Inc.
   
   X86 Foreign Function Interface 

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
  
/*
 * This file is based on sysv.S and then hacked up by Ronald who hasn't done
 * assembly programming in 8 years.
 */

#ifndef __x86_64__

#define LIBFFI_ASM	
#include <fficonfig.h>
#include <ffi.h>

#ifdef PyObjC_STRICT_DEBUGGING
  /* XXX: Debugging of stack alignment, to be removed */
#define ASSERT_STACK_ALIGNED movdqa -16(%esp), %xmm0
#else
#define ASSERT_STACK_ALIGNED 
#endif

.text

.globl _ffi_prep_args

.align 4
.globl _ffi_call_SYSV

_ffi_call_SYSV:
.LFB1:
        pushl %ebp
.LCFI0:
        movl  %esp,%ebp
	subl  $8,%esp
	ASSERT_STACK_ALIGNED
.LCFI1:
	/* Make room for all of the new args.  */
	movl  16(%ebp),%ecx
	subl  %ecx,%esp

	ASSERT_STACK_ALIGNED

	movl  %esp,%eax

	/* Place all of the ffi_prep_args in position  */
	subl  $8,%esp
	pushl 12(%ebp)
	pushl %eax
	call  *8(%ebp)

	ASSERT_STACK_ALIGNED

	/* Return stack to previous state and call the function  */
	addl  $16,%esp

	ASSERT_STACK_ALIGNED

	call  *28(%ebp)
	
	/* XXX: return returns return with 'ret $4', that upsets the stack! */
	movl  16(%ebp),%ecx
	addl  %ecx,%esp


	/* Load %ecx with the return type code  */
	movl  20(%ebp),%ecx	


	/* If the return value pointer is NULL, assume no return value.  */
	cmpl  $0,24(%ebp)
	jne   retint

	/* Even if there is no space for the return value, we are 
	   obliged to handle floating-point values.  */
	cmpl  $FFI_TYPE_FLOAT,%ecx
	jne   noretval
	fstp  %st(0)

        jmp   epilogue

retint:
	cmpl  $FFI_TYPE_INT,%ecx
	jne   retfloat
	/* Load %ecx with the pointer to storage for the return value  */
	movl  24(%ebp),%ecx	
	movl  %eax,0(%ecx)
	jmp   epilogue

retfloat:
	cmpl  $FFI_TYPE_FLOAT,%ecx
	jne   retdouble
	/* Load %ecx with the pointer to storage for the return value  */
	movl  24(%ebp),%ecx	
	fstps (%ecx)
	jmp   epilogue

retdouble:
	cmpl  $FFI_TYPE_DOUBLE,%ecx
	jne   retlongdouble
	/* Load %ecx with the pointer to storage for the return value  */
	movl  24(%ebp),%ecx	
	fstpl (%ecx)
	jmp   epilogue

retlongdouble:
	cmpl  $FFI_TYPE_LONGDOUBLE,%ecx
	jne   retint64
	/* Load %ecx with the pointer to storage for the return value  */
	movl  24(%ebp),%ecx	
	fstpt (%ecx)
	jmp   epilogue
	
retint64:	
	cmpl  $FFI_TYPE_SINT64,%ecx
        jne   retstruct1b
	/* Load %ecx with the pointer to storage for the return value  */
	movl  24(%ebp),%ecx	
	movl  %eax,0(%ecx)
	movl  %edx,4(%ecx)
	jmp   epilogue

retstruct1b:
   cmpl $FFI_TYPE_SINT8,%ecx
   jne  retstruct2b
   movl 24(%ebp),%ecx
   movb  %al,0(%ecx)
   jmp  epilogue

retstruct2b:
   cmpl $FFI_TYPE_SINT16,%ecx
   jne  retstruct
   movl 24(%ebp),%ecx
   movw %ax,0(%ecx)
   jmp  epilogue
	
retstruct:
   cmpl $FFI_TYPE_STRUCT,%ecx
   jne  noretval
	/* Nothing to do!  */

	subl $4,%esp

	ASSERT_STACK_ALIGNED

	addl $8,%esp
	movl %ebp, %esp
	popl %ebp
	ret

noretval:
epilogue:
	ASSERT_STACK_ALIGNED
	addl $8, %esp


        movl %ebp,%esp
        popl %ebp
        ret
.LFE1:
.ffi_call_SYSV_end:
#if 0
        .size    ffi_call_SYSV,.ffi_call_SYSV_end-ffi_call_SYSV
#endif

#if 0
	.section	.eh_frame,EH_FRAME_FLAGS,@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1	/* Length of Common Information Entry */
.LSCIE1:
	.long	0x0	/* CIE Identifier Tag */
	.byte	0x1	/* CIE Version */
#ifdef __PIC__
	.ascii "zR\0"	/* CIE Augmentation */
#else
	.ascii "\0"	/* CIE Augmentation */
#endif
	.byte	0x1	/* .uleb128 0x1; CIE Code Alignment Factor */
	.byte	0x7c	/* .sleb128 -4; CIE Data Alignment Factor */
	.byte	0x8	/* CIE RA Column */
#ifdef __PIC__
	.byte	0x1	/* .uleb128 0x1; Augmentation size */
	.byte	0x1b	/* FDE Encoding (pcrel sdata4) */
#endif
	.byte	0xc	/* DW_CFA_def_cfa */
	.byte	0x4	/* .uleb128 0x4 */
	.byte	0x4	/* .uleb128 0x4 */
	.byte	0x88	/* DW_CFA_offset, column 0x8 */
	.byte	0x1	/* .uleb128 0x1 */
	.align 4
.LECIE1:
.LSFDE1:
	.long	.LEFDE1-.LASFDE1	/* FDE Length */
.LASFDE1:
	.long	.LASFDE1-.Lframe1	/* FDE CIE offset */
#ifdef __PIC__
	.long	.LFB1-.	/* FDE initial location */
#else
	.long	.LFB1	/* FDE initial location */
#endif
	.long	.LFE1-.LFB1	/* FDE address range */
#ifdef __PIC__
	.byte	0x0	/* .uleb128 0x0; Augmentation size */
#endif
	.byte	0x4	/* DW_CFA_advance_loc4 */
	.long	.LCFI0-.LFB1
	.byte	0xe	/* DW_CFA_def_cfa_offset */
	.byte	0x8	/* .uleb128 0x8 */
	.byte	0x85	/* DW_CFA_offset, column 0x5 */
	.byte	0x2	/* .uleb128 0x2 */
	.byte	0x4	/* DW_CFA_advance_loc4 */
	.long	.LCFI1-.LCFI0
	.byte	0xd	/* DW_CFA_def_cfa_register */
	.byte	0x5	/* .uleb128 0x5 */
	.align 4
.LEFDE1:
#endif

#endif /* ifndef __x86_64__ */

#endif /* defined __i386__ */
