
/* This module provides the necessary stubs for when dynamic loading is
   not present. */

#include "Python.h"
#include "pycore_importdl.h"


const char *_PyImport_DynLoadFiletab[] = {NULL};
