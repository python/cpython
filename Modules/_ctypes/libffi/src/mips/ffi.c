/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1996 Red Hat, Inc.
   
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

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <sys/cachectl.h>

#if _MIPS_SIM == _ABIN32
#define FIX_ARGP \
FFI_ASSERT(argp <= &stack[bytes]); \
if (argp == &stack[bytes]) \
{ \
  argp = stack; \
  ffi_stop_here(); \
}
#else
#define FIX_ARGP 
#endif


/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

static void ffi_prep_args(char *stack, 
			  extended_cif *ecif,
			  int bytes,
			  int flags)
{
  int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

#if _MIPS_SIM == _ABIN32
  /* If more than 8 double words are used, the remainder go
     on the stack. We reorder stuff on the stack here to 
     support this easily. */
  if (bytes > 8 * sizeof(ffi_arg))
    argp = &stack[bytes - (8 * sizeof(ffi_arg))];
  else
    argp = stack;
#else
  argp = stack;
#endif

  memset(stack, 0, bytes);

#if _MIPS_SIM == _ABIN32
  if ( ecif->cif->rstruct_flag != 0 )
#else
  if ( ecif->cif->rtype->type == FFI_TYPE_STRUCT )
#endif  
    {
      *(ffi_arg *) argp = (ffi_arg) ecif->rvalue;
      argp += sizeof(ffi_arg);
      FIX_ARGP;
    }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types; i; i--, p_arg++)
    {
      size_t z;
      unsigned int a;

      /* Align if necessary.  */
      a = (*p_arg)->alignment;
      if (a < sizeof(ffi_arg))
        a = sizeof(ffi_arg);
      
      if ((a - 1) & (unsigned int) argp)
	{
	  argp = (char *) ALIGN(argp, a);
	  FIX_ARGP;
	}

      z = (*p_arg)->size;
      if (z <= sizeof(ffi_arg))
	{
	  z = sizeof(ffi_arg);

	  switch ((*p_arg)->type)
	    {
	      case FFI_TYPE_SINT8:
		*(ffi_arg *)argp = *(SINT8 *)(* p_argv);
		break;

	      case FFI_TYPE_UINT8:
		*(ffi_arg *)argp = *(UINT8 *)(* p_argv);
		break;
		  
	      case FFI_TYPE_SINT16:
		*(ffi_arg *)argp = *(SINT16 *)(* p_argv);
		break;
		  
	      case FFI_TYPE_UINT16:
		*(ffi_arg *)argp = *(UINT16 *)(* p_argv);
		break;
		  
	      case FFI_TYPE_SINT32:
		*(ffi_arg *)argp = *(SINT32 *)(* p_argv);
		break;
		  
	      case FFI_TYPE_UINT32:
	      case FFI_TYPE_POINTER:
		*(ffi_arg *)argp = *(UINT32 *)(* p_argv);
		break;

	      /* This can only happen with 64bit slots.  */
	      case FFI_TYPE_FLOAT:
		*(float *) argp = *(float *)(* p_argv);
		break;

	      /* Handle small structures.  */
	      case FFI_TYPE_STRUCT:
	      default:
		memcpy(argp, *p_argv, (*p_arg)->size);
		break;
	    }
	}
      else
	{
#if _MIPS_SIM == _ABIO32
	  memcpy(argp, *p_argv, z);
#else
	  {
	    unsigned end = (unsigned) argp+z;
	    unsigned cap = (unsigned) stack+bytes;

	    /* Check if the data will fit within the register space.
	       Handle it if it doesn't.  */

	    if (end <= cap)
	      memcpy(argp, *p_argv, z);
	    else
	      {
		unsigned portion = end - cap;

		memcpy(argp, *p_argv, portion);
		argp = stack;
		memcpy(argp,
		       (void*)((unsigned)(*p_argv)+portion), z - portion);
	      }
	  }
#endif
      }
      p_argv++;
      argp += z;
      FIX_ARGP;
    }
}

#if _MIPS_SIM == _ABIN32

/* The n32 spec says that if "a chunk consists solely of a double 
   float field (but not a double, which is part of a union), it
   is passed in a floating point register. Any other chunk is
   passed in an integer register". This code traverses structure
   definitions and generates the appropriate flags. */

