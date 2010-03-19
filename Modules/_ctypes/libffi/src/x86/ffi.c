/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 1996, 1998, 1999, 2001, 2007, 2008  Red Hat, Inc.
           Copyright (c) 2002  Ranjit Mathew
           Copyright (c) 2002  Bo Thorsen
           Copyright (c) 2002  Roger Sayle
           Copyright (C) 2008  Free Software Foundation, Inc.

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

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#if !defined(__x86_64__) || defined(_WIN64)

#ifdef _WIN64
#include <windows.h>
#endif

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

  if (ecif->cif->flags == FFI_TYPE_STRUCT
#ifdef X86_WIN64
      && (ecif->cif->rtype->size != 1 && ecif->cif->rtype->size != 2
          && ecif->cif->rtype->size != 4 && ecif->cif->rtype->size != 8)
#endif
      )
    {
      *(void **) argp = ecif->rvalue;
      argp += sizeof(void*);
    }

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if ((sizeof(void*) - 1) & (size_t) argp)
        argp = (char *) ALIGN(argp, sizeof(void*));

      z = (*p_arg)->size;
#ifdef X86_WIN64
      if (z > sizeof(ffi_arg)
          || ((*p_arg)->type == FFI_TYPE_STRUCT
              && (z != 1 && z != 2 && z != 4 && z != 8))
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
          || ((*p_arg)->type == FFI_TYPE_LONGDOUBLE)
#endif
          )
        {
          z = sizeof(ffi_arg);
          *(void **)argp = *p_argv;
        }
      else if ((*p_arg)->type == FFI_TYPE_FLOAT)
        {
          memcpy(argp, *p_argv, z);
        }
      else
#endif
      if (z < sizeof(ffi_arg))
        {
          z = sizeof(ffi_arg);
          switch ((*p_arg)->type)
            {
            case FFI_TYPE_SINT8:
              *(ffi_sarg *) argp = (ffi_sarg)*(SINT8 *)(* p_argv);
              break;

            case FFI_TYPE_UINT8:
              *(ffi_arg *) argp = (ffi_arg)*(UINT8 *)(* p_argv);
              break;

            case FFI_TYPE_SINT16:
              *(ffi_sarg *) argp = (ffi_sarg)*(SINT16 *)(* p_argv);
              break;

            case FFI_TYPE_UINT16:
              *(ffi_arg *) argp = (ffi_arg)*(UINT16 *)(* p_argv);
              break;

            case FFI_TYPE_SINT32:
              *(ffi_sarg *) argp = (ffi_sarg)*(SINT32 *)(* p_argv);
              break;

            case FFI_TYPE_UINT32:
              *(ffi_arg *) argp = (ffi_arg)*(UINT32 *)(* p_argv);
              break;

            case FFI_TYPE_STRUCT:
              *(ffi_arg *) argp = *(ffi_arg *)(* p_argv);
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
#ifdef X86_WIN64
      argp += (z + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
#else
      argp += z;
#endif
    }
  
  return;
}

/* Perform machine dependent cif processing */
ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
  unsigned int i;
  ffi_type **ptr;

  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
#if defined(X86) || defined (X86_WIN32) || defined(X86_FREEBSD) || defined(X86_DARWIN) || defined(X86_WIN64)
    case FFI_TYPE_UINT8:
    case FFI_TYPE_UINT16:
    case FFI_TYPE_SINT8:
    case FFI_TYPE_SINT16:
#endif
#ifdef X86_WIN64
    case FFI_TYPE_UINT32:
    case FFI_TYPE_SINT32:
#endif
    case FFI_TYPE_SINT64:
    case FFI_TYPE_FLOAT:
    case FFI_TYPE_DOUBLE:
#ifndef X86_WIN64
#if FFI_TYPE_DOUBLE != FFI_TYPE_LONGDOUBLE
    case FFI_TYPE_LONGDOUBLE:
#endif
#endif
      cif->flags = (unsigned) cif->rtype->type;
      break;

    case FFI_TYPE_UINT64:
#ifdef X86_WIN64
    case FFI_TYPE_POINTER:
#endif
      cif->flags = FFI_TYPE_SINT64;
      break;

    case FFI_TYPE_STRUCT:
#ifndef X86
      if (cif->rtype->size == 1)
        {
          cif->flags = FFI_TYPE_SMALL_STRUCT_1B; /* same as char size */
        }
      else if (cif->rtype->size == 2)
        {
          cif->flags = FFI_TYPE_SMALL_STRUCT_2B; /* same as short size */
        }
      else if (cif->rtype->size == 4)
        {
#ifdef X86_WIN64
          cif->flags = FFI_TYPE_SMALL_STRUCT_4B;
#else
          cif->flags = FFI_TYPE_INT; /* same as int type */
#endif
        }
      else if (cif->rtype->size == 8)
        {
          cif->flags = FFI_TYPE_SINT64; /* same as int64 type */
        }
      else
#endif
        {
          cif->flags = FFI_TYPE_STRUCT;
          // allocate space for return value pointer
          cif->bytes += ALIGN(sizeof(void*), FFI_SIZEOF_ARG);
        }
      break;

    default:
#ifdef X86_WIN64
      cif->flags = FFI_TYPE_SINT64;
      break;
    case FFI_TYPE_INT:
      cif->flags = FFI_TYPE_SINT32;
#else
      cif->flags = FFI_TYPE_INT;
#endif
      break;
    }

  for (ptr = cif->arg_types, i = cif->nargs; i > 0; i--, ptr++)
    {
      if (((*ptr)->alignment - 1) & cif->bytes)
        cif->bytes = ALIGN(cif->bytes, (*ptr)->alignment);
      cif->bytes += ALIGN((*ptr)->size, FFI_SIZEOF_ARG);
    }

