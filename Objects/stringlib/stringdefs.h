#ifndef STRINGLIB_STRINGDEFS_H
#define STRINGLIB_STRINGDEFS_H

/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     0

#define STRINGLIB_OBJECT         PyBytesObject
#define STRINGLIB_CHAR           char
#define STRINGLIB_TYPE_NAME      "string"
#define STRINGLIB_PARSE_CODE     "S"
#define STRINGLIB_EMPTY          nullstring
#define STRINGLIB_ISSPACE        Py_ISSPACE
#define STRINGLIB_ISLINEBREAK(x) ((x == '\n') || (x == '\r'))
#define STRINGLIB_ISDECIMAL(x)   ((x >= '0') && (x <= '9'))
#define STRINGLIB_TODECIMAL(x)   (STRINGLIB_ISDECIMAL(x) ? (x - '0') : -1)
#define STRINGLIB_TOUPPER        Py_TOUPPER
#define STRINGLIB_TOLOWER        Py_TOLOWER
#define STRINGLIB_FILL           memset
#define STRINGLIB_STR            PyBytes_AS_STRING
#define STRINGLIB_LEN            PyBytes_GET_SIZE
#define STRINGLIB_NEW            PyBytes_FromStringAndSize
#define STRINGLIB_RESIZE         _PyBytes_Resize
#define STRINGLIB_CHECK          PyBytes_Check
#define STRINGLIB_CHECK_EXACT    PyBytes_CheckExact
#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_GROUPING       _PyBytes_InsertThousandsGrouping
#define STRINGLIB_GROUPING_LOCALE _PyBytes_InsertThousandsGroupingLocale
#define STRINGLIB_TOASCII        PyObject_Repr
#endif /* !STRINGLIB_STRINGDEFS_H */
