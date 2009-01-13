/* NOTE: this API is -ONLY- for use with single byte character strings. */
/* Do not use it with Unicode. */

#include "bytes_methods.h"

#ifndef STRINGLIB_MUTABLE
#warning "STRINGLIB_MUTABLE not defined before #include, assuming 0"
#define STRINGLIB_MUTABLE 0
#endif

/* the more complicated methods.  parts of these should be pulled out into the
   shared code in bytes_methods.c to cut down on duplicate code bloat.  */

PyDoc_STRVAR(expandtabs__doc__,
"B.expandtabs([tabsize]) -> copy of B\n\
\n\
Return a copy of B where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.");

static PyObject*
stringlib_expandtabs(PyObject *self, PyObject *args)
{
    const char *e, *p;
    char *q;
    size_t i, j;
    PyObject *u;
    int tabsize = 8;
    
    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
        return NULL;
    
    /* First pass: determine size of output string */
    i = j = 0;
    e = STRINGLIB_STR(self) + STRINGLIB_LEN(self);
    for (p = STRINGLIB_STR(self); p < e; p++)
        if (*p == '\t') {
            if (tabsize > 0) {
                j += tabsize - (j % tabsize);
                if (j > PY_SSIZE_T_MAX) {
                    PyErr_SetString(PyExc_OverflowError,
                                    "result is too long");
                    return NULL;
                }
            }
        }
        else {
            j++;
            if (*p == '\n' || *p == '\r') {
                i += j;
                j = 0;
                if (i > PY_SSIZE_T_MAX) {
                    PyErr_SetString(PyExc_OverflowError,
                                    "result is too long");
                    return NULL;
                }
            }
        }
    
    if ((i + j) > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "result is too long");
        return NULL;
    }
    
    /* Second pass: create output string and fill it */
    u = STRINGLIB_NEW(NULL, i + j);
    if (!u)
        return NULL;
    
    j = 0;
    q = STRINGLIB_STR(u);
    
    for (p = STRINGLIB_STR(self); p < e; p++)
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

Py_LOCAL_INLINE(PyObject *)
pad(PyObject *self, Py_ssize_t left, Py_ssize_t right, char fill)
{
    PyObject *u;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0 && STRINGLIB_CHECK_EXACT(self)) {
#if STRINGLIB_MUTABLE
        /* We're defined as returning a copy;  If the object is mutable
         * that means we must make an identical copy. */
        return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
#else
        Py_INCREF(self);
        return (PyObject *)self;
#endif /* STRINGLIB_MUTABLE */
    }

    u = STRINGLIB_NEW(NULL,
				   left + STRINGLIB_LEN(self) + right);
    if (u) {
        if (left)
            memset(STRINGLIB_STR(u), fill, left);
        Py_MEMCPY(STRINGLIB_STR(u) + left,
	       STRINGLIB_STR(self),
	       STRINGLIB_LEN(self));
        if (right)
            memset(STRINGLIB_STR(u) + left + STRINGLIB_LEN(self),
		   fill, right);
    }

    return u;
}

PyDoc_STRVAR(ljust__doc__,
"B.ljust(width[, fillchar]) -> copy of B\n"
"\n"
"Return B left justified in a string of length width. Padding is\n"
"done using the specified fill character (default is a space).");

static PyObject *
stringlib_ljust(PyObject *self, PyObject *args)
{
    Py_ssize_t width;
    char fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|c:ljust", &width, &fillchar))
        return NULL;

    if (STRINGLIB_LEN(self) >= width && STRINGLIB_CHECK_EXACT(self)) {
#if STRINGLIB_MUTABLE
        /* We're defined as returning a copy;  If the object is mutable
         * that means we must make an identical copy. */
        return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
#else
        Py_INCREF(self);
        return (PyObject*) self;
#endif
    }

    return pad(self, 0, width - STRINGLIB_LEN(self), fillchar);
}


PyDoc_STRVAR(rjust__doc__,
"B.rjust(width[, fillchar]) -> copy of B\n"
"\n"
"Return B right justified in a string of length width. Padding is\n"
"done using the specified fill character (default is a space)");

