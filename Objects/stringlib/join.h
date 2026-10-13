/* stringlib: bytes joining implementation */

#if STRINGLIB_IS_UNICODE
#error join.h only compatible with byte-wise strings
#endif

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(bytes_join_lock_held)(PyObject *sep, PyObject *seq)
{
    const char *sepstr = STRINGLIB_STR(sep);
    Py_ssize_t seplen = STRINGLIB_LEN(sep);
    PyObject *res = NULL;
    char *p;
    Py_ssize_t seqlen = 0;
    Py_ssize_t sz = 0;
    Py_ssize_t i, nbufs;
    PyObject *item;
    Py_buffer *buffers = NULL;
#define NB_STATIC_BUFFERS 10
    Py_buffer static_buffers[NB_STATIC_BUFFERS];
#define GIL_THRESHOLD 1048576
    int drop_gil = 1;
    PyThreadState *save = NULL;

    seqlen = PySequence_Fast_GET_SIZE(seq);
    if (seqlen == 0) {
        return STRINGLIB_NEW(NULL, 0);
    }
#if !STRINGLIB_MUTABLE
    if (seqlen == 1) {
        item = PySequence_Fast_GET_ITEM(seq, 0);
        if (STRINGLIB_CHECK_EXACT(item)) {
            return Py_NewRef(item);
        }
    }
#endif

    Py_ssize_t limit = GIL_THRESHOLD - seplen;
    for (i = 0; i < seqlen; i++) {
        item = PySequence_Fast_GET_ITEM(seq, i);
        if (!PyBytes_CheckExact(item)
            || PyBytes_GET_SIZE(item) >= limit - sz) {
            break;
        }
        sz += PyBytes_GET_SIZE(item) + seplen;
    }
    /* Fast path: all items are exact bytes and the result is too small to
     * release the GIL for.  Nothing here runs Python code or suspends the
     * critical section, so seq keeps the items alive and they can be copied
     * without taking references. */
    if (i == seqlen) {
        res = STRINGLIB_NEW(NULL, sz - seplen);
        if (res == NULL) {
            return NULL;
        }
        p = STRINGLIB_STR(res);
        for (i = 0; i < seqlen; i++) {
            if (i != 0 && seplen != 0) {
                if (seplen == 1) {
                    *p++ = *sepstr;
                }
                else {
                    memcpy(p, sepstr, seplen);
                    p += seplen;
                }
            }
            item = PySequence_Fast_GET_ITEM(seq, i);
            memcpy(p, PyBytes_AS_STRING(item), PyBytes_GET_SIZE(item));
            p += PyBytes_GET_SIZE(item);
        }
        return res;
    }
    sz = 0;

    if (seqlen > NB_STATIC_BUFFERS) {
        buffers = PyMem_NEW(Py_buffer, seqlen);
        if (buffers == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }
    else {
        buffers = static_buffers;
    }

    /* Here is the general case.  Do a pre-pass to figure out the total
     * amount of space we'll need (sz), and see whether all arguments are
     * bytes-like.
     */
    for (i = 0, nbufs = 0; i < seqlen; i++) {
        Py_ssize_t itemlen;
        item = PySequence_Fast_GET_ITEM(seq, i);
        if (PyBytes_CheckExact(item)) {
            /* Fast path. */
            buffers[i].obj = Py_NewRef(item);
            buffers[i].buf = PyBytes_AS_STRING(item);
            buffers[i].len = PyBytes_GET_SIZE(item);
        }
        else {
            /* item is only borrowed; its __buffer__() may run Python that
               drops the sequence's last reference to it. */
            Py_INCREF(item);
            if (PyObject_GetBuffer(item, &buffers[i], PyBUF_SIMPLE) != 0) {
                PyErr_Format(PyExc_TypeError,
                             "sequence item %zd: expected a bytes-like object, "
                             "%.80s found",
                             i, Py_TYPE(item)->tp_name);
                Py_DECREF(item);
                goto error;
            }
            Py_DECREF(item);
            /* If the backing objects are mutable, then dropping the GIL
             * opens up race conditions where another thread tries to modify
             * the object which we hold a buffer on it. Such code has data
             * races anyway, but this is a conservative approach that avoids
             * changing the behaviour of that data race.
             */
            drop_gil = 0;
        }
        nbufs = i + 1;  /* for error cleanup */
        itemlen = buffers[i].len;
        if (itemlen > PY_SSIZE_T_MAX - sz) {
            PyErr_SetString(PyExc_OverflowError,
                            "join() result is too long");
            goto error;
        }
        sz += itemlen;
        if (i != 0) {
            if (seplen > PY_SSIZE_T_MAX - sz) {
                PyErr_SetString(PyExc_OverflowError,
                                "join() result is too long");
                goto error;
            }
            sz += seplen;
        }
        if (seqlen != PySequence_Fast_GET_SIZE(seq)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "sequence changed size during iteration");
            goto error;
        }
    }

    /* Allocate result space. */
    res = STRINGLIB_NEW(NULL, sz);
    if (res == NULL)
        goto error;

    /* Catenate everything. */
    p = STRINGLIB_STR(res);
    if (sz < GIL_THRESHOLD) {
        drop_gil = 0;   /* Benefits are likely outweighed by the overheads */
    }
    if (drop_gil) {
        save = PyEval_SaveThread();
    }
    if (!seplen) {
        /* fast path */
        for (i = 0; i < nbufs; i++) {
            Py_ssize_t n = buffers[i].len;
            char *q = buffers[i].buf;
            memcpy(p, q, n);
            p += n;
        }
    }
    else {
        Py_ssize_t n = buffers[0].len;
        char *q = buffers[0].buf;
        memcpy(p, q, n);
        p += n;

        for (i = 1; i < nbufs; i++) {
            memcpy(p, sepstr, seplen);
            p += seplen;

            n = buffers[i].len;
            q = buffers[i].buf;
            memcpy(p, q, n);
            p += n;
        }
    }
    if (drop_gil) {
        PyEval_RestoreThread(save);
    }
    goto done;

error:
    res = NULL;
done:
    for (i = 0; i < nbufs; i++) {
        PyObject *obj = buffers[i].obj;
        if (obj != NULL && PyBytes_CheckExact(obj)) {
            /* Same as PyBuffer_Release(), without the call: bytes has no
             * bf_releasebuffer, so only the reference to obj is dropped. */
            Py_DECREF(obj);
        }
        else {
            PyBuffer_Release(&buffers[i]);
        }
    }
    if (buffers != static_buffers)
        PyMem_Free(buffers);
    return res;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(bytes_join)(PyObject *sep, PyObject *iterable)
{
    PyObject *seq, *res;

    seq = PySequence_Fast(iterable, "can only join an iterable");
    if (seq == NULL) {
        return NULL;
    }

    Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(iterable);
    res = STRINGLIB(bytes_join_lock_held)(sep, seq);
    Py_END_CRITICAL_SECTION_SEQUENCE_FAST();

    Py_DECREF(seq);
    return res;
}

#undef NB_STATIC_BUFFERS
#undef GIL_THRESHOLD
