/* -----------------------------------------------------------------------
   ffi_darwin.c

   Copyright (C) 1998 Geoffrey Keating
   Copyright (C) 2001 John Hornkvist
   Copyright (C) 2002, 2006, 2007, 2009 Free Software Foundation, Inc.

   FFI support for Darwin and AIX.
   
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

#include <stdlib.h>

extern void ffi_closure_ASM (void);

enum {
  /* The assembly depends on these exact flags.  */
  FLAG_RETURNS_NOTHING  = 1 << (31-30), /* These go in cr7  */
  FLAG_RETURNS_FP       = 1 << (31-29),
  FLAG_RETURNS_64BITS   = 1 << (31-28),
  FLAG_RETURNS_128BITS  = 1 << (31-31),

  FLAG_ARG_NEEDS_COPY   = 1 << (31- 7),
  FLAG_FP_ARGUMENTS     = 1 << (31- 6), /* cr1.eq; specified by ABI  */
  FLAG_4_GPR_ARGUMENTS  = 1 << (31- 5),
  FLAG_RETVAL_REFERENCE = 1 << (31- 4)
};

/* About the DARWIN ABI.  */
enum {
  NUM_GPR_ARG_REGISTERS = 8,
  NUM_FPR_ARG_REGISTERS = 13
};
enum { ASM_NEEDS_REGISTERS = 4 };

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments.

   The stack layout we want looks like this:

   |   Return address from ffi_call_DARWIN      |	higher addresses
   |--------------------------------------------|
   |   Previous backchain pointer	4	|	stack pointer here
   |--------------------------------------------|<+ <<<	on entry to
   |   Saved r28-r31			4*4	| |	ffi_call_DARWIN
   |--------------------------------------------| |
   |   Parameters             (at least 8*4=32) | |
   |--------------------------------------------| |
   |   Space for GPR2                   4       | |
   |--------------------------------------------| |	stack	|
   |   Reserved                       2*4       | |	grows	|
   |--------------------------------------------| |	down	V
   |   Space for callee's LR		4	| |
   |--------------------------------------------| |	lower addresses
   |   Saved CR                         4       | |
   |--------------------------------------------| |     stack pointer here
   |   Current backchain pointer	4	|-/	during
   |--------------------------------------------|   <<<	ffi_call_DARWIN

   */