static PyObject *
stringlib_rjust(PyObject *self, PyObject *args)
{
    Py_ssize_t width;
    char fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|c:rjust", &width, &fillchar))
        return NULL;

    if (STRINGLIB_LEN(self) >= width && STRINGLIB_CHECK_EXACT(self)) {
#if STRINGLIB_MUTABLE
        /* We're defined as returning a copy;  If the object is mutable
         * that means we must make an identical copy. */
        return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
#else
        Py_INCREF(self);
        return (PyObject*) self;
#endif
    }

    return pad(self, width - STRINGLIB_LEN(self), 0, fillchar);
}


PyDoc_STRVAR(center__doc__,
"B.center(width[, fillchar]) -> copy of B\n"
"\n"
"Return B centered in a string of length width.  Padding is\n"
"done using the specified fill character (default is a space).");

static PyObject *
stringlib_center(PyObject *self, PyObject *args)
{
    Py_ssize_t marg, left;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|c:center", &width, &fillchar))
        return NULL;

    if (STRINGLIB_LEN(self) >= width && STRINGLIB_CHECK_EXACT(self)) {
#if STRINGLIB_MUTABLE
        /* We're defined as returning a copy;  If the object is mutable
         * that means we must make an identical copy. */
        return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
#else
        Py_INCREF(self);
        return (PyObject*) self;
#endif
    }

    marg = width - STRINGLIB_LEN(self);
    left = marg / 2 + (marg & width & 1);

    return pad(self, left, marg - left, fillchar);
}

PyDoc_STRVAR(zfill__doc__,
"B.zfill(width) -> copy of B\n"
"\n"
"Pad a numeric string B with zeros on the left, to fill a field\n"
"of the specified width.  B is never truncated.");

static PyObject *
stringlib_zfill(PyObject *self, PyObject *args)
{
    Py_ssize_t fill;
    PyObject *s;
    char *p;
    Py_ssize_t width;

    if (!PyArg_ParseTuple(args, "n:zfill", &width))
        return NULL;

    if (STRINGLIB_LEN(self) >= width) {
        if (STRINGLIB_CHECK_EXACT(self)) {
#if STRINGLIB_MUTABLE
            /* We're defined as returning a copy;  If the object is mutable
             * that means we must make an identical copy. */
            return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
#else
            Py_INCREF(self);
            return (PyObject*) self;
#endif
        }
        else
            return STRINGLIB_NEW(
                STRINGLIB_STR(self),
                STRINGLIB_LEN(self)
            );
    }

    fill = width - STRINGLIB_LEN(self);

    s = pad(self, fill, 0, '0');

    if (s == NULL)
        return NULL;

    p = STRINGLIB_STR(s);
    if (p[fill] == '+' || p[fill] == '-') {
        /* move sign to beginning of string */
        p[0] = p[fill];
        p[fill] = '0';
    }

    return (PyObject*) s;
}


#define _STRINGLIB_SPLIT_APPEND(data, left, right)		\
	str = STRINGLIB_NEW((data) + (left),	                \
					 (right) - (left));	\
	if (str == NULL)					\
		goto onError;					\
	if (PyList_Append(list, str)) {				\
		Py_DECREF(str);					\
		goto onError;					\
	}							\
	else							\
		Py_DECREF(str);

PyDoc_STRVAR(splitlines__doc__,
"B.splitlines([keepends]) -> list of lines\n\
\n\
Return a list of the lines in B, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.");

static PyObject*
stringlib_splitlines(PyObject *self, PyObject *args)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len;
    int keepends = 0;
    PyObject *list;
    PyObject *str;
    char *data;

    if (!PyArg_ParseTuple(args, "|i:splitlines", &keepends))
        return NULL;

    data = STRINGLIB_STR(self);
    len = STRINGLIB_LEN(self);

    /* This does not use the preallocated list because splitlines is
       usually run with hundreds of newlines.  The overhead of
       switching between PyList_SET_ITEM and append causes about a
       2-3% slowdown for that common case.  A smarter implementation
       could move the if check out, so the SET_ITEMs are done first
       and the appends only done when the prealloc buffer is full.
       That's too much work for little gain.*/

    list = PyList_New(0);
    if (!list)
        goto onError;

    for (i = j = 0; i < len; ) {
	Py_ssize_t eol;

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
	_STRINGLIB_SPLIT_APPEND(data, j, eol);
	j = i;
    }
    if (j < len) {
	_STRINGLIB_SPLIT_APPEND(data, j, len);
    }

    return list;

 onError:
    Py_XDECREF(list);
    return NULL;
}

#undef _STRINGLIB_SPLIT_APPEND

