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

extern void Py_FatalError(const char *msg);

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
      argp += sizeof(void *);
    }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if ((sizeof(void *) - 1) & (size_t) argp)
	argp = (char *) ALIGN(argp, sizeof(void *));

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

  if (argp - stack > ecif->cif->bytes) 
    {
      Py_FatalError("FFI BUG: not enough stack space for arguments");
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
#ifdef _WIN64
    case FFI_TYPE_POINTER:
#endif
      cif->flags = FFI_TYPE_SINT64;
      break;

    default:
      cif->flags = FFI_TYPE_INT;
      break;
    }

  return FFI_OK;
}

#ifdef _WIN32
extern int
ffi_call_x86(void (*)(char *, extended_cif *), 
	     /*@out@*/ extended_cif *, 
	     unsigned, unsigned, 
	     /*@out@*/ unsigned *, 
	     void (*fn)());
#endif

#ifdef _WIN64
extern int
ffi_call_AMD64(void (*)(char *, extended_cif *),
		 /*@out@*/ extended_cif *,
		 unsigned, unsigned,
		 /*@out@*/ unsigned *,
		 void (*fn)());
#endif

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
#if !defined(_WIN64)
    case FFI_SYSV:
    case FFI_STDCALL:
      return ffi_call_x86(ffi_prep_args, &ecif, cif->bytes, 
			  cif->flags, ecif.rvalue, fn);
      break;
#else
    case FFI_SYSV:
      /*@-usedef@*/
      /* Function call needs at least 40 bytes stack size, on win64 AMD64 */
      return ffi_call_AMD64(ffi_prep_args, &ecif, cif->bytes ? cif->bytes : 40,
			   cif->flags, ecif.rvalue, fn);
      /*@=usedef@*/
      break;
#endif

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

#ifdef _WIN64
void *
#else
static void __fastcall
#endif
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

#if defined(_WIN32) && !defined(_WIN64)
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
#endif

#ifdef _WIN64
  /* The result is returned in rax.  This does the right thing for
     result types except for floats; we have to 'mov xmm0, rax' in the
     caller to correct this.
  */
  return *(void **)resp;
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
      if ((sizeof(char *) - 1) & (size_t) argp) {
	argp = (char *) ALIGN(argp, sizeof(char*));
      }

      z = (*p_arg)->size;

      /* because we're little endian, this is what it turns into.   */

      *p_argv = (void*) argp;

      p_argv++;
      argp += z;
    }
  
  return;
}

/* the cif must already be prep'ed */
extern void ffi_closure_OUTER();

ffi_status
ffi_prep_closure (ffi_closure* closure,
		  ffi_cif* cif,
		  void (*fun)(ffi_cif*,void*,void**,void*),
		  void *user_data)
{
  short bytes;
  char *tramp;
#ifdef _WIN64
  int mask = 0;
#endif
  FFI_ASSERT (cif->abi == FFI_SYSV);
  
  if (cif->abi == FFI_SYSV)
    bytes = 0;
#if !defined(_WIN64)
  else if (cif->abi == FFI_STDCALL)
    bytes = cif->bytes;
#endif
  else
    return FFI_BAD_ABI;

  tramp = &closure->tramp[0];

#define BYTES(text) memcpy(tramp, text, sizeof(text)), tramp += sizeof(text)-1
#define POINTER(x) *(void**)tramp = (void*)(x), tramp += sizeof(void*)
#define SHORT(x) *(short*)tramp = x, tramp += sizeof(short)
#define INT(x) *(int*)tramp = x, tramp += sizeof(int)

#ifdef _WIN64
  if (cif->nargs >= 1 &&
      (cif->arg_types[0]->type == FFI_TYPE_FLOAT
       || cif->arg_types[0]->type == FFI_TYPE_DOUBLE))
    mask |= 1;
  if (cif->nargs >= 2 &&
      (cif->arg_types[1]->type == FFI_TYPE_FLOAT
       || cif->arg_types[1]->type == FFI_TYPE_DOUBLE))
    mask |= 2;
  if (cif->nargs >= 3 &&
      (cif->arg_types[2]->type == FFI_TYPE_FLOAT
       || cif->arg_types[2]->type == FFI_TYPE_DOUBLE))
    mask |= 4;
  if (cif->nargs >= 4 &&
      (cif->arg_types[3]->type == FFI_TYPE_FLOAT
       || cif->arg_types[3]->type == FFI_TYPE_DOUBLE))
    mask |= 8;

  /* 41 BB ----         mov         r11d,mask */
  BYTES("\x41\xBB"); INT(mask);

  /* 48 B8 --------     mov         rax, closure			*/
  BYTES("\x48\xB8"); POINTER(closure);

  /* 49 BA --------     mov         r10, ffi_closure_OUTER */
  BYTES("\x49\xBA"); POINTER(ffi_closure_OUTER);

  /* 41 FF E2           jmp         r10 */
  BYTES("\x41\xFF\xE2");

#else

  /* mov ecx, closure */
  BYTES("\xb9"); POINTER(closure);

  /* mov edx, esp */
  BYTES("\x8b\xd4");

  /* call ffi_closure_SYSV */
  BYTES("\xe8"); POINTER((char*)&ffi_closure_SYSV - (tramp + 4));

  /* ret bytes */
  BYTES("\xc2");
  SHORT(bytes);
  
#endif

  if (tramp - &closure->tramp[0] > FFI_TRAMPOLINE_SIZE)
    Py_FatalError("FFI_TRAMPOLINE_SIZE too small in " __FILE__);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}
