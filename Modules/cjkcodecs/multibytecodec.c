/*
 * multibytecodec.c: Common Multibyte Codec Implementation
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "multibytecodec.h"

typedef struct {
    const Py_UNICODE    *inbuf, *inbuf_top, *inbuf_end;
    unsigned char       *outbuf, *outbuf_end;
    PyObject            *excobj, *outobj;
} MultibyteEncodeBuffer;

typedef struct {
    const unsigned char *inbuf, *inbuf_top, *inbuf_end;
    PyObject            *excobj;
    _PyUnicodeWriter    writer;
} MultibyteDecodeBuffer;

PyDoc_STRVAR(MultibyteCodec_Encode__doc__,
"I.encode(unicode[, errors]) -> (string, length consumed)\n\
\n\
Return an encoded string version of `unicode'. errors may be given to\n\
set a different error handling scheme. Default is 'strict' meaning that\n\
encoding errors raise a UnicodeEncodeError. Other possible values are\n\
'ignore', 'replace' and 'xmlcharrefreplace' as well as any other name\n\
registered with codecs.register_error that can handle UnicodeEncodeErrors.");

PyDoc_STRVAR(MultibyteCodec_Decode__doc__,
"I.decode(string[, errors]) -> (unicodeobject, length consumed)\n\
\n\
Decodes `string' using I, an MultibyteCodec instance. errors may be given\n\
to set a different error handling scheme. Default is 'strict' meaning\n\
that encoding errors raise a UnicodeDecodeError. Other possible values\n\
are 'ignore' and 'replace' as well as any other name registered with\n\
codecs.register_error that is able to handle UnicodeDecodeErrors.");

static char *codeckwarglist[] = {"input", "errors", NULL};
static char *incnewkwarglist[] = {"errors", NULL};
static char *incrementalkwarglist[] = {"input", "final", NULL};
static char *streamkwarglist[] = {"stream", "errors", NULL};

static PyObject *multibytecodec_encode(MultibyteCodec *,
                MultibyteCodec_State *, const Py_UNICODE **, Py_ssize_t,
                PyObject *, int);

#define MBENC_RESET     MBENC_MAX<<1 /* reset after an encoding session */

static PyObject *
make_tuple(PyObject *object, Py_ssize_t len)
{
    PyObject *v, *w;

    if (object == NULL)
        return NULL;

    v = PyTuple_New(2);
    if (v == NULL) {
        Py_DECREF(object);
        return NULL;
    }
    PyTuple_SET_ITEM(v, 0, object);

    w = PyLong_FromSsize_t(len);
    if (w == NULL) {
        Py_DECREF(v);
        return NULL;
    }
    PyTuple_SET_ITEM(v, 1, w);

    return v;
}

static PyObject *
internal_error_callback(const char *errors)
{
    if (errors == NULL || strcmp(errors, "strict") == 0)
        return ERROR_STRICT;
    else if (strcmp(errors, "ignore") == 0)
        return ERROR_IGNORE;
    else if (strcmp(errors, "replace") == 0)
        return ERROR_REPLACE;
    else
        return PyUnicode_FromString(errors);
}

static PyObject *
call_error_callback(PyObject *errors, PyObject *exc)
{
    PyObject *args, *cb, *r;
    const char *str;

    assert(PyUnicode_Check(errors));
    str = _PyUnicode_AsString(errors);
    if (str == NULL)
        return NULL;
    cb = PyCodec_LookupError(str);
    if (cb == NULL)
        return NULL;

    args = PyTuple_New(1);
    if (args == NULL) {
        Py_DECREF(cb);
        return NULL;
    }

    PyTuple_SET_ITEM(args, 0, exc);
    Py_INCREF(exc);

    r = PyObject_CallObject(cb, args);
    Py_DECREF(args);
    Py_DECREF(cb);
    return r;
}

static PyObject *
codecctx_errors_get(MultibyteStatefulCodecContext *self)
{
    const char *errors;

    if (self->errors == ERROR_STRICT)
        errors = "strict";
    else if (self->errors == ERROR_IGNORE)
        errors = "ignore";
    else if (self->errors == ERROR_REPLACE)
        errors = "replace";
    else {
        Py_INCREF(self->errors);
        return self->errors;
    }

    return PyUnicode_FromString(errors);
}

static int
codecctx_errors_set(MultibyteStatefulCodecContext *self, PyObject *value,
                    void *closure)
{
    PyObject *cb;
    const char *str;

    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "errors must be a string");
        return -1;
    }

    str = _PyUnicode_AsString(value);
    if (str == NULL)
        return -1;

    cb = internal_error_callback(str);
    if (cb == NULL)
        return -1;

    ERROR_DECREF(self->errors);
    self->errors = cb;
    return 0;
}

/* This getset handlers list is used by all the stateful codec objects */
static PyGetSetDef codecctx_getsets[] = {
    {"errors",          (getter)codecctx_errors_get,
                    (setter)codecctx_errors_set,
                    PyDoc_STR("how to treat errors")},
    {NULL,}
};

static int
expand_encodebuffer(MultibyteEncodeBuffer *buf, Py_ssize_t esize)
{
    Py_ssize_t orgpos, orgsize, incsize;

    orgpos = (Py_ssize_t)((char *)buf->outbuf -
                            PyBytes_AS_STRING(buf->outobj));
    orgsize = PyBytes_GET_SIZE(buf->outobj);
    incsize = (esize < (orgsize >> 1) ? (orgsize >> 1) | 1 : esize);

    if (orgsize > PY_SSIZE_T_MAX - incsize)
        return -1;

    if (_PyBytes_Resize(&buf->outobj, orgsize + incsize) == -1)
        return -1;

    buf->outbuf = (unsigned char *)PyBytes_AS_STRING(buf->outobj) +orgpos;
    buf->outbuf_end = (unsigned char *)PyBytes_AS_STRING(buf->outobj)
        + PyBytes_GET_SIZE(buf->outobj);

    return 0;
}
#define REQUIRE_ENCODEBUFFER(buf, s) {                                  \
    if ((s) < 1 || (buf)->outbuf + (s) > (buf)->outbuf_end)             \
        if (expand_encodebuffer(buf, s) == -1)                          \
            goto errorexit;                                             \
}