unsigned calc_n32_struct_flags(ffi_type *arg, unsigned *shift)
{
  unsigned flags = 0;
  unsigned index = 0;

  ffi_type *e;

  while (e = arg->elements[index])
    {
      if (e->type == FFI_TYPE_DOUBLE)
	{
	  flags += (FFI_TYPE_DOUBLE << *shift);
	  *shift += FFI_FLAG_BITS;
	}
      else if (e->type == FFI_TYPE_STRUCT)
	  flags += calc_n32_struct_flags(e, shift);
      else
	*shift += FFI_FLAG_BITS;

      index++;
    }

  return flags;
}

unsigned calc_n32_return_struct_flags(ffi_type *arg)
{
  unsigned flags = 0;
  unsigned index = 0;
  unsigned small = FFI_TYPE_SMALLSTRUCT;
  ffi_type *e;

  /* Returning structures under n32 is a tricky thing.
     A struct with only one or two floating point fields 
     is returned in $f0 (and $f2 if necessary). Any other
     struct results at most 128 bits are returned in $2
     (the first 64 bits) and $3 (remainder, if necessary).
     Larger structs are handled normally. */
  
  if (arg->size > 16)
    return 0;

  if (arg->size > 8)
    small = FFI_TYPE_SMALLSTRUCT2;

  e = arg->elements[0];
  if (e->type == FFI_TYPE_DOUBLE)
    flags = FFI_TYPE_DOUBLE << FFI_FLAG_BITS;
  else if (e->type == FFI_TYPE_FLOAT)
    flags = FFI_TYPE_FLOAT << FFI_FLAG_BITS;

  if (flags && (e = arg->elements[1]))
    {
      if (e->type == FFI_TYPE_DOUBLE)
	flags += FFI_TYPE_DOUBLE;
      else if (e->type == FFI_TYPE_FLOAT)
	flags += FFI_TYPE_FLOAT;
      else 
	return small;

      if (flags && (arg->elements[2]))
	{
	  /* There are three arguments and the first two are 
	     floats! This must be passed the old way. */
	  return small;
	}
    }
  else
    if (!flags)
      return small;

  return flags;
}

#endif

/* Perform machine dependent cif processing */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  cif->flags = 0;

#if _MIPS_SIM == _ABIO32
  /* Set the flags necessary for O32 processing.  FFI_O32_SOFT_FLOAT
   * does not have special handling for floating point args.
   */

  if (cif->rtype->type != FFI_TYPE_STRUCT && cif->abi == FFI_O32)
    {
      if (cif->nargs > 0)
	{
	  switch ((cif->arg_types)[0]->type)
	    {
	    case FFI_TYPE_FLOAT:
	    case FFI_TYPE_DOUBLE:
	      cif->flags += (cif->arg_types)[0]->type;
	      break;
	      
	    default:
	      break;
	    }

	  if (cif->nargs > 1)
	    {
	      /* Only handle the second argument if the first
		 is a float or double. */
	      if (cif->flags)
		{
		  switch ((cif->arg_types)[1]->type)
		    {
		    case FFI_TYPE_FLOAT:
		    case FFI_TYPE_DOUBLE:
		      cif->flags += (cif->arg_types)[1]->type << FFI_FLAG_BITS;
		      break;
		      
		    default:
		      break;
		    }
		}
	    }
	}
    }
      
  /* Set the return type flag */

  if (cif->abi == FFI_O32_SOFT_FLOAT)
    {
      switch (cif->rtype->type)
        {
        case FFI_TYPE_VOID:
        case FFI_TYPE_STRUCT:
          cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 2);
          break;

        case FFI_TYPE_SINT64:
        case FFI_TYPE_UINT64:
        case FFI_TYPE_DOUBLE:
          cif->flags += FFI_TYPE_UINT64 << (FFI_FLAG_BITS * 2);
          break;
      
        case FFI_TYPE_FLOAT:
        default:
          cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 2);
          break;
        }
    }
  else
    {
      /* FFI_O32 */      
      switch (cif->rtype->type)
        {
        case FFI_TYPE_VOID:
        case FFI_TYPE_STRUCT:
        case FFI_TYPE_FLOAT:
        case FFI_TYPE_DOUBLE:
          cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 2);
          break;

        case FFI_TYPE_SINT64:
        case FFI_TYPE_UINT64:
          cif->flags += FFI_TYPE_UINT64 << (FFI_FLAG_BITS * 2);
          break;
      
        default:
          cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 2);
          break;
        }
    }
#endif

