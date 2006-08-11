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

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>

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

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT)
    {
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
    case FFI_TYPE_STRUCT:
    case FFI_TYPE_SINT64:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
    case FFI_TYPE_LONGDOUBLE:
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
extern int
ffi_call_SYSV(void (*)(char *, extended_cif *), 
	      /*@out@*/ extended_cif *, 
	      unsigned, unsigned, 
	      /*@out@*/ unsigned *, 
	      void (*fn)());
/*@=declundef@*/
/*@=exportheader@*/

/*@-declundef@*/
/*@-exportheader@*/
extern int
ffi_call_STDCALL(void (*)(char *, extended_cif *),
		 /*@out@*/ extended_cif *,
		 unsigned, unsigned,
		 /*@out@*/ unsigned *,
		 void (*fn)());
/*@=declundef@*/
/*@=exportheader@*/

int
ffi_call(/*@dependent@*/ ffi_cif *cif, 
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
      return ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, 
			   cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;

    case FFI_STDCALL:
      /*@-usedef@*/
      return ffi_call_STDCALL(ffi_prep_args, &ecif, cif->bytes,
			      cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;

    default:
      FFI_ASSERT(0);
      break;
    }
  return -1; /* theller: Hrm. */
}


/** private members **/

static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
					 void** args, ffi_cif* cif);
/* This function is jumped to by the trampoline */

static void __fastcall
ffi_closure_SYSV (ffi_closure *closure, int *argp)
{
  // this is our return value storage
  long double    res;

  // our various things...
  ffi_cif       *cif;
  void         **arg_area;
  unsigned short rtype;
  void          *resp = (void*)&res;
  void *args = &argp[1];

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  /* this call will initialize ARG_AREA, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will re-set RESP to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(args, (void**)&resp, arg_area, cif);
  
  (closure->fun) (cif, resp, arg_area, closure->user_data);

  rtype = cif->flags;

#ifdef _MSC_VER
  /* now, do a generic return based on the value of rtype */
  if (rtype == FFI_TYPE_INT)
    {
	    _asm mov eax, resp ;
	    _asm mov eax, [eax] ;
    }
  else if (rtype == FFI_TYPE_FLOAT)
    {
	    _asm mov eax, resp ;
	    _asm fld DWORD PTR [eax] ;
//      asm ("flds (%0)" : : "r" (resp) : "st" );
    }
  else if (rtype == FFI_TYPE_DOUBLE)
    {
	    _asm mov eax, resp ;
	    _asm fld QWORD PTR [eax] ;
//      asm ("fldl (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_LONGDOUBLE)
    {
//      asm ("fldt (%0)" : : "r" (resp) : "st", "st(1)" );
    }
  else if (rtype == FFI_TYPE_SINT64)
    {
	    _asm mov edx, resp ;
	    _asm mov eax, [edx] ;
	    _asm mov edx, [edx + 4] ;
//      asm ("movl 0(%0),%%eax;"
//	   "movl 4(%0),%%edx" 
//	   : : "r"(resp)
//	   : "eax", "edx");
    }
#else
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
      asm ("movl 0(%0),%%eax;"
	   "movl 4(%0),%%edx" 
	   : : "r"(resp)
	   : "eax", "edx");
    }
#endif
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

  if ( cif->rtype->type == FFI_TYPE_STRUCT ) {
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

/* How to make a trampoline.  Derived from gcc/config/i386/i386.c. */

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX,BYTES) \
{ unsigned char *__tramp = (unsigned char*)(TRAMP); \
   unsigned int  __fun = (unsigned int)(FUN); \
   unsigned int  __ctx = (unsigned int)(CTX); \
   unsigned int  __dis = __fun - ((unsigned int) __tramp + 8 + 4); \
   *(unsigned char*)  &__tramp[0] = 0xb9; \
   *(unsigned int*)   &__tramp[1] = __ctx; /* mov ecx, __ctx */ \
   *(unsigned char*)  &__tramp[5] = 0x8b; \
   *(unsigned char*)  &__tramp[6] = 0xd4; /* mov edx, esp */ \
   *(unsigned char*)  &__tramp[7] = 0xe8; \
   *(unsigned int*)   &__tramp[8] = __dis; /* call __fun  */ \
   *(unsigned char*)  &__tramp[12] = 0xC2; /* ret BYTES */ \
   *(unsigned short*) &__tramp[13] = BYTES; \
 }

/* the cif must already be prep'ed */

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  short bytes;
  FFI_ASSERT (cif->abi == FFI_SYSV);
  
  if (cif->abi == FFI_SYSV)
    bytes = 0;
  else if (cif->abi == FFI_STDCALL)
    bytes = cif->bytes;
  else
    return FFI_BAD_ABI;

  FFI_INIT_TRAMPOLINE (&closure->tramp[0],
		       &ffi_closure_SYSV,
		       (void*)closure,
		       bytes);
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}