/**
 * MultibyteCodec object
 */

static int
multibytecodec_encerror(MultibyteCodec *codec,
                        MultibyteCodec_State *state,
                        MultibyteEncodeBuffer *buf,
                        PyObject *errors, Py_ssize_t e)
{
    PyObject *retobj = NULL, *retstr = NULL, *tobj;
    Py_ssize_t retstrsize, newpos;
    Py_ssize_t esize, start, end;
    const char *reason;

    if (e > 0) {
        reason = "illegal multibyte sequence";
        esize = e;
    }
    else {
        switch (e) {
        case MBERR_TOOSMALL:
            REQUIRE_ENCODEBUFFER(buf, -1);
            return 0; /* retry it */
        case MBERR_TOOFEW:
            reason = "incomplete multibyte sequence";
            esize = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);
            break;
        case MBERR_INTERNAL:
            PyErr_SetString(PyExc_RuntimeError,
                            "internal codec error");
            return -1;
        default:
            PyErr_SetString(PyExc_RuntimeError,
                            "unknown runtime error");
            return -1;
        }
    }

    if (errors == ERROR_REPLACE) {
        const Py_UNICODE replchar = '?', *inbuf = &replchar;
        Py_ssize_t r;

        for (;;) {
            Py_ssize_t outleft;

            outleft = (Py_ssize_t)(buf->outbuf_end - buf->outbuf);
            r = codec->encode(state, codec->config, &inbuf, 1,
                              &buf->outbuf, outleft, 0);
            if (r == MBERR_TOOSMALL) {
                REQUIRE_ENCODEBUFFER(buf, -1);
                continue;
            }
            else
                break;
        }

        if (r != 0) {
            REQUIRE_ENCODEBUFFER(buf, 1);
            *buf->outbuf++ = '?';
        }
    }
    if (errors == ERROR_IGNORE || errors == ERROR_REPLACE) {
        buf->inbuf += esize;
        return 0;
    }

    start = (Py_ssize_t)(buf->inbuf - buf->inbuf_top);
    end = start + esize;

    /* use cached exception object if available */
    if (buf->excobj == NULL) {
        buf->excobj = PyUnicodeEncodeError_Create(codec->encoding,
                        buf->inbuf_top,
                        buf->inbuf_end - buf->inbuf_top,
                        start, end, reason);
        if (buf->excobj == NULL)
            goto errorexit;
    }
    else
        if (PyUnicodeEncodeError_SetStart(buf->excobj, start) != 0 ||
            PyUnicodeEncodeError_SetEnd(buf->excobj, end) != 0 ||
            PyUnicodeEncodeError_SetReason(buf->excobj, reason) != 0)
            goto errorexit;

    if (errors == ERROR_STRICT) {
        PyCodec_StrictErrors(buf->excobj);
        goto errorexit;
    }

    retobj = call_error_callback(errors, buf->excobj);
    if (retobj == NULL)
        goto errorexit;

    if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
        (!PyUnicode_Check((tobj = PyTuple_GET_ITEM(retobj, 0))) && !PyBytes_Check(tobj)) ||
        !PyLong_Check(PyTuple_GET_ITEM(retobj, 1))) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding error handler must return "
                        "(str, int) tuple");
        goto errorexit;
    }

    if (PyUnicode_Check(tobj)) {
        const Py_UNICODE *uraw = PyUnicode_AS_UNICODE(tobj);

        retstr = multibytecodec_encode(codec, state, &uraw,
                        PyUnicode_GET_SIZE(tobj), ERROR_STRICT,
                        MBENC_FLUSH);
        if (retstr == NULL)
            goto errorexit;
    }
    else {
        Py_INCREF(tobj);
        retstr = tobj;
    }

    assert(PyBytes_Check(retstr));
    retstrsize = PyBytes_GET_SIZE(retstr);
    REQUIRE_ENCODEBUFFER(buf, retstrsize);

    memcpy(buf->outbuf, PyBytes_AS_STRING(retstr), retstrsize);
    buf->outbuf += retstrsize;

    newpos = PyLong_AsSsize_t(PyTuple_GET_ITEM(retobj, 1));
    if (newpos < 0 && !PyErr_Occurred())
        newpos += (Py_ssize_t)(buf->inbuf_end - buf->inbuf_top);
    if (newpos < 0 || buf->inbuf_top + newpos > buf->inbuf_end) {
        PyErr_Clear();
        PyErr_Format(PyExc_IndexError,
                     "position %zd from error handler out of bounds",
                     newpos);
        goto errorexit;
    }
    buf->inbuf = buf->inbuf_top + newpos;

    Py_DECREF(retobj);
    Py_DECREF(retstr);
    return 0;

errorexit:
    Py_XDECREF(retobj);
    Py_XDECREF(retstr);
    return -1;
}

