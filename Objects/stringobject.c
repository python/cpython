
/* String object implementation */

#include "Python.h"

#include <ctype.h>

#ifdef COUNT_ALLOCS
int null_strings, one_strings;
#endif

#if !defined(HAVE_LIMITS_H) && !defined(UCHAR_MAX)
#define UCHAR_MAX 255
#endif

static PyStringObject *characters[UCHAR_MAX + 1];
#ifndef DONT_SHARE_SHORT_STRINGS
static PyStringObject *nullstring;
#endif

/*
   Newsizedstringobject() and newstringobject() try in certain cases
   to share string objects.  When the size of the string is zero,
   these routines always return a pointer to the same string object;
   when the size is one, they return a pointer to an already existing
   object if the contents of the string is known.  For
   newstringobject() this is always the case, for
   newsizedstringobject() this is the case when the first argument in
   not NULL.
   A common practice to allocate a string and then fill it in or
   change it must be done carefully.  It is only allowed to change the
   contents of the string if the obect was gotten from
   newsizedstringobject() with a NULL first argument, because in the
   future these routines may try to do even more sharing of objects.
*/
PyObject *
PyString_FromStringAndSize(const char *str, int size)
{
	register PyStringObject *op;
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
	if (size == 1 && str != NULL &&
	    (op = characters[*str & UCHAR_MAX]) != NULL)
	{
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */

	/* PyObject_NewVar is inlined */
	op = (PyStringObject *)
		PyObject_MALLOC(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	PyObject_INIT_VAR(op, &PyString_Type, size);
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	if (str != NULL)
		memcpy(op->ob_sval, str, size);
	op->ob_sval[size] = '\0';
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		Py_INCREF(op);
	} else if (size == 1 && str != NULL) {
		characters[*str & UCHAR_MAX] = op;
		Py_INCREF(op);
	}
#endif
	return (PyObject *) op;
}

PyObject *
PyString_FromString(const char *str)
{
	register size_t size = strlen(str);
	register PyStringObject *op;
	if (size > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"string is too long for a Python string");
		return NULL;
	}
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
	if (size == 1 && (op = characters[*str & UCHAR_MAX]) != NULL) {
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */

	/* PyObject_NewVar is inlined */
	op = (PyStringObject *)
		PyObject_MALLOC(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	PyObject_INIT_VAR(op, &PyString_Type, size);
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	strcpy(op->ob_sval, str);
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		Py_INCREF(op);
	} else if (size == 1) {
		characters[*str & UCHAR_MAX] = op;
		Py_INCREF(op);
	}
#endif
	return (PyObject *) op;
}

PyObject *PyString_Decode(const char *s,
			  int size,
			  const char *encoding,
			  const char *errors)
{
    PyObject *buffer = NULL, *str;
    
    if (encoding == NULL) 
	encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    buffer = PyBuffer_FromMemory((void *)s, size);
    if (buffer == NULL)
        goto onError;
    str = PyCodec_Decode(buffer, encoding, errors);
    if (str == NULL)
        goto onError;
    /* Convert Unicode to a string using the default encoding */
    if (PyUnicode_Check(str)) {
	PyObject *temp = str;
	str = PyUnicode_AsEncodedString(str, NULL, NULL);
	Py_DECREF(temp);
	if (str == NULL)
	    goto onError;
    }
    if (!PyString_Check(str)) {
        PyErr_Format(PyExc_TypeError,
                     "decoder did not return a string object (type=%.400s)",
                     str->ob_type->tp_name);
        Py_DECREF(str);
        goto onError;
    }
    Py_DECREF(buffer);
    return str;
    
 onError:
    Py_XDECREF(buffer);
    return NULL;
}

PyObject *PyString_Encode(const char *s,
			  int size,
			  const char *encoding,
			  const char *errors)
{
    PyObject *v, *str;
    
    str = PyString_FromStringAndSize(s, size);
    if (str == NULL)
	return NULL;
    v = PyString_AsEncodedString(str, encoding, errors);
    Py_DECREF(str);
    return v;
}

PyObject *PyString_AsEncodedString(PyObject *str,
				   const char *encoding,
				   const char *errors)
{
    PyObject *v;
    
    if (!PyString_Check(str)) {
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL) 
	encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    v = PyCodec_Encode(str, encoding, errors);
    if (v == NULL)
        goto onError;
    /* Convert Unicode to a string using the default encoding */
    if (PyUnicode_Check(v)) {
	PyObject *temp = v;
	v = PyUnicode_AsEncodedString(v, NULL, NULL);
	Py_DECREF(temp);
	if (v == NULL)
	    goto onError;
    }
    if (!PyString_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "encoder did not return a string object (type=%.400s)",
                     v->ob_type->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;
    
 onError:
    return NULL;
}

static void
string_dealloc(PyObject *op)
{
	PyObject_DEL(op);
}

static int
string_getsize(register PyObject *op)
{
    	char *s;
    	int len;
	if (PyString_AsStringAndSize(op, &s, &len))
		return -1;
	return len;
}

static /*const*/ char *
string_getbuffer(register PyObject *op)
{
    	char *s;
    	int len;
	if (PyString_AsStringAndSize(op, &s, &len))
		return NULL;
	return s;
}

int
PyString_Size(register PyObject *op)
{
	if (!PyString_Check(op))
		return string_getsize(op);
	return ((PyStringObject *)op) -> ob_size;
}

/*const*/ char *
PyString_AsString(register PyObject *op)
{
	if (!PyString_Check(op))
		return string_getbuffer(op);
	return ((PyStringObject *)op) -> ob_sval;
}

/* Internal API needed by PyString_AsStringAndSize(): */
extern 
PyObject *_PyUnicode_AsDefaultEncodedString(PyObject *unicode,
					    const char *errors);

int
PyString_AsStringAndSize(register PyObject *obj,
			 register char **s,
			 register int *len)
{
	if (s == NULL) {
		PyErr_BadInternalCall();
		return -1;
	}

	if (!PyString_Check(obj)) {
		if (PyUnicode_Check(obj)) {
			obj = _PyUnicode_AsDefaultEncodedString(obj, NULL);
			if (obj == NULL)
				return -1;
		}
		else {
			PyErr_Format(PyExc_TypeError,
				     "expected string or Unicode object, "
				     "%.200s found", obj->ob_type->tp_name);
			return -1;
		}
	}

	*s = PyString_AS_STRING(obj);
	if (len != NULL)
		*len = PyString_GET_SIZE(obj);
	else if ((int)strlen(*s) != PyString_GET_SIZE(obj)) {
		PyErr_SetString(PyExc_TypeError,
				"expected string without null bytes");
		return -1;
	}
	return 0;
}

/* Methods */

static int
string_print(PyStringObject *op, FILE *fp, int flags)
{
	int i;
	char c;
	int quote;
	/* XXX Ought to check for interrupts when writing long strings */
	if (flags & Py_PRINT_RAW) {
		fwrite(op->ob_sval, 1, (int) op->ob_size, fp);
		return 0;
	}

	/* figure out which quote to use; single is preferred */
	quote = '\'';
	if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
		quote = '"';

	fputc(quote, fp);
	for (i = 0; i < op->ob_size; i++) {
		c = op->ob_sval[i];
		if (c == quote || c == '\\')
			fprintf(fp, "\\%c", c);
                else if (c == '\t')
                        fprintf(fp, "\\t");
                else if (c == '\n')
                        fprintf(fp, "\\n");
                else if (c == '\r')
                        fprintf(fp, "\\r");
		else if (c < ' ' || c >= 0x7f)
			fprintf(fp, "\\x%02x", c & 0xff);
		else
			fputc(c, fp);
	}
	fputc(quote, fp);
	return 0;
}

static PyObject *
string_repr(register PyStringObject *op)
{
	size_t newsize = 2 + 4 * op->ob_size * sizeof(char);
	PyObject *v;
	if (newsize > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError,
			"string is too large to make repr");
	}
	v = PyString_FromStringAndSize((char *)NULL, newsize);
	if (v == NULL) {
		return NULL;
	}
	else {
		register int i;
		register char c;
		register char *p;
		int quote;

		/* figure out which quote to use; single is preferred */
		quote = '\'';
		if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
			quote = '"';

		p = ((PyStringObject *)v)->ob_sval;
		*p++ = quote;
		for (i = 0; i < op->ob_size; i++) {
			c = op->ob_sval[i];
			if (c == quote || c == '\\')
				*p++ = '\\', *p++ = c;
			else if (c == '\t')
				*p++ = '\\', *p++ = 't';
			else if (c == '\n')
				*p++ = '\\', *p++ = 'n';
			else if (c == '\r')
				*p++ = '\\', *p++ = 'r';
			else if (c < ' ' || c >= 0x7f) {
				sprintf(p, "\\x%02x", c & 0xff);
                                p += 4;
			}
			else
				*p++ = c;
		}
		*p++ = quote;
		*p = '\0';
		_PyString_Resize(
			&v, (int) (p - ((PyStringObject *)v)->ob_sval));
		return v;
	}
}

static PyObject *
string_str(PyObject *s)
{
	Py_INCREF(s);
	return s;
}

static int
string_length(PyStringObject *a)
{
	return a->ob_size;
}

static PyObject *
string_concat(register PyStringObject *a, register PyObject *bb)
{
	register unsigned int size;
	register PyStringObject *op;
	if (!PyString_Check(bb)) {
		if (PyUnicode_Check(bb))
		    return PyUnicode_Concat((PyObject *)a, bb);
		PyErr_Format(PyExc_TypeError, 
			     "cannot add type \"%.200s\" to string",
			     bb->ob_type->tp_name);
		return NULL;
	}
#define b ((PyStringObject *)bb)
	/* Optimize cases with empty left or right operand */
	if (a->ob_size == 0) {
		Py_INCREF(bb);
		return bb;
	}
	if (b->ob_size == 0) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	size = a->ob_size + b->ob_size;
	/* PyObject_NewVar is inlined */
	op = (PyStringObject *)
		PyObject_MALLOC(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	PyObject_INIT_VAR(op, &PyString_Type, size);
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	memcpy(op->ob_sval, a->ob_sval, (int) a->ob_size);
	memcpy(op->ob_sval + a->ob_size, b->ob_sval, (int) b->ob_size);
	op->ob_sval[size] = '\0';
	return (PyObject *) op;
#undef b
}

static PyObject *
string_repeat(register PyStringObject *a, register int n)
{
	register int i;
	register int size;
	register PyStringObject *op;
	size_t nbytes;
	if (n < 0)
		n = 0;
	/* watch out for overflows:  the size can overflow int,
	 * and the # of bytes needed can overflow size_t
	 */
	size = a->ob_size * n;
	if (n && size / n != a->ob_size) {
		PyErr_SetString(PyExc_OverflowError,
			"repeated string is too long");
		return NULL;
	}
	if (size == a->ob_size) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	nbytes = size * sizeof(char);
	if (nbytes / sizeof(char) != (size_t)size ||
	    nbytes + sizeof(PyStringObject) <= nbytes) {
		PyErr_SetString(PyExc_OverflowError,
			"repeated string is too long");
		return NULL;
	}
	op = (PyStringObject *)
		PyObject_MALLOC(sizeof(PyStringObject) + nbytes);
	if (op == NULL)
		return PyErr_NoMemory();
	PyObject_INIT_VAR(op, &PyString_Type, size);
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	for (i = 0; i < size; i += a->ob_size)
		memcpy(op->ob_sval+i, a->ob_sval, (int) a->ob_size);
	op->ob_sval[size] = '\0';
	return (PyObject *) op;
}

/* String slice a[i:j] consists of characters a[i] ... a[j-1] */

static PyObject *
string_slice(register PyStringObject *a, register int i, register int j)
     /* j -- may be negative! */
{
	if (i < 0)
		i = 0;
	if (j < 0)
		j = 0; /* Avoid signed/unsigned bug in next line */
	if (j > a->ob_size)
		j = a->ob_size;
	if (i == 0 && j == a->ob_size) { /* It's the same as a */
		Py_INCREF(a);
		return (PyObject *)a;
	}
	if (j < i)
		j = i;
	return PyString_FromStringAndSize(a->ob_sval + i, (int) (j-i));
}

static int
string_contains(PyObject *a, PyObject *el)
{
	register char *s, *end;
	register char c;
	if (PyUnicode_Check(el))
		return PyUnicode_Contains(a, el);
	if (!PyString_Check(el) || PyString_Size(el) != 1) {
		PyErr_SetString(PyExc_TypeError,
		    "'in <string>' requires character as left operand");
		return -1;
	}
	c = PyString_AsString(el)[0];
	s = PyString_AsString(a);
	end = s + PyString_Size(a);
	while (s < end) {
		if (c == *s++)
			return 1;
	}
	return 0;
}

static PyObject *
string_item(PyStringObject *a, register int i)
{
	int c;
	PyObject *v;
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "string index out of range");
		return NULL;
	}
	c = a->ob_sval[i] & UCHAR_MAX;
	v = (PyObject *) characters[c];
#ifdef COUNT_ALLOCS
	if (v != NULL)
		one_strings++;
#endif
	if (v == NULL) {
		v = PyString_FromStringAndSize((char *)NULL, 1);
		if (v == NULL)
			return NULL;
		characters[c] = (PyStringObject *) v;
		((PyStringObject *)v)->ob_sval[0] = c;
	}
	Py_INCREF(v);
	return v;
}

