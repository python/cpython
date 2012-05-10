/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     1

#define FASTSEARCH               ucs2lib_fastsearch
#define STRINGLIB(F)             ucs2lib_##F
#define STRINGLIB_OBJECT         PyUnicodeObject
#define STRINGLIB_SIZEOF_CHAR    2
#define STRINGLIB_MAX_CHAR       0xFFFFu
#define STRINGLIB_CHAR           Py_UCS2
#define STRINGLIB_TYPE_NAME      "unicode"
#define STRINGLIB_PARSE_CODE     "U"
#define STRINGLIB_EMPTY          unicode_empty
#define STRINGLIB_ISSPACE        Py_UNICODE_ISSPACE
#define STRINGLIB_ISLINEBREAK    BLOOM_LINEBREAK
#define STRINGLIB_ISDECIMAL      Py_UNICODE_ISDECIMAL
#define STRINGLIB_TODECIMAL      Py_UNICODE_TODECIMAL
#define STRINGLIB_STR            PyUnicode_2BYTE_DATA
#define STRINGLIB_LEN            PyUnicode_GET_LENGTH
#define STRINGLIB_NEW            _PyUnicode_FromUCS2
#define STRINGLIB_RESIZE         not_supported
#define STRINGLIB_CHECK          PyUnicode_Check
#define STRINGLIB_CHECK_EXACT    PyUnicode_CheckExact

#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_TOASCII        PyObject_ASCII

#define _Py_InsertThousandsGrouping _PyUnicode_ucs2_InsertThousandsGrouping