static int
multibytecodec_decerror(MultibyteCodec *codec,
                        MultibyteCodec_State *state,
                        MultibyteDecodeBuffer *buf,
                        PyObject *errors, Py_ssize_t e)
{
    PyObject *retobj = NULL, *retuni = NULL;
    Py_ssize_t newpos;
    const char *reason;
    Py_ssize_t esize, start, end;

    if (e > 0) {
        reason = "illegal multibyte sequence";
        esize = e;
    }
    else {
        switch (e) {
        case MBERR_TOOSMALL:
            return 0; /* retry it */
        case MBERR_TOOFEW:
            reason = "incomplete multibyte sequence";
            esize = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);
            break;
        case MBERR_INTERNAL:
            PyErr_SetString(PyExc_RuntimeError,
                            "internal codec error");
            return -1;
        default:
            PyErr_SetString(PyExc_RuntimeError,
                            "unknown runtime error");
            return -1;
        }
    }

    if (errors == ERROR_REPLACE) {
        if (_PyUnicodeWriter_WriteChar(&buf->writer,
                                       Py_UNICODE_REPLACEMENT_CHARACTER) < 0)
            goto errorexit;
    }
    if (errors == ERROR_IGNORE || errors == ERROR_REPLACE) {
        buf->inbuf += esize;
        return 0;
    }

    start = (Py_ssize_t)(buf->inbuf - buf->inbuf_top);
    end = start + esize;

    /* use cached exception object if available */
    if (buf->excobj == NULL) {
        buf->excobj = PyUnicodeDecodeError_Create(codec->encoding,
                        (const char *)buf->inbuf_top,
                        (Py_ssize_t)(buf->inbuf_end - buf->inbuf_top),
                        start, end, reason);
        if (buf->excobj == NULL)
            goto errorexit;
    }
    else
        if (PyUnicodeDecodeError_SetStart(buf->excobj, start) ||
            PyUnicodeDecodeError_SetEnd(buf->excobj, end) ||
            PyUnicodeDecodeError_SetReason(buf->excobj, reason))
            goto errorexit;

    if (errors == ERROR_STRICT) {
        PyCodec_StrictErrors(buf->excobj);
        goto errorexit;
    }

    retobj = call_error_callback(errors, buf->excobj);
    if (retobj == NULL)
        goto errorexit;

    if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
        !PyUnicode_Check((retuni = PyTuple_GET_ITEM(retobj, 0))) ||
        !PyLong_Check(PyTuple_GET_ITEM(retobj, 1))) {
        PyErr_SetString(PyExc_TypeError,
                        "decoding error handler must return "
                        "(str, int) tuple");
        goto errorexit;
    }

    if (_PyUnicodeWriter_WriteStr(&buf->writer, retuni) < 0)
        goto errorexit;

    newpos = PyLong_AsSsize_t(PyTuple_GET_ITEM(retobj, 1));
    if (newpos < 0 && !PyErr_Occurred())
        newpos += (Py_ssize_t)(buf->inbuf_end - buf->inbuf_top);
    if (newpos < 0 || buf->inbuf_top + newpos > buf->inbuf_end) {
        PyErr_Clear();
        PyErr_Format(PyExc_IndexError,
                     "position %zd from error handler out of bounds",
                     newpos);
        goto errorexit;
    }
    buf->inbuf = buf->inbuf_top + newpos;
    Py_DECREF(retobj);
    return 0;

errorexit:
    Py_XDECREF(retobj);
    return -1;
}

static PyObject *
multibytecodec_encode(MultibyteCodec *codec,
                      MultibyteCodec_State *state,
                      const Py_UNICODE **data, Py_ssize_t datalen,
                      PyObject *errors, int flags)
{
    MultibyteEncodeBuffer buf;
    Py_ssize_t finalsize, r = 0;

    if (datalen == 0 && !(flags & MBENC_RESET))
        return PyBytes_FromStringAndSize(NULL, 0);

    buf.excobj = NULL;
    buf.outobj = NULL;
    buf.inbuf = buf.inbuf_top = *data;
    buf.inbuf_end = buf.inbuf_top + datalen;

    if (datalen > (PY_SSIZE_T_MAX - 16) / 2) {
        PyErr_NoMemory();
        goto errorexit;
    }

    buf.outobj = PyBytes_FromStringAndSize(NULL, datalen * 2 + 16);
    if (buf.outobj == NULL)
        goto errorexit;
    buf.outbuf = (unsigned char *)PyBytes_AS_STRING(buf.outobj);
    buf.outbuf_end = buf.outbuf + PyBytes_GET_SIZE(buf.outobj);

    while (buf.inbuf < buf.inbuf_end) {
        Py_ssize_t inleft, outleft;

        /* we don't reuse inleft and outleft here.
         * error callbacks can relocate the cursor anywhere on buffer*/
        inleft = (Py_ssize_t)(buf.inbuf_end - buf.inbuf);
        outleft = (Py_ssize_t)(buf.outbuf_end - buf.outbuf);
        r = codec->encode(state, codec->config, &buf.inbuf, inleft,
                          &buf.outbuf, outleft, flags);
        if ((r == 0) || (r == MBERR_TOOFEW && !(flags & MBENC_FLUSH)))
            break;
        else if (multibytecodec_encerror(codec, state, &buf, errors,r))
            goto errorexit;
        else if (r == MBERR_TOOFEW)
            break;
    }

    if (codec->encreset != NULL && (flags & MBENC_RESET))
        for (;;) {
            Py_ssize_t outleft;

            outleft = (Py_ssize_t)(buf.outbuf_end - buf.outbuf);
            r = codec->encreset(state, codec->config, &buf.outbuf,
                                outleft);
            if (r == 0)
                break;
            else if (multibytecodec_encerror(codec, state,
                                             &buf, errors, r))
                goto errorexit;
        }

    finalsize = (Py_ssize_t)((char *)buf.outbuf -
                             PyBytes_AS_STRING(buf.outobj));

    if (finalsize != PyBytes_GET_SIZE(buf.outobj))
        if (_PyBytes_Resize(&buf.outobj, finalsize) == -1)
            goto errorexit;

    *data = buf.inbuf;
    Py_XDECREF(buf.excobj);
    return buf.outobj;

errorexit:
    Py_XDECREF(buf.excobj);
    Py_XDECREF(buf.outobj);
    return NULL;
}

