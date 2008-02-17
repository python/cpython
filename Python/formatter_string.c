/***********************************************************************/
/* Implements the string (as opposed to unicode) version of the
   built-in formatters for string, int, float.  That is, the versions
   of int.__float__, etc., that take and return string objects */

#include "Python.h"
#include "formatter_string.h"

#include "../Objects/stringlib/stringdefs.h"

#define FORMAT_STRING string__format__
#define FORMAT_LONG   string_long__format__
#define FORMAT_INT    string_int__format__
#define FORMAT_FLOAT  string_float__format__
#include "../Objects/stringlib/formatter.h"
