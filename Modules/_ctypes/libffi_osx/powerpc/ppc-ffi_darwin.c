#if defined(__ppc__) || defined(__ppc64__)

/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1998 Geoffrey Keating

   PowerPC Foreign Function Interface

   Darwin ABI support (c) 2001 John Hornkvist
   AIX ABI support (c) 2002 Free Software Foundation, Inc.

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

#include <ffi.h>
#include <ffi_common.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ppc-darwin.h>
#include <architecture/ppc/mode_independent_asm.h>

#if 0
#if defined(POWERPC_DARWIN)
#include <libkern/OSCacheControl.h>     // for sys_icache_invalidate()
#endif

#else

#pragma weak sys_icache_invalidate
extern void sys_icache_invalidate(void *start, size_t len);

#endif


extern void ffi_closure_ASM(void);

// The layout of a function descriptor.  A C function pointer really
// points to one of these.
typedef struct aix_fd_struct {
  void* code_pointer;
  void* toc;
} aix_fd;

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments.

   The stack layout we want looks like this:

   |   Return address from ffi_call_DARWIN      |       higher addresses
   |--------------------------------------------|
   |   Previous backchain pointer      4/8      |           stack pointer here
   |--------------------------------------------|-\ <<< on entry to
   |   Saved r28-r31                 (4/8)*4    | |         ffi_call_DARWIN
   |--------------------------------------------| |
   |   Parameters      (at least 8*(4/8)=32/64) | | (176) +112 - +288
   |--------------------------------------------| |
   |   Space for GPR2                  4/8      | |
   |--------------------------------------------| |     stack   |
   |   Reserved                                          (4/8)*2    | | grows   |
   |--------------------------------------------| |     down    V
   |   Space for callee's LR           4/8      | |
   |--------------------------------------------| |     lower addresses
   |   Saved CR                        4/8      | |
   |--------------------------------------------| |     stack pointer here
   |   Current backchain pointer       4/8      | |     during
   |--------------------------------------------|-/ <<< ffi_call_DARWIN

        Note: ppc64 CR is saved in the low word of a long on the stack.
*/

/*@-exportheader@*/
void
ffi_prep_args(
        extended_cif*   inEcif,
        unsigned *const stack)
/*@=exportheader@*/
{
        /*      Copy the ecif to a local var so we can trample the arg.
                BC note: test this with GP later for possible problems...       */
        volatile extended_cif*  ecif    = inEcif;

        const unsigned bytes    = ecif->cif->bytes;
        const unsigned flags    = ecif->cif->flags;

        /*      Cast the stack arg from int* to long*. sizeof(long) == 4 in 32-bit mode
                and 8 in 64-bit mode.   */
        unsigned long *const longStack  = (unsigned long *const)stack;

        /* 'stacktop' points at the previous backchain pointer. */
#if defined(__ppc64__)
        //      In ppc-darwin.s, an extra 96 bytes is reserved for the linkage area,
        //      saved registers, and an extra FPR.
        unsigned long *const stacktop   =
                (unsigned long *)(unsigned long)((char*)longStack + bytes + 96);
#elif defined(__ppc__)
        unsigned long *const stacktop   = longStack + (bytes / sizeof(long));
#else
#error undefined architecture
#endif

        /* 'fpr_base' points at the space for fpr1, and grows upwards as
                we use FPR registers.  */
        double*         fpr_base = (double*)(stacktop - ASM_NEEDS_REGISTERS) -
                NUM_FPR_ARG_REGISTERS;

#if defined(__ppc64__)
        //      64-bit saves an extra register, and uses an extra FPR. Knock fpr_base
        //      down a couple pegs.
        fpr_base -= 2;
#endif

        unsigned int    fparg_count = 0;

        /* 'next_arg' grows up as we put parameters in it.  */
        unsigned long*  next_arg = longStack + 6; /* 6 reserved positions.  */

        int                             i;
        double                  double_tmp;
        void**                  p_argv = ecif->avalue;
        unsigned long   gprvalue;
        ffi_type**              ptr = ecif->cif->arg_types;

        /* Check that everything starts aligned properly.  */
        FFI_ASSERT(stack == SF_ROUND(stack));
        FFI_ASSERT(stacktop == SF_ROUND(stacktop));
        FFI_ASSERT(bytes == SF_ROUND(bytes));

        /*      Deal with return values that are actually pass-by-reference.
                Rule:
                Return values are referenced by r3, so r4 is the first parameter.  */

        if (flags & FLAG_RETVAL_REFERENCE)
                *next_arg++ = (unsigned long)(char*)ecif->rvalue;

        /* Now for the arguments.  */
        for (i = ecif->cif->nargs; i > 0; i--, ptr++, p_argv++)
    {
                switch ((*ptr)->type)
                {
                        /*      If a floating-point parameter appears before all of the general-
                                purpose registers are filled, the corresponding GPRs that match
                                the size of the floating-point parameter are shadowed for the
                                benefit of vararg and pre-ANSI functions.       */
                        case FFI_TYPE_FLOAT:
                                double_tmp = *(float*)*p_argv;

                                if (fparg_count < NUM_FPR_ARG_REGISTERS)
                                        *fpr_base++ = double_tmp;

                                *(double*)next_arg = double_tmp;

                                next_arg++;
                                fparg_count++;
                                FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);

                                break;

                        case FFI_TYPE_DOUBLE:
                                double_tmp = *(double*)*p_argv;

                                if (fparg_count < NUM_FPR_ARG_REGISTERS)
                                        *fpr_base++ = double_tmp;

                                *(double*)next_arg = double_tmp;

                                next_arg += MODE_CHOICE(2,1);
                                fparg_count++;
                                FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);

                                break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
                        case FFI_TYPE_LONGDOUBLE:
#if defined(__ppc64__)
                                if (fparg_count < NUM_FPR_ARG_REGISTERS)
                                        *(long double*)fpr_base = *(long double*)*p_argv;
#elif defined(__ppc__)
                                if (fparg_count < NUM_FPR_ARG_REGISTERS - 1)
                                        *(long double*)fpr_base = *(long double*)*p_argv;
                                else if (fparg_count == NUM_FPR_ARG_REGISTERS - 1)
                                        *(double*)fpr_base      = *(double*)*p_argv;
#else
#error undefined architecture
#endif

                                *(long double*)next_arg = *(long double*)*p_argv;
                                fparg_count += 2;
                                fpr_base += 2;
                                next_arg += MODE_CHOICE(4,2);
                                FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);

                                break;
#endif  //      FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

                        case FFI_TYPE_UINT64:
                        case FFI_TYPE_SINT64:
#if defined(__ppc64__)
                                gprvalue = *(long long*)*p_argv;
                                goto putgpr;
#elif defined(__ppc__)
                                *(long long*)next_arg = *(long long*)*p_argv;
                                next_arg += 2;
                                break;