static PyObject *
MultibyteCodec_Encode(MultibyteCodecObject *self,
                      PyObject *args, PyObject *kwargs)
{
    MultibyteCodec_State state;
    Py_UNICODE *data;
    PyObject *errorcb, *r, *arg, *ucvt;
    const char *errors = NULL;
    Py_ssize_t datalen;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|z:encode",
                            codeckwarglist, &arg, &errors))
        return NULL;

    if (PyUnicode_Check(arg))
        ucvt = NULL;
    else {
        arg = ucvt = PyObject_Str(arg);
        if (arg == NULL)
            return NULL;
        else if (!PyUnicode_Check(arg)) {
            PyErr_SetString(PyExc_TypeError,
                "couldn't convert the object to unicode.");
            Py_DECREF(ucvt);
            return NULL;
        }
    }

    data = PyUnicode_AsUnicodeAndSize(arg, &datalen);
    if (data == NULL) {
        Py_XDECREF(ucvt);
        return NULL;
    }

    errorcb = internal_error_callback(errors);
    if (errorcb == NULL) {
        Py_XDECREF(ucvt);
        return NULL;
    }

    if (self->codec->encinit != NULL &&
        self->codec->encinit(&state, self->codec->config) != 0)
        goto errorexit;
    r = multibytecodec_encode(self->codec, &state,
                    (const Py_UNICODE **)&data, datalen, errorcb,
                    MBENC_FLUSH | MBENC_RESET);
    if (r == NULL)
        goto errorexit;

    ERROR_DECREF(errorcb);
    Py_XDECREF(ucvt);
    return make_tuple(r, datalen);

errorexit:
    ERROR_DECREF(errorcb);
    Py_XDECREF(ucvt);
    return NULL;
}

static PyObject *
MultibyteCodec_Decode(MultibyteCodecObject *self,
                      PyObject *args, PyObject *kwargs)
{
    MultibyteCodec_State state;
    MultibyteDecodeBuffer buf;
    PyObject *errorcb, *res;
    Py_buffer pdata;
    const char *data, *errors = NULL;
    Py_ssize_t datalen;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|z:decode",
                            codeckwarglist, &pdata, &errors))
        return NULL;
    data = pdata.buf;
    datalen = pdata.len;

    errorcb = internal_error_callback(errors);
    if (errorcb == NULL) {
        PyBuffer_Release(&pdata);
        return NULL;
    }

    if (datalen == 0) {
        PyBuffer_Release(&pdata);
        ERROR_DECREF(errorcb);
        return make_tuple(PyUnicode_New(0, 0), 0);
    }

    _PyUnicodeWriter_Init(&buf.writer, datalen);
    buf.excobj = NULL;
    buf.inbuf = buf.inbuf_top = (unsigned char *)data;
    buf.inbuf_end = buf.inbuf_top + datalen;

    if (self->codec->decinit != NULL &&
        self->codec->decinit(&state, self->codec->config) != 0)
        goto errorexit;

    while (buf.inbuf < buf.inbuf_end) {
        Py_ssize_t inleft, r;

        inleft = (Py_ssize_t)(buf.inbuf_end - buf.inbuf);

        r = self->codec->decode(&state, self->codec->config,
                        &buf.inbuf, inleft, &buf.writer);
        if (r == 0)
            break;
        else if (multibytecodec_decerror(self->codec, &state,
                                         &buf, errorcb, r))
            goto errorexit;
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    PyBuffer_Release(&pdata);
    Py_XDECREF(buf.excobj);
    ERROR_DECREF(errorcb);
    return make_tuple(res, datalen);

errorexit:
    PyBuffer_Release(&pdata);
    ERROR_DECREF(errorcb);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);

    return NULL;
}

static struct PyMethodDef multibytecodec_methods[] = {
    {"encode",          (PyCFunction)MultibyteCodec_Encode,
                    METH_VARARGS | METH_KEYWORDS,
                    MultibyteCodec_Encode__doc__},
    {"decode",          (PyCFunction)MultibyteCodec_Decode,
                    METH_VARARGS | METH_KEYWORDS,
                    MultibyteCodec_Decode__doc__},
    {NULL,              NULL},
};

static void
multibytecodec_dealloc(MultibyteCodecObject *self)
{
    PyObject_Del(self);
}

static PyTypeObject MultibyteCodec_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MultibyteCodec",                   /* tp_name */
    sizeof(MultibyteCodecObject),       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)multibytecodec_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iterext */
    multibytecodec_methods,             /* tp_methods */
};


/**
 * Utility functions for stateful codec mechanism
 */

#define STATEFUL_DCTX(o)        ((MultibyteStatefulDecoderContext *)(o))
#define STATEFUL_ECTX(o)        ((MultibyteStatefulEncoderContext *)(o))

static PyObject *
encoder_encode_stateful(MultibyteStatefulEncoderContext *ctx,
                        PyObject *unistr, int final)
{
    PyObject *ucvt, *r = NULL;
    Py_UNICODE *inbuf, *inbuf_end, *inbuf_tmp = NULL;
    Py_ssize_t datalen, origpending;
    wchar_t *data;

    if (PyUnicode_Check(unistr))
        ucvt = NULL;
    else {
        unistr = ucvt = PyObject_Str(unistr);
        if (unistr == NULL)
            return NULL;
        else if (!PyUnicode_Check(unistr)) {
            PyErr_SetString(PyExc_TypeError,
                "couldn't convert the object to str.");
            Py_DECREF(ucvt);
            return NULL;
        }
    }

    data = PyUnicode_AsUnicodeAndSize(unistr, &datalen);
    if (data == NULL)
        goto errorexit;
    origpending = ctx->pendingsize;

    if (origpending > 0) {
        if (datalen > PY_SSIZE_T_MAX - ctx->pendingsize) {
            PyErr_NoMemory();
            /* inbuf_tmp == NULL */
            goto errorexit;
        }
        inbuf_tmp = PyMem_New(Py_UNICODE, datalen + ctx->pendingsize);
        if (inbuf_tmp == NULL)
            goto errorexit;
        memcpy(inbuf_tmp, ctx->pending,
            Py_UNICODE_SIZE * ctx->pendingsize);
        memcpy(inbuf_tmp + ctx->pendingsize,
            PyUnicode_AS_UNICODE(unistr),
            Py_UNICODE_SIZE * datalen);
        datalen += ctx->pendingsize;
        ctx->pendingsize = 0;
        inbuf = inbuf_tmp;
    }
    else
        inbuf = (Py_UNICODE *)PyUnicode_AS_UNICODE(unistr);

    inbuf_end = inbuf + datalen;

    r = multibytecodec_encode(ctx->codec, &ctx->state,
                    (const Py_UNICODE **)&inbuf, datalen,
                    ctx->errors, final ? MBENC_FLUSH | MBENC_RESET : 0);
    if (r == NULL) {
        /* recover the original pending buffer */
        if (origpending > 0)
            memcpy(ctx->pending, inbuf_tmp,
                Py_UNICODE_SIZE * origpending);
        ctx->pendingsize = origpending;
        goto errorexit;
    }

    if (inbuf < inbuf_end) {
        ctx->pendingsize = (Py_ssize_t)(inbuf_end - inbuf);
        if (ctx->pendingsize > MAXENCPENDING) {
            /* normal codecs can't reach here */
            ctx->pendingsize = 0;
            PyErr_SetString(PyExc_UnicodeError,
                            "pending buffer overflow");
            goto errorexit;
        }
        memcpy(ctx->pending, inbuf,
            ctx->pendingsize * Py_UNICODE_SIZE);
    }

    if (inbuf_tmp != NULL)
        PyMem_Del(inbuf_tmp);
    Py_XDECREF(ucvt);
    return r;

errorexit:
    if (inbuf_tmp != NULL)
        PyMem_Del(inbuf_tmp);
    Py_XDECREF(r);
    Py_XDECREF(ucvt);
    return NULL;
}

