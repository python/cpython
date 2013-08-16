#ifndef STRINGLIB_UNICODEDEFS_H
#define STRINGLIB_UNICODEDEFS_H

/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     1

#define FASTSEARCH               fastsearch
#define STRINGLIB(F)             stringlib_##F
#define STRINGLIB_OBJECT         PyUnicodeObject
#define STRINGLIB_SIZEOF_CHAR    Py_UNICODE_SIZE
#define STRINGLIB_CHAR           Py_UNICODE
#define STRINGLIB_TYPE_NAME      "unicode"
#define STRINGLIB_PARSE_CODE     "U"
#define STRINGLIB_EMPTY          unicode_empty
#define STRINGLIB_ISSPACE        Py_UNICODE_ISSPACE
#define STRINGLIB_ISLINEBREAK    BLOOM_LINEBREAK
#define STRINGLIB_ISDECIMAL      Py_UNICODE_ISDECIMAL
#define STRINGLIB_TODECIMAL      Py_UNICODE_TODECIMAL
#define STRINGLIB_STR            PyUnicode_AS_UNICODE
#define STRINGLIB_LEN            PyUnicode_GET_SIZE
#define STRINGLIB_NEW            PyUnicode_FromUnicode
#define STRINGLIB_CHECK          PyUnicode_Check
#define STRINGLIB_CHECK_EXACT    PyUnicode_CheckExact

#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_TOASCII        PyObject_ASCII

#define STRINGLIB_WANT_CONTAINS_OBJ 1

#endif /* !STRINGLIB_UNICODEDEFS_H */