#else
#error undefined architecture
#endif

                        case FFI_TYPE_POINTER:
                                gprvalue = *(unsigned long*)*p_argv;
                                goto putgpr;

                        case FFI_TYPE_UINT8:
                                gprvalue = *(unsigned char*)*p_argv;
                                goto putgpr;

                        case FFI_TYPE_SINT8:
                                gprvalue = *(signed char*)*p_argv;
                                goto putgpr;

                        case FFI_TYPE_UINT16:
                                gprvalue = *(unsigned short*)*p_argv;
                                goto putgpr;

                        case FFI_TYPE_SINT16:
                                gprvalue = *(signed short*)*p_argv;
                                goto putgpr;

                        case FFI_TYPE_STRUCT:
                        {
#if defined(__ppc64__)
                                unsigned int    gprSize = 0;
                                unsigned int    fprSize = 0;

                                ffi64_struct_to_reg_form(*ptr, (char*)*p_argv, NULL, &fparg_count,
                                        (char*)next_arg, &gprSize, (char*)fpr_base, &fprSize);
                                next_arg += gprSize / sizeof(long);
                                fpr_base += fprSize / sizeof(double);

#elif defined(__ppc__)
                                char*   dest_cpy = (char*)next_arg;

                        /*      Structures that match the basic modes (QI 1 byte, HI 2 bytes,
                                SI 4 bytes) are aligned as if they were those modes.
                                Structures with 3 byte in size are padded upwards.  */
                                unsigned size_al = (*ptr)->size;

                        /*      If the first member of the struct is a double, then align
                                the struct to double-word.  */
                                if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
                                        size_al = ALIGN((*ptr)->size, 8);

                                if (ecif->cif->abi == FFI_DARWIN)
                                {
                                        if (size_al < 3)
                                                dest_cpy += 4 - size_al;
                                }

                                memcpy((char*)dest_cpy, (char*)*p_argv, size_al);
                                next_arg += (size_al + 3) / 4;
#else
#error undefined architecture
#endif
                                break;
                        }

                        case FFI_TYPE_INT:
                        case FFI_TYPE_UINT32:
                        case FFI_TYPE_SINT32:
                                gprvalue = *(unsigned*)*p_argv;

putgpr:
                                *next_arg++ = gprvalue;
                                break;

                        default:
                                break;
                }
        }

  /* Check that we didn't overrun the stack...  */
  //FFI_ASSERT(gpr_base <= stacktop - ASM_NEEDS_REGISTERS);
  //FFI_ASSERT((unsigned *)fpr_base
  //         <= stacktop - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS);
  //FFI_ASSERT(flags & FLAG_4_GPR_ARGUMENTS || intarg_count <= 4);
}

#if defined(__ppc64__)

bool
ffi64_struct_contains_fp(
        const ffi_type* inType)
{
        bool                    containsFP      = false;
        unsigned int    i;

        for (i = 0; inType->elements[i] != NULL && !containsFP; i++)
        {
                if (inType->elements[i]->type == FFI_TYPE_FLOAT         ||
                        inType->elements[i]->type == FFI_TYPE_DOUBLE    ||
                        inType->elements[i]->type == FFI_TYPE_LONGDOUBLE)
                        containsFP = true;
                else if (inType->elements[i]->type == FFI_TYPE_STRUCT)
                        containsFP = ffi64_struct_contains_fp(inType->elements[i]);
        }

        return containsFP;
}

#endif  // defined(__ppc64__)

/* Perform machine dependent cif processing.  */
ffi_status
ffi_prep_cif_machdep(
        ffi_cif*        cif)
{
        /* All this is for the DARWIN ABI.  */
        int                             i;
        ffi_type**              ptr;
        int                             intarg_count = 0;
        int                             fparg_count = 0;
        unsigned int    flags = 0;
        unsigned int    size_al = 0;

        /*      All the machine-independent calculation of cif->bytes will be wrong.
                Redo the calculation for DARWIN.  */

        /*      Space for the frame pointer, callee's LR, CR, etc, and for
                the asm's temp regs.  */
        unsigned int    bytes = (6 + ASM_NEEDS_REGISTERS) * sizeof(long);

        /*      Return value handling.  The rules are as follows:
                - 32-bit (or less) integer values are returned in gpr3;
                - Structures of size <= 4 bytes also returned in gpr3;
                - 64-bit integer values and structures between 5 and 8 bytes are
                        returned in gpr3 and gpr4;
                - Single/double FP values are returned in fpr1;
                - Long double FP (if not equivalent to double) values are returned in
                        fpr1 and fpr2;
                - Larger structures values are allocated space and a pointer is passed
                        as the first argument.  */
        switch (cif->rtype->type)
        {
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
                case FFI_TYPE_LONGDOUBLE:
                        flags |= FLAG_RETURNS_128BITS;
                        flags |= FLAG_RETURNS_FP;
                        break;
#endif  // FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

                case FFI_TYPE_DOUBLE:
                        flags |= FLAG_RETURNS_64BITS;
                        /* Fall through.  */
                case FFI_TYPE_FLOAT:
                        flags |= FLAG_RETURNS_FP;
                        break;

#if defined(__ppc64__)
                case FFI_TYPE_POINTER:
#endif
                case FFI_TYPE_UINT64:
                case FFI_TYPE_SINT64:
                        flags |= FLAG_RETURNS_64BITS;
                        break;

                case FFI_TYPE_STRUCT:
                {
#if defined(__ppc64__)

                        if (ffi64_stret_needs_ptr(cif->rtype, NULL, NULL))
                        {
                                flags |= FLAG_RETVAL_REFERENCE;
                                flags |= FLAG_RETURNS_NOTHING;
                                intarg_count++;
                        }
                        else
                        {
                                flags |= FLAG_RETURNS_STRUCT;

                                if (ffi64_struct_contains_fp(cif->rtype))
                                        flags |= FLAG_STRUCT_CONTAINS_FP;
                        }

#elif defined(__ppc__)

                        flags |= FLAG_RETVAL_REFERENCE;
                        flags |= FLAG_RETURNS_NOTHING;
                        intarg_count++;

#else
#error undefined architecture
#endif
                        break;
                }

                case FFI_TYPE_VOID:
                        flags |= FLAG_RETURNS_NOTHING;
                        break;

                default:
                        /* Returns 32-bit integer, or similar.  Nothing to do here.  */
                        break;
        }

        /*      The first NUM_GPR_ARG_REGISTERS words of integer arguments, and the
                first NUM_FPR_ARG_REGISTERS fp arguments, go in registers; the rest
                goes on the stack.  Structures are passed as a pointer to a copy of
                the structure. Stuff on the stack needs to keep proper alignment.  */
        for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
        {
                switch ((*ptr)->type)
                {
                        case FFI_TYPE_FLOAT:
                        case FFI_TYPE_DOUBLE:
                                fparg_count++;
                                /*      If this FP arg is going on the stack, it must be
                                        8-byte-aligned.  */
                                if (fparg_count > NUM_FPR_ARG_REGISTERS
                                        && intarg_count % 2 != 0)
                                        intarg_count++;
                                break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
                        case FFI_TYPE_LONGDOUBLE:
                                fparg_count += 2;
                                /*      If this FP arg is going on the stack, it must be
                                        8-byte-aligned.  */

                                if (
#if defined(__ppc64__)
                                        fparg_count > NUM_FPR_ARG_REGISTERS + 1
#elif defined(__ppc__)
                                        fparg_count > NUM_FPR_ARG_REGISTERS
#else
#error undefined architecture
#endif
                                        && intarg_count % 2 != 0)
                                        intarg_count++;

                                intarg_count += 2;
                                break;
#endif  // FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

                        case FFI_TYPE_UINT64:
                        case FFI_TYPE_SINT64:
                                /*      'long long' arguments are passed as two words, but
                                        either both words must fit in registers or both go
                                        on the stack.  If they go on the stack, they must
                                        be 8-byte-aligned.  */
                                if (intarg_count == NUM_GPR_ARG_REGISTERS - 1
                                        || (intarg_count >= NUM_GPR_ARG_REGISTERS
                                        && intarg_count % 2 != 0))
                                        intarg_count++;

                                intarg_count += MODE_CHOICE(2,1);

                                break;

                        case FFI_TYPE_STRUCT:
                                size_al = (*ptr)->size;
                                /*      If the first member of the struct is a double, then align
                                        the struct to double-word.  */
                                if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
                                        size_al = ALIGN((*ptr)->size, 8);

#if defined(__ppc64__)
                                // Look for FP struct members.
                                unsigned int    j;

                                for (j = 0; (*ptr)->elements[j] != NULL; j++)
                                {
                                        if ((*ptr)->elements[j]->type == FFI_TYPE_FLOAT ||
                                                (*ptr)->elements[j]->type == FFI_TYPE_DOUBLE)
                                        {
                                                fparg_count++;

                                                if (fparg_count > NUM_FPR_ARG_REGISTERS)
                                                        intarg_count++;
                                        }
                                        else if ((*ptr)->elements[j]->type == FFI_TYPE_LONGDOUBLE)
                                        {
                                                fparg_count += 2;

                                                if (fparg_count > NUM_FPR_ARG_REGISTERS + 1)
                                                        intarg_count += 2;
                                        }
                                        else
                                                intarg_count++;
                                }
#elif defined(__ppc__)
                                intarg_count += (size_al + 3) / 4;
#else
#error undefined architecture
#endif

                                break;

                        default:
                                /*      Everything else is passed as a 4/8-byte word in a GPR, either
                                        the object itself or a pointer to it.  */
                                intarg_count++;
                                break;
                }
        }

        /* Space for the FPR registers, if needed.  */
        if (fparg_count != 0)
        {
                flags |= FLAG_FP_ARGUMENTS;
#if defined(__ppc64__)
                bytes += (NUM_FPR_ARG_REGISTERS + 1) * sizeof(double);
#elif defined(__ppc__)
                bytes += NUM_FPR_ARG_REGISTERS * sizeof(double);
#else
#error undefined architecture
#endif
        }

        /* Stack space.  */
#if defined(__ppc64__)
        if ((intarg_count + fparg_count) > NUM_GPR_ARG_REGISTERS)
                bytes += (intarg_count + fparg_count) * sizeof(long);
#elif defined(__ppc__)
        if ((intarg_count + 2 * fparg_count) > NUM_GPR_ARG_REGISTERS)
                bytes += (intarg_count + 2 * fparg_count) * sizeof(long);
#else
#error undefined architecture
#endif
        else
                bytes += NUM_GPR_ARG_REGISTERS * sizeof(long);

        /* The stack space allocated needs to be a multiple of 16/32 bytes.  */
        bytes = SF_ROUND(bytes);

        cif->flags = flags;
        cif->bytes = bytes;

        return FFI_OK;
}

