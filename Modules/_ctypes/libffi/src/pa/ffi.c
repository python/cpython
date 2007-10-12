/* -----------------------------------------------------------------------
   ffi.c - (c) 2003-2004 Randolph Chung <tausq@debian.org>

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

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdio.h>

#define ROUND_UP(v, a)  (((size_t)(v) + (a) - 1) & ~((a) - 1))
#define ROUND_DOWN(v, a)  (((size_t)(v) - (a) + 1) & ~((a) - 1))
#define MIN_STACK_SIZE  64
#define FIRST_ARG_SLOT  9
#define DEBUG_LEVEL   0

#define fldw(addr, fpreg) asm volatile ("fldw 0(%0), %%" #fpreg "L" : : "r"(addr) : #fpreg)
#define fstw(fpreg, addr) asm volatile ("fstw %%" #fpreg "L, 0(%0)" : : "r"(addr))
#define fldd(addr, fpreg) asm volatile ("fldd 0(%0), %%" #fpreg : : "r"(addr) : #fpreg)
#define fstd(fpreg, addr) asm volatile ("fstd %%" #fpreg "L, 0(%0)" : : "r"(addr))

#define debug(lvl, x...) do { if (lvl <= DEBUG_LEVEL) { printf(x); } } while (0)

static inline int ffi_struct_type(ffi_type *t)
{
  size_t sz = t->size;

  /* Small structure results are passed in registers,
     larger ones are passed by pointer.  */

  if (sz <= 1)
    return FFI_TYPE_UINT8;
  else if (sz == 2)
    return FFI_TYPE_UINT16;
  else if (sz == 3)
    return FFI_TYPE_SMALL_STRUCT3;
  else if (sz == 4)
    return FFI_TYPE_UINT32;
  else if (sz == 5)
    return FFI_TYPE_SMALL_STRUCT5;
  else if (sz == 6)
    return FFI_TYPE_SMALL_STRUCT6;
  else if (sz == 7)
    return FFI_TYPE_SMALL_STRUCT7;
  else if (sz <= 8)
    return FFI_TYPE_UINT64;
  else
    return FFI_TYPE_STRUCT; /* else, we pass it by pointer.  */
}

/* PA has a downward growing stack, which looks like this:
  
   Offset
        [ Variable args ]
   SP = (4*(n+9))       arg word N
   ...
   SP-52                arg word 4
        [ Fixed args ]
   SP-48                arg word 3
   SP-44                arg word 2
   SP-40                arg word 1
   SP-36                arg word 0
        [ Frame marker ]
   ...
   SP-20                RP
   SP-4                 previous SP
  
   First 4 non-FP 32-bit args are passed in gr26, gr25, gr24 and gr23
   First 2 non-FP 64-bit args are passed in register pairs, starting
     on an even numbered register (i.e. r26/r25 and r24+r23)
   First 4 FP 32-bit arguments are passed in fr4L, fr5L, fr6L and fr7L
   First 2 FP 64-bit arguments are passed in fr5 and fr7
   The rest are passed on the stack starting at SP-52, but 64-bit
     arguments need to be aligned to an 8-byte boundary
  
   This means we can have holes either in the register allocation,
   or in the stack.  */

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments
  
   The following code will put everything into the stack frame
   (which was allocated by the asm routine), and on return
   the asm routine will load the arguments that should be
   passed by register into the appropriate registers
  
   NOTE: We load floating point args in this function... that means we
   assume gcc will not mess with fp regs in here.  */

