#ifndef STRINGLIB_UNICODEDEFS_H
#define STRINGLIB_UNICODEDEFS_H

/* this is sort of a hack.  there's at least one place (formatting
   floats) where some stringlib code takes a different path if it's
   compiled as unicode. */
#define STRINGLIB_IS_UNICODE     1

#define STRINGLIB_CHAR           Py_UNICODE
#define STRINGLIB_TYPE_NAME      "unicode"
#define STRINGLIB_PARSE_CODE     "U"
#define STRINGLIB_EMPTY          unicode_empty
#define STRINGLIB_ISDECIMAL      Py_UNICODE_ISDECIMAL
#define STRINGLIB_TODECIMAL      Py_UNICODE_TODECIMAL
#define STRINGLIB_TOUPPER        Py_UNICODE_TOUPPER
#define STRINGLIB_TOLOWER        Py_UNICODE_TOLOWER
#define STRINGLIB_FILL           Py_UNICODE_FILL
#define STRINGLIB_STR            PyUnicode_AS_UNICODE
#define STRINGLIB_LEN            PyUnicode_GET_SIZE
#define STRINGLIB_NEW            PyUnicode_FromUnicode
#define STRINGLIB_RESIZE         PyUnicode_Resize
#define STRINGLIB_CHECK          PyUnicode_Check
#define STRINGLIB_TOSTR          PyObject_Str

#define STRINGLIB_WANT_CONTAINS_OBJ 1

/* STRINGLIB_CMP was defined as:

Py_LOCAL_INLINE(int)
STRINGLIB_CMP(const Py_UNICODE* str, const Py_UNICODE* other, Py_ssize_t len)
{
    if (str[0] != other[0])
        return 1;
    return memcmp((void*) str, (void*) other, len * sizeof(Py_UNICODE));
}

but unfortunately that gives a error if the function isn't used in a file that
includes this file.  So, reluctantly convert it to a macro instead. */

#define STRINGLIB_CMP(str, other, len) \
    (((str)[0] != (other)[0]) ? \
     1 : \
     memcmp((void*) (str), (void*) (other), (len) * sizeof(Py_UNICODE)))


#endif /* !STRINGLIB_UNICODEDEFS_H */