/*@-declundef@*/
/*@-exportheader@*/
extern void
ffi_call_AIX(
/*@out@*/       extended_cif*,
                        unsigned,
                        unsigned,
/*@out@*/       unsigned*,
                        void (*fn)(void),
                        void (*fn2)(extended_cif*, unsigned *const));

extern void
ffi_call_DARWIN(
/*@out@*/       extended_cif*,
                        unsigned long,
                        unsigned,
/*@out@*/       unsigned*,
                        void (*fn)(void),
                        void (*fn2)(extended_cif*, unsigned *const));
/*@=declundef@*/
/*@=exportheader@*/

void
ffi_call(
/*@dependent@*/ ffi_cif*        cif,
                                void            (*fn)(void),
/*@out@*/               void*           rvalue,
/*@dependent@*/ void**          avalue)
{
        extended_cif ecif;

        ecif.cif = cif;
        ecif.avalue = avalue;

        /*      If the return value is a struct and we don't have a return
                value address then we need to make one.  */
        if ((rvalue == NULL) &&
                (cif->rtype->type == FFI_TYPE_STRUCT))
        {
                /*@-sysunrecog@*/
                ecif.rvalue = alloca(cif->rtype->size);
                /*@=sysunrecog@*/
        }
        else
                ecif.rvalue = rvalue;

        switch (cif->abi)
        {
                case FFI_AIX:
                        /*@-usedef@*/
                        ffi_call_AIX(&ecif, -cif->bytes,
                                cif->flags, ecif.rvalue, fn, ffi_prep_args);
                        /*@=usedef@*/
                        break;

                case FFI_DARWIN:
                        /*@-usedef@*/
                        ffi_call_DARWIN(&ecif, -(long)cif->bytes,
                                cif->flags, ecif.rvalue, fn, ffi_prep_args);
                        /*@=usedef@*/
                        break;

                default:
                        FFI_ASSERT(0);
                        break;
    }
}

/* here I'd like to add the stack frame layout we use in darwin_closure.S
   and aix_clsoure.S

   SP previous -> +---------------------------------------+ <--- child frame
                  | back chain to caller 4                |
                  +---------------------------------------+ 4
                  | saved CR 4                            |
                  +---------------------------------------+ 8
                  | saved LR 4                            |
                  +---------------------------------------+ 12
                  | reserved for compilers 4              |
                  +---------------------------------------+ 16
                  | reserved for binders 4                |
                  +---------------------------------------+ 20
                  | saved TOC pointer 4                   |
                  +---------------------------------------+ 24
                  | always reserved 8*4=32 (previous GPRs)|
                  | according to the linkage convention   |
                  | from AIX                              |
                  +---------------------------------------+ 56
                  | our FPR area 13*8=104                 |
                  | f1                                    |
                  | .                                     |
                  | f13                                   |
                  +---------------------------------------+ 160
                  | result area 8                         |
                  +---------------------------------------+ 168
                  | alignment to the next multiple of 16  |
SP current -->    +---------------------------------------+ 176 <- parent frame
                  | back chain to caller 4                |
                  +---------------------------------------+ 180
                  | saved CR 4                            |
                  +---------------------------------------+ 184
                  | saved LR 4                            |
                  +---------------------------------------+ 188
                  | reserved for compilers 4              |
                  +---------------------------------------+ 192
                  | reserved for binders 4                |
                  +---------------------------------------+ 196
                  | saved TOC pointer 4                   |
                  +---------------------------------------+ 200
                  | always reserved 8*4=32  we store our  |
                  | GPRs here                             |
                  | r3                                    |
                  | .                                     |
                  | r10                                   |
                  +---------------------------------------+ 232
                  | overflow part                         |
                  +---------------------------------------+ xxx
                  | ????                                  |
                  +---------------------------------------+ xxx
*/

