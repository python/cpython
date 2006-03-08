/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1998  Red Hat, Inc.
   
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

#ifdef _WIN32_WCE
#pragma warning (disable : 4142)    /* benign redefinition of type */
#include <windows.h>
#endif

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

/*@-exportheader@*/
void ffi_prep_args(char *stack, extended_cif *ecif)
/*@=exportheader@*/
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if ( ecif->cif->rtype->type == FFI_TYPE_STRUCT ) {
    *(void **) argp = ecif->rvalue;
    argp += 4;
  }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       (i != 0);
       i--, p_arg++)
    {
      size_t z;
      size_t argalign = (*p_arg)->alignment;

#ifdef _WIN32_WCE
      if (argalign > 4)
        argalign = 4;
#endif
      /* Align if necessary */
      if ((argalign - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, argalign);
      }

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
                  /* *p_argv may not be aligned for a UINT32 */
                  memcpy(argp, *p_argv, z);
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
  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
    case FFI_TYPE_STRUCT:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_SINT64:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_UINT64:
      cif->flags = FFI_TYPE_SINT64;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  return FFI_OK;
}

/*@-declundef@*/
/*@-exportheader@*/
extern void ffi_call_SYSV(void (*)(char *, extended_cif *), 
			  /*@out@*/ extended_cif *, 
			  unsigned, unsigned, 
			  /*@out@*/ unsigned *, 
			  void (*fn)());
/*@=declundef@*/
/*@=exportheader@*/

/* Return type changed from void for ctypes */
int ffi_call(/*@dependent@*/ ffi_cif *cif, 
	      void (*fn)(), 
	      /*@out@*/ void *rvalue, 
	      /*@dependent@*/ void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

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
    case FFI_SYSV:
      /*@-usedef@*/
      ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, 
		    cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
    default:
      FFI_ASSERT(0);
      break;
    }
  /* I think calculating the real stack pointer delta is not useful
     because stdcall is not supported */
  return 0;
}

/** private members **/

static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					 void** args, ffi_cif* cif);

/* This function is called by ffi_closure_SYSV in sysv.asm */

unsigned int
ffi_closure_SYSV_inner (ffi_closure *closure, char *in_args, void *rvalue)
{
  ffi_cif *cif = closure->cif;
  void **out_args;

  out_args = (void **) alloca(cif->nargs * sizeof (void *));  

  /* this call will initialize out_args, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will re-set rvalue to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(in_args, &rvalue, out_args, cif);
  
  (closure->fun)(cif, rvalue, out_args, closure->user_data);

  /* Tell ffi_closure_SYSV what the returntype is */
  return cif->flags;
}

/*@-exportheader@*/
static void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif)
/*@=exportheader@*/
{
  unsigned int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  argp = stack;

  if ( cif->rtype->type == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;
      size_t argalign = (*p_arg)->alignment;

#ifdef _WIN32_WCE
      if (argalign > 4)
        argalign = 4;
#endif
      /* Align if necessary */
      if ((argalign - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, argalign);
      }

      z = (*p_arg)->size;
      if (z < sizeof(int))
        z = sizeof(int);

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
}

/*
    add   ip, pc, #-8     ; ip = address of this trampoline == address of ffi_closure
    ldr   pc, [pc, #-4]   ; jump to __fun
    DCD __fun
*/
#define FFI_INIT_TRAMPOLINE(TRAMP,FUN) \
{ \
    unsigned int *__tramp = (unsigned int *)(TRAMP); \
    __tramp[0] = 0xe24fc008;            /* add   ip, pc, #-8    */ \
    __tramp[1] = 0xe51ff004;            /* ldr   pc, [pc, #-4]  */ \
    __tramp[2] = (unsigned int)(FUN); \
  }

/* the cif must already be prep'ed */

/* defined in sysv.asm */
void ffi_closure_SYSV(void);

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  FFI_ASSERT (cif->abi == FFI_SYSV);
  
  FFI_INIT_TRAMPOLINE (&closure->tramp[0], &ffi_closure_SYSV);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

#ifdef _WIN32_WCE
  /* This is important to allow calling the trampoline safely */
  FlushInstructionCache(GetCurrentProcess(), 0, 0);
#endif

  return FFI_OK;
}