static int
string_compare(PyStringObject *a, PyStringObject *b)
{
	int len_a = a->ob_size, len_b = b->ob_size;
	int min_len = (len_a < len_b) ? len_a : len_b;
	int cmp;
	if (min_len > 0) {
		cmp = Py_CHARMASK(*a->ob_sval) - Py_CHARMASK(*b->ob_sval);
		if (cmp == 0)
			cmp = memcmp(a->ob_sval, b->ob_sval, min_len);
		if (cmp != 0)
			return cmp;
	}
	return (len_a < len_b) ? -1 : (len_a > len_b) ? 1 : 0;
}

static long
string_hash(PyStringObject *a)
{
	register int len;
	register unsigned char *p;
	register long x;

#ifdef CACHE_HASH
	if (a->ob_shash != -1)
		return a->ob_shash;
#ifdef INTERN_STRINGS
	if (a->ob_sinterned != NULL)
		return (a->ob_shash =
			((PyStringObject *)(a->ob_sinterned))->ob_shash);
#endif
#endif
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= a->ob_size;
	if (x == -1)
		x = -2;
#ifdef CACHE_HASH
	a->ob_shash = x;
#endif
	return x;
}

static int
string_buffer_getreadbuf(PyStringObject *self, int index, const void **ptr)
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent string segment");
		return -1;
	}
	*ptr = (void *)self->ob_sval;
	return self->ob_size;
}

static int
string_buffer_getwritebuf(PyStringObject *self, int index, const void **ptr)
{
	PyErr_SetString(PyExc_TypeError,
			"Cannot use string as modifiable buffer");
	return -1;
}

static int
string_buffer_getsegcount(PyStringObject *self, int *lenp)
{
	if ( lenp )
		*lenp = self->ob_size;
	return 1;
}

static int
string_buffer_getcharbuf(PyStringObject *self, int index, const char **ptr)
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"accessing non-existent string segment");
		return -1;
	}
	*ptr = self->ob_sval;
	return self->ob_size;
}

static PySequenceMethods string_as_sequence = {
	(inquiry)string_length, /*sq_length*/
	(binaryfunc)string_concat, /*sq_concat*/
	(intargfunc)string_repeat, /*sq_repeat*/
	(intargfunc)string_item, /*sq_item*/
	(intintargfunc)string_slice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
	(objobjproc)string_contains /*sq_contains*/
};

static PyBufferProcs string_as_buffer = {
	(getreadbufferproc)string_buffer_getreadbuf,
	(getwritebufferproc)string_buffer_getwritebuf,
	(getsegcountproc)string_buffer_getsegcount,
	(getcharbufferproc)string_buffer_getcharbuf,
};



#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2


static PyObject *
split_whitespace(const char *s, int len, int maxsplit)
{
	int i, j, err;
	PyObject* item;
	PyObject *list = PyList_New(0);

	if (list == NULL)
		return NULL;

	for (i = j = 0; i < len; ) {
		while (i < len && isspace(Py_CHARMASK(s[i])))
			i++;
		j = i;
		while (i < len && !isspace(Py_CHARMASK(s[i])))
			i++;
		if (j < i) {
			if (maxsplit-- <= 0)
				break;
			item = PyString_FromStringAndSize(s+j, (int)(i-j));
			if (item == NULL)
				goto finally;
			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto finally;
			while (i < len && isspace(Py_CHARMASK(s[i])))
				i++;
			j = i;
		}
	}
	if (j < len) {
		item = PyString_FromStringAndSize(s+j, (int)(len - j));
		if (item == NULL)
			goto finally;
		err = PyList_Append(list, item);
		Py_DECREF(item);
		if (err < 0)
			goto finally;
	}
	return list;
  finally:
	Py_DECREF(list);
	return NULL;
}


static char split__doc__[] =
"S.split([sep [,maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in the string S, using sep as the\n\
delimiter string.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified, any whitespace string\n\
is a separator.";

static PyObject *
string_split(PyStringObject *self, PyObject *args)
{
	int len = PyString_GET_SIZE(self), n, i, j, err;
	int maxsplit = -1;
	const char *s = PyString_AS_STRING(self), *sub;
	PyObject *list, *item, *subobj = Py_None;

	if (!PyArg_ParseTuple(args, "|Oi:split", &subobj, &maxsplit))
		return NULL;
	if (maxsplit < 0)
		maxsplit = INT_MAX;
	if (subobj == Py_None)
		return split_whitespace(s, len, maxsplit);
	if (PyString_Check(subobj)) {
		sub = PyString_AS_STRING(subobj);
		n = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj))
		return PyUnicode_Split((PyObject *)self, subobj, maxsplit);
	else if (PyObject_AsCharBuffer(subobj, &sub, &n))
		return NULL;
	if (n == 0) {
		PyErr_SetString(PyExc_ValueError, "empty separator");
		return NULL;
	}

	list = PyList_New(0);
	if (list == NULL)
		return NULL;

	i = j = 0;
	while (i+n <= len) {
		if (s[i] == sub[0] && memcmp(s+i, sub, n) == 0) {
			if (maxsplit-- <= 0)
				break;
			item = PyString_FromStringAndSize(s+j, (int)(i-j));
			if (item == NULL)
				goto fail;
			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto fail;
			i = j = i + n;
		}
		else
			i++;
	}
	item = PyString_FromStringAndSize(s+j, (int)(len-j));
	if (item == NULL)
		goto fail;
	err = PyList_Append(list, item);
	Py_DECREF(item);
	if (err < 0)
		goto fail;

	return list;

 fail:
	Py_DECREF(list);
	return NULL;
}


static char join__doc__[] =
"S.join(sequence) -> string\n\
\n\
Return a string which is the concatenation of the strings in the\n\
sequence.  The separator between elements is S.";