#if !defined(POWERPC_DARWIN)

#define MIN_LINE_SIZE 32

static void
flush_icache(
        char*   addr)
{
#ifndef _AIX
        __asm__ volatile (
                "dcbf 0,%0\n"
                "sync\n"
                "icbi 0,%0\n"
                "sync\n"
                "isync"
                : : "r" (addr) : "memory");
#endif
}

static void
flush_range(
        char*   addr,
        int             size)
{
        int i;

        for (i = 0; i < size; i += MIN_LINE_SIZE)
                flush_icache(addr + i);

        flush_icache(addr + size - 1);
}

#endif  // !defined(POWERPC_DARWIN)

ffi_status
ffi_prep_closure(
        ffi_closure*    closure,
        ffi_cif*                cif,
        void                    (*fun)(ffi_cif*, void*, void**, void*),
        void*                   user_data)
{
        switch (cif->abi)
        {
                case FFI_DARWIN:
                {
                        FFI_ASSERT (cif->abi == FFI_DARWIN);

                        unsigned int*   tramp = (unsigned int*)&closure->tramp[0];

#if defined(__ppc64__)
                        tramp[0] = 0x7c0802a6;  //      mflr    r0
                        tramp[1] = 0x429f0005;  //      bcl             20,31,+0x8
                        tramp[2] = 0x7d6802a6;  //      mflr    r11
                        tramp[3] = 0x7c0803a6;  //      mtlr    r0
                        tramp[4] = 0xe98b0018;  //      ld              r12,24(r11)
                        tramp[5] = 0x7d8903a6;  //      mtctr   r12
                        tramp[6] = 0xe96b0020;  //      ld              r11,32(r11)
                        tramp[7] = 0x4e800420;  //      bctr
                        *(unsigned long*)&tramp[8] = (unsigned long)ffi_closure_ASM;
                        *(unsigned long*)&tramp[10] = (unsigned long)closure;
#elif defined(__ppc__)
                        tramp[0] = 0x7c0802a6;  //      mflr    r0
                        tramp[1] = 0x429f0005;  //      bcl             20,31,+0x8
                        tramp[2] = 0x7d6802a6;  //      mflr    r11
                        tramp[3] = 0x7c0803a6;  //      mtlr    r0
                        tramp[4] = 0x818b0018;  //      lwz             r12,24(r11)
                        tramp[5] = 0x7d8903a6;  //      mtctr   r12
                        tramp[6] = 0x816b001c;  //      lwz             r11,28(r11)
                        tramp[7] = 0x4e800420;  //      bctr
                        tramp[8] = (unsigned long)ffi_closure_ASM;
                        tramp[9] = (unsigned long)closure;
#else
#error undefined architecture
#endif

                        closure->cif = cif;
                        closure->fun = fun;
                        closure->user_data = user_data;

                        // Flush the icache. Only necessary on Darwin.
#if defined(POWERPC_DARWIN)
                        sys_icache_invalidate(closure->tramp, FFI_TRAMPOLINE_SIZE);
#else
                        flush_range(closure->tramp, FFI_TRAMPOLINE_SIZE);
#endif

                        break;
                }

                case FFI_AIX:
                {
                        FFI_ASSERT (cif->abi == FFI_AIX);

                        ffi_aix_trampoline_struct*      tramp_aix =
                                (ffi_aix_trampoline_struct*)(closure->tramp);
                        aix_fd* fd = (aix_fd*)(void*)ffi_closure_ASM;

                        tramp_aix->code_pointer = fd->code_pointer;
                        tramp_aix->toc = fd->toc;
                        tramp_aix->static_chain = closure;
                        closure->cif = cif;
                        closure->fun = fun;
                        closure->user_data = user_data;
                        break;
                }

                default:
                        return FFI_BAD_ABI;
        }

        return FFI_OK;
}

#if defined(__ppc__)
        typedef double ldbits[2];

        typedef union
        {
                ldbits lb;
                long double ld;
        } ldu;
#endif

typedef union
{
        float   f;
        double  d;
} ffi_dblfl;

/*      The trampoline invokes ffi_closure_ASM, and on entry, r11 holds the
        address of the closure. After storing the registers that could possibly
        contain parameters to be passed into the stack frame and setting up space
        for a return value, ffi_closure_ASM invokes the following helper function
        to do most of the work.  */