#if _MIPS_SIM == _ABIN32
  /* Set the flags necessary for N32 processing */
  {
    unsigned shift = 0;
    unsigned count = (cif->nargs < 8) ? cif->nargs : 8;
    unsigned index = 0;

    unsigned struct_flags = 0;

    if (cif->rtype->type == FFI_TYPE_STRUCT)
      {
	struct_flags = calc_n32_return_struct_flags(cif->rtype);

	if (struct_flags == 0)
	  {
	    /* This means that the structure is being passed as
	       a hidden argument */

	    shift = FFI_FLAG_BITS;
	    count = (cif->nargs < 7) ? cif->nargs : 7;

	    cif->rstruct_flag = !0;
	  }
	else
	    cif->rstruct_flag = 0;
      }
    else
      cif->rstruct_flag = 0;

    while (count-- > 0)
      {
	switch ((cif->arg_types)[index]->type)
	  {
	  case FFI_TYPE_FLOAT:
	  case FFI_TYPE_DOUBLE:
	    cif->flags += ((cif->arg_types)[index]->type << shift);
	    shift += FFI_FLAG_BITS;
	    break;

	  case FFI_TYPE_STRUCT:
	    cif->flags += calc_n32_struct_flags((cif->arg_types)[index],
						&shift);
	    break;

	  default:
	    shift += FFI_FLAG_BITS;
	  }

	index++;
      }

  /* Set the return type flag */
    switch (cif->rtype->type)
      {
      case FFI_TYPE_STRUCT:
	{
	  if (struct_flags == 0)
	    {
	      /* The structure is returned through a hidden
		 first argument. Do nothing, 'cause FFI_TYPE_VOID 
		 is 0 */
	    }
	  else
	    {
	      /* The structure is returned via some tricky
		 mechanism */
	      cif->flags += FFI_TYPE_STRUCT << (FFI_FLAG_BITS * 8);
	      cif->flags += struct_flags << (4 + (FFI_FLAG_BITS * 8));
	    }
	  break;
	}
      
      case FFI_TYPE_VOID:
	/* Do nothing, 'cause FFI_TYPE_VOID is 0 */
	break;
	
      case FFI_TYPE_FLOAT:
      case FFI_TYPE_DOUBLE:
	cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 8);
	break;
	
      default:
	cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 8);
	break;
      }
  }
#endif
  
  return FFI_OK;
}

/* Low level routine for calling O32 functions */
extern int ffi_call_O32(void (*)(char *, extended_cif *, int, int), 
			extended_cif *, unsigned, 
			unsigned, unsigned *, void (*)());

/* Low level routine for calling N32 functions */
extern int ffi_call_N32(void (*)(char *, extended_cif *, int, int), 
			extended_cif *, unsigned, 
			unsigned, unsigned *, void (*)());

void ffi_call(ffi_cif *cif, void (*fn)(), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */
  
  if ((rvalue == NULL) && 
      (cif->rtype->type == FFI_TYPE_STRUCT))
    ecif.rvalue = alloca(cif->rtype->size);
  else
    ecif.rvalue = rvalue;
    
  switch (cif->abi) 
    {
#if _MIPS_SIM == _ABIO32
    case FFI_O32:
    case FFI_O32_SOFT_FLOAT:
      ffi_call_O32(ffi_prep_args, &ecif, cif->bytes, 
		   cif->flags, ecif.rvalue, fn);
      break;
#endif

#if _MIPS_SIM == _ABIN32
    case FFI_N32:
      ffi_call_N32(ffi_prep_args, &ecif, cif->bytes, 
		   cif->flags, ecif.rvalue, fn);
      break;
#endif

    default:
      FFI_ASSERT(0);
      break;
    }
}

#if FFI_CLOSURES  /* N32 not implemented yet, FFI_CLOSURES not defined */
#if defined(FFI_MIPS_O32)
extern void ffi_closure_O32(void);
#endif /* FFI_MIPS_O32 */