#ifdef X86_WIN64
  // ensure space for storing four registers
  cif->bytes += 4 * sizeof(ffi_arg);
#endif

#ifdef X86_DARWIN
  cif->bytes = (cif->bytes + 15) & ~0xF;
#endif

  return FFI_OK;
}

#ifdef X86_WIN64
extern int
ffi_call_win64(void (*)(char *, extended_cif *), extended_cif *,
               unsigned, unsigned, unsigned *, void (*fn)(void));
#elif defined(X86_WIN32)
extern void
ffi_call_win32(void (*)(char *, extended_cif *), extended_cif *,
               unsigned, unsigned, unsigned *, void (*fn)(void));
#else
extern void ffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *,
                          unsigned, unsigned, unsigned *, void (*fn)(void));
#endif

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return */
  /* value address then we need to make one                     */

#ifdef X86_WIN64
  if (rvalue == NULL
      && cif->flags == FFI_TYPE_STRUCT
      && cif->rtype->size != 1 && cif->rtype->size != 2
      && cif->rtype->size != 4 && cif->rtype->size != 8)
    {
      ecif.rvalue = alloca((cif->rtype->size + 0xF) & ~0xF);
    }
#else
  if (rvalue == NULL
      && cif->flags == FFI_TYPE_STRUCT)
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
#endif
  else
    ecif.rvalue = rvalue;
    
  
  switch (cif->abi) 
    {
#ifdef X86_WIN64
    case FFI_WIN64:
      {
        // Make copies of all struct arguments
        // NOTE: not sure if responsibility should be here or in caller
        unsigned int i;
        for (i=0; i < cif->nargs;i++) {
          size_t size = cif->arg_types[i]->size;
          if ((cif->arg_types[i]->type == FFI_TYPE_STRUCT
               && (size != 1 && size != 2 && size != 4 && size != 8))
#if FFI_TYPE_LONGDOUBLE != FFI_TYPE_DOUBLE
              || cif->arg_types[i]->type == FFI_TYPE_LONGDOUBLE
#endif
              )
            {
              void *local = alloca(size);
              memcpy(local, avalue[i], size);
              avalue[i] = local;
            }
        }
        ffi_call_win64(ffi_prep_args, &ecif, cif->bytes,
                       cif->flags, ecif.rvalue, fn);
      }
      break;
#elif defined(X86_WIN32)
    case FFI_SYSV:
    case FFI_STDCALL:
      ffi_call_win32(ffi_prep_args, &ecif, cif->bytes, cif->flags,
                     ecif.rvalue, fn);
      break;
#else
    case FFI_SYSV:
      ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue,
                    fn);
      break;