static int
decoder_append_pending(MultibyteStatefulDecoderContext *ctx,
                       MultibyteDecodeBuffer *buf)
{
    Py_ssize_t npendings;

    npendings = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);
    if (npendings + ctx->pendingsize > MAXDECPENDING ||
        npendings > PY_SSIZE_T_MAX - ctx->pendingsize) {
            PyErr_SetString(PyExc_UnicodeError, "pending buffer overflow");
            return -1;
    }
    memcpy(ctx->pending + ctx->pendingsize, buf->inbuf, npendings);
    ctx->pendingsize += npendings;
    return 0;
}

static int
decoder_prepare_buffer(MultibyteDecodeBuffer *buf, const char *data,
                       Py_ssize_t size)
{
    buf->inbuf = buf->inbuf_top = (const unsigned char *)data;
    buf->inbuf_end = buf->inbuf_top + size;
    _PyUnicodeWriter_Init(&buf->writer, size);
    return 0;
}

static int
decoder_feed_buffer(MultibyteStatefulDecoderContext *ctx,
                    MultibyteDecodeBuffer *buf)
{
    while (buf->inbuf < buf->inbuf_end) {
        Py_ssize_t inleft;
        Py_ssize_t r;

        inleft = (Py_ssize_t)(buf->inbuf_end - buf->inbuf);

        r = ctx->codec->decode(&ctx->state, ctx->codec->config,
            &buf->inbuf, inleft, &buf->writer);
        if (r == 0 || r == MBERR_TOOFEW)
            break;
        else if (multibytecodec_decerror(ctx->codec, &ctx->state,
                                         buf, ctx->errors, r))
            return -1;
    }
    return 0;
}


/**
 * MultibyteIncrementalEncoder object
 */

static PyObject *
mbiencoder_encode(MultibyteIncrementalEncoderObject *self,
                  PyObject *args, PyObject *kwargs)
{
    PyObject *data;
    int final = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:encode",
                    incrementalkwarglist, &data, &final))
        return NULL;

    return encoder_encode_stateful(STATEFUL_ECTX(self), data, final);
}

static PyObject *
mbiencoder_reset(MultibyteIncrementalEncoderObject *self)
{
    /* Longest output: 4 bytes (b'\x0F\x1F(B') with ISO 2022 */
    unsigned char buffer[4], *outbuf;
    Py_ssize_t r;
    if (self->codec->encreset != NULL) {
        outbuf = buffer;
        r = self->codec->encreset(&self->state, self->codec->config,
                                  &outbuf, sizeof(buffer));
        if (r != 0)
            return NULL;
    }
    self->pendingsize = 0;
    Py_RETURN_NONE;
}

static struct PyMethodDef mbiencoder_methods[] = {
    {"encode",          (PyCFunction)mbiencoder_encode,
                    METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset",           (PyCFunction)mbiencoder_reset,
                    METH_NOARGS, NULL},
    {NULL,              NULL},
};

static PyObject *
mbiencoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteIncrementalEncoderObject *self;
    PyObject *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s:IncrementalEncoder",
                                     incnewkwarglist, &errors))
        return NULL;

    self = (MultibyteIncrementalEncoderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;
    if (!MultibyteCodec_Check(codec)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->encinit != NULL &&
        self->codec->encinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbiencoder_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbiencoder_traverse(MultibyteIncrementalEncoderObject *self,
                    visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    return 0;
}

static void
mbiencoder_dealloc(MultibyteIncrementalEncoderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject MultibyteIncrementalEncoder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MultibyteIncrementalEncoder",      /* tp_name */
    sizeof(MultibyteIncrementalEncoderObject), /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /*  methods  */
    (destructor)mbiencoder_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
        | Py_TPFLAGS_BASETYPE,          /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)mbiencoder_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iterext */
    mbiencoder_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    codecctx_getsets,                   /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    mbiencoder_init,                    /* tp_init */
    0,                                  /* tp_alloc */
    mbiencoder_new,                     /* tp_new */
};


/**
 * MultibyteIncrementalDecoder object
 */

static PyObject *
mbidecoder_decode(MultibyteIncrementalDecoderObject *self,
                  PyObject *args, PyObject *kwargs)
{
    MultibyteDecodeBuffer buf;
    char *data, *wdata = NULL;
    Py_buffer pdata;
    Py_ssize_t wsize, size, origpending;
    int final = 0;
    PyObject *res;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|i:decode",
                    incrementalkwarglist, &pdata, &final))
        return NULL;
    data = pdata.buf;
    size = pdata.len;

    _PyUnicodeWriter_Init(&buf.writer, 1);
    buf.excobj = NULL;
    origpending = self->pendingsize;

    if (self->pendingsize == 0) {
        wsize = size;
        wdata = data;
    }
    else {
        if (size > PY_SSIZE_T_MAX - self->pendingsize) {
            PyErr_NoMemory();
            goto errorexit;
        }
        wsize = size + self->pendingsize;
        wdata = PyMem_Malloc(wsize);
        if (wdata == NULL)
            goto errorexit;
        memcpy(wdata, self->pending, self->pendingsize);
        memcpy(wdata + self->pendingsize, data, size);
        self->pendingsize = 0;
    }

    if (decoder_prepare_buffer(&buf, wdata, wsize) != 0)
        goto errorexit;

    if (decoder_feed_buffer(STATEFUL_DCTX(self), &buf))
        goto errorexit;

    if (final && buf.inbuf < buf.inbuf_end) {
        if (multibytecodec_decerror(self->codec, &self->state,
                        &buf, self->errors, MBERR_TOOFEW)) {
            /* recover the original pending buffer */
            memcpy(self->pending, wdata, origpending);
            self->pendingsize = origpending;
            goto errorexit;
        }
    }

    if (buf.inbuf < buf.inbuf_end) { /* pending sequence still exists */
        if (decoder_append_pending(STATEFUL_DCTX(self), &buf) != 0)
            goto errorexit;
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    PyBuffer_Release(&pdata);
    if (wdata != data)
        PyMem_Del(wdata);
    Py_XDECREF(buf.excobj);
    return res;

errorexit:
    PyBuffer_Release(&pdata);
    if (wdata != NULL && wdata != data)
        PyMem_Del(wdata);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);
    return NULL;
}

