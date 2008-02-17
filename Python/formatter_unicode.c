/* Implements the unicode (as opposed to string) version of the
   built-in formatter for unicode.  That is, unicode.__format__(). */

#include "Python.h"
#include "formatter_unicode.h"

#include "../Objects/stringlib/unicodedefs.h"

#define FORMAT_STRING unicode__format__
/* don't define FORMAT_LONG and FORMAT_FLOAT, since we can live
   with only the string versions of those.  The builtin format()
   will convert them to unicode. */
#include "../Objects/stringlib/formatter.h"