#endif
    default:
      FFI_ASSERT(0);
      break;
    }
}


/** private members **/

/* The following __attribute__((regparm(1))) decorations will have no effect
   on MSVC - standard cdecl convention applies. */
static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
                                         void** args, ffi_cif* cif);
void FFI_HIDDEN ffi_closure_SYSV (ffi_closure *)
     __attribute__ ((regparm(1)));
unsigned int FFI_HIDDEN ffi_closure_SYSV_inner (ffi_closure *, void **, void *)
     __attribute__ ((regparm(1)));
void FFI_HIDDEN ffi_closure_raw_SYSV (ffi_raw_closure *)
     __attribute__ ((regparm(1)));
#ifdef X86_WIN32
void FFI_HIDDEN ffi_closure_STDCALL (ffi_closure *)
     __attribute__ ((regparm(1)));
#endif
#ifdef X86_WIN64
void FFI_HIDDEN ffi_closure_win64 (ffi_closure *);
#endif

/* This function is jumped to by the trampoline */

#ifdef X86_WIN64
void * FFI_HIDDEN
ffi_closure_win64_inner (ffi_closure *closure, void *args) {
  ffi_cif       *cif;
  void         **arg_area;
  void          *result;
  void          *resp = &result;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  /* this call will initialize ARG_AREA, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will change RESP to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(args, &resp, arg_area, cif);
  
  (closure->fun) (cif, resp, arg_area, closure->user_data);

  /* The result is returned in rax.  This does the right thing for
     result types except for floats; we have to 'mov xmm0, rax' in the
     caller to correct this.
     TODO: structure sizes of 3 5 6 7 are returned by reference, too!!!
  */
  return cif->rtype->size > sizeof(void *) ? resp : *(void **)resp;
}

#else
unsigned int FFI_HIDDEN __attribute__ ((regparm(1)))
ffi_closure_SYSV_inner (ffi_closure *closure, void **respp, void *args)
{
  /* our various things...  */
  ffi_cif       *cif;
  void         **arg_area;

  cif         = closure->cif;
  arg_area    = (void**) alloca (cif->nargs * sizeof (void*));  

  /* this call will initialize ARG_AREA, such that each
   * element in that array points to the corresponding 
   * value on the stack; and if the function returns
   * a structure, it will change RESP to point to the
   * structure return address.  */

  ffi_prep_incoming_args_SYSV(args, respp, arg_area, cif);

  (closure->fun) (cif, *respp, arg_area, closure->user_data);

  return cif->flags;
}
#endif /* !X86_WIN64 */