static PyObject *
mbidecoder_reset(MultibyteIncrementalDecoderObject *self)
{
    if (self->codec->decreset != NULL &&
        self->codec->decreset(&self->state, self->codec->config) != 0)
        return NULL;
    self->pendingsize = 0;

    Py_RETURN_NONE;
}

static struct PyMethodDef mbidecoder_methods[] = {
    {"decode",          (PyCFunction)mbidecoder_decode,
                    METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset",           (PyCFunction)mbidecoder_reset,
                    METH_NOARGS, NULL},
    {NULL,              NULL},
};

static PyObject *
mbidecoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteIncrementalDecoderObject *self;
    PyObject *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|s:IncrementalDecoder",
                                     incnewkwarglist, &errors))
        return NULL;

    self = (MultibyteIncrementalDecoderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;
    if (!MultibyteCodec_Check(codec)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->decinit != NULL &&
        self->codec->decinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbidecoder_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbidecoder_traverse(MultibyteIncrementalDecoderObject *self,
                    visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    return 0;
}

static void
mbidecoder_dealloc(MultibyteIncrementalDecoderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject MultibyteIncrementalDecoder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MultibyteIncrementalDecoder",      /* tp_name */
    sizeof(MultibyteIncrementalDecoderObject), /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /*  methods  */
    (destructor)mbidecoder_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
        | Py_TPFLAGS_BASETYPE,          /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)mbidecoder_traverse,          /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iterext */
    mbidecoder_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    codecctx_getsets,                   /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    mbidecoder_init,                    /* tp_init */
    0,                                  /* tp_alloc */
    mbidecoder_new,                     /* tp_new */
};


/**
 * MultibyteStreamReader object
 */

static PyObject *
mbstreamreader_iread(MultibyteStreamReaderObject *self,
                     const char *method, Py_ssize_t sizehint)
{
    MultibyteDecodeBuffer buf;
    PyObject *cres, *res;
    Py_ssize_t rsize;

    if (sizehint == 0)
        return PyUnicode_New(0, 0);

    _PyUnicodeWriter_Init(&buf.writer, 1);
    buf.excobj = NULL;
    cres = NULL;

    for (;;) {
        int endoffile;

        if (sizehint < 0)
            cres = PyObject_CallMethod(self->stream,
                            (char *)method, NULL);
        else
            cres = PyObject_CallMethod(self->stream,
                            (char *)method, "i", sizehint);
        if (cres == NULL)
            goto errorexit;

        if (!PyBytes_Check(cres)) {
            PyErr_Format(PyExc_TypeError,
                         "stream function returned a "
                         "non-bytes object (%.100s)",
                         cres->ob_type->tp_name);
            goto errorexit;
        }

        endoffile = (PyBytes_GET_SIZE(cres) == 0);

        if (self->pendingsize > 0) {
            PyObject *ctr;
            char *ctrdata;

            if (PyBytes_GET_SIZE(cres) > PY_SSIZE_T_MAX - self->pendingsize) {
                PyErr_NoMemory();
                goto errorexit;
        }
                    rsize = PyBytes_GET_SIZE(cres) + self->pendingsize;
                    ctr = PyBytes_FromStringAndSize(NULL, rsize);
                    if (ctr == NULL)
                            goto errorexit;
                    ctrdata = PyBytes_AS_STRING(ctr);
                    memcpy(ctrdata, self->pending, self->pendingsize);
                    memcpy(ctrdata + self->pendingsize,
                            PyBytes_AS_STRING(cres),
                            PyBytes_GET_SIZE(cres));
                    Py_DECREF(cres);
                    cres = ctr;
                    self->pendingsize = 0;
        }

        rsize = PyBytes_GET_SIZE(cres);
        if (decoder_prepare_buffer(&buf, PyBytes_AS_STRING(cres),
                                   rsize) != 0)
            goto errorexit;

        if (rsize > 0 && decoder_feed_buffer(
                        (MultibyteStatefulDecoderContext *)self, &buf))
            goto errorexit;

        if (endoffile || sizehint < 0) {
            if (buf.inbuf < buf.inbuf_end &&
                multibytecodec_decerror(self->codec, &self->state,
                            &buf, self->errors, MBERR_TOOFEW))
                goto errorexit;
        }

        if (buf.inbuf < buf.inbuf_end) { /* pending sequence exists */
            if (decoder_append_pending(STATEFUL_DCTX(self),
                                       &buf) != 0)
                goto errorexit;
        }

        Py_DECREF(cres);
        cres = NULL;

        if (sizehint < 0 || buf.writer.pos != 0 || rsize == 0)
            break;

        sizehint = 1; /* read 1 more byte and retry */
    }

    res = _PyUnicodeWriter_Finish(&buf.writer);
    if (res == NULL)
        goto errorexit;

    Py_XDECREF(cres);
    Py_XDECREF(buf.excobj);
    return res;

errorexit:
    Py_XDECREF(cres);
    Py_XDECREF(buf.excobj);
    _PyUnicodeWriter_Dealloc(&buf.writer);
    return NULL;
}

static PyObject *
mbstreamreader_read(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *sizeobj = NULL;
    Py_ssize_t size;

    if (!PyArg_UnpackTuple(args, "read", 0, 1, &sizeobj))
        return NULL;

    if (sizeobj == Py_None || sizeobj == NULL)
        size = -1;
    else if (PyLong_Check(sizeobj))
        size = PyLong_AsSsize_t(sizeobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (size == -1 && PyErr_Occurred())
        return NULL;

    return mbstreamreader_iread(self, "read", size);
}

static PyObject *
mbstreamreader_readline(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *sizeobj = NULL;
    Py_ssize_t size;

    if (!PyArg_UnpackTuple(args, "readline", 0, 1, &sizeobj))
        return NULL;

    if (sizeobj == Py_None || sizeobj == NULL)
        size = -1;
    else if (PyLong_Check(sizeobj))
        size = PyLong_AsSsize_t(sizeobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (size == -1 && PyErr_Occurred())
        return NULL;

    return mbstreamreader_iread(self, "readline", size);
}

static PyObject *
mbstreamreader_readlines(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *sizehintobj = NULL, *r, *sr;
    Py_ssize_t sizehint;

    if (!PyArg_UnpackTuple(args, "readlines", 0, 1, &sizehintobj))
        return NULL;

    if (sizehintobj == Py_None || sizehintobj == NULL)
        sizehint = -1;
    else if (PyLong_Check(sizehintobj))
        sizehint = PyLong_AsSsize_t(sizehintobj);
    else {
        PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
        return NULL;
    }

    if (sizehint == -1 && PyErr_Occurred())
        return NULL;

    r = mbstreamreader_iread(self, "read", sizehint);
    if (r == NULL)
        return NULL;

    sr = PyUnicode_Splitlines(r, 1);
    Py_DECREF(r);
    return sr;
}

static PyObject *
mbstreamreader_reset(MultibyteStreamReaderObject *self)
{
    if (self->codec->decreset != NULL &&
        self->codec->decreset(&self->state, self->codec->config) != 0)
        return NULL;
    self->pendingsize = 0;

    Py_RETURN_NONE;
}

static struct PyMethodDef mbstreamreader_methods[] = {
    {"read",            (PyCFunction)mbstreamreader_read,
                    METH_VARARGS, NULL},
    {"readline",        (PyCFunction)mbstreamreader_readline,
                    METH_VARARGS, NULL},
    {"readlines",       (PyCFunction)mbstreamreader_readlines,
                    METH_VARARGS, NULL},
    {"reset",           (PyCFunction)mbstreamreader_reset,
                    METH_NOARGS, NULL},
    {NULL,              NULL},
};

static PyMemberDef mbstreamreader_members[] = {
    {"stream",          T_OBJECT,
                    offsetof(MultibyteStreamReaderObject, stream),
                    READONLY, NULL},
    {NULL,}
};

static PyObject *
mbstreamreader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteStreamReaderObject *self;
    PyObject *stream, *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s:StreamReader",
                            streamkwarglist, &stream, &errors))
        return NULL;

    self = (MultibyteStreamReaderObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;
    if (!MultibyteCodec_Check(codec)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->stream = stream;
    Py_INCREF(stream);
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->decinit != NULL &&
        self->codec->decinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbstreamreader_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbstreamreader_traverse(MultibyteStreamReaderObject *self,
                        visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    Py_VISIT(self->stream);
    return 0;
}

static void
mbstreamreader_dealloc(MultibyteStreamReaderObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_XDECREF(self->stream);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject MultibyteStreamReader_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MultibyteStreamReader",            /* tp_name */
    sizeof(MultibyteStreamReaderObject), /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /*  methods  */
    (destructor)mbstreamreader_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
        | Py_TPFLAGS_BASETYPE,          /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)mbstreamreader_traverse,      /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iterext */
    mbstreamreader_methods,             /* tp_methods */
    mbstreamreader_members,             /* tp_members */
    codecctx_getsets,                   /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    mbstreamreader_init,                /* tp_init */
    0,                                  /* tp_alloc */
    mbstreamreader_new,                 /* tp_new */
};


/**
 * MultibyteStreamWriter object
 */

static int
mbstreamwriter_iwrite(MultibyteStreamWriterObject *self,
                      PyObject *unistr)
{
    PyObject *str, *wr;
    _Py_IDENTIFIER(write);

    str = encoder_encode_stateful(STATEFUL_ECTX(self), unistr, 0);
    if (str == NULL)
        return -1;

    wr = _PyObject_CallMethodId(self->stream, &PyId_write, "O", str);
    Py_DECREF(str);
    if (wr == NULL)
        return -1;

    Py_DECREF(wr);
    return 0;
}

static PyObject *
mbstreamwriter_write(MultibyteStreamWriterObject *self, PyObject *strobj)
{
    if (mbstreamwriter_iwrite(self, strobj))
        return NULL;
    else
        Py_RETURN_NONE;
}

static PyObject *
mbstreamwriter_writelines(MultibyteStreamWriterObject *self, PyObject *lines)
{
    PyObject *strobj;
    int i, r;

    if (!PySequence_Check(lines)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg must be a sequence object");
        return NULL;
    }

    for (i = 0; i < PySequence_Length(lines); i++) {
        /* length can be changed even within this loop */
        strobj = PySequence_GetItem(lines, i);
        if (strobj == NULL)
            return NULL;

        r = mbstreamwriter_iwrite(self, strobj);
        Py_DECREF(strobj);
        if (r == -1)
            return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
mbstreamwriter_reset(MultibyteStreamWriterObject *self)
{
    const Py_UNICODE *pending;
    PyObject *pwrt;

    pending = self->pending;
    pwrt = multibytecodec_encode(self->codec, &self->state,
                    &pending, self->pendingsize, self->errors,
                    MBENC_FLUSH | MBENC_RESET);
    /* some pending buffer can be truncated when UnicodeEncodeError is
     * raised on 'strict' mode. but, 'reset' method is designed to
     * reset the pending buffer or states so failed string sequence
     * ought to be missed */
    self->pendingsize = 0;
    if (pwrt == NULL)
        return NULL;

    assert(PyBytes_Check(pwrt));
    if (PyBytes_Size(pwrt) > 0) {
        PyObject *wr;
        _Py_IDENTIFIER(write);

        wr = _PyObject_CallMethodId(self->stream, &PyId_write, "O", pwrt);
        if (wr == NULL) {
            Py_DECREF(pwrt);
            return NULL;
        }
    }
    Py_DECREF(pwrt);

    Py_RETURN_NONE;
}

static PyObject *
mbstreamwriter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MultibyteStreamWriterObject *self;
    PyObject *stream, *codec = NULL;
    char *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|s:StreamWriter",
                            streamkwarglist, &stream, &errors))
        return NULL;

    self = (MultibyteStreamWriterObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    codec = PyObject_GetAttrString((PyObject *)type, "codec");
    if (codec == NULL)
        goto errorexit;
    if (!MultibyteCodec_Check(codec)) {
        PyErr_SetString(PyExc_TypeError, "codec is unexpected type");
        goto errorexit;
    }

    self->codec = ((MultibyteCodecObject *)codec)->codec;
    self->stream = stream;
    Py_INCREF(stream);
    self->pendingsize = 0;
    self->errors = internal_error_callback(errors);
    if (self->errors == NULL)
        goto errorexit;
    if (self->codec->encinit != NULL &&
        self->codec->encinit(&self->state, self->codec->config) != 0)
        goto errorexit;

    Py_DECREF(codec);
    return (PyObject *)self;

errorexit:
    Py_XDECREF(self);
    Py_XDECREF(codec);
    return NULL;
}

static int
mbstreamwriter_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static int
mbstreamwriter_traverse(MultibyteStreamWriterObject *self,
                        visitproc visit, void *arg)
{
    if (ERROR_ISCUSTOM(self->errors))
        Py_VISIT(self->errors);
    Py_VISIT(self->stream);
    return 0;
}

static void
mbstreamwriter_dealloc(MultibyteStreamWriterObject *self)
{
    PyObject_GC_UnTrack(self);
    ERROR_DECREF(self->errors);
    Py_XDECREF(self->stream);
    Py_TYPE(self)->tp_free(self);
}

static struct PyMethodDef mbstreamwriter_methods[] = {
    {"write",           (PyCFunction)mbstreamwriter_write,
                    METH_O, NULL},
    {"writelines",      (PyCFunction)mbstreamwriter_writelines,
                    METH_O, NULL},
    {"reset",           (PyCFunction)mbstreamwriter_reset,
                    METH_NOARGS, NULL},
    {NULL,              NULL},
};

static PyMemberDef mbstreamwriter_members[] = {
    {"stream",          T_OBJECT,
                    offsetof(MultibyteStreamWriterObject, stream),
                    READONLY, NULL},
    {NULL,}
};

static PyTypeObject MultibyteStreamWriter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MultibyteStreamWriter",            /* tp_name */
    sizeof(MultibyteStreamWriterObject), /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /*  methods  */
    (destructor)mbstreamwriter_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
        | Py_TPFLAGS_BASETYPE,          /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)mbstreamwriter_traverse,      /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iterext */
    mbstreamwriter_methods,             /* tp_methods */
    mbstreamwriter_members,             /* tp_members */
    codecctx_getsets,                   /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    mbstreamwriter_init,                /* tp_init */
    0,                                  /* tp_alloc */
    mbstreamwriter_new,                 /* tp_new */
};


