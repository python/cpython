#ifdef __i386__
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

void ffi_prep_args(char *stack, extended_cif *ecif);

void ffi_prep_args(char *stack, extended_cif *ecif)
{
    register unsigned int i;
    register void **p_argv;
    register char *argp;
    register ffi_type **p_arg;

    argp = stack;

    if (ecif->cif->flags == FFI_TYPE_STRUCT)
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
#ifdef X86
        case FFI_TYPE_STRUCT:
        case FFI_TYPE_UINT8:
        case FFI_TYPE_UINT16:
        case FFI_TYPE_SINT8:
        case FFI_TYPE_SINT16:
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

#ifndef X86
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

#ifdef X86_DARWIN
    cif->bytes = (cif->bytes + 15) & ~0xF;
#endif

    return FFI_OK;
}

extern void ffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *,
                          unsigned, unsigned, unsigned *, void (*fn)());

#ifdef X86_WIN32
extern void ffi_call_STDCALL(void (*)(char *, extended_cif *), extended_cif *,
                             unsigned, unsigned, unsigned *, void (*fn)());

#endif /* X86_WIN32 */

void ffi_call(ffi_cif *cif, void (*fn)(), void *rvalue, void **avalue)
{
    extended_cif ecif;

    ecif.cif = cif;
    ecif.avalue = avalue;

    /* If the return value is a struct and we don't have a return	*/
    /* value address then we need to make one		        */

    if ((rvalue == NULL) &&
        (cif->flags == FFI_TYPE_STRUCT))
    {
        ecif.rvalue = alloca(cif->rtype->size);
    }
    else
        ecif.rvalue = rvalue;


    switch (cif->abi)
    {
        case FFI_SYSV:
            ffi_call_SYSV(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue,
                          fn);
            break;
#ifdef X86_WIN32
        case FFI_STDCALL:
            ffi_call_STDCALL(ffi_prep_args, &ecif, cif->bytes, cif->flags,
                             ecif.rvalue, fn);
            break;
#endif /* X86_WIN32 */
        default:
            FFI_ASSERT(0);
            break;
    }
}


/** private members **/

static void ffi_prep_incoming_args_SYSV (char *stack, void **ret,
                                         void** args, ffi_cif* cif);
void FFI_HIDDEN ffi_closure_SYSV (ffi_closure *)
__attribute__ ((regparm(1)));
unsigned int FFI_HIDDEN ffi_closure_SYSV_inner (ffi_closure *, void **, void *)
__attribute__ ((regparm(1)));
void FFI_HIDDEN ffi_closure_raw_SYSV (ffi_raw_closure *)
__attribute__ ((regparm(1)));

/* This function is jumped to by the trampoline */

unsigned int FFI_HIDDEN
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

static void
ffi_prep_incoming_args_SYSV(char *stack, void **rvalue, void **avalue,
                            ffi_cif *cif)
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

#define FFI_INIT_TRAMPOLINE(TRAMP,FUN,CTX) \
({ unsigned char *__tramp = (unsigned char*)(TRAMP); \
unsigned int  __fun = (unsigned int)(FUN); \
unsigned int  __ctx = (unsigned int)(CTX); \
unsigned int  __dis = __fun - (__ctx + FFI_TRAMPOLINE_SIZE); \
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
	if (cif->abi != FFI_SYSV)
		return FFI_BAD_ABI;

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

ffi_status
ffi_prep_raw_closure_loc (ffi_raw_closure* closure,
                          ffi_cif* cif,
                          void (*fun)(ffi_cif*,void*,ffi_raw*,void*),
                          void *user_data,
                          void *codeloc)
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

extern void
ffi_call_SYSV(void (*)(char *, extended_cif *), extended_cif *, unsigned,
              unsigned, unsigned *, void (*fn)());

#ifdef X86_WIN32
extern void
ffi_call_STDCALL(void (*)(char *, extended_cif *), extended_cif *, unsigned,
                 unsigned, unsigned *, void (*fn)());
#endif /* X86_WIN32 */

void
ffi_raw_call(ffi_cif *cif, void (*fn)(), void *rvalue, ffi_raw *fake_avalue)
{
    extended_cif ecif;
    void **avalue = (void **)fake_avalue;

    ecif.cif = cif;
    ecif.avalue = avalue;

    /* If the return value is a struct and we don't have a return	*/
    /* value address then we need to make one		        */

    if ((rvalue == NULL) &&
        (cif->rtype->type == FFI_TYPE_STRUCT))
    {
        ecif.rvalue = alloca(cif->rtype->size);
    }
    else
        ecif.rvalue = rvalue;


    switch (cif->abi)
    {
        case FFI_SYSV:
            ffi_call_SYSV(ffi_prep_args_raw, &ecif, cif->bytes, cif->flags,
                          ecif.rvalue, fn);
            break;
#ifdef X86_WIN32
        case FFI_STDCALL:
            ffi_call_STDCALL(ffi_prep_args_raw, &ecif, cif->bytes, cif->flags,
                             ecif.rvalue, fn);
            break;
#endif /* X86_WIN32 */
        default:
            FFI_ASSERT(0);
            break;
    }
}

#endif
#endif	// __i386__
