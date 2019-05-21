/* Fast unicode equal function optimized for dictobject.c and setobject.c */

/* Return 1 if two unicode objects are equal, 0 if not.
 * unicode_eq() is called when the hash of two unicode objects is equal.
 */
Py_LOCAL_INLINE(int)
unicode_eq(PyObject *aa, PyObject *bb)
{
    PyUnicodeObject *a = (PyUnicodeObject *)aa;
    PyUnicodeObject *b = (PyUnicodeObject *)bb;

    if (PyUnicode_READY(a) == -1 || PyUnicode_READY(b) == -1) {
        Py_UNREACHABLE();
    }

    if (PyUnicode_GET_LENGTH(a) != PyUnicode_GET_LENGTH(b))
        return 0;
    if (PyUnicode_GET_LENGTH(a) == 0)
        return 1;
    if (PyUnicode_KIND(a) != PyUnicode_KIND(b))
        return 0;
    return memcmp(PyUnicode_1BYTE_DATA(a), PyUnicode_1BYTE_DATA(b),
                  PyUnicode_GET_LENGTH(a) * PyUnicode_KIND(a)) == 0;
}