int
ffi_closure_helper_DARWIN(
        ffi_closure*    closure,
        void*                   rvalue,
        unsigned long*  pgr,
        ffi_dblfl*              pfr)
{
        /*      rvalue is the pointer to space for return value in closure assembly
                pgr is the pointer to where r3-r10 are stored in ffi_closure_ASM
                pfr is the pointer to where f1-f13 are stored in ffi_closure_ASM.  */

#if defined(__ppc__)
        ldu     temp_ld;
#endif

        double                          temp;
        unsigned int            i;
        unsigned int            nf = 0; /* number of FPRs already used.  */
        unsigned int            ng = 0; /* number of GPRs already used.  */
        ffi_cif*                        cif = closure->cif;
        long                            avn = cif->nargs;
        void**                          avalue = alloca(cif->nargs * sizeof(void*));
        ffi_type**                      arg_types = cif->arg_types;

        /*      Copy the caller's structure return value address so that the closure
                returns the data directly to the caller.  */
#if defined(__ppc64__)
        if (cif->rtype->type == FFI_TYPE_STRUCT &&
                ffi64_stret_needs_ptr(cif->rtype, NULL, NULL))
#elif defined(__ppc__)
        if (cif->rtype->type == FFI_TYPE_STRUCT)
#else
#error undefined architecture
#endif
        {
                rvalue = (void*)*pgr;
                pgr++;
                ng++;
        }

        /* Grab the addresses of the arguments from the stack frame.  */
        for (i = 0; i < avn; i++)
        {
                switch (arg_types[i]->type)
                {
                        case FFI_TYPE_SINT8:
                        case FFI_TYPE_UINT8:
                                avalue[i] = (char*)pgr + MODE_CHOICE(3,7);
                                ng++;
                                pgr++;
                                break;

                        case FFI_TYPE_SINT16:
                        case FFI_TYPE_UINT16:
                                avalue[i] = (char*)pgr + MODE_CHOICE(2,6);
                                ng++;
                                pgr++;
                                break;

#if defined(__ppc__)
                        case FFI_TYPE_POINTER:
#endif
                        case FFI_TYPE_SINT32:
                        case FFI_TYPE_UINT32:
                                avalue[i] = (char*)pgr + MODE_CHOICE(0,4);
                                ng++;
                                pgr++;

                                break;

                        case FFI_TYPE_STRUCT:
                                if (cif->abi == FFI_DARWIN)
                                {
#if defined(__ppc64__)
                                        unsigned int    gprSize = 0;
                                        unsigned int    fprSize = 0;
                                        unsigned int    savedFPRSize = fprSize;

                                        avalue[i] = alloca(arg_types[i]->size);
                                        ffi64_struct_to_ram_form(arg_types[i], (const char*)pgr,
                                                &gprSize, (const char*)pfr, &fprSize, &nf, avalue[i], NULL);
 
                                        ng      += gprSize / sizeof(long);
                                        pgr     += gprSize / sizeof(long);
                                        pfr     += (fprSize - savedFPRSize) / sizeof(double);

#elif defined(__ppc__)
                                        /*      Structures that match the basic modes (QI 1 byte, HI 2 bytes,
                                                SI 4 bytes) are aligned as if they were those modes.  */
                                        unsigned int    size_al = size_al = arg_types[i]->size;

                                        /*      If the first member of the struct is a double, then align
                                                the struct to double-word.  */
                                        if (arg_types[i]->elements[0]->type == FFI_TYPE_DOUBLE)
                                                size_al = ALIGN(arg_types[i]->size, 8);

                                        if (size_al < 3)
                                                avalue[i] = (void*)pgr + MODE_CHOICE(4,8) - size_al;
                                        else
                                                avalue[i] = (void*)pgr;

                                        ng      += (size_al + 3) / sizeof(long);
                                        pgr += (size_al + 3) / sizeof(long);
#else
#error undefined architecture
#endif
                                }

                                break;

#if defined(__ppc64__)
                        case FFI_TYPE_POINTER:
#endif
                        case FFI_TYPE_SINT64:
                        case FFI_TYPE_UINT64:
                                /* Long long ints are passed in 1 or 2 GPRs.  */
                                avalue[i] = pgr;
                                ng += MODE_CHOICE(2,1);
                                pgr += MODE_CHOICE(2,1);

                                break;

                        case FFI_TYPE_FLOAT:
                                /*      A float value consumes a GPR.
                                        There are 13 64-bit floating point registers.  */
                                if (nf < NUM_FPR_ARG_REGISTERS)
                                {
                                        temp = pfr->d;
                                        pfr->f = (float)temp;
                                        avalue[i] = pfr;
                                        pfr++;
                                }
                                else
                                        avalue[i] = pgr;

                                nf++;
                                ng++;
                                pgr++;
                                break;

                        case FFI_TYPE_DOUBLE:
                                /*      A double value consumes one or two GPRs.
                                        There are 13 64bit floating point registers.  */
                                if (nf < NUM_FPR_ARG_REGISTERS)
                                {
                                        avalue[i] = pfr;
                                        pfr++;
                                }
                                else
                                        avalue[i] = pgr;

                                nf++;
                                ng += MODE_CHOICE(2,1);
                                pgr += MODE_CHOICE(2,1);

                                break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

                        case FFI_TYPE_LONGDOUBLE:
#if defined(__ppc64__)
                                if (nf < NUM_FPR_ARG_REGISTERS)
                                {
                                        avalue[i] = pfr;
                                        pfr += 2;
                                }
#elif defined(__ppc__)
                                /*      A long double value consumes 2/4 GPRs and 2 FPRs.
                                        There are 13 64bit floating point registers.  */
                                if (nf < NUM_FPR_ARG_REGISTERS - 1)
                                {
                                        avalue[i] = pfr;
                                        pfr += 2;
                                }
                                /*      Here we have the situation where one part of the long double
                                        is stored in fpr13 and the other part is already on the stack.
                                        We use a union to pass the long double to avalue[i].  */
                                else if (nf == NUM_FPR_ARG_REGISTERS - 1)
                                {
                                        memcpy (&temp_ld.lb[0], pfr, sizeof(temp_ld.lb[0]));
                                        memcpy (&temp_ld.lb[1], pgr + 2, sizeof(temp_ld.lb[1]));
                                        avalue[i] = &temp_ld.ld;
                                }
#else
#error undefined architecture
#endif
                                else
                                        avalue[i] = pgr;

                                nf += 2;
                                ng += MODE_CHOICE(4,2);
                                pgr += MODE_CHOICE(4,2);

                                break;

#endif  /*      FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE  */

                        default:
                                FFI_ASSERT(0);
                                break;
                }
        }

        (closure->fun)(cif, rvalue, avalue, closure->user_data);

        /* Tell ffi_closure_ASM to perform return type promotions.  */
        return cif->rtype->type;
}

#if defined(__ppc64__)