/*@-exportheader@*/
void ffi_prep_args_LINUX(UINT32 *stack, extended_cif *ecif, unsigned bytes)
/*@=exportheader@*/
{
  register unsigned int i;
  register ffi_type **p_arg;
  register void **p_argv;
  unsigned int slot = FIRST_ARG_SLOT - 1;
  char *dest_cpy;

  debug(1, "%s: stack = %p, ecif = %p, bytes = %u\n", __FUNCTION__, stack, ecif, bytes);

  p_arg = ecif->cif->arg_types;
  p_argv = ecif->avalue;

  for (i = 0; i < ecif->cif->nargs; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case FFI_TYPE_SINT8:
	  slot++;
	  *(SINT32 *)(stack - slot) = *(SINT8 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT8:
	  slot++;
	  *(UINT32 *)(stack - slot) = *(UINT8 *)(*p_argv);
	  break;

	case FFI_TYPE_SINT16:
	  slot++;
	  *(SINT32 *)(stack - slot) = *(SINT16 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT16:
	  slot++;
	  *(UINT32 *)(stack - slot) = *(UINT16 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT32:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_POINTER:
	  slot++;
	  debug(3, "Storing UINT32 %u in slot %u\n", *(UINT32 *)(*p_argv), slot);
	  *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	  break;

	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  slot += 2;
	  if (slot & 1)
	    slot++;

	  *(UINT32 *)(stack - slot) = (*(UINT64 *)(*p_argv)) >> 32;
	  *(UINT32 *)(stack - slot + 1) = (*(UINT64 *)(*p_argv)) & 0xffffffffUL;
	  break;

	case FFI_TYPE_FLOAT:
	  /* First 4 args go in fr4L - fr7L */
	  slot++;
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 0: fldw(*p_argv, fr4); break;
	    case 1: fldw(*p_argv, fr5); break;
	    case 2: fldw(*p_argv, fr6); break;
	    case 3: fldw(*p_argv, fr7); break;
	    default:
	      /* Other ones are just passed on the stack.  */
	      debug(3, "Storing UINT32(float) in slot %u\n", slot);
	      *(UINT32 *)(stack - slot) = *(UINT32 *)(*p_argv);
	      break;
	    }
	    break;

	case FFI_TYPE_DOUBLE:
	  slot += 2;
	  if (slot & 1)
	    slot++;
	  switch (slot - FIRST_ARG_SLOT + 1)
	    {
	      /* First 2 args go in fr5, fr7 */
	      case 2: fldd(*p_argv, fr5); break;
	      case 4: fldd(*p_argv, fr7); break;
	      default:
	        debug(3, "Storing UINT64(double) at slot %u\n", slot);
	        *(UINT64 *)(stack - slot) = *(UINT64 *)(*p_argv);
	        break;
	    }
	  break;

	case FFI_TYPE_STRUCT:

	  /* Structs smaller or equal than 4 bytes are passed in one
	     register. Structs smaller or equal 8 bytes are passed in two
	     registers. Larger structures are passed by pointer.  */

	  if((*p_arg)->size <= 4) 
	    {
	      slot++;
	      dest_cpy = (char *)(stack - slot);
	      dest_cpy += 4 - (*p_arg)->size;
	      memcpy((char *)dest_cpy, (char *)*p_argv, (*p_arg)->size);
	    }
	  else if ((*p_arg)->size <= 8) 
	    {
	      slot += 2;
	      if (slot & 1)
	        slot++;
	      dest_cpy = (char *)(stack - slot);
	      dest_cpy += 8 - (*p_arg)->size;
	      memcpy((char *)dest_cpy, (char *)*p_argv, (*p_arg)->size);
	    } 
	  else 
	    {
	      slot++;
	      *(UINT32 *)(stack - slot) = (UINT32)(*p_argv);
	    }
	  break;

	default:
	  FFI_ASSERT(0);
	}

      p_arg++;
      p_argv++;
    }

  /* Make sure we didn't mess up and scribble on the stack.  */
  {
    int n;

    debug(5, "Stack setup:\n");
    for (n = 0; n < (bytes + 3) / 4; n++)
      {
	if ((n%4) == 0) { debug(5, "\n%08x: ", (unsigned int)(stack - n)); }
	debug(5, "%08x ", *(stack - n));
      }
    debug(5, "\n");
  }

  FFI_ASSERT(slot * 4 <= bytes);

  return;
}

static void ffi_size_stack_LINUX(ffi_cif *cif)
{
  ffi_type **ptr;
  int i;
  int z = 0; /* # stack slots */

  for (ptr = cif->arg_types, i = 0; i < cif->nargs; ptr++, i++)
    {
      int type = (*ptr)->type;

      switch (type)
	{
	case FFI_TYPE_DOUBLE:
	case FFI_TYPE_UINT64:
	case FFI_TYPE_SINT64:
	  z += 2 + (z & 1); /* must start on even regs, so we may waste one */
	  break;

	case FFI_TYPE_STRUCT:
	  z += 1; /* pass by ptr, callee will copy */
	  break;

	default: /* <= 32-bit values */
	  z++;
	}
    }

  /* We can fit up to 6 args in the default 64-byte stack frame,
     if we need more, we need more stack.  */
  if (z <= 6)
    cif->bytes = MIN_STACK_SIZE; /* min stack size */
  else
    cif->bytes = 64 + ROUND_UP((z - 6) * sizeof(UINT32), MIN_STACK_SIZE);

  debug(3, "Calculated stack size is %u bytes\n", cif->bytes);
}

/* Perform machine dependent cif processing.  */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_STRUCT:
      /* For the return type we have to check the size of the structures.
	 If the size is smaller or equal 4 bytes, the result is given back
	 in one register. If the size is smaller or equal 8 bytes than we
	 return the result in two registers. But if the size is bigger than
	 8 bytes, we work with pointers.  */
      cif->flags = ffi_struct_type(cif->rtype);
      break;

    case FFI_TYPE_UINT64:
    case FFI_TYPE_SINT64:
      cif->flags = FFI_TYPE_UINT64;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  /* Lucky us, because of the unique PA ABI we get to do our
     own stack sizing.  */
  switch (cif->abi)
    {
    case FFI_LINUX:
      ffi_size_stack_LINUX(cif);
      break;

    default:
      FFI_ASSERT(0);
      break;
    }

  return FFI_OK;
}