static void
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue, void **avalue,
                            ffi_cif *cif)
{
  register unsigned int i;
  register void **p_argv;
  register char *argp;
  register ffi_type **p_arg;

  argp = stack;

#ifdef X86_WIN64
  if (cif->rtype->size > sizeof(ffi_arg)
      || (cif->flags == FFI_TYPE_STRUCT
          && (cif->rtype->size != 1 && cif->rtype->size != 2
              && cif->rtype->size != 4 && cif->rtype->size != 8))) {
    *rvalue = *(void **) argp;
    argp += sizeof(void *);
  }
#else
  if ( cif->flags == FFI_TYPE_STRUCT ) {
    *rvalue = *(void **) argp;
    argp += sizeof(void *);
  }
#endif

  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      /* Align if necessary */
      if ((sizeof(void*) - 1) & (size_t) argp) {
        argp = (char *) ALIGN(argp, sizeof(void*));
      }

#ifdef X86_WIN64
      if ((*p_arg)->size > sizeof(ffi_arg)
          || ((*p_arg)->type == FFI_TYPE_STRUCT
              && ((*p_arg)->size != 1 && (*p_arg)->size != 2
                  && (*p_arg)->size != 4 && (*p_arg)->size != 8)))
        {
          z = sizeof(void *);
          *p_argv = *(void **)argp;
        }
      else
#endif
        {
          z = (*p_arg)->size;
          
          /* because we're little endian, this is what it turns into.   */
          
          *p_argv = (void*) argp;
        }
          
      p_argv++;
#ifdef X86_WIN64
      argp += (z + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
#else
      argp += z;
#endif
    }
  
  return;
}

#define FFI_INIT_TRAMPOLINE_WIN64(TRAMP,FUN,CTX,MASK) \
{ unsigned char *__tramp = (unsigned char*)(TRAMP); \
   void*  __fun = (void*)(FUN); \
   void*  __ctx = (void*)(CTX); \
   *(unsigned char*) &__tramp[0] = 0x41; \
   *(unsigned char*) &__tramp[1] = 0xbb; \
   *(unsigned int*) &__tramp[2] = MASK; /* mov $mask, %r11 */ \
   *(unsigned char*) &__tramp[6] = 0x48; \
   *(unsigned char*) &__tramp[7] = 0xb8; \
   *(void**) &__tramp[8] = __ctx; /* mov __ctx, %rax */ \
   *(unsigned char *)  &__tramp[16] = 0x49; \
   *(unsigned char *)  &__tramp[17] = 0xba; \
   *(void**) &__tramp[18] = __fun; /* mov __fun, %r10 */ \
   *(unsigned char *)  &__tramp[26] = 0x41; \
   *(unsigned char *)  &__tramp[27] = 0xff; \
   *(unsigned char *)  &__tramp[28] = 0xe2; /* jmp %r10 */ \
 }

/* How to make a trampoline.  Derived from gcc/config/i386/i386.c. */

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX) \
{ unsigned char *__tramp = (unsigned char*)(TRAMP); \
   unsigned int  __fun = (unsigned int)(FUN); \
   unsigned int  __ctx = (unsigned int)(CTX); \
   unsigned int  __dis = __fun - (__ctx + 10);  \
   *(unsigned char*) &__tramp[0] = 0xb8; \
   *(unsigned int*)  &__tramp[1] = __ctx; /* movl __ctx, %eax */ \
   *(unsigned char *)  &__tramp[5] = 0xe9; \
   *(unsigned int*)  &__tramp[6] = __dis; /* jmp __fun  */ \
 }

#define FFI_INIT_TRAMPOLINE_STDCALL(TRAMP,FUN,CTX,SIZE)  \
{ unsigned char *__tramp = (unsigned char*)(TRAMP); \
   unsigned int  __fun = (unsigned int)(FUN); \
   unsigned int  __ctx = (unsigned int)(CTX); \
   unsigned int  __dis = __fun - (__ctx + 10); \
   unsigned short __size = (unsigned short)(SIZE); \
   *(unsigned char*) &__tramp[0] = 0xb8; \
   *(unsigned int*)  &__tramp[1] = __ctx; /* movl __ctx, %eax */ \
   *(unsigned char *)  &__tramp[5] = 0xe8; \
   *(unsigned int*)  &__tramp[6] = __dis; /* call __fun  */ \
   *(unsigned char *)  &__tramp[10] = 0xc2; \
   *(unsigned short*)  &__tramp[11] = __size; /* ret __size  */ \
 }