/*      ffi64_struct_to_ram_form

        Rebuild a struct's natural layout from buffers of concatenated registers.
        Return the number of registers used.
        inGPRs[0-7] == r3, inFPRs[0-7] == f1 ...
*/
void
ffi64_struct_to_ram_form(
        const ffi_type* inType,
        const char*             inGPRs,
        unsigned int*   ioGPRMarker,
        const char*             inFPRs,
        unsigned int*   ioFPRMarker,
        unsigned int*   ioFPRsUsed,
        char*                   outStruct,      // caller-allocated
        unsigned int*   ioStructMarker)
{
        unsigned int    srcGMarker              = 0;
        unsigned int    srcFMarker              = 0;
        unsigned int    savedFMarker    = 0;
        unsigned int    fprsUsed                = 0;
        unsigned int    savedFPRsUsed   = 0;
        unsigned int    destMarker              = 0;

        static unsigned int     recurseCount    = 0;

        if (ioGPRMarker)
                srcGMarker      = *ioGPRMarker;

        if (ioFPRMarker)
        {
                srcFMarker              = *ioFPRMarker;
                savedFMarker    = srcFMarker;
        }

        if (ioFPRsUsed)
        {
                fprsUsed                = *ioFPRsUsed;
                savedFPRsUsed   = fprsUsed;
        }

        if (ioStructMarker)
                destMarker      = *ioStructMarker;

        size_t                  i;

        switch (inType->size)
        {
                case 1: case 2: case 4:
                        srcGMarker += 8 - inType->size;
                        break;

                default:
                        break;
        }

        for (i = 0; inType->elements[i] != NULL; i++)
        {
                switch (inType->elements[i]->type)
                {
                        case FFI_TYPE_FLOAT:
                                srcFMarker = ALIGN(srcFMarker, 4);
                                srcGMarker = ALIGN(srcGMarker, 4);
                                destMarker = ALIGN(destMarker, 4);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        *(float*)&outStruct[destMarker] =
                                                (float)*(double*)&inFPRs[srcFMarker];
                                        srcFMarker += 8;
                                        fprsUsed++;
                                }
                                else
                                        *(float*)&outStruct[destMarker] =
                                                (float)*(double*)&inGPRs[srcGMarker];

                                srcGMarker += 4;
                                destMarker += 4;

                                // Skip to next GPR if next element won't fit and we're
                                // not already at a register boundary.
                                if (inType->elements[i + 1] != NULL && (destMarker % 8))
                                {
                                        if (!FFI_TYPE_1_BYTE(inType->elements[i + 1]->type) &&
                                                (!FFI_TYPE_2_BYTE(inType->elements[i + 1]->type) ||
                                                (ALIGN(srcGMarker, 8) - srcGMarker) < 2) &&
                                                (!FFI_TYPE_4_BYTE(inType->elements[i + 1]->type) ||
                                                (ALIGN(srcGMarker, 8) - srcGMarker) < 4))
                                                srcGMarker      = ALIGN(srcGMarker, 8);
                                }

                                break;

                        case FFI_TYPE_DOUBLE:
                                srcFMarker = ALIGN(srcFMarker, 8);
                                destMarker = ALIGN(destMarker, 8);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        *(double*)&outStruct[destMarker]        =
                                                *(double*)&inFPRs[srcFMarker];
                                        srcFMarker += 8;
                                        fprsUsed++;
                                }
                                else
                                        *(double*)&outStruct[destMarker]        =
                                                *(double*)&inGPRs[srcGMarker];

                                destMarker += 8;

                                // Skip next GPR
                                srcGMarker += 8;
                                srcGMarker = ALIGN(srcGMarker, 8);

                                break;

                        case FFI_TYPE_LONGDOUBLE:
                                destMarker = ALIGN(destMarker, 16);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        srcFMarker = ALIGN(srcFMarker, 8);
                                        srcGMarker = ALIGN(srcGMarker, 8);
                                        *(long double*)&outStruct[destMarker]   =
                                                *(long double*)&inFPRs[srcFMarker];
                                        srcFMarker += 16;
                                        fprsUsed += 2;
                                }
                                else
                                {
                                        srcFMarker = ALIGN(srcFMarker, 16);
                                        srcGMarker = ALIGN(srcGMarker, 16);
                                        *(long double*)&outStruct[destMarker]   =
                                                *(long double*)&inGPRs[srcGMarker];
                                }

                                destMarker += 16;

                                // Skip next 2 GPRs
                                srcGMarker += 16;
                                srcGMarker = ALIGN(srcGMarker, 8);

                                break;

                        case FFI_TYPE_UINT8:
                        case FFI_TYPE_SINT8:
                        {
                                if (inType->alignment == 1)     // chars only
                                {
                                        if (inType->size == 1)
                                                outStruct[destMarker++] = inGPRs[srcGMarker++];
                                        else if (inType->size == 2)
                                        {
                                                outStruct[destMarker++] = inGPRs[srcGMarker++];
                                                outStruct[destMarker++] = inGPRs[srcGMarker++];
                                                i++;
                                        }
                                        else
                                        {
                                                memcpy(&outStruct[destMarker],
                                                        &inGPRs[srcGMarker], inType->size);
                                                srcGMarker += inType->size;
                                                destMarker += inType->size;
                                                i += inType->size - 1;
                                        }
                                }
                                else    // chars and other stuff
                                {
                                        outStruct[destMarker++] = inGPRs[srcGMarker++];

                                        // Skip to next GPR if next element won't fit and we're
                                        // not already at a register boundary.
                                        if (inType->elements[i + 1] != NULL && (srcGMarker % 8))
                                        {
                                                if (!FFI_TYPE_1_BYTE(inType->elements[i + 1]->type) &&
                                                        (!FFI_TYPE_2_BYTE(inType->elements[i + 1]->type) ||
                                                        (ALIGN(srcGMarker, 8) - srcGMarker) < 2) &&
                                                        (!FFI_TYPE_4_BYTE(inType->elements[i + 1]->type) ||
                                                        (ALIGN(srcGMarker, 8) - srcGMarker) < 4))
                                                        srcGMarker      = ALIGN(srcGMarker, inType->alignment); // was 8
                                        }
                                }

                                break;
                        }

                        case FFI_TYPE_UINT16:
                        case FFI_TYPE_SINT16:
                                srcGMarker = ALIGN(srcGMarker, 2);
                                destMarker = ALIGN(destMarker, 2);

                                *(short*)&outStruct[destMarker] =
                                        *(short*)&inGPRs[srcGMarker];
                                srcGMarker += 2;
                                destMarker += 2;

                                break;

                        case FFI_TYPE_INT:
                        case FFI_TYPE_UINT32:
                        case FFI_TYPE_SINT32:
                                srcGMarker = ALIGN(srcGMarker, 4);
                                destMarker = ALIGN(destMarker, 4);

                                *(int*)&outStruct[destMarker] =
                                        *(int*)&inGPRs[srcGMarker];
                                srcGMarker += 4;
                                destMarker += 4;

                                break;

                        case FFI_TYPE_POINTER:
                        case FFI_TYPE_UINT64:
                        case FFI_TYPE_SINT64:
                                srcGMarker = ALIGN(srcGMarker, 8);
                                destMarker = ALIGN(destMarker, 8);

                                *(long long*)&outStruct[destMarker] =
                                        *(long long*)&inGPRs[srcGMarker];
                                srcGMarker += 8;
                                destMarker += 8;

                                break;

                        case FFI_TYPE_STRUCT:
                                recurseCount++;
                                ffi64_struct_to_ram_form(inType->elements[i], inGPRs,
                                        &srcGMarker, inFPRs, &srcFMarker, &fprsUsed,
                                        outStruct, &destMarker);
                                recurseCount--;
                                break;

                        default:
                                FFI_ASSERT(0);  // unknown element type
                                break;
                }
        }

        srcGMarker = ALIGN(srcGMarker, inType->alignment);

        // Take care of the special case for 16-byte structs, but not for
        // nested structs.
        if (recurseCount == 0 && srcGMarker == 16)
        {
                *(long double*)&outStruct[0] = *(long double*)&inGPRs[0];
                srcFMarker      = savedFMarker;
                fprsUsed        = savedFPRsUsed;
        }

        if (ioGPRMarker)
                *ioGPRMarker = ALIGN(srcGMarker, 8);

        if (ioFPRMarker)
                *ioFPRMarker = srcFMarker;

        if (ioFPRsUsed)
                *ioFPRsUsed     = fprsUsed;

        if (ioStructMarker)
                *ioStructMarker = ALIGN(destMarker, 8);
}