void
ffi_prep_args (extended_cif *ecif, unsigned long *const stack)
{
  const unsigned bytes = ecif->cif->bytes;
  const unsigned flags = ecif->cif->flags;
  const unsigned nargs = ecif->cif->nargs;
  const ffi_abi abi = ecif->cif->abi;

  /* 'stacktop' points at the previous backchain pointer.  */
  unsigned long *const stacktop = stack + (bytes / sizeof(unsigned long));

  /* 'fpr_base' points at the space for fpr1, and grows upwards as
     we use FPR registers.  */
  double *fpr_base = (double *) (stacktop - ASM_NEEDS_REGISTERS) - NUM_FPR_ARG_REGISTERS;
  int fparg_count = 0;


  /* 'next_arg' grows up as we put parameters in it.  */
  unsigned long *next_arg = stack + 6; /* 6 reserved positions.  */

  int i;
  double double_tmp;
  void **p_argv = ecif->avalue;
  unsigned long gprvalue;
  ffi_type** ptr = ecif->cif->arg_types;
  char *dest_cpy;
  unsigned size_al = 0;

  /* Check that everything starts aligned properly.  */
  FFI_ASSERT(((unsigned) (char *) stack & 0xF) == 0);
  FFI_ASSERT(((unsigned) (char *) stacktop & 0xF) == 0);
  FFI_ASSERT((bytes & 0xF) == 0);

  /* Deal with return values that are actually pass-by-reference.
     Rule:
     Return values are referenced by r3, so r4 is the first parameter.  */

  if (flags & FLAG_RETVAL_REFERENCE)
    *next_arg++ = (unsigned long) (char *) ecif->rvalue;

  /* Now for the arguments.  */
  for (i = nargs; i > 0; i--, ptr++, p_argv++)
    {
      switch ((*ptr)->type)
	{
	/* If a floating-point parameter appears before all of the general-
	   purpose registers are filled, the corresponding GPRs that match
	   the size of the floating-point parameter are skipped.  */
	case FFI_TYPE_FLOAT:
	  double_tmp = *(float *) *p_argv;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    *(double *)next_arg = double_tmp;
	  else
	    *fpr_base++ = double_tmp;
	  next_arg++;
	  fparg_count++;
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

	case FFI_TYPE_DOUBLE:
	  double_tmp = *(double *) *p_argv;
	  if (fparg_count >= NUM_FPR_ARG_REGISTERS)
	    *(double *)next_arg = double_tmp;
	  else
	    *fpr_base++ = double_tmp;
#ifdef POWERPC64
	  next_arg++;
#else
	  next_arg += 2;
#endif
	  fparg_count++;
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
#ifdef POWERPC64
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *(long double *) fpr_base++ = *(long double *) *p_argv;
	  else
	    *(long double *) next_arg = *(long double *) *p_argv;
	  next_arg += 2;
	  fparg_count += 2;
#else
	  double_tmp = ((double *) *p_argv)[0];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  else
	    *(double *) next_arg = double_tmp;
	  next_arg += 2;
	  fparg_count++;

	  double_tmp = ((double *) *p_argv)[1];
	  if (fparg_count < NUM_FPR_ARG_REGISTERS)
	    *fpr_base++ = double_tmp;
	  else
	    *(double *) next_arg = double_tmp;
	  next_arg += 2;
	  fparg_count++;
#endif
	  FFI_ASSERT(flags & FLAG_FP_ARGUMENTS);
	  break;
#endif
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
#ifdef POWERPC64
	  gprvalue = *(long long *) *p_argv;
	  goto putgpr;
#else
	  *(long long *) next_arg = *(long long *) *p_argv;
	  next_arg += 2;
#endif
	  break;
	case FFI_TYPE_POINTER:
	  gprvalue = *(unsigned long *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_UINT8:
	  gprvalue = *(unsigned char *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_SINT8:
	  gprvalue = *(signed char *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_UINT16:
	  gprvalue = *(unsigned short *) *p_argv;
	  goto putgpr;
	case FFI_TYPE_SINT16:
	  gprvalue = *(signed short *) *p_argv;
	  goto putgpr;

	case FFI_TYPE_STRUCT:
#ifdef POWERPC64
	  dest_cpy = (char *) next_arg;
	  size_al = (*ptr)->size;
	  if ((*ptr)->elements[0]->type == 3)
	    size_al = ALIGN((*ptr)->size, 8);
	  if (size_al < 3 && abi == FFI_DARWIN)
	    dest_cpy += 4 - size_al;

	  memcpy ((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 7) / 8;
#else
	  dest_cpy = (char *) next_arg;

	  /* Structures that match the basic modes (QI 1 byte, HI 2 bytes,
	     SI 4 bytes) are aligned as if they were those modes.
	     Structures with 3 byte in size are padded upwards.  */
	  size_al = (*ptr)->size;
	  /* If the first member of the struct is a double, then align
	     the struct to double-word.  */
	  if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN((*ptr)->size, 8);
	  if (size_al < 3 && abi == FFI_DARWIN)
	    dest_cpy += 4 - size_al;

	  memcpy((char *) dest_cpy, (char *) *p_argv, size_al);
	  next_arg += (size_al + 3) / 4;
#endif
	  break;

	case FFI_TYPE_INT:
	case FFI_TYPE_SINT32:
	  gprvalue = *(signed int *) *p_argv;
	  goto putgpr;

	case FFI_TYPE_UINT32:
	  gprvalue = *(unsigned int *) *p_argv;
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
  //	     <= stacktop - ASM_NEEDS_REGISTERS - NUM_GPR_ARG_REGISTERS);
  //FFI_ASSERT(flags & FLAG_4_GPR_ARGUMENTS || intarg_count <= 4);
}

/* Adjust the size of S to be correct for Darwin.
   On Darwin, the first field of a structure has natural alignment.  */

static void
darwin_adjust_aggregate_sizes (ffi_type *s)
{
  int i;

  if (s->type != FFI_TYPE_STRUCT)
    return;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      ffi_type *p;
      int align;
      
      p = s->elements[i];
      darwin_adjust_aggregate_sizes (p);
      if (i == 0
	  && (p->type == FFI_TYPE_UINT64
	      || p->type == FFI_TYPE_SINT64
	      || p->type == FFI_TYPE_DOUBLE
	      || p->alignment == 8))
	align = 8;
      else if (p->alignment == 16 || p->alignment < 4)
	align = p->alignment;
      else
	align = 4;
      s->size = ALIGN(s->size, align) + p->size;
    }
  
  s->size = ALIGN(s->size, s->alignment);
  
  if (s->elements[0]->type == FFI_TYPE_UINT64
      || s->elements[0]->type == FFI_TYPE_SINT64
      || s->elements[0]->type == FFI_TYPE_DOUBLE
      || s->elements[0]->alignment == 8)
    s->alignment = s->alignment > 8 ? s->alignment : 8;
  /* Do not add additional tail padding.  */
}

/* Adjust the size of S to be correct for AIX.
   Word-align double unless it is the first member of a structure.  */

static void
aix_adjust_aggregate_sizes (ffi_type *s)
{
  int i;

  if (s->type != FFI_TYPE_STRUCT)
    return;

  s->size = 0;
  for (i = 0; s->elements[i] != NULL; i++)
    {
      ffi_type *p;
      int align;
      
      p = s->elements[i];
      aix_adjust_aggregate_sizes (p);
      align = p->alignment;
      if (i != 0 && p->type == FFI_TYPE_DOUBLE)
	align = 4;
      s->size = ALIGN(s->size, align) + p->size;
    }
  
  s->size = ALIGN(s->size, s->alignment);
  
  if (s->elements[0]->type == FFI_TYPE_UINT64
      || s->elements[0]->type == FFI_TYPE_SINT64
      || s->elements[0]->type == FFI_TYPE_DOUBLE
      || s->elements[0]->alignment == 8)
    s->alignment = s->alignment > 8 ? s->alignment : 8;
  /* Do not add additional tail padding.  */
}

/* Perform machine dependent cif processing.  */
ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  /* All this is for the DARWIN ABI.  */
  int i;
  ffi_type **ptr;
  unsigned bytes;
  int fparg_count = 0, intarg_count = 0;
  unsigned flags = 0;
  unsigned size_al = 0;

  /* All the machine-independent calculation of cif->bytes will be wrong.
     All the calculation of structure sizes will also be wrong.
     Redo the calculation for DARWIN.  */

  if (cif->abi == FFI_DARWIN)
    {
      darwin_adjust_aggregate_sizes (cif->rtype);
      for (i = 0; i < cif->nargs; i++)
	darwin_adjust_aggregate_sizes (cif->arg_types[i]);
    }

  if (cif->abi == FFI_AIX)
    {
      aix_adjust_aggregate_sizes (cif->rtype);
      for (i = 0; i < cif->nargs; i++)
	aix_adjust_aggregate_sizes (cif->arg_types[i]);
    }

  /* Space for the frame pointer, callee's LR, CR, etc, and for
     the asm's temp regs.  */

  bytes = (6 + ASM_NEEDS_REGISTERS) * sizeof(long);

  /* Return value handling.  The rules are as follows:
     - 32-bit (or less) integer values are returned in gpr3;
     - Structures of size <= 4 bytes also returned in gpr3;
     - 64-bit integer values and structures between 5 and 8 bytes are returned
       in gpr3 and gpr4;
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
#endif

    case FFI_TYPE_DOUBLE:
      flags |= FLAG_RETURNS_64BITS;
      /* Fall through.  */
    case FFI_TYPE_FLOAT:
      flags |= FLAG_RETURNS_FP;
      break;

    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
#ifdef POWERPC64
    case FFI_TYPE_POINTER:
#endif
      flags |= FLAG_RETURNS_64BITS;
      break;

    case FFI_TYPE_STRUCT:
      flags |= FLAG_RETVAL_REFERENCE;
      flags |= FLAG_RETURNS_NOTHING;
      intarg_count++;
      break;
    case FFI_TYPE_VOID:
      flags |= FLAG_RETURNS_NOTHING;
      break;

    default:
      /* Returns 32-bit integer, or similar.  Nothing to do here.  */
      break;
    }

  /* The first NUM_GPR_ARG_REGISTERS words of integer arguments, and the
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
	  /* If this FP arg is going on the stack, it must be
	     8-byte-aligned.  */
	  if (fparg_count > NUM_FPR_ARG_REGISTERS
	      && intarg_count%2 != 0)
	    intarg_count++;
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
	  fparg_count += 2;
	  /* If this FP arg is going on the stack, it must be
	     8-byte-aligned.  */
	  if (fparg_count > NUM_FPR_ARG_REGISTERS
	      && intarg_count%2 != 0)
	    intarg_count++;
	  intarg_count +=2;
	  break;
#endif

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  /* 'long long' arguments are passed as two words, but
	     either both words must fit in registers or both go
	     on the stack.  If they go on the stack, they must
	     be 8-byte-aligned.  */
	  if (intarg_count == NUM_GPR_ARG_REGISTERS-1
	      || (intarg_count >= NUM_GPR_ARG_REGISTERS && intarg_count%2 != 0))
	    intarg_count++;
	  intarg_count += 2;
	  break;

	case FFI_TYPE_STRUCT:
	  size_al = (*ptr)->size;
	  /* If the first member of the struct is a double, then align
	     the struct to double-word.  */
	  if ((*ptr)->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN((*ptr)->size, 8);
#ifdef POWERPC64
	  intarg_count += (size_al + 7) / 8;
#else
	  intarg_count += (size_al + 3) / 4;
#endif
	  break;

	default:
	  /* Everything else is passed as a 4-byte word in a GPR, either
	     the object itself or a pointer to it.  */
	  intarg_count++;
	  break;
	}
    }

  if (fparg_count != 0)
    flags |= FLAG_FP_ARGUMENTS;

  /* Space for the FPR registers, if needed.  */
  if (fparg_count != 0)
    bytes += NUM_FPR_ARG_REGISTERS * sizeof(double);

  /* Stack space.  */
#ifdef POWERPC64
  if ((intarg_count + fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + fparg_count) * sizeof(long);
#else
  if ((intarg_count + 2 * fparg_count) > NUM_GPR_ARG_REGISTERS)
    bytes += (intarg_count + 2 * fparg_count) * sizeof(long);
#endif
  else
    bytes += NUM_GPR_ARG_REGISTERS * sizeof(long);

  /* The stack space allocated needs to be a multiple of 16 bytes.  */
  bytes = (bytes + 15) & ~0xF;

  cif->flags = flags;
  cif->bytes = bytes;

  return FFI_OK;
}

extern void ffi_call_AIX(extended_cif *, long, unsigned, unsigned *,
			 void (*fn)(void), void (*fn2)(void));
extern void ffi_call_DARWIN(extended_cif *, long, unsigned, unsigned *,
			    void (*fn)(void), void (*fn2)(void));

void
ffi_call (ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  /* If the return value is a struct and we don't have a return
     value address then we need to make one.  */

  if ((rvalue == NULL) &&
      (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca (cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_AIX:
      ffi_call_AIX(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		   ffi_prep_args);
      break;
    case FFI_DARWIN:
      ffi_call_DARWIN(&ecif, -(long)cif->bytes, cif->flags, ecif.rvalue, fn,
		      ffi_prep_args);
      break;
    default:
      FFI_ASSERT(0);
      break;
    }
}

static void flush_icache(char *);
static void flush_range(char *, int);

/* The layout of a function descriptor.  A C function pointer really
   points to one of these.  */

typedef struct aix_fd_struct {
  void *code_pointer;
  void *toc;
} aix_fd;

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
ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*, void*, void**, void*),
		      void *user_data,
		      void *codeloc)
{
  unsigned int *tramp;
  struct ffi_aix_trampoline_struct *tramp_aix;
  aix_fd *fd;

  switch (cif->abi)
    {
    case FFI_DARWIN:

      FFI_ASSERT (cif->abi == FFI_DARWIN);

      tramp = (unsigned int *) &closure->tramp[0];
      tramp[0] = 0x7c0802a6;  /*   mflr    r0  */
      tramp[1] = 0x429f000d;  /*   bcl-    20,4*cr7+so,0x10  */
      tramp[4] = 0x7d6802a6;  /*   mflr    r11  */
      tramp[5] = 0x818b0000;  /*   lwz     r12,0(r11) function address  */
      tramp[6] = 0x7c0803a6;  /*   mtlr    r0   */
      tramp[7] = 0x7d8903a6;  /*   mtctr   r12  */
      tramp[8] = 0x816b0004;  /*   lwz     r11,4(r11) static chain  */
      tramp[9] = 0x4e800420;  /*   bctr  */
      tramp[2] = (unsigned long) ffi_closure_ASM; /* function  */
      tramp[3] = (unsigned long) codeloc; /* context  */

      closure->cif = cif;
      closure->fun = fun;
      closure->user_data = user_data;

      /* Flush the icache. Only necessary on Darwin.  */
      flush_range(codeloc, FFI_TRAMPOLINE_SIZE);

      break;

    case FFI_AIX:

      tramp_aix = (struct ffi_aix_trampoline_struct *) (closure->tramp);
      fd = (aix_fd *)(void *)ffi_closure_ASM;

      FFI_ASSERT (cif->abi == FFI_AIX);

      tramp_aix->code_pointer = fd->code_pointer;
      tramp_aix->toc = fd->toc;
      tramp_aix->static_chain = codeloc;
      closure->cif = cif;
      closure->fun = fun;
      closure->user_data = user_data;

    default:

      FFI_ASSERT(0);
      break;
    }
  return FFI_OK;
}

static void
flush_icache(char *addr)
{
#ifndef _AIX
  __asm__ volatile (
		"dcbf 0,%0\n"
		"\tsync\n"
		"\ticbi 0,%0\n"
		"\tsync\n"
		"\tisync"
		: : "r"(addr) : "memory");
#endif
}

static void
flush_range(char * addr1, int size)
{
#define MIN_LINE_SIZE 32
  int i;
  for (i = 0; i < size; i += MIN_LINE_SIZE)
    flush_icache(addr1+i);
  flush_icache(addr1+size-1);
}

typedef union
{
  float f;
  double d;
} ffi_dblfl;

int
ffi_closure_helper_DARWIN (ffi_closure *, void *,
			   unsigned long *, ffi_dblfl *);

/* Basically the trampoline invokes ffi_closure_ASM, and on
   entry, r11 holds the address of the closure.
   After storing the registers that could possibly contain
   parameters to be passed into the stack frame and setting
   up space for a return value, ffi_closure_ASM invokes the
   following helper function to do most of the work.  */

int
ffi_closure_helper_DARWIN (ffi_closure *closure, void *rvalue,
			   unsigned long *pgr, ffi_dblfl *pfr)
{
  /* rvalue is the pointer to space for return value in closure assembly
     pgr is the pointer to where r3-r10 are stored in ffi_closure_ASM
     pfr is the pointer to where f1-f13 are stored in ffi_closure_ASM.  */

  typedef double ldbits[2];

  union ldu
  {
    ldbits lb;
    long double ld;
  };

  void **          avalue;
  ffi_type **      arg_types;
  long             i, avn;
  ffi_cif *        cif;
  ffi_dblfl *      end_pfr = pfr + NUM_FPR_ARG_REGISTERS;
  unsigned         size_al;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof(void *));

  /* Copy the caller's structure return value address so that the closure
     returns the data directly to the caller.  */
  if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) *pgr;
      pgr++;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  /* Grab the addresses of the arguments from the stack frame.  */
  while (i < avn)
    {
      switch (arg_types[i]->type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 7;
#else
	  avalue[i] = (char *) pgr + 3;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 6;
#else
	  avalue[i] = (char *) pgr + 2;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
#ifdef POWERPC64
	  avalue[i] = (char *) pgr + 4;
#else
	case FFI_TYPE_POINTER:
	  avalue[i] = pgr;
#endif
	  pgr++;
	  break;

	case FFI_TYPE_STRUCT:
#ifdef POWERPC64
	  size_al = arg_types[i]->size;
	  if (arg_types[i]->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN (arg_types[i]->size, 8);
	  if (size_al < 3 && cif->abi == FFI_DARWIN)
	    avalue[i] = (void *) pgr + 8 - size_al;
	  else
	    avalue[i] = (void *) pgr;
	  pgr += (size_al + 7) / 8;
#else
	  /* Structures that match the basic modes (QI 1 byte, HI 2 bytes,
	     SI 4 bytes) are aligned as if they were those modes.  */
	  size_al = arg_types[i]->size;
	  /* If the first member of the struct is a double, then align
	     the struct to double-word.  */
	  if (arg_types[i]->elements[0]->type == FFI_TYPE_DOUBLE)
	    size_al = ALIGN(arg_types[i]->size, 8);
	  if (size_al < 3 && cif->abi == FFI_DARWIN)
	    avalue[i] = (void*) pgr + 4 - size_al;
	  else
	    avalue[i] = (void*) pgr;
	  pgr += (size_al + 3) / 4;
#endif
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
#ifdef POWERPC64
	case FFI_TYPE_POINTER:
	  avalue[i] = pgr;
	  pgr++;
	  break;
#else
	  /* Long long ints are passed in two gpr's.  */
	  avalue[i] = pgr;
	  pgr += 2;
	  break;
#endif

	case FFI_TYPE_FLOAT:
	  /* A float value consumes a GPR.
	     There are 13 64bit floating point registers.  */
	  if (pfr < end_pfr)
	    {
	      double temp = pfr->d;
	      pfr->f = (float) temp;
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr++;
	  break;

	case FFI_TYPE_DOUBLE:
	  /* A double value consumes two GPRs.
	     There are 13 64bit floating point registers.  */
	  if (pfr < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
#ifdef POWERPC64
	  pgr++;
#else
	  pgr += 2;
#endif
	  break;

#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE

	case FFI_TYPE_LONGDOUBLE:
#ifdef POWERPC64
	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  else
	    {
	      if (pfr < end_pfr)
		{
		  *pgr = *(unsigned long *) pfr;
		  pfr++;
		}
	      avalue[i] = pgr;
	    }
	  pgr += 2;
#else  /* POWERPC64 */
	  /* A long double value consumes four GPRs and two FPRs.
	     There are 13 64bit floating point registers.  */
	  if (pfr + 1 < end_pfr)
	    {
	      avalue[i] = pfr;
	      pfr += 2;
	    }
	  /* Here we have the situation where one part of the long double
	     is stored in fpr13 and the other part is already on the stack.
	     We use a union to pass the long double to avalue[i].  */
	  else if (pfr + 1 == end_pfr)
	    {
	      union ldu temp_ld;
	      memcpy (&temp_ld.lb[0], pfr, sizeof(ldbits));
	      memcpy (&temp_ld.lb[1], pgr + 2, sizeof(ldbits));
	      avalue[i] = &temp_ld.ld;
	      pfr++;
	    }
	  else
	    {
	      avalue[i] = pgr;
	    }
	  pgr += 4;
#endif  /* POWERPC64 */
	  break;
#endif
	default:
	  FFI_ASSERT(0);
	}
      i++;
    }

  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  /* Tell ffi_closure_ASM to perform return type promotions.  */
  return cif->rtype->type;
}
