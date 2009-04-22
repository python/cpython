#ifndef STRINGLIB_STRINGDEFS_H
#define STRINGLIB_STRINGDEFS_H

/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     0

/* _tolower and _toupper are defined by SUSv2, but they're not ISO C */
/* This needs to be cleaned up. See issue 5793. */
#ifndef _tolower
#define _tolower tolower
#endif
#ifndef _toupper
#define _toupper toupper
#endif

#define STRINGLIB_OBJECT         PyStringObject
#define STRINGLIB_CHAR           char
#define STRINGLIB_TYPE_NAME      "string"
#define STRINGLIB_PARSE_CODE     "S"
#define STRINGLIB_EMPTY          nullstring
#define STRINGLIB_ISDECIMAL(x)   ((x >= '0') && (x <= '9'))
#define STRINGLIB_TODECIMAL(x)   (STRINGLIB_ISDECIMAL(x) ? (x - '0') : -1)
#define STRINGLIB_TOUPPER(x)     _toupper(Py_CHARMASK(x))
#define STRINGLIB_TOLOWER(x)     _tolower(Py_CHARMASK(x))
#define STRINGLIB_FILL           memset
#define STRINGLIB_STR            PyString_AS_STRING
#define STRINGLIB_LEN            PyString_GET_SIZE
#define STRINGLIB_NEW            PyString_FromStringAndSize
#define STRINGLIB_RESIZE         _PyString_Resize
#define STRINGLIB_CHECK          PyString_Check
#define STRINGLIB_CMP            memcmp
#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_GROUPING       _PyString_InsertThousandsGrouping
#define STRINGLIB_GROUPING_LOCALE _PyString_InsertThousandsGroupingLocale

#endif /* !STRINGLIB_STRINGDEFS_H */
