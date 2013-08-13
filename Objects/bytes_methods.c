#include "Python.h"
#include "bytes_methods.h"

PyDoc_STRVAR_shared(_Py_isspace__doc__,
"B.isspace() -> bool\n\
\n\
Return True if all characters in B are whitespace\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isspace(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && Py_ISSPACE(*p))
        Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
        if (!Py_ISSPACE(*p))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isalpha__doc__,
"B.isalpha() -> bool\n\
\n\
Return True if all characters in B are alphabetic\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isalpha(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && Py_ISALPHA(*p))
        Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
        if (!Py_ISALPHA(*p))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isalnum__doc__,
"B.isalnum() -> bool\n\
\n\
Return True if all characters in B are alphanumeric\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isalnum(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && Py_ISALNUM(*p))
        Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
        if (!Py_ISALNUM(*p))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_isdigit__doc__,
"B.isdigit() -> bool\n\
\n\
Return True if all characters in B are digits\n\
and there is at least one character in B, False otherwise.");

PyObject*
_Py_bytes_isdigit(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;

    /* Shortcut for single character strings */
    if (len == 1 && Py_ISDIGIT(*p))
        Py_RETURN_TRUE;

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    for (; p < e; p++) {
        if (!Py_ISDIGIT(*p))
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}


PyDoc_STRVAR_shared(_Py_islower__doc__,
"B.islower() -> bool\n\
\n\
Return True if all cased characters in B are lowercase and there is\n\
at least one cased character in B, False otherwise.");

PyObject*
_Py_bytes_islower(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;
    int cased;

    /* Shortcut for single character strings */
    if (len == 1)
        return PyBool_FromLong(Py_ISLOWER(*p));

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    for (; p < e; p++) {
        if (Py_ISUPPER(*p))
            Py_RETURN_FALSE;
        else if (!cased && Py_ISLOWER(*p))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_isupper__doc__,
"B.isupper() -> bool\n\
\n\
Return True if all cased characters in B are uppercase and there is\n\
at least one cased character in B, False otherwise.");

PyObject*
_Py_bytes_isupper(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;
    int cased;

    /* Shortcut for single character strings */
    if (len == 1)
        return PyBool_FromLong(Py_ISUPPER(*p));

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    for (; p < e; p++) {
        if (Py_ISLOWER(*p))
            Py_RETURN_FALSE;
        else if (!cased && Py_ISUPPER(*p))
            cased = 1;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_istitle__doc__,
"B.istitle() -> bool\n\
\n\
Return True if B is a titlecased string and there is at least one\n\
character in B, i.e. uppercase characters may only follow uncased\n\
characters and lowercase characters only cased ones. Return False\n\
otherwise.");

PyObject*
_Py_bytes_istitle(const char *cptr, Py_ssize_t len)
{
    const unsigned char *p
        = (unsigned char *) cptr;
    const unsigned char *e;
    int cased, previous_is_cased;

    /* Shortcut for single character strings */
    if (len == 1)
        return PyBool_FromLong(Py_ISUPPER(*p));

    /* Special case for empty strings */
    if (len == 0)
        Py_RETURN_FALSE;

    e = p + len;
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) {
        const unsigned char ch = *p;

        if (Py_ISUPPER(ch)) {
            if (previous_is_cased)
                Py_RETURN_FALSE;
            previous_is_cased = 1;
            cased = 1;
        }
        else if (Py_ISLOWER(ch)) {
            if (!previous_is_cased)
                Py_RETURN_FALSE;
            previous_is_cased = 1;
            cased = 1;
        }
        else
            previous_is_cased = 0;
    }
    return PyBool_FromLong(cased);
}


PyDoc_STRVAR_shared(_Py_lower__doc__,
"B.lower() -> copy of B\n\
\n\
Return a copy of B with all ASCII characters converted to lowercase.");

void
_Py_bytes_lower(char *result, const char *cptr, Py_ssize_t len)
{
    Py_ssize_t i;

    for (i = 0; i < len; i++) {
        result[i] = Py_TOLOWER((unsigned char) cptr[i]);
    }
}


PyDoc_STRVAR_shared(_Py_upper__doc__,
"B.upper() -> copy of B\n\
\n\
Return a copy of B with all ASCII characters converted to uppercase.");

void
_Py_bytes_upper(char *result, const char *cptr, Py_ssize_t len)
{
    Py_ssize_t i;

    for (i = 0; i < len; i++) {
        result[i] = Py_TOUPPER((unsigned char) cptr[i]);
    }
}


PyDoc_STRVAR_shared(_Py_title__doc__,
"B.title() -> copy of B\n\
\n\
Return a titlecased version of B, i.e. ASCII words start with uppercase\n\
characters, all remaining cased characters have lowercase.");

void
_Py_bytes_title(char *result, char *s, Py_ssize_t len)
{
    Py_ssize_t i;
    int previous_is_cased = 0;

    for (i = 0; i < len; i++) {
        int c = Py_CHARMASK(*s++);
        if (Py_ISLOWER(c)) {
            if (!previous_is_cased)
                c = Py_TOUPPER(c);
            previous_is_cased = 1;
        } else if (Py_ISUPPER(c)) {
            if (previous_is_cased)
                c = Py_TOLOWER(c);
            previous_is_cased = 1;
        } else
            previous_is_cased = 0;
        *result++ = c;
    }
}


PyDoc_STRVAR_shared(_Py_capitalize__doc__,
"B.capitalize() -> copy of B\n\
\n\
Return a copy of B with only its first character capitalized (ASCII)\n\
and the rest lower-cased.");

void
_Py_bytes_capitalize(char *result, char *s, Py_ssize_t len)
{
    Py_ssize_t i;

    if (0 < len) {
        int c = Py_CHARMASK(*s++);
        if (Py_ISLOWER(c))
            *result = Py_TOUPPER(c);
        else
            *result = c;
        result++;
    }
    for (i = 1; i < len; i++) {
        int c = Py_CHARMASK(*s++);
        if (Py_ISUPPER(c))
            *result = Py_TOLOWER(c);
        else
            *result = c;
        result++;
    }
}


PyDoc_STRVAR_shared(_Py_swapcase__doc__,
"B.swapcase() -> copy of B\n\
\n\
Return a copy of B with uppercase ASCII characters converted\n\
to lowercase ASCII and vice versa.");

void
_Py_bytes_swapcase(char *result, char *s, Py_ssize_t len)
{
    Py_ssize_t i;

    for (i = 0; i < len; i++) {
        int c = Py_CHARMASK(*s++);
        if (Py_ISLOWER(c)) {
            *result = Py_TOUPPER(c);
        }
        else if (Py_ISUPPER(c)) {
            *result = Py_TOLOWER(c);
        }
        else
            *result = c;
        result++;
    }
}


PyDoc_STRVAR_shared(_Py_maketrans__doc__,
"B.maketrans(frm, to) -> translation table\n\
\n\
Return a translation table (a bytes object of length 256) suitable\n\
for use in the bytes or bytearray translate method where each byte\n\
in frm is mapped to the byte at the same position in to.\n\
The bytes objects frm and to must be of the same length.");

static Py_ssize_t
_getbuffer(PyObject *obj, Py_buffer *view)
{
    PyBufferProcs *buffer = Py_TYPE(obj)->tp_as_buffer;

    if (buffer == NULL || buffer->bf_getbuffer == NULL)
    {
        PyErr_Format(PyExc_TypeError,
                     "Type %.100s doesn't support the buffer API",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }

    if (buffer->bf_getbuffer(obj, view, PyBUF_SIMPLE) < 0)
        return -1;
    return view->len;
}

PyObject *
_Py_bytes_maketrans(PyObject *args)
{
    PyObject *frm, *to, *res = NULL;
    Py_buffer bfrm, bto;
    Py_ssize_t i;
    char *p;

    bfrm.len = -1;
    bto.len = -1;

    if (!PyArg_ParseTuple(args, "OO:maketrans", &frm, &to))
        return NULL;
    if (_getbuffer(frm, &bfrm) < 0)
        return NULL;
    if (_getbuffer(to, &bto) < 0)
        goto done;
    if (bfrm.len != bto.len) {
        PyErr_Format(PyExc_ValueError,
                     "maketrans arguments must have same length");
        goto done;
    }
    res = PyBytes_FromStringAndSize(NULL, 256);
    if (!res) {
        goto done;
    }
    p = PyBytes_AS_STRING(res);
    for (i = 0; i < 256; i++)
        p[i] = (char) i;
    for (i = 0; i < bfrm.len; i++) {
        p[((unsigned char *)bfrm.buf)[i]] = ((char *)bto.buf)[i];
    }

done:
    if (bfrm.len != -1)
        PyBuffer_Release(&bfrm);
    if (bto.len != -1)
        PyBuffer_Release(&bto);
    return res;
}