/*@-declundef@*/
/*@-exportheader@*/
extern void ffi_call_LINUX(void (*)(UINT32 *, extended_cif *, unsigned),
			   /*@out@*/ extended_cif *,
			   unsigned, unsigned,
			   /*@out@*/ unsigned *,
			   void (*fn)(void));
/*@=declundef@*/
/*@=exportheader@*/

void ffi_call(/*@dependent@*/ ffi_cif *cif,
	      void (*fn)(void),
	      /*@out@*/ void *rvalue,
	      /*@dependent@*/ void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  /* If the return value is a struct and we don't have a return
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
    case FFI_LINUX:
      /*@-usedef@*/
      debug(2, "Calling ffi_call_LINUX: ecif=%p, bytes=%u, flags=%u, rvalue=%p, fn=%p\n", &ecif, cif->bytes, cif->flags, ecif.rvalue, (void *)fn);
      ffi_call_LINUX(ffi_prep_args_LINUX, &ecif, cif->bytes,
		     cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;

    default:
      FFI_ASSERT(0);
      break;
    }
}

#if FFI_CLOSURES
/* This is more-or-less an inverse of ffi_call -- we have arguments on
   the stack, and we need to fill them into a cif structure and invoke
   the user function. This really ought to be in asm to make sure
   the compiler doesn't do things we don't expect.  */
