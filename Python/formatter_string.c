/***********************************************************************/
/* Implements the string (as opposed to unicode) version of the
   built-in formatters for string, int, float.  That is, the versions
   of int.__float__, etc., that take and return string objects */

#include "Python.h"
#include "../Objects/stringlib/stringdefs.h"

#define FORMAT_STRING _PyBytes_FormatAdvanced
#define FORMAT_LONG   _PyLong_FormatAdvanced
#define FORMAT_INT    _PyInt_FormatAdvanced
#define FORMAT_FLOAT  _PyFloat_FormatAdvanced

#include "../Objects/stringlib/formatter.h"