/**
 * Exposed factory function
 */

static PyObject *
__create_codec(PyObject *ignore, PyObject *arg)
{
    MultibyteCodecObject *self;
    MultibyteCodec *codec;

    if (!PyCapsule_IsValid(arg, PyMultibyteCodec_CAPSULE_NAME)) {
        PyErr_SetString(PyExc_ValueError, "argument type invalid");
        return NULL;
    }

    codec = PyCapsule_GetPointer(arg, PyMultibyteCodec_CAPSULE_NAME);
    if (codec->codecinit != NULL && codec->codecinit(codec->config) != 0)
        return NULL;

    self = PyObject_New(MultibyteCodecObject, &MultibyteCodec_Type);
    if (self == NULL)
        return NULL;
    self->codec = codec;

    return (PyObject *)self;
}

static struct PyMethodDef __methods[] = {
    {"__create_codec", (PyCFunction)__create_codec, METH_O},
    {NULL, NULL},
};


static struct PyModuleDef _multibytecodecmodule = {
    PyModuleDef_HEAD_INIT,
    "_multibytecodec",
    NULL,
    -1,
    __methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__multibytecodec(void)
{
    int i;
    PyObject *m;
    PyTypeObject *typelist[] = {
        &MultibyteIncrementalEncoder_Type,
        &MultibyteIncrementalDecoder_Type,
        &MultibyteStreamReader_Type,
        &MultibyteStreamWriter_Type,
        NULL
    };

    if (PyType_Ready(&MultibyteCodec_Type) < 0)
        return NULL;

    m = PyModule_Create(&_multibytecodecmodule);
    if (m == NULL)
        return NULL;

    for (i = 0; typelist[i] != NULL; i++) {
        if (PyType_Ready(typelist[i]) < 0)
            return NULL;
        Py_INCREF(typelist[i]);
        PyModule_AddObject(m, typelist[i]->tp_name,
                           (PyObject *)typelist[i]);
    }

    if (PyErr_Occurred()) {
        Py_FatalError("can't initialize the _multibytecodec module");
        Py_DECREF(m);
        m = NULL;
    }
    return m;
}
