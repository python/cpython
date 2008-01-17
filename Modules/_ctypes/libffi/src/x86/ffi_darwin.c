# ifdef __i386__
/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1996, 1998, 1999, 2001  Red Hat, Inc.
           Copyright (c) 2002  Ranjit Mathew
           Copyright (c) 2002  Bo Thorsen
           Copyright (c) 2002  Roger Sayle
   
   x86 Foreign Function Interface 

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

#ifndef __x86_64__

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

/*@-exportheader@*/
void ffi_prep_args(char *stack, extended_cif *ecif);

static inline int retval_on_stack(ffi_type* tp)
{
	if (tp->type == FFI_TYPE_STRUCT) {
		int sz = tp->size;
		if (sz > 8) {
			return 1;
		}
		switch (sz) {
		case 1: case 2: case 4: case 8: return 0;
		default: return 1;
		}
	}
	return 0;
}


void ffi_prep_args(char *stack, extended_cif *ecif)
/*@=exportheader@*/
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if (retval_on_stack(ecif->cif->rtype)) {
      *(void **) argp = ecif->rvalue;
      argp += 4;
  }


  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if ((sizeof(int) - 1) & (unsigned) argp)
	      argp = (char *) ALIGN(argp, sizeof(int));

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

	    case FFI_TYPE_SINT32:
	      *(signed int *) argp = (signed int)*(SINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_UINT32:
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    case FFI_TYPE_STRUCT:
	      *(unsigned int *) argp = (unsigned int)*(UINT32 *)(* p_argv);
	      break;

	    default:
	      FFI_ASSERT(0);
	    }
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
#if !defined(X86_WIN32)  && !defined(X86_DARWIN)
    case FFI_TYPE_STRUCT:
#endif
    case FFI_TYPE_SINT64:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_LONGDOUBLE:
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_UINT64:
      cif->flags = FFI_TYPE_SINT64;
      break;

#if defined(X86_WIN32) || defined(X86_DARWIN)

    case FFI_TYPE_STRUCT:
      if (cif->rtype->size == 1)
        {
          cif->flags = FFI_TYPE_SINT8; /* same as char size */
        }
      else if (cif->rtype->size == 2)
        {
          cif->flags = FFI_TYPE_SINT16; /* same as short size */
        }
      else if (cif->rtype->size == 4)
        {
          cif->flags = FFI_TYPE_INT; /* same as int type */
        }
      else if (cif->rtype->size == 8)
        {
          cif->flags = FFI_TYPE_SINT64; /* same as int64 type */
        }
      else
        {
          cif->flags = FFI_TYPE_STRUCT;
        }
      break;
#endif

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  /* Darwin: The stack needs to be aligned to a multiple of 16 bytes */
#if 1
  cif->bytes = (cif->bytes + 15) & ~0xF;
#endif


  return FFI_OK;
}

/*@-declundef@*/
/*@-exportheader@*/
extern void ffi_call_SYSV(void (*)(char *, extended_cif *), 
			  /*@out@*/ extended_cif *, 
			  unsigned, unsigned, 
			  /*@out@*/ unsigned *, 
			  void (*fn)(void));
/*@=declundef@*/
/*@=exportheader@*/

#ifdef X86_WIN32
/*@-declundef@*/
/*@-exportheader@*/
extern void ffi_call_STDCALL(void (*)(char *, extended_cif *),
			  /*@out@*/ extended_cif *,
			  unsigned, unsigned,
			  /*@out@*/ unsigned *,
			  void (*fn)(void));
/*@=declundef@*/
/*@=exportheader@*/
#endif /* X86_WIN32 */

void ffi_call(/*@dependent@*/ ffi_cif *cif, 
	      void (*fn)(void), 
	      /*@out@*/ void *rvalue, 
	      /*@dependent@*/ void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

  if ((rvalue == NULL) && retval_on_stack(cif->rtype))
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
      /* To avoid changing the assembly code make sure the size of the argument 
       * block is a multiple of 16. Then add 8 to compensate for local variables
       * in ffi_call_SYSV.
       */
      ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, 
		    cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
#ifdef X86_WIN32
    case FFI_STDCALL:
      /*@-usedef@*/
      ffi_call_STDCALL(ffi_prep_args, &ecif, cif->bytes,
		    cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
#endif /* X86_WIN32 */
    default:
      FFI_ASSERT(0);
      break;
    }
}


