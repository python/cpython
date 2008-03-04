/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1998, 2008  Red Hat, Inc.
   
   ARM Foreign Function Interface 

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

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

void ffi_prep_args(char *stack, extended_cif *ecif)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( ecif->cif->flags == FFI_TYPE_STRUCT ) {
    *(void **) argp = ecif->rvalue;
    argp += 4;
  }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if (((*p_arg)->alignment - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, (*p_arg)->alignment);
      }

      if ((*p_arg)->type == FFI_TYPE_STRUCT)
	argp = (char *) ALIGN(argp, 4);

	  z = (*p_arg)->size;
	  if (z < sizeof(int))
	    {
	      z = sizeof(int);
	      switch ((*p_arg)->type)
		{
		case FFI_TYPE_SINT8:
		  *(signed int *) argp = (signed int)*(SINT8 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_UINT8:
		  *(unsigned int *) argp = (unsigned int)*(UINT8 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_SINT16:
		  *(signed int *) argp = (signed int)*(SINT16 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_UINT16:
		  *(unsigned int *) argp = (unsigned int)*(UINT16 *)(* p_argv);
		  break;
		  
		case FFI_TYPE_STRUCT:
		  memcpy(argp, *p_argv, (*p_arg)->size);
		  break;

		default:
		  FFI_ASSERT(0);
		}
	    }
	  else if (z == sizeof(int))
	    {
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	    }
	  else
	    {
	      memcpy(argp, *p_argv, z);
	    }
	  p_argv++;
	  argp += z;
    }
  
  return;
}

/* Perform machine dependent cif processing */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  /* Round the stack up to a multiple of 8 bytes.  This isn't needed 
     everywhere, but it is on some platforms, and it doesn't harm anything
     when it isn't needed.  */
  cif->bytes = (cif->bytes + 7) & ~7;

  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = (unsigned) FFI_TYPE_SINT64;
      break;

    case FFI_TYPE_STRUCT:
      if (cif->rtype->size <= 4)
	/* A Composite Type not larger than 4 bytes is returned in r0.  */
	cif->flags = (unsigned)FFI_TYPE_INT;
      else
	/* A Composite Type larger than 4 bytes, or whose size cannot
	   be determined statically ... is stored in memory at an
	   address passed [in r0].  */
	cif->flags = (unsigned)FFI_TYPE_STRUCT;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  return FFI_OK;
}

extern void ffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *,
			  unsigned, unsigned, unsigned *, void (*fn)(void));

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  int small_struct = (cif->flags == FFI_TYPE_INT 
		      && cif->rtype->type == FFI_TYPE_STRUCT);

  ecif.cif = cif;
  ecif.avalue = avalue;

  unsigned int temp;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

  if ((rvalue == NULL) && 
      (cif->flags == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else if (small_struct)
    ecif.rvalue = &temp;
  else
    ecif.rvalue = rvalue;

  switch (cif->abi) 
    {
    case FFI_SYSV:
      ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue,
		    fn);

      break;
    default:
      FFI_ASSERT(0);
      break;
    }
  if (small_struct)
    memcpy (rvalue, &temp, cif->rtype->size);
}

/** private members **/

static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					 void** args, ffi_cif* cif);

void ffi_closure_SYSV (ffi_closure *);

/* This function is jumped to by the trampoline */

unsigned int
ffi_closure_SYSV_inner (closure, respp, args)
     ffi_closure *closure;
     void **respp;
     void *args;
{
  // our various things...
  ffi_cif       *cif;
  void         **arg_area;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  /* this call will initialize ARG_AREA, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will re-set RESP to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(args, respp, arg_area, cif);

  (closure->fun) (cif, *respp, arg_area, closure->user_data);

  return cif->flags;
}

/*@-exportheader@*/
static void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif)
/*@=exportheader@*/
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( cif->flags == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      size_t alignment = (*p_arg)->alignment;
      if (alignment < 4)
	alignment = 4;
      /* Align if necessary */
      if ((alignment - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, alignment);
      }

      z = (*p_arg)->size;

      /* because we're little endian, this is what it turns into.   */

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}

/* How to make a trampoline.  */

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX)				\
({ unsigned char *__tramp = (unsigned char*)(TRAMP);			\
   unsigned int  __fun = (unsigned int)(FUN);				\
   unsigned int  __ctx = (unsigned int)(CTX);				\
   *(unsigned int*) &__tramp[0] = 0xe92d000f; /* stmfd sp!, {r0-r3} */	\
   *(unsigned int*) &__tramp[4] = 0xe59f0000; /* ldr r0, [pc] */	\
   *(unsigned int*) &__tramp[8] = 0xe59ff000; /* ldr pc, [pc] */	\
   *(unsigned int*) &__tramp[12] = __ctx;				\
   *(unsigned int*) &__tramp[16] = __fun;				\
   __clear_cache((&__tramp[0]), (&__tramp[19]));			\
 })


/* the cif must already be prep'ed */

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  FFI_ASSERT (cif->abi == FFI_SYSV);

  FFI_INIT_TRAMPOLINE (&closure->tramp[0], \
		       &ffi_closure_SYSV,  \
		       codeloc);
    
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}