UINT32 ffi_closure_inner_LINUX(ffi_closure *closure, UINT32 *stack)
{
  ffi_cif *cif;
  void **avalue;
  void *rvalue;
  UINT32 ret[2]; /* function can return up to 64-bits in registers */
  ffi_type **p_arg;
  char *tmp;
  int i, avn, slot = FIRST_ARG_SLOT - 1;
  register UINT32 r28 asm("r28");

  cif = closure->cif;

  /* If returning via structure, callee will write to our pointer.  */
  if (cif->flags == FFI_TYPE_STRUCT)
    rvalue = (void *)r28;
  else
    rvalue = &ret[0];

  avalue = (void **)alloca(cif->nargs * FFI_SIZEOF_ARG);
  avn = cif->nargs;
  p_arg = cif->arg_types;

  for (i = 0; i < avn; i++)
    {
      int type = (*p_arg)->type;

      switch (type)
	{
	case FFI_TYPE_SINT8:
	case FFI_TYPE_UINT8:
	case FFI_TYPE_SINT16:
	case FFI_TYPE_UINT16:
	case FFI_TYPE_SINT32:
	case FFI_TYPE_UINT32:
	case FFI_TYPE_POINTER:
	  slot++;
	  avalue[i] = (char *)(stack - slot) + sizeof(UINT32) - (*p_arg)->size;
	  break;

	case FFI_TYPE_SINT64:
	case FFI_TYPE_UINT64:
	  slot += 2;
	  if (slot & 1)
	    slot++;
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_FLOAT:
	  slot++;
	  switch (slot - FIRST_ARG_SLOT)
	    {
	    case 0: fstw(fr4, (void *)(stack - slot)); break;
	    case 1: fstw(fr5, (void *)(stack - slot)); break;
	    case 2: fstw(fr6, (void *)(stack - slot)); break;
	    case 3: fstw(fr7, (void *)(stack - slot)); break;
	    }
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_DOUBLE:
	  slot += 2;
	  if (slot & 1)
	    slot++;
	  switch (slot - FIRST_ARG_SLOT + 1)
	    {
	    case 2: fstd(fr5, (void *)(stack - slot)); break;
	    case 4: fstd(fr7, (void *)(stack - slot)); break;
	    }
	  avalue[i] = (void *)(stack - slot);
	  break;

	case FFI_TYPE_STRUCT:
	  /* Structs smaller or equal than 4 bytes are passed in one
	     register. Structs smaller or equal 8 bytes are passed in two
	     registers. Larger structures are passed by pointer.  */
	  if((*p_arg)->size <= 4) {
	    slot++;
	    avalue[i] = (void *)(stack - slot) + sizeof(UINT32) -
	      (*p_arg)->size;
	  } else if ((*p_arg)->size <= 8) {
	    slot += 2;
	    if (slot & 1)
	      slot++;
	    avalue[i] = (void *)(stack - slot) + sizeof(UINT64) -
	      (*p_arg)->size;
	  } else {
	    slot++;
	    avalue[i] = (void *) *(stack - slot);
	  }
	  break;

	default:
	  FFI_ASSERT(0);
	}

      p_arg++;
    }

  /* Invoke the closure.  */
  (closure->fun) (cif, rvalue, avalue, closure->user_data);

  debug(3, "after calling function, ret[0] = %08x, ret[1] = %08x\n", ret[0], ret[1]);

  /* Store the result */
  switch (cif->flags)
    {
    case FFI_TYPE_UINT8:
      *(stack - FIRST_ARG_SLOT) = (UINT8)(ret[0] >> 24);
      break;
    case FFI_TYPE_SINT8:
      *(stack - FIRST_ARG_SLOT) = (SINT8)(ret[0] >> 24);
      break;
    case FFI_TYPE_UINT16:
      *(stack - FIRST_ARG_SLOT) = (UINT16)(ret[0] >> 16);
      break;
    case FFI_TYPE_SINT16:
      *(stack - FIRST_ARG_SLOT) = (SINT16)(ret[0] >> 16);
      break;
    case FFI_TYPE_INT:
    case FFI_TYPE_SINT32:
    case FFI_TYPE_UINT32:
      *(stack - FIRST_ARG_SLOT) = ret[0];
      break;
    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      *(stack - FIRST_ARG_SLOT) = ret[0];
      *(stack - FIRST_ARG_SLOT - 1) = ret[1];
      break;

    case FFI_TYPE_DOUBLE:
      fldd(rvalue, fr4);
      break;

    case FFI_TYPE_FLOAT:
      fldw(rvalue, fr4);
      break;

    case FFI_TYPE_STRUCT:
      /* Don't need a return value, done by caller.  */
      break;

    case FFI_TYPE_SMALL_STRUCT3:
      tmp = (void*)(stack -  FIRST_ARG_SLOT);
      tmp += 4 - cif->rtype->size;
      memcpy((void*)tmp, &ret[0], cif->rtype->size);
      break;

    case FFI_TYPE_SMALL_STRUCT5:
    case FFI_TYPE_SMALL_STRUCT6:
    case FFI_TYPE_SMALL_STRUCT7:
      {
	unsigned int ret2[2];
	int off;

	/* Right justify ret[0] and ret[1] */
	switch (cif->flags)
	  {
	    case FFI_TYPE_SMALL_STRUCT5: off = 3; break;
	    case FFI_TYPE_SMALL_STRUCT6: off = 2; break;
	    case FFI_TYPE_SMALL_STRUCT7: off = 1; break;
	    default: off = 0; break;
	  }

	memset (ret2, 0, sizeof (ret2));
	memcpy ((char *)ret2 + off, ret, 8 - off);

	*(stack - FIRST_ARG_SLOT) = ret2[0];
	*(stack - FIRST_ARG_SLOT - 1) = ret2[1];
      }
      break;

    case FFI_TYPE_POINTER:
    case FFI_TYPE_VOID:
      break;

    default:
      debug(0, "assert with cif->flags: %d\n",cif->flags);
      FFI_ASSERT(0);
      break;
    }
  return FFI_OK;
}

/* Fill in a closure to refer to the specified fun and user_data.
   cif specifies the argument and result types for fun.
   The cif must already be prep'ed.  */

void ffi_closure_LINUX(void);

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  UINT32 *tramp = (UINT32 *)(closure->tramp);

  FFI_ASSERT (cif->abi == FFI_LINUX);

  /* Make a small trampoline that will branch to our
     handler function. Use PC-relative addressing.  */

  tramp[0] = 0xeaa00000; /* b,l  .+8, %r21      ; %r21 <- pc+8 */
  tramp[1] = 0xd6a01c1e; /* depi 0,31,2, %r21   ; mask priv bits */
  tramp[2] = 0x4aa10028; /* ldw  20(%r21), %r1  ; load plabel */
  tramp[3] = 0x36b53ff1; /* ldo  -8(%r21), %r21 ; get closure addr */
  tramp[4] = 0x0c201096; /* ldw  0(%r1), %r22   ; address of handler */
  tramp[5] = 0xeac0c000; /* bv	 %r0(%r22)      ; branch to handler */
  tramp[6] = 0x0c281093; /* ldw  4(%r1), %r19   ; GP of handler */
  tramp[7] = ((UINT32)(ffi_closure_LINUX) & ~2);

  /* Flush d/icache -- have to flush up 2 two lines because of
     alignment.  */
  asm volatile (
		"fdc 0(%0)\n"
		"fdc %1(%0)\n"
		"fic 0(%%sr4, %0)\n"
		"fic %1(%%sr4, %0)\n"
		"sync\n"
		: : "r"((unsigned long)tramp & ~31), "r"(32 /* stride */));

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}
#endif
