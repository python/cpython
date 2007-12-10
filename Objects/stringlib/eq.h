/* Fast unicode equal function optimized for dictobject.c and setobject.c */

/* Return 1 if two unicode objects are equal, 0 if not.
 * unicode_eq() is called when the hash of two unicode objects is equal.
 */
Py_LOCAL_INLINE(int)
unicode_eq(PyObject *aa, PyObject *bb)
{
	register PyUnicodeObject *a = (PyUnicodeObject *)aa;
	register PyUnicodeObject *b = (PyUnicodeObject *)bb;

	if (a->length != b->length)
		return 0;
	if (a->length == 0)
		return 1;
	if (a->str[0] != b->str[0])
		return 0;
	if (a->length == 1)
		return 1;
	return memcmp(a->str, b->str, a->length * sizeof(Py_UNICODE)) == 0;
}
