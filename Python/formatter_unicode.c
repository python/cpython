/* implements the unicode (as opposed to string) version of the
   built-in formatters for string, int, float.  that is, the versions
   of int.__float__, etc., that take and return unicode objects */

#include "Python.h"
#include "../Objects/stringlib/unicodedefs.h"


#define FORMAT_STRING  _PyUnicode_FormatAdvanced
#define FORMAT_LONG    _PyLong_FormatAdvanced
#define FORMAT_FLOAT   _PyFloat_FormatAdvanced
#define FORMAT_COMPLEX _PyComplex_FormatAdvanced

#include "../Objects/stringlib/formatter.h"
