/* implements the unicode (as opposed to string) version of the
   built-in formatters for string, int, float.  that is, the versions
   of int.__float__, etc., that take and return unicode objects */

#include "Python.h"
#include "formatter_unicode.h"

#include "../Objects/stringlib/unicodedefs.h"

#define FORMAT_STRING unicode_unicode__format__
#define FORMAT_LONG   unicode_long__format__
#define FORMAT_FLOAT  unicode_float__format__
#include "../Objects/stringlib/formatter.h"
