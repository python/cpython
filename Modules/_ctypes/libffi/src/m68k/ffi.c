/* -----------------------------------------------------------------------
   ffi.c
   
   m68k Foreign Function Interface 
   ----------------------------------------------------------------------- */

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <asm/cachectl.h>

void ffi_call_SYSV (extended_cif *,
		    unsigned, unsigned,
		    void *, void (*fn) ());
void *ffi_prep_args (void *stack, extended_cif *ecif);
void ffi_closure_SYSV (ffi_closure *);
void ffi_closure_struct_SYSV (ffi_closure *);
unsigned int ffi_closure_SYSV_inner (ffi_closure *closure,
				     void *resp, void *args);

/* ffi_prep_args is called by the assembly routine once stack space has
   been allocated for the function's arguments.  */

void *
ffi_prep_args (void *stack, extended_cif *ecif)
{
  unsigned int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;
  void *struct_value_ptr;

  argp = stack;

  if (ecif->cif->rtype->type == FFI_TYPE_STRUCT
      && !ecif->cif->flags)
    struct_value_ptr = ecif->rvalue;
  else
    struct_value_ptr = NULL;

  p_argv = ecif->avalue;

  for (i = ecif->cif->nargs, p_arg = ecif->cif->arg_types;
       i != 0;
       i--, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z < sizeof (int))
	{
	  switch ((*p_arg)->type)
	    {
	    case FFI_TYPE_SINT8:
	      *(signed int *) argp = (signed int) *(SINT8 *) *p_argv;
	      break;

	    case FFI_TYPE_UINT8:
	      *(unsigned int *) argp = (unsigned int) *(UINT8 *) *p_argv;
	      break;

	    case FFI_TYPE_SINT16:
	      *(signed int *) argp = (signed int) *(SINT16 *) *p_argv;
	      break;

	    case FFI_TYPE_UINT16:
	      *(unsigned int *) argp = (unsigned int) *(UINT16 *) *p_argv;
	      break;

	    case FFI_TYPE_STRUCT:
	      memcpy (argp + sizeof (int) - z, *p_argv, z);
	      break;

	    default:
	      FFI_ASSERT (0);
	    }
	  z = sizeof (int);
	}
      else
	{
	  memcpy (argp, *p_argv, z);

	  /* Align if necessary.  */
	  if ((sizeof(int) - 1) & z)
	    z = ALIGN(z, sizeof(int));
	}

      p_argv++;
      argp += z;
    }

  return struct_value_ptr;
}

#define CIF_FLAGS_INT		1
#define CIF_FLAGS_DINT		2
#define CIF_FLAGS_FLOAT		4
#define CIF_FLAGS_DOUBLE	8
#define CIF_FLAGS_LDOUBLE	16
#define CIF_FLAGS_POINTER	32
#define CIF_FLAGS_STRUCT1	64
#define CIF_FLAGS_STRUCT2	128

/* Perform machine dependent cif processing */
ffi_status
ffi_prep_cif_machdep (ffi_cif *cif)
{
  /* Set the return type flag */
  switch (cif->rtype->type)
    {
    case FFI_TYPE_VOID:
      cif->flags = 0;
      break;

    case FFI_TYPE_STRUCT:
      switch (cif->rtype->size)
	{
	case 1:
	  cif->flags = CIF_FLAGS_STRUCT1;
	  break;
	case 2:
	  cif->flags = CIF_FLAGS_STRUCT2;
	  break;
	case 4:
	  cif->flags = CIF_FLAGS_INT;
	  break;
	case 8:
	  cif->flags = CIF_FLAGS_DINT;
	  break;
	default:
	  cif->flags = 0;
	  break;
	}
      break;

    case FFI_TYPE_FLOAT:
      cif->flags = CIF_FLAGS_FLOAT;
      break;

    case FFI_TYPE_DOUBLE:
      cif->flags = CIF_FLAGS_DOUBLE;
      break;

    case FFI_TYPE_LONGDOUBLE:
      cif->flags = CIF_FLAGS_LDOUBLE;
      break;

    case FFI_TYPE_POINTER:
      cif->flags = CIF_FLAGS_POINTER;
      break;

    case FFI_TYPE_SINT64:
    case FFI_TYPE_UINT64:
      cif->flags = CIF_FLAGS_DINT;
      break;

    default:
      cif->flags = CIF_FLAGS_INT;
      break;
    }

  return FFI_OK;
}

void
ffi_call (ffi_cif *cif, void (*fn) (), void *rvalue, void **avalue)
{
  extended_cif ecif;

  ecif.cif = cif;
  ecif.avalue = avalue;

  /* If the return value is a struct and we don't have a return value
     address then we need to make one.  */

  if (rvalue == NULL
      && cif->rtype->type == FFI_TYPE_STRUCT
      && cif->rtype->size > 8)
    ecif.rvalue = alloca (cif->rtype->size);
  else
    ecif.rvalue = rvalue;

  switch (cif->abi)
    {
    case FFI_SYSV:
      ffi_call_SYSV (&ecif, cif->bytes, cif->flags,
		     ecif.rvalue, fn);
      break;

    default:
      FFI_ASSERT (0);
      break;
    }
}

static void
ffi_prep_incoming_args_SYSV (char *stack, void **avalue, ffi_cif *cif)
{
  unsigned int i;
  void **p_argv;
  char *argp;
  ffi_type **p_arg;

  argp = stack;
  p_argv = avalue;

  for (i = cif->nargs, p_arg = cif->arg_types; (i != 0); i--, p_arg++)
    {
      size_t z;

      z = (*p_arg)->size;
      if (z <= 4)
	{
	  *p_argv = (void *) (argp + 4 - z);

	  z = 4;
	}
      else
	{
	  *p_argv = (void *) argp;

	  /* Align if necessary */
	  if ((sizeof(int) - 1) & z)
	    z = ALIGN(z, sizeof(int));
	}

      p_argv++;
      argp += z;
    }
}

unsigned int
ffi_closure_SYSV_inner (ffi_closure *closure, void *resp, void *args)
{
  ffi_cif *cif;
  void **arg_area;

  cif = closure->cif;
  arg_area = (void**) alloca (cif->nargs * sizeof (void *));

  ffi_prep_incoming_args_SYSV(args, arg_area, cif);

  (closure->fun) (cif, resp, arg_area, closure->user_data);

  return cif->flags;
}

ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
		      ffi_cif* cif,
		      void (*fun)(ffi_cif*,void*,void**,void*),
		      void *user_data,
		      void *codeloc)
{
  FFI_ASSERT (cif->abi == FFI_SYSV);

  *(unsigned short *)closure->tramp = 0x207c;
  *(void **)(closure->tramp + 2) = codeloc;
  *(unsigned short *)(closure->tramp + 6) = 0x4ef9;
  if (cif->rtype->type == FFI_TYPE_STRUCT
      && !cif->flags)
    *(void **)(closure->tramp + 8) = ffi_closure_struct_SYSV;
  else
    *(void **)(closure->tramp + 8) = ffi_closure_SYSV;

  syscall(SYS_cacheflush, codeloc, FLUSH_SCOPE_LINE,
	  FLUSH_CACHE_BOTH, FFI_TRAMPOLINE_SIZE);

  closure->cif  = cif;
  closure->user_data = user_data;
  closure->fun  = fun;

  return FFI_OK;
}