/*      ffi64_struct_to_reg_form

        Copy a struct's elements into buffers that can be sliced into registers.
        Return the sizes of the output buffers in bytes. Pass NULL buffer pointers
        to calculate size only.
        outGPRs[0-7] == r3, outFPRs[0-7] == f1 ...
*/
void
ffi64_struct_to_reg_form(
        const ffi_type* inType,
        const char*             inStruct,
        unsigned int*   ioStructMarker,
        unsigned int*   ioFPRsUsed,
        char*                   outGPRs,        // caller-allocated
        unsigned int*   ioGPRSize,
        char*                   outFPRs,        // caller-allocated
        unsigned int*   ioFPRSize)
{
        size_t                  i;
        unsigned int    srcMarker               = 0;
        unsigned int    destGMarker             = 0;
        unsigned int    destFMarker             = 0;
        unsigned int    savedFMarker    = 0;
        unsigned int    fprsUsed                = 0;
        unsigned int    savedFPRsUsed   = 0;

        static unsigned int     recurseCount    = 0;

        if (ioStructMarker)
                srcMarker       = *ioStructMarker;

        if (ioFPRsUsed)
        {
                fprsUsed                = *ioFPRsUsed;
                savedFPRsUsed   = fprsUsed;
        }

        if (ioGPRSize)
                destGMarker     = *ioGPRSize;

        if (ioFPRSize)
        {
                destFMarker             = *ioFPRSize;
                savedFMarker    = destFMarker;
        }

        switch (inType->size)
        {
                case 1: case 2: case 4:
                        destGMarker += 8 - inType->size;
                        break;

                default:
                        break;
        }

        for (i = 0; inType->elements[i] != NULL; i++)
        {
                switch (inType->elements[i]->type)
                {
                        // Shadow floating-point types in GPRs for vararg and pre-ANSI
                        // functions.
                        case FFI_TYPE_FLOAT:
                                // Nudge markers to next 4/8-byte boundary
                                srcMarker = ALIGN(srcMarker, 4);
                                destGMarker = ALIGN(destGMarker, 4);
                                destFMarker = ALIGN(destFMarker, 8);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        if (outFPRs != NULL && inStruct != NULL)
                                                *(double*)&outFPRs[destFMarker] =
                                                        (double)*(float*)&inStruct[srcMarker];

                                        destFMarker += 8;
                                        fprsUsed++;
                                }

                                if (outGPRs != NULL && inStruct != NULL)
                                        *(double*)&outGPRs[destGMarker] =
                                                (double)*(float*)&inStruct[srcMarker];

                                srcMarker += 4;
                                destGMarker += 4;

                                // Skip to next GPR if next element won't fit and we're
                                // not already at a register boundary.
                                if (inType->elements[i + 1] != NULL && (srcMarker % 8))
                                {
                                        if (!FFI_TYPE_1_BYTE(inType->elements[i + 1]->type) &&
                                                (!FFI_TYPE_2_BYTE(inType->elements[i + 1]->type) ||
                                                (ALIGN(destGMarker, 8) - destGMarker) < 2) &&
                                                (!FFI_TYPE_4_BYTE(inType->elements[i + 1]->type) ||
                                                (ALIGN(destGMarker, 8) - destGMarker) < 4))
                                                destGMarker     = ALIGN(destGMarker, 8);
                                }

                                break;

                        case FFI_TYPE_DOUBLE:
                                srcMarker = ALIGN(srcMarker, 8);
                                destFMarker = ALIGN(destFMarker, 8);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        if (outFPRs != NULL && inStruct != NULL)
                                                *(double*)&outFPRs[destFMarker] =
                                                        *(double*)&inStruct[srcMarker];

                                        destFMarker += 8;
                                        fprsUsed++;
                                }

                                if (outGPRs != NULL && inStruct != NULL)
                                        *(double*)&outGPRs[destGMarker] =
                                                *(double*)&inStruct[srcMarker];

                                srcMarker += 8;

                                // Skip next GPR
                                destGMarker += 8;
                                destGMarker = ALIGN(destGMarker, 8);

                                break;

                        case FFI_TYPE_LONGDOUBLE:
                                srcMarker = ALIGN(srcMarker, 16);

                                if (fprsUsed < NUM_FPR_ARG_REGISTERS)
                                {
                                        destFMarker = ALIGN(destFMarker, 8);
                                        destGMarker = ALIGN(destGMarker, 8);

                                        if (outFPRs != NULL && inStruct != NULL)
                                                *(long double*)&outFPRs[destFMarker] =
                                                        *(long double*)&inStruct[srcMarker];

                                        if (outGPRs != NULL && inStruct != NULL)
                                                *(long double*)&outGPRs[destGMarker] =
                                                        *(long double*)&inStruct[srcMarker];

                                        destFMarker += 16;
                                        fprsUsed += 2;
                                }
                                else
                                {
                                        destGMarker = ALIGN(destGMarker, 16);

                                         if (outGPRs != NULL && inStruct != NULL)
                                                *(long double*)&outGPRs[destGMarker] =
                                                        *(long double*)&inStruct[srcMarker];
                                }

                                srcMarker += 16;
                                destGMarker += 16;      // Skip next 2 GPRs
                                destGMarker = ALIGN(destGMarker, 8);    // was 16

                                break;

                        case FFI_TYPE_UINT8:
                        case FFI_TYPE_SINT8:
                                if (inType->alignment == 1)     // bytes only
                                {
                                        if (inType->size == 1)
                                        {
                                                if (outGPRs != NULL && inStruct != NULL)
                                                        outGPRs[destGMarker] = inStruct[srcMarker];

                                                srcMarker++;
                                                destGMarker++;
                                        }
                                        else if (inType->size == 2)
                                        {
                                                if (outGPRs != NULL && inStruct != NULL)
                                                {
                                                        outGPRs[destGMarker] = inStruct[srcMarker];
                                                        outGPRs[destGMarker + 1] = inStruct[srcMarker + 1];
                                                }

                                                srcMarker += 2;
                                                destGMarker += 2;

                                                i++;
                                        }
                                        else
                                        {
                                                if (outGPRs != NULL && inStruct != NULL)
                                                {
                                                        // Avoid memcpy for small chunks.
                                                        if (inType->size <= sizeof(long))
                                                                *(long*)&outGPRs[destGMarker] =
                                                                        *(long*)&inStruct[srcMarker];
                                                        else
                                                                memcpy(&outGPRs[destGMarker],
                                                                        &inStruct[srcMarker], inType->size);
                                                }
                                                
                                                srcMarker += inType->size;
                                                destGMarker += inType->size;
                                                i += inType->size - 1;
                                        }
                                }
                                else    // bytes and other stuff
                                {
                                        if (outGPRs != NULL && inStruct != NULL)
                                                outGPRs[destGMarker] = inStruct[srcMarker];

                                        srcMarker++;
                                        destGMarker++;

                                        // Skip to next GPR if next element won't fit and we're
                                        // not already at a register boundary.
                                        if (inType->elements[i + 1] != NULL && (destGMarker % 8))
                                        {
                                                if (!FFI_TYPE_1_BYTE(inType->elements[i + 1]->type) &&
                                                        (!FFI_TYPE_2_BYTE(inType->elements[i + 1]->type) ||
                                                        (ALIGN(destGMarker, 8) - destGMarker) < 2) &&
                                                        (!FFI_TYPE_4_BYTE(inType->elements[i + 1]->type) ||
                                                        (ALIGN(destGMarker, 8) - destGMarker) < 4))
                                                        destGMarker     = ALIGN(destGMarker, inType->alignment);        // was 8
                                        }
                                }

                                break;

                        case FFI_TYPE_UINT16:
                        case FFI_TYPE_SINT16:
                                srcMarker = ALIGN(srcMarker, 2);
                                destGMarker = ALIGN(destGMarker, 2);

                                if (outGPRs != NULL && inStruct != NULL)
                                        *(short*)&outGPRs[destGMarker] =
                                                *(short*)&inStruct[srcMarker];

                                srcMarker += 2;
                                destGMarker += 2;

                                if (inType->elements[i + 1] == NULL)
                                        destGMarker     = ALIGN(destGMarker, inType->alignment);

                                break;

                        case FFI_TYPE_INT:
                        case FFI_TYPE_UINT32:
                        case FFI_TYPE_SINT32:
                                srcMarker = ALIGN(srcMarker, 4);
                                destGMarker = ALIGN(destGMarker, 4);

                                if (outGPRs != NULL && inStruct != NULL)
                                        *(int*)&outGPRs[destGMarker] =
                                                *(int*)&inStruct[srcMarker];

                                srcMarker += 4;
                                destGMarker += 4;

                                break;

                        case FFI_TYPE_POINTER:
                        case FFI_TYPE_UINT64:
                        case FFI_TYPE_SINT64:
                                srcMarker = ALIGN(srcMarker, 8);
                                destGMarker = ALIGN(destGMarker, 8);

                                if (outGPRs != NULL && inStruct != NULL)
                                        *(long long*)&outGPRs[destGMarker] =
                                                *(long long*)&inStruct[srcMarker];

                                srcMarker += 8;
                                destGMarker += 8;

                                if (inType->elements[i + 1] == NULL)
                                        destGMarker     = ALIGN(destGMarker, inType->alignment);

                                break;

                        case FFI_TYPE_STRUCT:
                                recurseCount++;
                                ffi64_struct_to_reg_form(inType->elements[i],
                                        inStruct, &srcMarker, &fprsUsed, outGPRs, 
                                        &destGMarker, outFPRs, &destFMarker);
                                recurseCount--;
                                break;

                        default:
                                FFI_ASSERT(0);
                                break;
                }
        }

        destGMarker     = ALIGN(destGMarker, inType->alignment);

        // Take care of the special case for 16-byte structs, but not for
        // nested structs.
        if (recurseCount == 0 && destGMarker == 16)
        {
                if (outGPRs != NULL && inStruct != NULL)
                        *(long double*)&outGPRs[0] = *(long double*)&inStruct[0];

                destFMarker     = savedFMarker;
                fprsUsed        = savedFPRsUsed;
        }

        if (ioStructMarker)
                *ioStructMarker = ALIGN(srcMarker, 8);

        if (ioFPRsUsed)
                *ioFPRsUsed     = fprsUsed;

        if (ioGPRSize)
                *ioGPRSize = ALIGN(destGMarker, 8);

        if (ioFPRSize)
                *ioFPRSize = ALIGN(destFMarker, 8);
}