ffi_status
ffi_prep_closure (ffi_closure *closure,
		  ffi_cif *cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  unsigned int *tramp = (unsigned int *) &closure->tramp[0];
  unsigned int fn;
  unsigned int ctx = (unsigned int) closure;

#if defined(FFI_MIPS_O32)
  FFI_ASSERT(cif->abi == FFI_O32 || cif->abi == FFI_O32_SOFT_FLOAT);
  fn = (unsigned int) ffi_closure_O32;
#else /* FFI_MIPS_N32 */
  FFI_ASSERT(cif->abi == FFI_N32);
  FFI_ASSERT(!"not implemented");
#endif /* FFI_MIPS_O32 */

  tramp[0] = 0x3c190000 | (fn >> 16);     /* lui  $25,high(fn) */
  tramp[1] = 0x37390000 | (fn & 0xffff);  /* ori  $25,low(fn)  */
  tramp[2] = 0x3c080000 | (ctx >> 16);    /* lui  $8,high(ctx) */
  tramp[3] = 0x03200008;                  /* jr   $25          */
  tramp[4] = 0x35080000 | (ctx & 0xffff); /* ori  $8,low(ctx)  */

  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;

  /* XXX this is available on Linux, but anything else? */
  cacheflush (tramp, FFI_TRAMPOLINE_SIZE, ICACHE);

  return FFI_OK;
}

/*
 * Decodes the arguments to a function, which will be stored on the
 * stack. AR is the pointer to the beginning of the integer arguments
 * (and, depending upon the arguments, some floating-point arguments
 * as well). FPR is a pointer to the area where floating point
 * registers have been saved, if any.
 *
 * RVALUE is the location where the function return value will be
 * stored. CLOSURE is the prepared closure to invoke.
 *
 * This function should only be called from assembly, which is in
 * turn called from a trampoline.
 *
 * Returns the function return type.
 *
 * Based on the similar routine for sparc.
 */
int
ffi_closure_mips_inner_O32 (ffi_closure *closure,
			    void *rvalue, ffi_arg *ar,
			    double *fpr)
{
  ffi_cif *cif;
  void **avaluep;
  ffi_arg *avalue;
  ffi_type **arg_types;
  int i, avn, argn, seen_int;

  cif = closure->cif;
  avalue = alloca (cif->nargs * sizeof (ffi_arg));
  avaluep = alloca (cif->nargs * sizeof (ffi_arg));

  seen_int = (cif->abi == FFI_O32_SOFT_FLOAT);
  argn = 0;

  if ((cif->flags >> (FFI_FLAG_BITS * 2)) == FFI_TYPE_STRUCT)
    {
      rvalue = (void *) ar[0];
      argn = 1;
    }

  i = 0;
  avn = cif->nargs;
  arg_types = cif->arg_types;

  while (i < avn)
    {
      if (i < 2 && !seen_int &&
	  (arg_types[i]->type == FFI_TYPE_FLOAT ||
	   arg_types[i]->type == FFI_TYPE_DOUBLE))
	{
#ifdef __MIPSEB__
	  if (arg_types[i]->type == FFI_TYPE_FLOAT)
	    avaluep[i] = ((char *) &fpr[i]) + sizeof (float);
	  else
#endif
	    avaluep[i] = (char *) &fpr[i];
	}
      else
	{
	  if (arg_types[i]->alignment == 8 && (argn & 0x1))
	    argn++;
	  switch (arg_types[i]->type)
	    {
	      case FFI_TYPE_SINT8:
		avaluep[i] = &avalue[i];
		*(SINT8 *) &avalue[i] = (SINT8) ar[argn];
		break;

	      case FFI_TYPE_UINT8:
		avaluep[i] = &avalue[i];
		*(UINT8 *) &avalue[i] = (UINT8) ar[argn];
		break;
		  
	      case FFI_TYPE_SINT16:
		avaluep[i] = &avalue[i];
		*(SINT16 *) &avalue[i] = (SINT16) ar[argn];
		break;
		  
	      case FFI_TYPE_UINT16:
		avaluep[i] = &avalue[i];
		*(UINT16 *) &avalue[i] = (UINT16) ar[argn];
		break;

	      default:
		avaluep[i] = (char *) &ar[argn];
		break;
	    }
	  seen_int = 1;
	}
      argn += ALIGN(arg_types[i]->size, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
      i++;
    }

  /* Invoke the closure. */
  (closure->fun) (cif, rvalue, avaluep, closure->user_data);

  if (cif->abi == FFI_O32_SOFT_FLOAT)
    {
      switch (cif->rtype->type)
        {
        case FFI_TYPE_FLOAT:
          return FFI_TYPE_INT;
        case FFI_TYPE_DOUBLE:
          return FFI_TYPE_UINT64;
        default:
          return cif->rtype->type;
        }
    }
  else
    {
      return cif->rtype->type;
    }
}

#endif /* FFI_CLOSURES */