/* the cif must already be prep'ed */

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
                      ffi_cif* cif,
                      void (*fun)(ffi_cif*,void*,void**,void*),
                      void *user_data,
                      void *codeloc)
{
#ifdef X86_WIN64
#define ISFLOAT(IDX) (cif->arg_types[IDX]->type == FFI_TYPE_FLOAT || cif->arg_types[IDX]->type == FFI_TYPE_DOUBLE)
#define FLAG(IDX) (cif->nargs>(IDX)&&ISFLOAT(IDX)?(1<<(IDX)):0)
  if (cif->abi == FFI_WIN64) 
    {
      int mask = FLAG(0)|FLAG(1)|FLAG(2)|FLAG(3);
      FFI_INIT_TRAMPOLINE_WIN64 (&closure->tramp[0],
                                 &ffi_closure_win64,
                                 codeloc, mask);
      /* make sure we can execute here */
    }
#else
  if (cif->abi == FFI_SYSV)
    {
      FFI_INIT_TRAMPOLINE (&closure->tramp[0],
                           &ffi_closure_SYSV,
                           (void*)codeloc);
    }
#ifdef X86_WIN32
  else if (cif->abi == FFI_STDCALL)
    {
      FFI_INIT_TRAMPOLINE_STDCALL (&closure->tramp[0],
                                   &ffi_closure_STDCALL,
                                   (void*)codeloc, cif->bytes);
    }
#endif /* X86_WIN32 */
#endif /* !X86_WIN64 */
  else
    {
      return FFI_BAD_ABI;
    }
    
  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

/* ------- Native raw API support -------------------------------- */

#if !FFI_NO_RAW_API

ffi_status
ffi_prep_raw_closure_loc (ffi_raw_closure* closure,
                          ffi_cif* cif,
                          void (*fun)(ffi_cif*,void*,ffi_raw*,void*),
                          void *user_data,
                          void *codeloc)
{
  int i;

  if (cif->abi != FFI_SYSV) {
    return FFI_BAD_ABI;
  }

  /* we currently don't support certain kinds of arguments for raw
     closures.  This should be implemented by a separate assembly
     language routine, since it would require argument processing,
     something we don't do now for performance.  */

  for (i = cif->nargs-1; i >= 0; i--)
    {
      FFI_ASSERT (cif->arg_types[i]->type != FFI_TYPE_STRUCT);
      FFI_ASSERT (cif->arg_types[i]->type != FFI_TYPE_LONGDOUBLE);
    }
  

  FFI_INIT_TRAMPOLINE (&closure->tramp[0], &ffi_closure_raw_SYSV,
                       codeloc);
    
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

void
ffi_raw_call(ffi_cif *cif, void (*fn)(void), void *rvalue, ffi_raw *fake_avalue)
{
  extended_cif ecif;
  void **avalue = (void **)fake_avalue;

  ecif.cif = cif;
  ecif.avalue = avalue;
  
  /* If the return value is a struct and we don't have a return */
  /* value address then we need to make one                     */

  if ((rvalue == NULL) && 
      (cif->rtype->type == FFI_TYPE_STRUCT))
    {
      ecif.rvalue = alloca(cif->rtype->size);
    }
  else
    ecif.rvalue = rvalue;
    
  
  switch (cif->abi) 
    {
#ifdef X86_WIN32
    case FFI_SYSV:
    case FFI_STDCALL:
      ffi_call_win32(ffi_prep_args_raw, &ecif, cif->bytes, cif->flags,
                     ecif.rvalue, fn);
      break;
#else
    case FFI_SYSV:
      ffi_call_SYSV(ffi_prep_args_raw, &ecif, cif->bytes, cif->flags,
                    ecif.rvalue, fn);
      break;
#endif
    default:
      FFI_ASSERT(0);
      break;
    }
}

#endif

#endif /* !__x86_64__  || X86_WIN64 */

