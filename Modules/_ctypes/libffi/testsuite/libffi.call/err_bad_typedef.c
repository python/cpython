/* Area:		ffi_prep_cif
   Purpose:		Test error return for bad typedefs.
   Limitations:	none.
   PR:			none.
   Originator:	Blake Chaffin 6/6/2007	 */

/* { dg-do run { xfail *-*-* } } */
#include "ffitest.h"

int main (void)
{
	ffi_cif cif;
	ffi_type* arg_types[1];

	arg_types[0] = NULL;

	ffi_type	badType	= ffi_type_void;

	badType.size = 0;

	CHECK(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, &badType,
		arg_types) == FFI_BAD_TYPEDEF);

	exit(0);
}
