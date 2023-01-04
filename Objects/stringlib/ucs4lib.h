/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     1

#define FASTSEARCH               ucs4lib_fastsearch
#define STRINGLIB(F)             ucs4lib_##F
#define STRINGLIB_OBJECT         PyUnicodeObject
#define STRINGLIB_SIZEOF_CHAR    4
#define STRINGLIB_MAX_CHAR       0x10FFFFu
#define STRINGLIB_CHAR           Py_UCS4
#define STRINGLIB_TYPE_NAME      "unicode"
#define STRINGLIB_PARSE_CODE     "U"
#define STRINGLIB_ISSPACE        Py_UNICODE_ISSPACE
#define STRINGLIB_ISLINEBREAK    BLOOM_LINEBREAK
#define STRINGLIB_ISDECIMAL      Py_UNICODE_ISDECIMAL
#define STRINGLIB_TODECIMAL      Py_UNICODE_TODECIMAL
#define STRINGLIB_STR            PyUnicode_4BYTE_DATA
#define STRINGLIB_LEN            PyUnicode_GET_LENGTH
#define STRINGLIB_NEW            _PyUnicode_FromUCS4
#define STRINGLIB_CHECK          PyUnicode_Check
#define STRINGLIB_CHECK_EXACT    PyUnicode_CheckExact
#define STRINGLIB_MUTABLE 0
#if SIZEOF_WCHAR_T == 4
#define STRINGLIB_FAST_MEMCHR(s, c, n)              \
    (Py_UCS4 *)wmemchr((const wchar_t *)(s), c, n)
#endif

#define STRINGLIB_TOSTR          PyObject_Str
#define STRINGLIB_TOASCII        PyObject_ASCII

