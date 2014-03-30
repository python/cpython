/* NOTE: this API is -ONLY- for use with single byte character strings. */
/* Do not use it with Unicode. */

/* the more complicated methods.  parts of these should be pulled out into the
   shared code in bytes_methods.c to cut down on duplicate code bloat.  */

PyDoc_STRVAR(expandtabs__doc__,
"B.expandtabs(tabsize=8) -> copy of B\n\
\n\
Return a copy of B where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.");

static PyObject*
stringlib_expandtabs(PyObject *self, PyObject *args, PyObject *kwds)
{
    const char *e, *p;
    char *q;
    Py_ssize_t i, j;
    PyObject *u;
    static char *kwlist[] = {"tabsize", 0};
    int tabsize = 8;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:expandtabs",
                                     kwlist, &tabsize))
        return NULL;

    /* First pass: determine size of output string */
    i = j = 0;
    e = STRINGLIB_STR(self) + STRINGLIB_LEN(self);
    for (p = STRINGLIB_STR(self); p < e; p++) {
        if (*p == '\t') {
            if (tabsize > 0) {
                Py_ssize_t incr = tabsize - (j % tabsize);
                if (j > PY_SSIZE_T_MAX - incr)
                    goto overflow;
                j += incr;
            }
        }
        else {
            if (j > PY_SSIZE_T_MAX - 1)
                goto overflow;
            j++;
            if (*p == '\n' || *p == '\r') {
                if (i > PY_SSIZE_T_MAX - j)
                    goto overflow;
                i += j;
                j = 0;
            }
        }
    }

    if (i > PY_SSIZE_T_MAX - j)
        goto overflow;

    /* Second pass: create output string and fill it */
    u = STRINGLIB_NEW(NULL, i + j);
    if (!u)
        return NULL;

    j = 0;
    q = STRINGLIB_STR(u);
    
    for (p = STRINGLIB_STR(self); p < e; p++) {
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
    }

    return u;
  overflow:
    PyErr_SetString(PyExc_OverflowError, "result too long");
    return NULL;
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