static PyObject *
string_join(PyStringObject *self, PyObject *args)
{
	char *sep = PyString_AS_STRING(self);
	const int seplen = PyString_GET_SIZE(self);
	PyObject *res = NULL;
	char *p;
	int seqlen = 0;
	size_t sz = 0;
	int i;
	PyObject *orig, *seq, *item;

	if (!PyArg_ParseTuple(args, "O:join", &orig))
		return NULL;

	seq = PySequence_Fast(orig, "");
	if (seq == NULL) {
		if (PyErr_ExceptionMatches(PyExc_TypeError))
			PyErr_Format(PyExc_TypeError,
				     "sequence expected, %.80s found",
				     orig->ob_type->tp_name);
		return NULL;
	}

	seqlen = PySequence_Size(seq);
	if (seqlen == 0) {
		Py_DECREF(seq);
		return PyString_FromString("");
	}
	if (seqlen == 1) {
		item = PySequence_Fast_GET_ITEM(seq, 0);
		if (!PyString_Check(item) && !PyUnicode_Check(item)) {
			PyErr_Format(PyExc_TypeError,
				     "sequence item 0: expected string,"
				     " %.80s found",
				     item->ob_type->tp_name);
			Py_DECREF(seq);
			return NULL;
		}
		Py_INCREF(item);
		Py_DECREF(seq);
		return item;
	}

	/* There are at least two things to join.  Do a pre-pass to figure out
	 * the total amount of space we'll need (sz), see whether any argument
	 * is absurd, and defer to the Unicode join if appropriate.
	 */
	for (i = 0; i < seqlen; i++) {
		const size_t old_sz = sz;
		item = PySequence_Fast_GET_ITEM(seq, i);
		if (!PyString_Check(item)){
			if (PyUnicode_Check(item)) {
				Py_DECREF(seq);
				return PyUnicode_Join((PyObject *)self, orig);
			}
			PyErr_Format(PyExc_TypeError,
				     "sequence item %i: expected string,"
				     " %.80s found",
				     i, item->ob_type->tp_name);
			Py_DECREF(seq);
			return NULL;
		}
		sz += PyString_GET_SIZE(item);
		if (i != 0)
			sz += seplen;
		if (sz < old_sz || sz > INT_MAX) {
			PyErr_SetString(PyExc_OverflowError,
				"join() is too long for a Python string");
			Py_DECREF(seq);
			return NULL;
		}
	}

	/* Allocate result space. */
	res = PyString_FromStringAndSize((char*)NULL, (int)sz);
	if (res == NULL) {
		Py_DECREF(seq);
		return NULL;
	}

	/* Catenate everything. */
	p = PyString_AS_STRING(res);
	for (i = 0; i < seqlen; ++i) {
		size_t n;
		item = PySequence_Fast_GET_ITEM(seq, i);
		n = PyString_GET_SIZE(item);
		memcpy(p, PyString_AS_STRING(item), n);
		p += n;
		if (i < seqlen - 1) {
			memcpy(p, sep, seplen);
			p += seplen;
		}
	}

	Py_DECREF(seq);
	return res;
}

static long
string_find_internal(PyStringObject *self, PyObject *args, int dir)
{
	const char *s = PyString_AS_STRING(self), *sub;
	int len = PyString_GET_SIZE(self);
	int n, i = 0, last = INT_MAX;
	PyObject *subobj;

	if (!PyArg_ParseTuple(args, "O|O&O&:find/rfind/index/rindex", 
		&subobj, _PyEval_SliceIndex, &i, _PyEval_SliceIndex, &last))
		return -2;
	if (PyString_Check(subobj)) {
		sub = PyString_AS_STRING(subobj);
		n = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj))
		return PyUnicode_Find((PyObject *)self, subobj, i, last, 1);
	else if (PyObject_AsCharBuffer(subobj, &sub, &n))
		return -2;

	if (last > len)
		last = len;
	if (last < 0)
		last += len;
	if (last < 0)
		last = 0;
	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;

	if (dir > 0) {
		if (n == 0 && i <= last)
			return (long)i;
		last -= n;
		for (; i <= last; ++i)
			if (s[i] == sub[0] && memcmp(&s[i], sub, n) == 0)
				return (long)i;
	}
	else {
		int j;
	    
        	if (n == 0 && i <= last)
			return (long)last;
		for (j = last-n; j >= i; --j)
			if (s[j] == sub[0] && memcmp(&s[j], sub, n) == 0)
				return (long)j;
	}
	
	return -1;
}


static char find__doc__[] =
"S.find(sub [,start [,end]]) -> int\n\
\n\
Return the lowest index in S where substring sub is found,\n\
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.";

static PyObject *
string_find(PyStringObject *self, PyObject *args)
{
	long result = string_find_internal(self, args, +1);
	if (result == -2)
		return NULL;
	return PyInt_FromLong(result);
}


static char index__doc__[] =
"S.index(sub [,start [,end]]) -> int\n\
\n\
Like S.find() but raise ValueError when the substring is not found.";

static PyObject *
string_index(PyStringObject *self, PyObject *args)
{
	long result = string_find_internal(self, args, +1);
	if (result == -2)
		return NULL;
	if (result == -1) {
		PyErr_SetString(PyExc_ValueError,
				"substring not found in string.index");
		return NULL;
	}
	return PyInt_FromLong(result);
}


static char rfind__doc__[] =
"S.rfind(sub [,start [,end]]) -> int\n\
\n\
Return the highest index in S where substring sub is found,\n\
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.";

static PyObject *
string_rfind(PyStringObject *self, PyObject *args)
{
	long result = string_find_internal(self, args, -1);
	if (result == -2)
		return NULL;
	return PyInt_FromLong(result);
}


static char rindex__doc__[] =
"S.rindex(sub [,start [,end]]) -> int\n\
\n\
Like S.rfind() but raise ValueError when the substring is not found.";

static PyObject *
string_rindex(PyStringObject *self, PyObject *args)
{
	long result = string_find_internal(self, args, -1);
	if (result == -2)
		return NULL;
	if (result == -1) {
		PyErr_SetString(PyExc_ValueError,
				"substring not found in string.rindex");
		return NULL;
	}
	return PyInt_FromLong(result);
}


static PyObject *
do_strip(PyStringObject *self, PyObject *args, int striptype)
{
	char *s = PyString_AS_STRING(self);
	int len = PyString_GET_SIZE(self), i, j;

	if (!PyArg_ParseTuple(args, ":strip"))
		return NULL;

	i = 0;
	if (striptype != RIGHTSTRIP) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i++;
		}
	}

	j = len;
	if (striptype != LEFTSTRIP) {
		do {
			j--;
		} while (j >= i && isspace(Py_CHARMASK(s[j])));
		j++;
	}

	if (i == 0 && j == len) {
		Py_INCREF(self);
		return (PyObject*)self;
	}
	else
		return PyString_FromStringAndSize(s+i, j-i);
}


static char strip__doc__[] =
"S.strip() -> string\n\
\n\
Return a copy of the string S with leading and trailing\n\
whitespace removed.";

static PyObject *
string_strip(PyStringObject *self, PyObject *args)
{
	return do_strip(self, args, BOTHSTRIP);
}


static char lstrip__doc__[] =
"S.lstrip() -> string\n\
\n\
Return a copy of the string S with leading whitespace removed.";

static PyObject *
string_lstrip(PyStringObject *self, PyObject *args)
{
	return do_strip(self, args, LEFTSTRIP);
}


static char rstrip__doc__[] =
"S.rstrip() -> string\n\
\n\
Return a copy of the string S with trailing whitespace removed.";

static PyObject *
string_rstrip(PyStringObject *self, PyObject *args)
{
	return do_strip(self, args, RIGHTSTRIP);
}


static char lower__doc__[] =
"S.lower() -> string\n\
\n\
Return a copy of the string S converted to lowercase.";

static PyObject *
string_lower(PyStringObject *self, PyObject *args)
{
	char *s = PyString_AS_STRING(self), *s_new;
	int i, n = PyString_GET_SIZE(self);
	PyObject *new;

	if (!PyArg_ParseTuple(args, ":lower"))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c)) {
			*s_new = tolower(c);
		} else
			*s_new = c;
		s_new++;
	}
	return new;
}


static char upper__doc__[] =
"S.upper() -> string\n\
\n\
Return a copy of the string S converted to uppercase.";

static PyObject *
string_upper(PyStringObject *self, PyObject *args)
{
	char *s = PyString_AS_STRING(self), *s_new;
	int i, n = PyString_GET_SIZE(self);
	PyObject *new;

	if (!PyArg_ParseTuple(args, ":upper"))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			*s_new = toupper(c);
		} else
			*s_new = c;
		s_new++;
	}
	return new;
}


static char title__doc__[] =
"S.title() -> string\n\
\n\
Return a titlecased version of S, i.e. words start with uppercase\n\
characters, all remaining cased characters have lowercase.";

static PyObject*
string_title(PyStringObject *self, PyObject *args)
{
	char *s = PyString_AS_STRING(self), *s_new;
	int i, n = PyString_GET_SIZE(self);
	int previous_is_cased = 0;
	PyObject *new;

	if (!PyArg_ParseTuple(args, ":title"))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			if (!previous_is_cased)
			    c = toupper(c);
			previous_is_cased = 1;
		} else if (isupper(c)) {
			if (previous_is_cased)
			    c = tolower(c);
			previous_is_cased = 1;
		} else
			previous_is_cased = 0;
		*s_new++ = c;
	}
	return new;
}

static char capitalize__doc__[] =
"S.capitalize() -> string\n\
\n\
Return a copy of the string S with only its first character\n\
capitalized.";

static PyObject *
string_capitalize(PyStringObject *self, PyObject *args)
{
	char *s = PyString_AS_STRING(self), *s_new;
	int i, n = PyString_GET_SIZE(self);
	PyObject *new;

	if (!PyArg_ParseTuple(args, ":capitalize"))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	if (0 < n) {
		int c = Py_CHARMASK(*s++);
		if (islower(c))
			*s_new = toupper(c);
		else
			*s_new = c;
		s_new++;
	}
	for (i = 1; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c))
			*s_new = tolower(c);
		else
			*s_new = c;
		s_new++;
	}
	return new;
}


static char count__doc__[] =
"S.count(sub[, start[, end]]) -> int\n\
\n\
Return the number of occurrences of substring sub in string\n\
S[start:end].  Optional arguments start and end are\n\
interpreted as in slice notation.";

static PyObject *
string_count(PyStringObject *self, PyObject *args)
{
	const char *s = PyString_AS_STRING(self), *sub;
	int len = PyString_GET_SIZE(self), n;
	int i = 0, last = INT_MAX;
	int m, r;
	PyObject *subobj;

	if (!PyArg_ParseTuple(args, "O|O&O&:count", &subobj,
		_PyEval_SliceIndex, &i, _PyEval_SliceIndex, &last))
		return NULL;

	if (PyString_Check(subobj)) {
		sub = PyString_AS_STRING(subobj);
		n = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj)) {
		int count;
		count = PyUnicode_Count((PyObject *)self, subobj, i, last);
		if (count == -1)
			return NULL;
		else
		    	return PyInt_FromLong((long) count);
	}
	else if (PyObject_AsCharBuffer(subobj, &sub, &n))
		return NULL;

	if (last > len)
		last = len;
	if (last < 0)
		last += len;
	if (last < 0)
		last = 0;
	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;
	m = last + 1 - n;
	if (n == 0)
		return PyInt_FromLong((long) (m-i));

	r = 0;
	while (i < m) {
		if (!memcmp(s+i, sub, n)) {
			r++;
			i += n;
		} else {
			i++;
		}
	}
	return PyInt_FromLong((long) r);
}


