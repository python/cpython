
/* This module provides the necessary stubs for when dynamic loading is
   not present. */

#include "Python.h"
#include "importdl.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{0, 0}
};
