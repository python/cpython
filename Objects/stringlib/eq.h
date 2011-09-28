/* Fast unicode equal function optimized for dictobject.c and setobject.c */

/* Return 1 if two unicode objects are equal, 0 if not.
 * unicode_eq() is called when the hash of two unicode objects is equal.
 */
Py_LOCAL_INLINE(int)
unicode_eq(PyObject *aa, PyObject *bb)
{
    register PyUnicodeObject *a = (PyUnicodeObject *)aa;
    register PyUnicodeObject *b = (PyUnicodeObject *)bb;

    if (PyUnicode_READY(a) == -1 || PyUnicode_READY(b) == -1) {
        assert(0 && "unicode_eq ready fail");
        return 0;
    }

    if (PyUnicode_GET_LENGTH(a) != PyUnicode_GET_LENGTH(b))
        return 0;
    if (PyUnicode_GET_LENGTH(a) == 0)
        return 1;
    if (PyUnicode_KIND(a) != PyUnicode_KIND(b))
        return 0;
    /* Just comparing the first byte is enough to see if a and b differ.
     * If they are 2 byte or 4 byte character most differences will happen in
     * the lower bytes anyways.
     */
    if (PyUnicode_1BYTE_DATA(a)[0] != PyUnicode_1BYTE_DATA(b)[0])
        return 0;
    if (PyUnicode_KIND(a) == PyUnicode_1BYTE_KIND &&
        PyUnicode_GET_LENGTH(a) == 1)
        return 1;
    return memcmp(PyUnicode_1BYTE_DATA(a), PyUnicode_1BYTE_DATA(b),
                  PyUnicode_GET_LENGTH(a) * PyUnicode_CHARACTER_SIZE(a)) == 0;
}