static char swapcase__doc__[] =
"S.swapcase() -> string\n\
\n\
Return a copy of the string S with uppercase characters\n\
converted to lowercase and vice versa.";

static PyObject *
string_swapcase(PyStringObject *self, PyObject *args)
{
	char *s = PyString_AS_STRING(self), *s_new;
	int i, n = PyString_GET_SIZE(self);
	PyObject *new;

	if (!PyArg_ParseTuple(args, ":swapcase"))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			*s_new = toupper(c);
		}
		else if (isupper(c)) {
			*s_new = tolower(c);
		}
		else
			*s_new = c;
		s_new++;
	}
	return new;
}


static char translate__doc__[] =
"S.translate(table [,deletechars]) -> string\n\
\n\
Return a copy of the string S, where all characters occurring\n\
in the optional argument deletechars are removed, and the\n\
remaining characters have been mapped through the given\n\
translation table, which must be a string of length 256.";

static PyObject *
string_translate(PyStringObject *self, PyObject *args)
{
	register char *input, *output;
	register const char *table;
	register int i, c, changed = 0;
	PyObject *input_obj = (PyObject*)self;
	const char *table1, *output_start, *del_table=NULL;
	int inlen, tablen, dellen = 0;
	PyObject *result;
	int trans_table[256];
	PyObject *tableobj, *delobj = NULL;

	if (!PyArg_ParseTuple(args, "O|O:translate",
			      &tableobj, &delobj))
		return NULL;

	if (PyString_Check(tableobj)) {
		table1 = PyString_AS_STRING(tableobj);
		tablen = PyString_GET_SIZE(tableobj);
	}
	else if (PyUnicode_Check(tableobj)) {
		/* Unicode .translate() does not support the deletechars 
		   parameter; instead a mapping to None will cause characters
		   to be deleted. */
		if (delobj != NULL) {
			PyErr_SetString(PyExc_TypeError,
			"deletions are implemented differently for unicode");
			return NULL;
		}
		return PyUnicode_Translate((PyObject *)self, tableobj, NULL);
	}
	else if (PyObject_AsCharBuffer(tableobj, &table1, &tablen))
		return NULL;

	if (delobj != NULL) {
		if (PyString_Check(delobj)) {
			del_table = PyString_AS_STRING(delobj);
			dellen = PyString_GET_SIZE(delobj);
		}
		else if (PyUnicode_Check(delobj)) {
			PyErr_SetString(PyExc_TypeError,
			"deletions are implemented differently for unicode");
			return NULL;
		}
		else if (PyObject_AsCharBuffer(delobj, &del_table, &dellen))
			return NULL;

		if (tablen != 256) {
			PyErr_SetString(PyExc_ValueError,
			  "translation table must be 256 characters long");
			return NULL;
		}
	}
	else {
		del_table = NULL;
		dellen = 0;
	}

	table = table1;
	inlen = PyString_Size(input_obj);
	result = PyString_FromStringAndSize((char *)NULL, inlen);
	if (result == NULL)
		return NULL;
	output_start = output = PyString_AsString(result);
	input = PyString_AsString(input_obj);

	if (dellen == 0) {
		/* If no deletions are required, use faster code */
		for (i = inlen; --i >= 0; ) {
			c = Py_CHARMASK(*input++);
			if (Py_CHARMASK((*output++ = table[c])) != c)
				changed = 1;
		}
		if (changed)
			return result;
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}

	for (i = 0; i < 256; i++)
		trans_table[i] = Py_CHARMASK(table[i]);

	for (i = 0; i < dellen; i++)
		trans_table[(int) Py_CHARMASK(del_table[i])] = -1;

	for (i = inlen; --i >= 0; ) {
		c = Py_CHARMASK(*input++);
		if (trans_table[c] != -1)
			if (Py_CHARMASK(*output++ = (char)trans_table[c]) == c)
				continue;
		changed = 1;
	}
	if (!changed) {
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}
	/* Fix the size of the resulting string */
	if (inlen > 0 &&_PyString_Resize(&result, output-output_start))
		return NULL;
	return result;
}


/* What follows is used for implementing replace().  Perry Stoll. */

/*
  mymemfind

  strstr replacement for arbitrary blocks of memory.

  Locates the first occurrence in the memory pointed to by MEM of the
  contents of memory pointed to by PAT.  Returns the index into MEM if
  found, or -1 if not found.  If len of PAT is greater than length of
  MEM, the function returns -1.
*/
static int 
mymemfind(const char *mem, int len, const char *pat, int pat_len)
{
	register int ii;

	/* pattern can not occur in the last pat_len-1 chars */
	len -= pat_len;

	for (ii = 0; ii <= len; ii++) {
		if (mem[ii] == pat[0] && memcmp(&mem[ii], pat, pat_len) == 0) {
			return ii;
		}
	}
	return -1;
}

/*
  mymemcnt

   Return the number of distinct times PAT is found in MEM.
   meaning mem=1111 and pat==11 returns 2.
           mem=11111 and pat==11 also return 2.
 */
static int 
mymemcnt(const char *mem, int len, const char *pat, int pat_len)
{
	register int offset = 0;
	int nfound = 0;

	while (len >= 0) {
		offset = mymemfind(mem, len, pat, pat_len);
		if (offset == -1)
			break;
		mem += offset + pat_len;
		len -= offset + pat_len;
		nfound++;
	}
	return nfound;
}

/*
   mymemreplace

   Return a string in which all occurrences of PAT in memory STR are
   replaced with SUB.

   If length of PAT is less than length of STR or there are no occurrences
   of PAT in STR, then the original string is returned. Otherwise, a new
   string is allocated here and returned.

   on return, out_len is:
       the length of output string, or
       -1 if the input string is returned, or
       unchanged if an error occurs (no memory).

   return value is:
       the new string allocated locally, or
       NULL if an error occurred.
*/
static char *
mymemreplace(const char *str, int len,		/* input string */
             const char *pat, int pat_len,	/* pattern string to find */
             const char *sub, int sub_len,	/* substitution string */
             int count,				/* number of replacements */
             int *out_len)
{
	char *out_s;
	char *new_s;
	int nfound, offset, new_len;

	if (len == 0 || pat_len > len)
		goto return_same;

	/* find length of output string */
	nfound = mymemcnt(str, len, pat, pat_len);
	if (count < 0)
		count = INT_MAX;
	else if (nfound > count)
		nfound = count;
	if (nfound == 0)
		goto return_same;
	new_len = len + nfound*(sub_len - pat_len);

	new_s = (char *)PyMem_MALLOC(new_len);
	if (new_s == NULL) return NULL;

	*out_len = new_len;
	out_s = new_s;

	while (len > 0) {
		/* find index of next instance of pattern */
		offset = mymemfind(str, len, pat, pat_len);
		/* if not found,  break out of loop */
		if (offset == -1) break;

		/* copy non matching part of input string */
		memcpy(new_s, str, offset); /* copy part of str before pat */
		str += offset + pat_len; /* move str past pattern */
		len -= offset + pat_len; /* reduce length of str remaining */

		/* copy substitute into the output string */
		new_s += offset; /* move new_s to dest for sub string */
		memcpy(new_s, sub, sub_len); /* copy substring into new_s */
		new_s += sub_len; /* offset new_s past sub string */

		/* break when we've done count replacements */
		if (--count == 0) break;
	}
	/* copy any remaining values into output string */
	if (len > 0)
		memcpy(new_s, str, len);
	return out_s;

  return_same:
	*out_len = -1;
	return (char*)str;	/* have to cast away constness here */
}


static char replace__doc__[] =
"S.replace (old, new[, maxsplit]) -> string\n\
\n\
Return a copy of string S with all occurrences of substring\n\
old replaced by new.  If the optional argument maxsplit is\n\
given, only the first maxsplit occurrences are replaced.";

static PyObject *
string_replace(PyStringObject *self, PyObject *args)
{
	const char *str = PyString_AS_STRING(self), *sub, *repl;
	char *new_s;
	int len = PyString_GET_SIZE(self), sub_len, repl_len, out_len;
	int count = -1;
	PyObject *new;
	PyObject *subobj, *replobj;

	if (!PyArg_ParseTuple(args, "OO|i:replace",
			      &subobj, &replobj, &count))
		return NULL;

	if (PyString_Check(subobj)) {
		sub = PyString_AS_STRING(subobj);
		sub_len = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj))
		return PyUnicode_Replace((PyObject *)self, 
					 subobj, replobj, count);
	else if (PyObject_AsCharBuffer(subobj, &sub, &sub_len))
		return NULL;

	if (PyString_Check(replobj)) {
		repl = PyString_AS_STRING(replobj);
		repl_len = PyString_GET_SIZE(replobj);
	}
	else if (PyUnicode_Check(replobj))
		return PyUnicode_Replace((PyObject *)self, 
					 subobj, replobj, count);
	else if (PyObject_AsCharBuffer(replobj, &repl, &repl_len))
		return NULL;

	if (sub_len <= 0) {
		PyErr_SetString(PyExc_ValueError, "empty pattern string");
		return NULL;
	}
	new_s = mymemreplace(str,len,sub,sub_len,repl,repl_len,count,&out_len);
	if (new_s == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	if (out_len == -1) {
		/* we're returning another reference to self */
		new = (PyObject*)self;
		Py_INCREF(new);
	}
	else {
		new = PyString_FromStringAndSize(new_s, out_len);
		PyMem_FREE(new_s);
	}
	return new;
}