/** private members **/

static void ffi_closure_SYSV (ffi_closure *)
     __attribute__ ((regparm(1)));
#if !FFI_NO_RAW_API
static void ffi_closure_raw_SYSV (ffi_raw_closure *)
     __attribute__ ((regparm(1)));
#endif

/*@-exportheader@*/
static inline void 
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue,
			    void **avalue, ffi_cif *cif)
/*@=exportheader@*/
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

  if (retval_on_stack(cif->rtype)) {
    *rvalue = *(void **) argp;
    argp += 4;
  }

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if ((sizeof(int) - 1) & (unsigned) argp) {
	argp = (char *) ALIGN(argp, sizeof(int));
      }

      z = (*p_arg)->size;

      /* because we're little endian, this is what it turns into.   */

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}

/* This function is jumped to by the trampoline */

static void
ffi_closure_SYSV (closure)
     ffi_closure *closure;
{
  // this is our return value storage
  long double  res;

  // our various things...
  ffi_cif       *cif;
  void         **arg_area;
  void          *resp = (void*)&res;
  void *args = __builtin_dwarf_cfa ();


  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  /* this call will initialize ARG_AREA, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will re-set RESP to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(args, (void**)&resp, arg_area, cif);
  
  (closure->fun) (cif, resp, arg_area, closure->user_data);

  /* now, do a generic return based on the value of rtype */
  if (cif->flags == FFI_TYPE_INT)
    {
      asm ("movl (%0),%%eax" : : "r" (resp) : "eax");
    }
  else if (cif->flags == FFI_TYPE_FLOAT)
    {
      asm ("flds (%0)" : : "r" (resp) : "st" );
    }
  else if (cif->flags == FFI_TYPE_DOUBLE)
    {
      asm ("fldl (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (cif->flags == FFI_TYPE_LONGDOUBLE)
    {
      asm ("fldt (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (cif->flags == FFI_TYPE_SINT64)
    {
      asm ("movl 0(%0),%%eax;"
	   "movl 4(%0),%%edx" 
	   : : "r"(resp)
	   : "eax", "edx");
    }
#if defined(X86_WIN32) || defined(X86_DARWIN)
  else if (cif->flags == FFI_TYPE_SINT8) /* 1-byte struct  */
    {
      asm ("movsbl (%0),%%eax" : : "r" (resp) : "eax");
    }
  else if (cif->flags == FFI_TYPE_SINT16) /* 2-bytes struct */
    {
      asm ("movswl (%0),%%eax" : : "r" (resp) : "eax");
    }
#endif

  else if (cif->flags == FFI_TYPE_STRUCT)
    {
      asm ("lea -8(%ebp),%esp;"
	   "pop %esi;"
	   "pop %edi;"
	   "pop %ebp;"
	   "ret $4"); 
    }
}


/* How to make a trampoline.  Derived from gcc/config/i386/i386.c. */

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX) \
({ unsigned char *__tramp = (unsigned char*)(TRAMP); \
   unsigned int  __fun = (unsigned int)(FUN); \
   unsigned int  __ctx = (unsigned int)(CTX); \
   unsigned int  __dis = __fun - ((unsigned int) __tramp + FFI_TRAMPOLINE_SIZE); \
   *(unsigned char*) &__tramp[0] = 0xb8; \
   *(unsigned int*)  &__tramp[1] = __ctx; /* movl __ctx, %eax */ \
   *(unsigned char *)  &__tramp[5] = 0xe9; \
   *(unsigned int*)  &__tramp[6] = __dis; /* jmp __fun  */ \
 })


/* the cif must already be prep'ed */

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  FFI_ASSERT (cif->abi == FFI_SYSV);

  FFI_INIT_TRAMPOLINE (&closure->tramp[0], \
		       &ffi_closure_SYSV,  \
		       (void*)closure);
    
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

/* ------- Native raw API support -------------------------------- */

#if !FFI_NO_RAW_API

static void
ffi_closure_raw_SYSV (closure)
     ffi_raw_closure *closure;
{
  // this is our return value storage
  long double    res;

  // our various things...
  ffi_raw         *raw_args;
  ffi_cif         *cif;
  unsigned short   rtype;
  void            *resp = (void*)&res;

  /* get the cif */
  cif = closure->cif;

  /* the SYSV/X86 abi matches the RAW API exactly, well.. almost */
  raw_args = (ffi_raw*) __builtin_dwarf_cfa ();

  (closure->fun) (cif, resp, raw_args, closure->user_data);

  rtype = cif->flags;

  /* now, do a generic return based on the value of rtype */
  if (rtype == FFI_TYPE_INT)
    {
      asm ("movl (%0),%%eax" : : "r" (resp) : "eax");
    }
  else if (rtype == FFI_TYPE_FLOAT)
    {
      asm ("flds (%0)" : : "r" (resp) : "st" );
    }
  else if (rtype == FFI_TYPE_DOUBLE)
    {
      asm ("fldl (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_LONGDOUBLE)
    {
      asm ("fldt (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_SINT64)
    {
      asm ("movl 0(%0),%%eax; movl 4(%0),%%edx" 
	   : : "r"(resp)
	   : "eax", "edx");
    }
}

 


ffi_status
ffi_prep_raw_closure (ffi_raw_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,ffi_raw*,void*),
		      void *user_data)
{
  int i;

  FFI_ASSERT (cif->abi == FFI_SYSV);

  // we currently don't support certain kinds of arguments for raw
  // closures.  This should be implemented by a separate assembly language
  // routine, since it would require argument processing, something we
  // don't do now for performance.

  for (i = cif->nargs-1; i >= 0; i--)
    {
      FFI_ASSERT (cif->arg_types[i]->type != FFI_TYPE_STRUCT);
      FFI_ASSERT (cif->arg_types[i]->type != FFI_TYPE_LONGDOUBLE);
    }
  

  FFI_INIT_TRAMPOLINE (&closure->tramp[0], &ffi_closure_raw_SYSV,
		       (void*)closure);
    
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

static void 
ffi_prep_args_raw(char *stack, extended_cif *ecif)
{
  memcpy (stack, ecif->avalue, ecif->cif->bytes);
}

/* we borrow this routine from libffi (it must be changed, though, to
 * actually call the function passed in the first argument.  as of
 * libffi-1.20, this is not the case.)
 */

extern void 
ffi_call_SYSV(void (*)(char *, extended_cif *), 
	      /*@out@*/ extended_cif *, 
	      unsigned, unsigned, 
	      /*@out@*/ unsigned *, 
	      void (*fn)());

#ifdef X86_WIN32
extern void
ffi_call_STDCALL(void (*)(char *, extended_cif *),
	      /*@out@*/ extended_cif *,
	      unsigned, unsigned,
	      /*@out@*/ unsigned *,
	      void (*fn)());
#endif /* X86_WIN32 */

void
ffi_raw_call(/*@dependent@*/ ffi_cif *cif, 
	     void (*fn)(), 
	     /*@out@*/ void *rvalue, 
	     /*@dependent@*/ ffi_raw *fake_avalue)
{
  extended_cif ecif;
  void **avalue = (void **)fake_avalue;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return	*/
  /* value address then we need to make one		        */

  if ((rvalue == NULL) && retval_on_stack(cif->rtype)) 
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
      ffi_call_SYSV(ffi_prep_args_raw, &ecif, cif->bytes, 
		    cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
#ifdef X86_WIN32
    case FFI_STDCALL:
      /*@-usedef@*/
      ffi_call_STDCALL(ffi_prep_args_raw, &ecif, cif->bytes,
		    cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
#endif /* X86_WIN32 */
    default:
      FFI_ASSERT(0);
      break;
    }
}

#endif

#endif /* __x86_64__  */

#endif /* __i386__ */
