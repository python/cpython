/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     1

#define FASTSEARCH               asciilib_fastsearch
#define STRINGLIB(F)             asciilib_##F
#define STRINGLIB_OBJECT         PyUnicodeObject
#define STRINGLIB_SIZEOF_CHAR    1
#define STRINGLIB_MAX_CHAR       0x7Fu
#define STRINGLIB_CHAR           Py_UCS1
#define STRINGLIB_TYPE_NAME      "unicode"
#define STRINGLIB_PARSE_CODE     "U"
#define STRINGLIB_ISSPACE        Py_UNICODE_ISSPACE
#define STRINGLIB_ISLINEBREAK    BLOOM_LINEBREAK
#define STRINGLIB_ISDECIMAL      Py_UNICODE_ISDECIMAL
#define STRINGLIB_TODECIMAL      Py_UNICODE_TODECIMAL
#define STRINGLIB_STR            PyUnicode_1BYTE_DATA
#define STRINGLIB_LEN            PyUnicode_GET_LENGTH
#define STRINGLIB_NEW(STR,LEN)   _PyUnicode_FromASCII((const char*)(STR),(LEN))
#define STRINGLIB_CHECK          PyUnicode_Check
#define STRINGLIB_CHECK_EXACT    PyUnicode_CheckExact
#define STRINGLIB_MUTABLE 0
#define STRINGLIB_FAST_MEMCHR    memchr

#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_TOASCII        PyObject_ASCII