static char startswith__doc__[] =
"S.startswith(prefix[, start[, end]]) -> int\n\
\n\
Return 1 if S starts with the specified prefix, otherwise return 0.  With\n\
optional start, test S beginning at that position.  With optional end, stop\n\
comparing S at that position.";

static PyObject *
string_startswith(PyStringObject *self, PyObject *args)
{
	const char* str = PyString_AS_STRING(self);
	int len = PyString_GET_SIZE(self);
	const char* prefix;
	int plen;
	int start = 0;
	int end = -1;
	PyObject *subobj;

	if (!PyArg_ParseTuple(args, "O|O&O&:startswith", &subobj,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
		return NULL;
	if (PyString_Check(subobj)) {
		prefix = PyString_AS_STRING(subobj);
		plen = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj)) {
	    	int rc;
		rc = PyUnicode_Tailmatch((PyObject *)self, 
					  subobj, start, end, -1);
		if (rc == -1)
			return NULL;
		else
			return PyInt_FromLong((long) rc);
	}
	else if (PyObject_AsCharBuffer(subobj, &prefix, &plen))
		return NULL;

	/* adopt Java semantics for index out of range.  it is legal for
	 * offset to be == plen, but this only returns true if prefix is
	 * the empty string.
	 */
	if (start < 0 || start+plen > len)
		return PyInt_FromLong(0);

	if (!memcmp(str+start, prefix, plen)) {
		/* did the match end after the specified end? */
		if (end < 0)
			return PyInt_FromLong(1);
		else if (end - start < plen)
			return PyInt_FromLong(0);
		else
			return PyInt_FromLong(1);
	}
	else return PyInt_FromLong(0);
}


static char endswith__doc__[] =
"S.endswith(suffix[, start[, end]]) -> int\n\
\n\
Return 1 if S ends with the specified suffix, otherwise return 0.  With\n\
optional start, test S beginning at that position.  With optional end, stop\n\
comparing S at that position.";

static PyObject *
string_endswith(PyStringObject *self, PyObject *args)
{
	const char* str = PyString_AS_STRING(self);
	int len = PyString_GET_SIZE(self);
	const char* suffix;
	int slen;
	int start = 0;
	int end = -1;
	int lower, upper;
	PyObject *subobj;

	if (!PyArg_ParseTuple(args, "O|O&O&:endswith", &subobj,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
		return NULL;
	if (PyString_Check(subobj)) {
		suffix = PyString_AS_STRING(subobj);
		slen = PyString_GET_SIZE(subobj);
	}
	else if (PyUnicode_Check(subobj)) {
	    	int rc;
		rc = PyUnicode_Tailmatch((PyObject *)self, 
					  subobj, start, end, +1);
		if (rc == -1)
			return NULL;
		else
			return PyInt_FromLong((long) rc);
	}
	else if (PyObject_AsCharBuffer(subobj, &suffix, &slen))
		return NULL;

	if (start < 0 || start > len || slen > len)
		return PyInt_FromLong(0);

	upper = (end >= 0 && end <= len) ? end : len;
	lower = (upper - slen) > start ? (upper - slen) : start;

	if (upper-lower >= slen && !memcmp(str+lower, suffix, slen))
		return PyInt_FromLong(1);
	else return PyInt_FromLong(0);
}


static char encode__doc__[] =
"S.encode([encoding[,errors]]) -> string\n\
\n\
Return an encoded string version of S. Default encoding is the current\n\
default string encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore' and 'replace'.";

static PyObject *
string_encode(PyStringObject *self, PyObject *args)
{
    char *encoding = NULL;
    char *errors = NULL;
    if (!PyArg_ParseTuple(args, "|ss:encode", &encoding, &errors))
        return NULL;
    return PyString_AsEncodedString((PyObject *)self, encoding, errors);
}


static char expandtabs__doc__[] =
"S.expandtabs([tabsize]) -> string\n\
\n\
Return a copy of S where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.";

static PyObject*
string_expandtabs(PyStringObject *self, PyObject *args)
{
    const char *e, *p;
    char *q;
    int i, j;
    PyObject *u;
    int tabsize = 8;

    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
	return NULL;

    /* First pass: determine size of output string */
    i = j = 0;
    e = PyString_AS_STRING(self) + PyString_GET_SIZE(self);
    for (p = PyString_AS_STRING(self); p < e; p++)
        if (*p == '\t') {
	    if (tabsize > 0)
		j += tabsize - (j % tabsize);
	}
        else {
            j++;
            if (*p == '\n' || *p == '\r') {
                i += j;
                j = 0;
            }
        }

    /* Second pass: create output string and fill it */
    u = PyString_FromStringAndSize(NULL, i + j);
    if (!u)
        return NULL;

    j = 0;
    q = PyString_AS_STRING(u);

    for (p = PyString_AS_STRING(self); p < e; p++)
        if (*p == '\t') {
	    if (tabsize > 0) {
		i = tabsize - (j % tabsize);
		j += i;
		while (i--)
		    *q++ = ' ';
	    }
	}
	else {
            j++;
	    *q++ = *p;
            if (*p == '\n' || *p == '\r')
                j = 0;
        }

    return u;
}

static 
PyObject *pad(PyStringObject *self, 
	      int left, 
	      int right,
	      char fill)
{
    PyObject *u;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0) {
        Py_INCREF(self);
        return (PyObject *)self;
    }

    u = PyString_FromStringAndSize(NULL, 
				   left + PyString_GET_SIZE(self) + right);
    if (u) {
        if (left)
            memset(PyString_AS_STRING(u), fill, left);
        memcpy(PyString_AS_STRING(u) + left, 
	       PyString_AS_STRING(self), 
	       PyString_GET_SIZE(self));
        if (right)
            memset(PyString_AS_STRING(u) + left + PyString_GET_SIZE(self),
		   fill, right);
    }

    return u;
}

static char ljust__doc__[] =
"S.ljust(width) -> string\n\
\n\
Return S left justified in a string of length width. Padding is\n\
done using spaces.";

static PyObject *
string_ljust(PyStringObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:ljust", &width))
        return NULL;

    if (PyString_GET_SIZE(self) >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return pad(self, 0, width - PyString_GET_SIZE(self), ' ');
}


static char rjust__doc__[] =
"S.rjust(width) -> string\n\
\n\
Return S right justified in a string of length width. Padding is\n\
done using spaces.";

static PyObject *
string_rjust(PyStringObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:rjust", &width))
        return NULL;

    if (PyString_GET_SIZE(self) >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return pad(self, width - PyString_GET_SIZE(self), 0, ' ');
}


static char center__doc__[] =
"S.center(width) -> string\n\
\n\
Return S centered in a string of length width. Padding is done\n\
using spaces.";

static PyObject *
string_center(PyStringObject *self, PyObject *args)
{
    int marg, left;
    int width;

    if (!PyArg_ParseTuple(args, "i:center", &width))
        return NULL;

    if (PyString_GET_SIZE(self) >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    marg = width - PyString_GET_SIZE(self);
    left = marg / 2 + (marg & width & 1);

    return pad(self, left, marg - left, ' ');
}

#if 0
static char zfill__doc__[] =
"S.zfill(width) -> string\n\
\n\
Pad a numeric string x with zeros on the left, to fill a field\n\
of the specified width. The string x is never truncated.";

static PyObject *
string_zfill(PyStringObject *self, PyObject *args)
{
    int fill;
    PyObject *u;
    char *str;

    int width;
    if (!PyArg_ParseTuple(args, "i:zfill", &width))
        return NULL;

    if (PyString_GET_SIZE(self) >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    fill = width - PyString_GET_SIZE(self);

    u = pad(self, fill, 0, '0');
    if (u == NULL)
	return NULL;

    str = PyString_AS_STRING(u);
    if (str[fill] == '+' || str[fill] == '-') {
        /* move sign to beginning of string */
        str[0] = str[fill];
        str[fill] = '0';
    }

    return u;
}
#endif

static char isspace__doc__[] =
"S.isspace() -> int\n\
\n\
Return 1 if there are only whitespace characters in S,\n\
0 otherwise.";

static PyObject*
string_isspace(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1 &&
	isspace(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    for (; p < e; p++) {
	if (!isspace(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}


static char isalpha__doc__[] =
"S.isalpha() -> int\n\
\n\
Return 1 if  all characters in S are alphabetic\n\
and there is at least one character in S, 0 otherwise.";

static PyObject*
string_isalpha(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1 &&
	isalpha(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    for (; p < e; p++) {
	if (!isalpha(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}


static char isalnum__doc__[] =
"S.isalnum() -> int\n\
\n\
Return 1 if  all characters in S are alphanumeric\n\
and there is at least one character in S, 0 otherwise.";

static PyObject*
string_isalnum(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1 &&
	isalnum(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    for (; p < e; p++) {
	if (!isalnum(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}


static char isdigit__doc__[] =
"S.isdigit() -> int\n\
\n\
Return 1 if there are only digit characters in S,\n\
0 otherwise.";

static PyObject*
string_isdigit(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1 &&
	isdigit(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    for (; p < e; p++) {
	if (!isdigit(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}


static char islower__doc__[] =
"S.islower() -> int\n\
\n\
Return 1 if  all cased characters in S are lowercase and there is\n\
at least one cased character in S, 0 otherwise.";

static PyObject*
string_islower(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;
    int cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1)
	return PyInt_FromLong(islower(*p) != 0);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	if (isupper(*p))
	    return PyInt_FromLong(0);
	else if (!cased && islower(*p))
	    cased = 1;
    }
    return PyInt_FromLong(cased);
}


static char isupper__doc__[] =
"S.isupper() -> int\n\
\n\
Return 1 if  all cased characters in S are uppercase and there is\n\
at least one cased character in S, 0 otherwise.";

static PyObject*
string_isupper(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;
    int cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1)
	return PyInt_FromLong(isupper(*p) != 0);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	if (islower(*p))
	    return PyInt_FromLong(0);
	else if (!cased && isupper(*p))
	    cased = 1;
    }
    return PyInt_FromLong(cased);
}


static char istitle__doc__[] =
"S.istitle() -> int\n\
\n\
Return 1 if S is a titlecased string, i.e. uppercase characters\n\
may only follow uncased characters and lowercase characters only cased\n\
ones. Return 0 otherwise.";

static PyObject*
string_istitle(PyStringObject *self, PyObject *args)
{
    register const unsigned char *p
        = (unsigned char *) PyString_AS_STRING(self);
    register const unsigned char *e;
    int cased, previous_is_cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyString_GET_SIZE(self) == 1)
	return PyInt_FromLong(isupper(*p) != 0);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyString_GET_SIZE(self);
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) {
	register const unsigned char ch = *p;

	if (isupper(ch)) {
	    if (previous_is_cased)
		return PyInt_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else if (islower(ch)) {
	    if (!previous_is_cased)
		return PyInt_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else
	    previous_is_cased = 0;
    }
    return PyInt_FromLong(cased);
}


static char splitlines__doc__[] =
"S.splitlines([keepends]]) -> list of strings\n\
\n\
Return a list of the lines in S, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.";

#define SPLIT_APPEND(data, left, right)					\
	str = PyString_FromStringAndSize(data + left, right - left);	\
	if (!str)							\
	    goto onError;						\
	if (PyList_Append(list, str)) {					\
	    Py_DECREF(str);						\
	    goto onError;						\
	}								\
        else								\
            Py_DECREF(str);

static PyObject*
string_splitlines(PyStringObject *self, PyObject *args)
{
    register int i;
    register int j;
    int len;
    int keepends = 0;
    PyObject *list;
    PyObject *str;
    char *data;

    if (!PyArg_ParseTuple(args, "|i:splitlines", &keepends))
        return NULL;

    data = PyString_AS_STRING(self);
    len = PyString_GET_SIZE(self);

    list = PyList_New(0);
    if (!list)
        goto onError;

    for (i = j = 0; i < len; ) {
	int eol;

	/* Find a line and append it */
	while (i < len && data[i] != '\n' && data[i] != '\r')
	    i++;

	/* Skip the line break reading CRLF as one line break */
	eol = i;
	if (i < len) {
	    if (data[i] == '\r' && i + 1 < len &&
		data[i+1] == '\n')
		i += 2;
	    else
		i++;
	    if (keepends)
		eol = i;
	}
	SPLIT_APPEND(data, j, eol);
	j = i;
    }
    if (j < len) {
	SPLIT_APPEND(data, j, len);
    }

    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

#undef SPLIT_APPEND


static PyMethodDef 
string_methods[] = {
	/* Counterparts of the obsolete stropmodule functions; except
	   string.maketrans(). */
	{"join",       (PyCFunction)string_join,       1, join__doc__},
	{"split",       (PyCFunction)string_split,       1, split__doc__},
	{"lower",      (PyCFunction)string_lower,      1, lower__doc__},
	{"upper",       (PyCFunction)string_upper,       1, upper__doc__},
	{"islower", (PyCFunction)string_islower, 0, islower__doc__},
	{"isupper", (PyCFunction)string_isupper, 0, isupper__doc__},
	{"isspace", (PyCFunction)string_isspace, 0, isspace__doc__},
	{"isdigit", (PyCFunction)string_isdigit, 0, isdigit__doc__},
	{"istitle", (PyCFunction)string_istitle, 0, istitle__doc__},
	{"isalpha", (PyCFunction)string_isalpha, 0, isalpha__doc__},
	{"isalnum", (PyCFunction)string_isalnum, 0, isalnum__doc__},
	{"capitalize", (PyCFunction)string_capitalize, 1, capitalize__doc__},
	{"count",      (PyCFunction)string_count,      1, count__doc__},
	{"endswith",   (PyCFunction)string_endswith,   1, endswith__doc__},
	{"find",       (PyCFunction)string_find,       1, find__doc__},
	{"index",      (PyCFunction)string_index,      1, index__doc__},
	{"lstrip",     (PyCFunction)string_lstrip,     1, lstrip__doc__},
	{"replace",     (PyCFunction)string_replace,     1, replace__doc__},
	{"rfind",       (PyCFunction)string_rfind,       1, rfind__doc__},
	{"rindex",      (PyCFunction)string_rindex,      1, rindex__doc__},
	{"rstrip",      (PyCFunction)string_rstrip,      1, rstrip__doc__},
	{"startswith",  (PyCFunction)string_startswith,  1, startswith__doc__},
	{"strip",       (PyCFunction)string_strip,       1, strip__doc__},
	{"swapcase",    (PyCFunction)string_swapcase,    1, swapcase__doc__},
	{"translate",   (PyCFunction)string_translate,   1, translate__doc__},
	{"title",       (PyCFunction)string_title,       1, title__doc__},
	{"ljust",       (PyCFunction)string_ljust,       1, ljust__doc__},
	{"rjust",       (PyCFunction)string_rjust,       1, rjust__doc__},
	{"center",      (PyCFunction)string_center,      1, center__doc__},
	{"encode",      (PyCFunction)string_encode,      1, encode__doc__},
	{"expandtabs",  (PyCFunction)string_expandtabs,  1, expandtabs__doc__},
	{"splitlines",  (PyCFunction)string_splitlines,  1, splitlines__doc__},
#if 0
	{"zfill",       (PyCFunction)string_zfill,       1, zfill__doc__},
#endif
	{NULL,     NULL}		     /* sentinel */
};

static PyObject *
string_getattr(PyStringObject *s, char *name)
{
	return Py_FindMethod(string_methods, (PyObject*)s, name);
}


PyTypeObject PyString_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"string",
	sizeof(PyStringObject),
	sizeof(char),
	(destructor)string_dealloc, /*tp_dealloc*/
	(printfunc)string_print, /*tp_print*/
	(getattrfunc)string_getattr,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)string_compare, /*tp_compare*/
	(reprfunc)string_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&string_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)string_hash, /*tp_hash*/
	0,		/*tp_call*/
	(reprfunc)string_str,	/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	&string_as_buffer,	/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/*tp_flags*/
	0,		/*tp_doc*/
};

void
PyString_Concat(register PyObject **pv, register PyObject *w)
{
	register PyObject *v;
	if (*pv == NULL)
		return;
	if (w == NULL || !PyString_Check(*pv)) {
		Py_DECREF(*pv);
		*pv = NULL;
		return;
	}
	v = string_concat((PyStringObject *) *pv, w);
	Py_DECREF(*pv);
	*pv = v;
}

void
PyString_ConcatAndDel(register PyObject **pv, register PyObject *w)
{
	PyString_Concat(pv, w);
	Py_XDECREF(w);
}


/* The following function breaks the notion that strings are immutable:
   it changes the size of a string.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new string object and destroying the old one, only
   more efficiently.  In any case, don't use this if the string may
   already be known to some other part of the code... */

int
_PyString_Resize(PyObject **pv, int newsize)
{
	register PyObject *v;
	register PyStringObject *sv;
	v = *pv;
	if (!PyString_Check(v) || v->ob_refcnt != 1) {
		*pv = 0;
		Py_DECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	_Py_ForgetReference(v);
	*pv = (PyObject *)
		PyObject_REALLOC((char *)v,
			sizeof(PyStringObject) + newsize * sizeof(char));
	if (*pv == NULL) {
		PyObject_DEL(v);
		PyErr_NoMemory();
		return -1;
	}
	_Py_NewReference(*pv);
	sv = (PyStringObject *) *pv;
	sv->ob_size = newsize;
	sv->ob_sval[newsize] = '\0';
	return 0;
}

/* Helpers for formatstring */

static PyObject *
getnextarg(PyObject *args, int arglen, int *p_argidx)
{
	int argidx = *p_argidx;
	if (argidx < arglen) {
		(*p_argidx)++;
		if (arglen < 0)
			return args;
		else
			return PyTuple_GetItem(args, argidx);
	}
	PyErr_SetString(PyExc_TypeError,
			"not enough arguments for format string");
	return NULL;
}

/* Format codes
 * F_LJUST	'-'
 * F_SIGN	'+'
 * F_BLANK	' '
 * F_ALT	'#'
 * F_ZERO	'0'
 */
#define F_LJUST (1<<0)
#define F_SIGN	(1<<1)
#define F_BLANK (1<<2)
#define F_ALT	(1<<3)
#define F_ZERO	(1<<4)

static int
formatfloat(char *buf, size_t buflen, int flags,
            int prec, int type, PyObject *v)
{
	/* fmt = '%#.' + `prec` + `type`
	   worst case length = 3 + 10 (len of INT_MAX) + 1 = 14 (use 20)*/
	char fmt[20];
	double x;
	if (!PyArg_Parse(v, "d;float argument required", &x))
		return -1;
	if (prec < 0)
		prec = 6;
	if (type == 'f' && fabs(x)/1e25 >= 1e25)
		type = 'g';
	sprintf(fmt, "%%%s.%d%c", (flags&F_ALT) ? "#" : "", prec, type);
	/* worst case length calc to ensure no buffer overrun:
	     fmt = %#.<prec>g
	     buf = '-' + [0-9]*prec + '.' + 'e+' + (longest exp
	        for any double rep.) 
	     len = 1 + prec + 1 + 2 + 5 = 9 + prec
	   If prec=0 the effective precision is 1 (the leading digit is
	   always given), therefore increase by one to 10+prec. */
	if (buflen <= (size_t)10 + (size_t)prec) {
		PyErr_SetString(PyExc_OverflowError,
			"formatted float is too long (precision too large?)");
		return -1;
	}
	sprintf(buf, fmt, x);
	return strlen(buf);
}

/* _PyString_FormatLong emulates the format codes d, u, o, x and X, and
 * the F_ALT flag, for Python's long (unbounded) ints.  It's not used for
 * Python's regular ints.
 * Return value:  a new PyString*, or NULL if error.
 *  .  *pbuf is set to point into it,
 *     *plen set to the # of chars following that.
 *     Caller must decref it when done using pbuf.
 *     The string starting at *pbuf is of the form
 *         "-"? ("0x" | "0X")? digit+
 *     "0x"/"0X" are present only for x and X conversions, with F_ALT
 *         set in flags.  The case of hex digits will be correct, 
 *     There will be at least prec digits, zero-filled on the left if
 *         necessary to get that many.
 * val		object to be converted
 * flags	bitmask of format flags; only F_ALT is looked at
 * prec		minimum number of digits; 0-fill on left if needed
 * type		a character in [duoxX]; u acts the same as d
 *
 * CAUTION:  o, x and X conversions on regular ints can never
 * produce a '-' sign, but can for Python's unbounded ints.
 */
PyObject*
_PyString_FormatLong(PyObject *val, int flags, int prec, int type,
		     char **pbuf, int *plen)
{
	PyObject *result = NULL;
	char *buf;
	int i;
	int sign;	/* 1 if '-', else 0 */
	int len;	/* number of characters */
	int numdigits;	/* len == numnondigits + numdigits */
	int numnondigits = 0;

	switch (type) {
	case 'd':
	case 'u':
		result = val->ob_type->tp_str(val);
		break;
	case 'o':
		result = val->ob_type->tp_as_number->nb_oct(val);
		break;
	case 'x':
	case 'X':
		numnondigits = 2;
		result = val->ob_type->tp_as_number->nb_hex(val);
		break;
	default:
		assert(!"'type' not in [duoxX]");
	}
	if (!result)
		return NULL;

	/* To modify the string in-place, there can only be one reference. */
	if (result->ob_refcnt != 1) {
		PyErr_BadInternalCall();
		return NULL;
	}
	buf = PyString_AsString(result);
	len = PyString_Size(result);
	if (buf[len-1] == 'L') {
		--len;
		buf[len] = '\0';
	}
	sign = buf[0] == '-';
	numnondigits += sign;
	numdigits = len - numnondigits;
	assert(numdigits > 0);

	/* Get rid of base marker unless F_ALT */
	if ((flags & F_ALT) == 0) {
		/* Need to skip 0x, 0X or 0. */
		int skipped = 0;
		switch (type) {
		case 'o':
			assert(buf[sign] == '0');
			/* If 0 is only digit, leave it alone. */
			if (numdigits > 1) {
				skipped = 1;
				--numdigits;
			}
			break;
		case 'x':
		case 'X':
			assert(buf[sign] == '0');
			assert(buf[sign + 1] == 'x');
			skipped = 2;
			numnondigits -= 2;
			break;
		}
		if (skipped) {
			buf += skipped;
			len -= skipped;
			if (sign)
				buf[0] = '-';
		}
		assert(len == numnondigits + numdigits);
		assert(numdigits > 0);
	}

	/* Fill with leading zeroes to meet minimum width. */
	if (prec > numdigits) {
		PyObject *r1 = PyString_FromStringAndSize(NULL,
					numnondigits + prec);
		char *b1;
		if (!r1) {
			Py_DECREF(result);
			return NULL;
		}
		b1 = PyString_AS_STRING(r1);
		for (i = 0; i < numnondigits; ++i)
			*b1++ = *buf++;
		for (i = 0; i < prec - numdigits; i++)
			*b1++ = '0';
		for (i = 0; i < numdigits; i++)
			*b1++ = *buf++;
		*b1 = '\0';
		Py_DECREF(result);
		result = r1;
		buf = PyString_AS_STRING(result);
		len = numnondigits + prec;
	}

	/* Fix up case for hex conversions. */
	switch (type) {
	case 'x':
		/* Need to convert all upper case letters to lower case. */
		for (i = 0; i < len; i++)
			if (buf[i] >= 'A' && buf[i] <= 'F')
				buf[i] += 'a'-'A';
		break;
	case 'X':
		/* Need to convert 0x to 0X (and -0x to -0X). */
		if (buf[sign + 1] == 'x')
			buf[sign + 1] = 'X';
		break;
	}
	*pbuf = buf;
	*plen = len;
	return result;
}

static int
formatint(char *buf, size_t buflen, int flags,
          int prec, int type, PyObject *v)
{
	/* fmt = '%#.' + `prec` + 'l' + `type`
	   worst case length = 3 + 19 (worst len of INT_MAX on 64-bit machine)
	   + 1 + 1 = 24 */
	char fmt[64];	/* plenty big enough! */
	long x;
	if (!PyArg_Parse(v, "l;int argument required", &x))
		return -1;
	if (prec < 0)
		prec = 1;
	sprintf(fmt, "%%%s.%dl%c", (flags&F_ALT) ? "#" : "", prec, type);
	/* buf = '+'/'-'/'0'/'0x' + '[0-9]'*max(prec, len(x in octal))
	   worst case buf = '0x' + [0-9]*prec, where prec >= 11 */
	if (buflen <= 13 || buflen <= (size_t)2 + (size_t)prec) {
		PyErr_SetString(PyExc_OverflowError,
			"formatted integer is too long (precision too large?)");
		return -1;
	}
	sprintf(buf, fmt, x);
	/* When converting 0 under %#x or %#X, C leaves off the base marker,
	 * but we want it (for consistency with other %#x conversions, and
	 * for consistency with Python's hex() function).
	 * BUG 28-Apr-2001 tim:  At least two platform Cs (Metrowerks &
	 * Compaq Tru64) violate the std by converting 0 w/ leading 0x anyway.
	 * So add it only if the platform didn't already.
	 */
	if (x == 0 && (flags & F_ALT) && (type == 'x' || type == 'X') &&
	    buf[1] != (char)type) /* this last always true under std C */
		{
		memmove(buf+2, buf, strlen(buf) + 1);
		buf[0] = '0';
		buf[1] = (char)type;
	}
	return strlen(buf);
}

static int
formatchar(char *buf, size_t buflen, PyObject *v)
{
	/* presume that the buffer is at least 2 characters long */
	if (PyString_Check(v)) {
		if (!PyArg_Parse(v, "c;%c requires int or char", &buf[0]))
			return -1;
	}
	else {
		if (!PyArg_Parse(v, "b;%c requires int or char", &buf[0]))
			return -1;
	}
	buf[1] = '\0';
	return 1;
}


/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...)

   FORMATBUFLEN is the length of the buffer in which the floats, ints, &
   chars are formatted. XXX This is a magic number. Each formatting
   routine does bounds checking to ensure no overflow, but a better
   solution may be to malloc a buffer of appropriate size for each
   format. For now, the current solution is sufficient.
*/
#define FORMATBUFLEN (size_t)120

PyObject *
PyString_Format(PyObject *format, PyObject *args)
{
	char *fmt, *res;
	int fmtcnt, rescnt, reslen, arglen, argidx;
	int args_owned = 0;
	PyObject *result, *orig_args, *v, *w;
	PyObject *dict = NULL;
	if (format == NULL || !PyString_Check(format) || args == NULL) {
		PyErr_BadInternalCall();
		return NULL;
	}
	orig_args = args;
	fmt = PyString_AsString(format);
	fmtcnt = PyString_Size(format);
	reslen = rescnt = fmtcnt + 100;
	result = PyString_FromStringAndSize((char *)NULL, reslen);
	if (result == NULL)
		return NULL;
	res = PyString_AsString(result);
	if (PyTuple_Check(args)) {
		arglen = PyTuple_Size(args);
		argidx = 0;
	}
	else {
		arglen = -1;
		argidx = -2;
	}
	if (args->ob_type->tp_as_mapping)
		dict = args;
	while (--fmtcnt >= 0) {
		if (*fmt != '%') {
			if (--rescnt < 0) {
				rescnt = fmtcnt + 100;
				reslen += rescnt;
				if (_PyString_Resize(&result, reslen) < 0)
					return NULL;
				res = PyString_AsString(result)
					+ reslen - rescnt;
				--rescnt;
			}
			*res++ = *fmt++;
		}
		else {
			/* Got a format specifier */
			int flags = 0;
			int width = -1;
			int prec = -1;
			int size = 0;
			int c = '\0';
			int fill;
			PyObject *v = NULL;
			PyObject *temp = NULL;
			char *pbuf;
			int sign;
			int len;
			char formatbuf[FORMATBUFLEN]; /* For format{float,int,char}() */
			char *fmt_start = fmt;
			int argidx_start = argidx;
			
			fmt++;
			if (*fmt == '(') {
				char *keystart;
				int keylen;
				PyObject *key;
				int pcount = 1;

				if (dict == NULL) {
					PyErr_SetString(PyExc_TypeError,
						 "format requires a mapping"); 
					goto error;
				}
				++fmt;
				--fmtcnt;
				keystart = fmt;
				/* Skip over balanced parentheses */
				while (pcount > 0 && --fmtcnt >= 0) {
					if (*fmt == ')')
						--pcount;
					else if (*fmt == '(')
						++pcount;
					fmt++;
				}
				keylen = fmt - keystart - 1;
				if (fmtcnt < 0 || pcount > 0) {
					PyErr_SetString(PyExc_ValueError,
						   "incomplete format key");
					goto error;
				}
				key = PyString_FromStringAndSize(keystart,
								 keylen);
				if (key == NULL)
					goto error;
				if (args_owned) {
					Py_DECREF(args);
					args_owned = 0;
				}
				args = PyObject_GetItem(dict, key);
				Py_DECREF(key);
				if (args == NULL) {
					goto error;
				}
				args_owned = 1;
				arglen = -1;
				argidx = -2;
			}
			while (--fmtcnt >= 0) {
				switch (c = *fmt++) {
				case '-': flags |= F_LJUST; continue;
				case '+': flags |= F_SIGN; continue;
				case ' ': flags |= F_BLANK; continue;
				case '#': flags |= F_ALT; continue;
				case '0': flags |= F_ZERO; continue;
				}
				break;
			}
			if (c == '*') {
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
					goto error;
				if (!PyInt_Check(v)) {
					PyErr_SetString(PyExc_TypeError,
							"* wants int");
					goto error;
				}
				width = PyInt_AsLong(v);
				if (width < 0) {
					flags |= F_LJUST;
					width = -width;
				}
				if (--fmtcnt >= 0)
					c = *fmt++;
			}
			else if (c >= 0 && isdigit(c)) {
				width = c - '0';
				while (--fmtcnt >= 0) {
					c = Py_CHARMASK(*fmt++);
					if (!isdigit(c))
						break;
					if ((width*10) / 10 != width) {
						PyErr_SetString(
							PyExc_ValueError,
							"width too big");
						goto error;
					}
					width = width*10 + (c - '0');
				}
			}
			if (c == '.') {
				prec = 0;
				if (--fmtcnt >= 0)
					c = *fmt++;
				if (c == '*') {
					v = getnextarg(args, arglen, &argidx);
					if (v == NULL)
						goto error;
					if (!PyInt_Check(v)) {
						PyErr_SetString(
							PyExc_TypeError,
							"* wants int");
						goto error;
					}
					prec = PyInt_AsLong(v);
					if (prec < 0)
						prec = 0;
					if (--fmtcnt >= 0)
						c = *fmt++;
				}
				else if (c >= 0 && isdigit(c)) {
					prec = c - '0';
					while (--fmtcnt >= 0) {
						c = Py_CHARMASK(*fmt++);
						if (!isdigit(c))
							break;
						if ((prec*10) / 10 != prec) {
							PyErr_SetString(
							    PyExc_ValueError,
							    "prec too big");
							goto error;
						}
						prec = prec*10 + (c - '0');
					}
				}
			} /* prec */
			if (fmtcnt >= 0) {
				if (c == 'h' || c == 'l' || c == 'L') {
					size = c;
					if (--fmtcnt >= 0)
						c = *fmt++;
				}
			}
			if (fmtcnt < 0) {
				PyErr_SetString(PyExc_ValueError,
						"incomplete format");
				goto error;
			}
			if (c != '%') {
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
					goto error;
			}
			sign = 0;
			fill = ' ';
			switch (c) {
			case '%':
				pbuf = "%";
				len = 1;
				break;
			case 's':
  			case 'r':
				if (PyUnicode_Check(v)) {
					fmt = fmt_start;
					argidx = argidx_start;
					goto unicode;
				}
				if (c == 's')
				temp = PyObject_Str(v);
				else
					temp = PyObject_Repr(v);
				if (temp == NULL)
					goto error;
				if (!PyString_Check(temp)) {
					PyErr_SetString(PyExc_TypeError,
					  "%s argument has non-string str()");
					goto error;
				}
				pbuf = PyString_AsString(temp);
				len = PyString_Size(temp);
				if (prec >= 0 && len > prec)
					len = prec;
				break;
			case 'i':
			case 'd':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				if (c == 'i')
					c = 'd';
				if (PyLong_Check(v)) {
					temp = _PyString_FormatLong(v, flags,
						prec, c, &pbuf, &len);
					if (!temp)
						goto error;
					/* unbounded ints can always produce
					   a sign character! */
					sign = 1;
				}
				else {
					pbuf = formatbuf;
					len = formatint(pbuf, sizeof(formatbuf),
							flags, prec, c, v);
					if (len < 0)
						goto error;
					/* only d conversion is signed */
					sign = c == 'd';
				}
				if (flags & F_ZERO)
					fill = '0';
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				pbuf = formatbuf;
				len = formatfloat(pbuf, sizeof(formatbuf), flags, prec, c, v);
				if (len < 0)
					goto error;
				sign = 1;
				if (flags & F_ZERO)
					fill = '0';
				break;
			case 'c':
				pbuf = formatbuf;
				len = formatchar(pbuf, sizeof(formatbuf), v);
				if (len < 0)
					goto error;
				break;
			default:
				PyErr_Format(PyExc_ValueError,
				  "unsupported format character '%c' (0x%x) "
				  "at index %i",
				  c, c, fmt - 1 - PyString_AsString(format));
				goto error;
			}
			if (sign) {
				if (*pbuf == '-' || *pbuf == '+') {
					sign = *pbuf++;
					len--;
				}
				else if (flags & F_SIGN)
					sign = '+';
				else if (flags & F_BLANK)
					sign = ' ';
				else
					sign = 0;
			}
			if (width < len)
				width = len;
			if (rescnt < width + (sign != 0)) {
				reslen -= rescnt;
				rescnt = width + fmtcnt + 100;
				reslen += rescnt;
				if (_PyString_Resize(&result, reslen) < 0)
					return NULL;
				res = PyString_AsString(result)
					+ reslen - rescnt;
			}
			if (sign) {
				if (fill != ' ')
					*res++ = sign;
				rescnt--;
				if (width > len)
					width--;
			}
			if ((flags & F_ALT) && (c == 'x' || c == 'X')) {
				assert(pbuf[0] == '0');
				assert(pbuf[1] == c);
				if (fill != ' ') {
					*res++ = *pbuf++;
					*res++ = *pbuf++;
				}
				rescnt -= 2;
				width -= 2;
				if (width < 0)
					width = 0;
				len -= 2;
			}
			if (width > len && !(flags & F_LJUST)) {
				do {
					--rescnt;
					*res++ = fill;
				} while (--width > len);
			}
			if (fill == ' ') {
				if (sign)
					*res++ = sign;
				if ((flags & F_ALT) &&
				    (c == 'x' || c == 'X')) {
					assert(pbuf[0] == '0');
					assert(pbuf[1] == c);
					*res++ = *pbuf++;
					*res++ = *pbuf++;
				}
			}
			memcpy(res, pbuf, len);
			res += len;
			rescnt -= len;
			while (--width >= len) {
				--rescnt;
				*res++ = ' ';
			}
                        if (dict && (argidx < arglen) && c != '%') {
                                PyErr_SetString(PyExc_TypeError,
                                           "not all arguments converted");
                                goto error;
                        }
			Py_XDECREF(temp);
		} /* '%' */
	} /* until end */
	if (argidx < arglen && !dict) {
		PyErr_SetString(PyExc_TypeError,
				"not all arguments converted");
		goto error;
	}
	if (args_owned) {
		Py_DECREF(args);
	}
	_PyString_Resize(&result, reslen - rescnt);
	return result;

 unicode:
	if (args_owned) {
		Py_DECREF(args);
		args_owned = 0;
	}
	/* Fiddle args right (remove the first argidx arguments) */
	if (PyTuple_Check(orig_args) && argidx > 0) {
		PyObject *v;
		int n = PyTuple_GET_SIZE(orig_args) - argidx;
		v = PyTuple_New(n);
		if (v == NULL)
			goto error;
		while (--n >= 0) {
			PyObject *w = PyTuple_GET_ITEM(orig_args, n + argidx);
			Py_INCREF(w);
			PyTuple_SET_ITEM(v, n, w);
		}
		args = v;
	} else {
		Py_INCREF(orig_args);
		args = orig_args;
	}
	args_owned = 1;
	/* Take what we have of the result and let the Unicode formatting
	   function format the rest of the input. */
	rescnt = res - PyString_AS_STRING(result);
	if (_PyString_Resize(&result, rescnt))
		goto error;
	fmtcnt = PyString_GET_SIZE(format) - \
		 (fmt - PyString_AS_STRING(format));
	format = PyUnicode_Decode(fmt, fmtcnt, NULL, NULL);
	if (format == NULL)
		goto error;
	v = PyUnicode_Format(format, args);
	Py_DECREF(format);
	if (v == NULL)
		goto error;
	/* Paste what we have (result) to what the Unicode formatting
	   function returned (v) and return the result (or error) */
	w = PyUnicode_Concat(result, v);
	Py_DECREF(result);
	Py_DECREF(v);
	Py_DECREF(args);
	return w;
	
 error:
	Py_DECREF(result);
	if (args_owned) {
		Py_DECREF(args);
	}
	return NULL;
}


#ifdef INTERN_STRINGS

/* This dictionary will leak at PyString_Fini() time.  That's acceptable
 * because PyString_Fini() specifically frees interned strings that are
 * only referenced by this dictionary.  The CVS log entry for revision 2.45
 * says:
 *
 *    Change the Fini function to only remove otherwise unreferenced
 *    strings from the interned table.  There are references in
 *    hard-to-find static variables all over the interpreter, and it's not
 *    worth trying to get rid of all those; but "uninterning" isn't fair
 *    either and may cause subtle failures later -- so we have to keep them
 *    in the interned table.
 */
static PyObject *interned;

void
PyString_InternInPlace(PyObject **p)
{
	register PyStringObject *s = (PyStringObject *)(*p);
	PyObject *t;
	if (s == NULL || !PyString_Check(s))
		Py_FatalError("PyString_InternInPlace: strings only please!");
	if ((t = s->ob_sinterned) != NULL) {
		if (t == (PyObject *)s)
			return;
		Py_INCREF(t);
		*p = t;
		Py_DECREF(s);
		return;
	}
	if (interned == NULL) {
		interned = PyDict_New();
		if (interned == NULL)
			return;
	}
	if ((t = PyDict_GetItem(interned, (PyObject *)s)) != NULL) {
		Py_INCREF(t);
		*p = s->ob_sinterned = t;
		Py_DECREF(s);
		return;
	}
	t = (PyObject *)s;
	if (PyDict_SetItem(interned, t, t) == 0) {
		s->ob_sinterned = t;
		return;
	}
	PyErr_Clear();
}


PyObject *
PyString_InternFromString(const char *cp)
{
	PyObject *s = PyString_FromString(cp);
	if (s == NULL)
		return NULL;
	PyString_InternInPlace(&s);
	return s;
}

#endif

void
PyString_Fini(void)
{
	int i;
	for (i = 0; i < UCHAR_MAX + 1; i++) {
		Py_XDECREF(characters[i]);
		characters[i] = NULL;
	}
#ifndef DONT_SHARE_SHORT_STRINGS
	Py_XDECREF(nullstring);
	nullstring = NULL;
#endif
#ifdef INTERN_STRINGS
	if (interned) {
		int pos, changed;
		PyObject *key, *value;
		do {
			changed = 0;
			pos = 0;
			while (PyDict_Next(interned, &pos, &key, &value)) {
				if (key->ob_refcnt == 2 && key == value) {
					PyDict_DelItem(interned, key);
					changed = 1;
				}
			}
		} while (changed);
	}
#endif
}

#ifdef INTERN_STRINGS
void _Py_ReleaseInternedStrings(void)
{
	if (interned) {
		Py_DECREF(interned);
		interned = NULL;
	}
}
#endif /* INTERN_STRINGS */