/*      ffi64_stret_needs_ptr

        Determine whether a returned struct needs a pointer in r3 or can fit
        in registers.
*/

bool
ffi64_stret_needs_ptr(
        const ffi_type* inType,
        unsigned short* ioGPRCount,
        unsigned short* ioFPRCount)
{
        // Obvious case first- struct is larger than combined FPR size.
        if (inType->size > 14 * 8)
                return true;

        // Now the struct can physically fit in registers, determine if it
        // also fits logically.
        bool                    needsPtr        = false;
        unsigned short  gprsUsed        = 0;
        unsigned short  fprsUsed        = 0;
        size_t                  i;

        if (ioGPRCount)
                gprsUsed = *ioGPRCount;

        if (ioFPRCount)
                fprsUsed = *ioFPRCount;

        for (i = 0; inType->elements[i] != NULL && !needsPtr; i++)
        {
                switch (inType->elements[i]->type)
                {
                        case FFI_TYPE_FLOAT:
                        case FFI_TYPE_DOUBLE:
                                gprsUsed++;
                                fprsUsed++;

                                if (fprsUsed > 13)
                                        needsPtr = true;

                                break;

                        case FFI_TYPE_LONGDOUBLE:
                                gprsUsed += 2;
                                fprsUsed += 2;

                                if (fprsUsed > 14)
                                        needsPtr = true;

                                break;

                        case FFI_TYPE_UINT8:
                        case FFI_TYPE_SINT8:
                        {
                                gprsUsed++;

                                if (gprsUsed > 8)
                                {
                                        needsPtr = true;
                                        break;
                                }

                                if (inType->elements[i + 1] == NULL)    // last byte in the struct
                                        break;

                                // Count possible contiguous bytes ahead, up to 8.
                                unsigned short j;

                                for (j = 1; j < 8; j++)
                                {
                                        if (inType->elements[i + j] == NULL ||
                                                !FFI_TYPE_1_BYTE(inType->elements[i + j]->type))
                                                break;
                                }

                                i += j - 1;     // allow for i++ before the test condition

                                break;
                        }

                        case FFI_TYPE_UINT16:
                        case FFI_TYPE_SINT16:
                        case FFI_TYPE_INT:
                        case FFI_TYPE_UINT32:
                        case FFI_TYPE_SINT32:
                        case FFI_TYPE_POINTER:
                        case FFI_TYPE_UINT64:
                        case FFI_TYPE_SINT64:
                                gprsUsed++;

                                if (gprsUsed > 8)
                                        needsPtr = true;

                                break;

                        case FFI_TYPE_STRUCT:
                                needsPtr = ffi64_stret_needs_ptr(
                                        inType->elements[i], &gprsUsed, &fprsUsed);

                                break;

                        default:
                                FFI_ASSERT(0);
                                break;
                }
        }

        if (ioGPRCount)
                *ioGPRCount = gprsUsed;

        if (ioFPRCount)
                *ioFPRCount = fprsUsed;

        return needsPtr;
}

/*      ffi64_data_size

        Calculate the size in bytes of an ffi type.
*/

unsigned int
ffi64_data_size(
        const ffi_type* inType)
{
        unsigned int    size = 0;

        switch (inType->type)
        {
                case FFI_TYPE_UINT8:
                case FFI_TYPE_SINT8:
                        size = 1;
                        break;

                case FFI_TYPE_UINT16:
                case FFI_TYPE_SINT16:
                        size = 2;
                        break;

                case FFI_TYPE_INT:
                case FFI_TYPE_UINT32:
                case FFI_TYPE_SINT32:
                case FFI_TYPE_FLOAT:
                        size = 4;
                        break;

                case FFI_TYPE_POINTER:
                case FFI_TYPE_UINT64:
                case FFI_TYPE_SINT64:
                case FFI_TYPE_DOUBLE:
                        size = 8;
                        break;

                case FFI_TYPE_LONGDOUBLE:
                        size = 16;
                        break;

                case FFI_TYPE_STRUCT:
                        ffi64_struct_to_reg_form(
                                inType, NULL, NULL, NULL, NULL, &size, NULL, NULL);
                        break;

                case FFI_TYPE_VOID:
                        break;

                default:
                        FFI_ASSERT(0);
                        break;
        }

        return size;
}

#endif  /*      defined(__ppc64__)      */
#endif  /* __ppc__ || __ppc64__ */
