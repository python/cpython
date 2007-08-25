#ifndef STRINGLIB_STRINGDEFS_H
#define STRINGLIB_STRINGDEFS_H

/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     0

#define STRINGLIB_CHAR           char
#define STRINGLIB_TYPE_NAME      "string"
#define STRINGLIB_EMPTY          string_empty
#define STRINGLIB_ISDECIMAL(x)   ((x >= '0') && (x <= '9'))
#define STRINGLIB_TODECIMAL(x)   (STRINGLIB_ISDECIMAL(x) ? (x - '0') : -1)
#define STRINGLIB_FILL           memset
#define STRINGLIB_STR            PyString_AS_STRING
#define STRINGLIB_LEN            PyString_GET_SIZE
#define STRINGLIB_NEW            PyString_FromStringAndSize
#define STRINGLIB_RESIZE         _PyString_Resize
#define STRINGLIB_CHECK          PyString_Check
#define STRINGLIB_CMP            memcmp
#define STRINGLIB_TOSTR          PyObject_Str

#endif /* !STRINGLIB_STRINGDEFS_H */
