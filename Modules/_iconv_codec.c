/*
 * _iconv_codec.c
 *
 * libiconv adaptor for Python iconvcodec
 *
 * Author  : Hye-Shik Chang <perky@FreeBSD.org>
 * Created : 17 January 2003
 */

#include "Python.h"
#include <string.h>
#include <iconv.h>

static const char *__version__ = "$Revision$";

#if Py_USING_UNICODE
# if Py_UNICODE_SIZE == 2
#  ifdef __GNU_LIBRARY__
#   define UNICODE_ENCODING        "ucs-2"
#  else
#   define UNICODE_ENCODING        "ucs-2-internal"
#  endif
#  define MBENCODED_LENGTH_MAX    4
# elif Py_UNICODE_SIZE == 4
#  ifdef __GNU_LIBRARY__
#   define UNICODE_ENCODING        "ucs-4"
#  else
#   define UNICODE_ENCODING        "ucs-4-internal"
#  endif
#  define MBENCODED_LENGTH_MAX    6
# endif
#else
# error "Unicode is not available"
#endif

typedef struct {
    PyObject_HEAD
    iconv_t  enchdl, dechdl;
    char    *encoding;
} iconvcodecObject;
PyDoc_STRVAR(iconvcodec_doc, "iconvcodec object");

staticforward PyTypeObject iconvcodec_Type;


#define ERROR_STRICT                (PyObject *)(1)
#define ERROR_IGNORE                (PyObject *)(2)
#define ERROR_REPLACE               (PyObject *)(3)
#define ERROR_MAX                   ERROR_REPLACE

#define REPLACEMENT_CHAR_DECODE     0xFFFD
#define REPLACEMENT_CHAR_ENCODE     '?'

#define DEFAULT_ENCODING            "utf-8"


static PyObject *
get_errorcallback(const char *errors)
{
    if (errors == NULL || strcmp(errors, "strict") == 0)
        return ERROR_STRICT;
    else if (strcmp(errors, "ignore") == 0)
        return ERROR_IGNORE;
    else if (strcmp(errors, "replace") == 0)
        return ERROR_REPLACE;
    else
        return PyCodec_LookupError(errors);
}


PyDoc_STRVAR(iconvcodec_encode__doc__,
"I.encode(unicode, [,errors]) -> (string, length consumed)\n\
\n\
Return an encoded string version of `unicode'. errors may be given to\n\
set a different error handling scheme. Default is 'strict' meaning that\n\
encoding errors raise a UnicodeEncodeError. Other possible values are\n\
'ignore', 'replace' and 'xmlcharrefreplace' as well as any other name\n\
registered with codecs.register_error that can handle UnicodeEncodeErrors.");

static PyObject *
iconvcodec_encode(iconvcodecObject *self, PyObject *args, PyObject *kwargs)
{
    static char         *kwlist[] = { "input", "errors", NULL };
    Py_UNICODE          *input;
    int                  inputlen;
    char                *errors = NULL/*strict*/, *out, *out_top;
    const char          *inp, *inp_top;
    size_t               inplen, inplen_total, outlen, outlen_total, estep;
    PyObject            *outputobj = NULL, *errorcb = NULL,
                        *exceptionobj = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "u#|s:encode",
                kwlist, &input, &inputlen, &errors))
        return NULL; /* TypeError */

    errorcb = get_errorcallback(errors);
    if (errorcb == NULL)
        return NULL; /* LookupError or something else from error handler */

    inp = inp_top = (char *)input;
    inplen = inplen_total = (size_t)(inputlen * Py_UNICODE_SIZE);

    outlen = inputlen * MBENCODED_LENGTH_MAX;
    if (outlen < 16)
        outlen = 16; /* for iso-2022 codecs */

    outputobj = PyString_FromStringAndSize(NULL, outlen);
    if (outputobj == NULL)
        return NULL;
    out = out_top = PyString_AS_STRING(outputobj);
    outlen_total = outlen;

    estep = inputlen * Py_UNICODE_SIZE / 2;

#define RESIZE_OUTBUFFER(size) {                            \
    size_t   toadd = (size);                                \
    outlen_total += toadd;                                  \
    outlen += toadd;                                        \
    if (_PyString_Resize(&outputobj, outlen_total) == -1)   \
        goto errorexit;                                     \
    out = PyString_AS_STRING(outputobj) + (out - out_top);  \
    out_top = PyString_AS_STRING(outputobj);                \
}
    while (inplen > 0) {
        if (iconv(self->enchdl, &inp, &inplen, &out, &outlen) == -1) {
            char         reason[128];
            int          errpos;

            if (errno == E2BIG) {
                RESIZE_OUTBUFFER(estep);
                continue;
            }

            if (errorcb == ERROR_IGNORE || errorcb == ERROR_REPLACE) {
                inplen -= Py_UNICODE_SIZE;
                inp += Py_UNICODE_SIZE;
                if (errorcb == ERROR_REPLACE) {
                    if (outlen < 1)
                        RESIZE_OUTBUFFER(errno == EINVAL ?  1 : estep);
                    outlen--;
                    *out++ = REPLACEMENT_CHAR_ENCODE;
                }
                if (errno == EINVAL) break;
                else continue;
            }

            errpos = (int)(inp - inp_top) / Py_UNICODE_SIZE;
            sprintf(reason, "Undefined character map from "
#if Py_UNICODE_SIZE == 2
                        "\\u%04x"
#elif Py_UNICODE_SIZE == 4
                        "\\u%08x"
#endif
                        , *(Py_UNICODE *)inp);

            if (exceptionobj == NULL) {
                if ((exceptionobj = PyUnicodeEncodeError_Create(
                                        self->encoding, input, inputlen,
                                        errpos, errpos + 1, reason)) == NULL)
                    goto errorexit;
            } else {
                if (PyUnicodeEncodeError_SetStart(exceptionobj, errpos) != 0)
                    goto errorexit;
                if (PyUnicodeEncodeError_SetEnd(exceptionobj, errpos + 1) != 0)
                    goto errorexit;
                if (PyUnicodeEncodeError_SetReason(exceptionobj, reason) != 0)
                    goto errorexit;
            }

            if (errorcb == ERROR_STRICT) {
                PyCodec_StrictErrors(exceptionobj);
                goto errorexit;
            } else {
                PyObject        *argsobj, *retobj, *retuni;
                long             newpos;

                argsobj = PyTuple_New(1);
                if (argsobj == NULL)
                    goto errorexit;
                PyTuple_SET_ITEM(argsobj, 0, exceptionobj);
                Py_INCREF(exceptionobj);
                retobj = PyObject_CallObject(errorcb, argsobj);
                Py_DECREF(argsobj);
                if (retobj == NULL)
                    goto errorexit;

                if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
                    !PyUnicode_Check((retuni = PyTuple_GET_ITEM(retobj, 0))) ||
                    !PyInt_Check(PyTuple_GET_ITEM(retobj, 1))) {
                    Py_DECREF(retobj);
                    PyErr_SetString(PyExc_ValueError, "encoding error handler "
                                    "must return (unicode, int) tuple");
                    goto errorexit;
                }
                if (PyUnicode_GET_SIZE(retuni) > 0) {
#define errorexit errorexit_cbpad
                    PyObject    *retstr = NULL;
                    int          retstrsize;

                    retstr = PyUnicode_AsEncodedString(
                                    retuni, self->encoding, NULL);
                    if (retstr == NULL || !PyString_Check(retstr))
                        goto errorexit;

                    retstrsize = PyString_GET_SIZE(retstr);
                    if (outlen < retstrsize)
                        RESIZE_OUTBUFFER(errno == EINVAL || retstrsize > estep
                                         ? retstrsize - outlen : estep);

                    memcpy(out, PyString_AS_STRING(retstr), retstrsize);
                    out += retstrsize;
                    outlen -= retstrsize;
#undef errorexit
                    if (0) {
errorexit_cbpad:        Py_XDECREF(retobj);
                        Py_XDECREF(retstr);
                        goto errorexit;
                    }
                    Py_DECREF(retstr);
                }

                newpos = PyInt_AS_LONG(PyTuple_GET_ITEM(retobj, 1));
                Py_DECREF(retobj);

                if (newpos < 0)
                    newpos = inputlen - newpos;
                if (newpos < 0 || newpos >= inputlen)
                    break;
                inp = inp_top + Py_UNICODE_SIZE * newpos;
                inplen = inplen_total - Py_UNICODE_SIZE * newpos;
            }
        } else
            break;
    }
#undef RESIZE_OUTBUFFER

    {
        PyObject    *rettup;
        int          finalsize;

        finalsize = (int)(out - out_top);

        if (finalsize != outlen_total) {
            if (_PyString_Resize(&outputobj, finalsize) == -1)
                goto errorexit;
        }

        if (errorcb > ERROR_MAX) {
            Py_DECREF(errorcb);
        }
        Py_XDECREF(exceptionobj);

        rettup = PyTuple_New(2);
        if (rettup == NULL) {
            Py_DECREF(outputobj);
            return NULL;
        }
        PyTuple_SET_ITEM(rettup, 0, outputobj);
        PyTuple_SET_ITEM(rettup, 1, PyInt_FromLong(inputlen));
        return rettup;
    }

errorexit:
    Py_XDECREF(outputobj);
    if (errorcb > ERROR_MAX) {
        Py_DECREF(errorcb);
    }
    Py_XDECREF(exceptionobj);

    return NULL;
}

PyDoc_STRVAR(iconvcodec_decode__doc__,
"I.decode(string, [,errors]) -> (unicodeobject, length consumed)\n\
\n\
Decodes `string' using I, an iconvcodec instance. errors may be given\n\
to set a different error handling scheme. Default is 'strict' meaning\n\
that encoding errors raise a UnicodeDecodeError. Other possible values\n\
are 'ignore' and 'replace' as well as any other name registerd with\n\
codecs.register_error that is able to handle UnicodeDecodeErrors.");

static PyObject *
iconvcodec_decode(iconvcodecObject *self, PyObject *args, PyObject *kwargs)
{
    static char         *kwlist[] = { "input", "errors", NULL };
    char                *errors = NULL/*strict*/, *out, *out_top;
    const char          *inp, *inp_top;
    int                  inplen_int;
    size_t               inplen, inplen_total, outlen, outlen_total, estep;
    PyObject            *outputobj = NULL, *errorcb = NULL,
                        *exceptionobj = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s:decode",
                kwlist, &inp, &inplen_int, &errors))
        return NULL; /* TypeError */

    errorcb = get_errorcallback(errors);
    if (errorcb == NULL)
        return NULL; /* LookupError or something else from error handler */

    inp_top = inp;
    inplen_total = inplen = (size_t)inplen_int;

    outputobj = PyUnicode_FromUnicode(NULL, inplen);
    if (outputobj == NULL)
        return NULL;
    outlen_total = outlen = PyUnicode_GET_DATA_SIZE(outputobj);
    out = out_top = (char *)PyUnicode_AS_UNICODE(outputobj);

    estep = outlen / 2;

#define RESIZE_OUTBUFFER(size) {                                            \
    size_t   toadd = (size);                                                \
    outlen_total += toadd;                                                  \
    outlen += toadd;                                                        \
    if (PyUnicode_Resize(&outputobj, outlen_total/Py_UNICODE_SIZE) == -1)   \
        goto errorexit;                                                     \
    out = (char *)PyUnicode_AS_UNICODE(outputobj) + (out - out_top);        \
    out_top = (char *)PyUnicode_AS_UNICODE(outputobj);                      \
}
    while (inplen > 0) {
        if (iconv(self->dechdl, &inp, &inplen, &out, &outlen) == -1) {
            char         reason[128], *reasonpos = (char *)reason;
            int          errpos;

            if (errno == E2BIG) {
                RESIZE_OUTBUFFER(estep);
                continue;
            }

            if (errorcb == ERROR_IGNORE || errorcb == ERROR_REPLACE) {
                inplen--; inp++;
                if (errorcb == ERROR_REPLACE) {
                    Py_UNICODE      *replp;

                    if (outlen < Py_UNICODE_SIZE)
                        RESIZE_OUTBUFFER(
                            errno == EINVAL || Py_UNICODE_SIZE > estep
                            ? Py_UNICODE_SIZE : estep);

                    /* some compilers hate casted lvalue */
                    replp   = (Py_UNICODE *)out;
                    assert((long)replp % Py_UNICODE_SIZE == 0);/* aligned? */
                    *replp  = REPLACEMENT_CHAR_DECODE;

                    out     += Py_UNICODE_SIZE;
                    outlen  -= Py_UNICODE_SIZE;
                }
                if (errno == EINVAL) break;
                else continue;
            }

            errpos = (int)(inp - inp_top);
            reasonpos += sprintf(reason, "Invalid multibyte sequence \\x%02x",
                                            (unsigned char)*inp);
            if (inplen > 1) {
                reasonpos += sprintf(reasonpos,
                                     "\\x%02x", (unsigned char)*(inp+1));
                if (inplen > 2)
                    sprintf(reasonpos, "\\x%02x", (unsigned char)*(inp+2));
            }

            if (exceptionobj == NULL) {
                exceptionobj = PyUnicodeDecodeError_Create(
                                    self->encoding, inp_top, inplen_total,
                                    errpos, errpos + 1, reason);
                if (exceptionobj == NULL)
                    goto errorexit;
            } else {
                if (PyUnicodeDecodeError_SetStart(exceptionobj, errpos) != 0)
                    goto errorexit;
                if (PyUnicodeDecodeError_SetEnd(exceptionobj, errpos + 1) != 0)
                    goto errorexit;
                if (PyUnicodeDecodeError_SetReason(exceptionobj, reason) != 0)
                    goto errorexit;
            }

            if (errorcb == ERROR_STRICT) {
                PyCodec_StrictErrors(exceptionobj);
                goto errorexit;
            } else {
                PyObject        *argsobj, *retobj, *retuni;
                long             newpos;

                argsobj = PyTuple_New(1);
                if (argsobj == NULL)
                    goto errorexit;
                PyTuple_SET_ITEM(argsobj, 0, exceptionobj);
                Py_INCREF(exceptionobj);
                retobj = PyObject_CallObject(errorcb, argsobj);
                Py_DECREF(argsobj);
                if (retobj == NULL)
                    goto errorexit;

                if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
                    !PyUnicode_Check((retuni = PyTuple_GET_ITEM(retobj, 0))) ||
                    !PyInt_Check(PyTuple_GET_ITEM(retobj, 1))) {
                    Py_DECREF(retobj);
                    PyErr_SetString(PyExc_ValueError, "decoding error handler "
                                    "must return (unicode, int) tuple");
                    goto errorexit;
                }
                if (PyUnicode_GET_SIZE(retuni) > 0) {
#define errorexit errorexit_cbpad
                    size_t       retunisize;

                    retunisize = PyUnicode_GET_DATA_SIZE(retuni);
                    if (outlen < retunisize)
                        RESIZE_OUTBUFFER(errno == EINVAL || retunisize > estep
                                         ? retunisize - outlen : estep);

                    memcpy(out, PyUnicode_AS_DATA(retuni), retunisize);
                    out += retunisize;
                    outlen -= retunisize;
#undef errorexit
                    if (0) {
errorexit_cbpad:        Py_DECREF(retobj);
                        goto errorexit;
                    }
                }

                newpos = PyInt_AS_LONG(PyTuple_GET_ITEM(retobj, 1));
                Py_DECREF(retobj);

                if (newpos < 0)
                    newpos = inplen_total - newpos;
                if (newpos < 0 || newpos >= inplen_total)
                    break;
                inp = inp_top + newpos;
                inplen = inplen_total - newpos;
            }
        } else
            break;
    }
#undef RESIZE_OUTBUFFER

    {
        PyObject    *rettup;
        int          finalsize;

        finalsize = (int)(out - out_top);
        if (finalsize != outlen_total) {
            if (PyUnicode_Resize(&outputobj, finalsize / Py_UNICODE_SIZE) == -1)
                goto errorexit;
        }

        if (errorcb > ERROR_MAX) {
            Py_DECREF(errorcb);
        }
        Py_XDECREF(exceptionobj);

        rettup = PyTuple_New(2);
        if (rettup == NULL) {
            Py_DECREF(outputobj);
            return NULL;
        }
        PyTuple_SET_ITEM(rettup, 0, outputobj);
        PyTuple_SET_ITEM(rettup, 1, PyInt_FromLong(inplen_total));
        return rettup;
    }

errorexit:
    Py_XDECREF(outputobj);
    if (errorcb > ERROR_MAX) {
        Py_DECREF(errorcb);
    }
    Py_XDECREF(exceptionobj);

    return NULL;
}

static struct PyMethodDef iconvcodec_methods[] = {
    {"encode",      (PyCFunction)iconvcodec_encode,
                    METH_VARARGS | METH_KEYWORDS,
                    iconvcodec_encode__doc__},
    {"decode",      (PyCFunction)iconvcodec_decode,
                    METH_VARARGS | METH_KEYWORDS,
                    iconvcodec_decode__doc__},
    {NULL, NULL},
};

static PyObject *
iconvcodec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject            *encobj = NULL;
    iconvcodecObject    *new = NULL;

    new = (iconvcodecObject *)type->tp_alloc(type, 0);
    if (new == NULL)
        return NULL;

    new->encoding = NULL;
    new->enchdl = new->dechdl = (iconv_t)(-1);

    encobj = PyObject_GetAttrString((PyObject *)new, "encoding");
    if (encobj == NULL) {
        PyErr_Clear();
        new->encoding = PyMem_Malloc(sizeof(DEFAULT_ENCODING));
        strcpy(new->encoding, DEFAULT_ENCODING);
    } else if (!PyString_Check(encobj)) {
        Py_DECREF(encobj);
        PyErr_SetString(PyExc_TypeError, 
                        "`encoding' attribute must be a string.");
        goto errorexit;
    } else {
        new->encoding = PyMem_Malloc(PyString_GET_SIZE(encobj) + 1);
        strcpy(new->encoding, PyString_AS_STRING(encobj));
        Py_DECREF(encobj);
    }

    new->dechdl = iconv_open(UNICODE_ENCODING, new->encoding);
    if (new->dechdl == (iconv_t)(-1)) {
        PyErr_SetString(PyExc_ValueError, "unsupported decoding");
        goto errorexit;
    }

    new->enchdl = iconv_open(new->encoding, UNICODE_ENCODING);
    if (new->enchdl == (iconv_t)(-1)) {
        PyErr_SetString(PyExc_ValueError, "unsupported encoding");
        iconv_close(new->dechdl);
        new->dechdl = (iconv_t)(-1);
        goto errorexit;
    }

    return (PyObject *)new;

errorexit:
    Py_XDECREF(new);

    return NULL;
}

static void
iconvcodec_dealloc(iconvcodecObject *self)
{
    _PyObject_GC_UNTRACK(self);

    if (self->enchdl != (iconv_t)-1)
        iconv_close(self->enchdl);
    if (self->dechdl != (iconv_t)-1)
        iconv_close(self->dechdl);
    if (self->encoding != NULL)
        PyMem_Free(self->encoding);

    PyObject_GC_Del(self);
}

static PyObject *
iconvcodec_repr(PyObject *self)
{
    return PyString_FromFormat("<iconvcodec encoding='%s'>",
                    ((iconvcodecObject *)self)->encoding);
}

statichere PyTypeObject iconvcodec_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                              /* Number of items for varobject */
    "iconvcodec",                   /* Name of this type */
    sizeof(iconvcodecObject),       /* Basic object size */
    0,                              /* Item size for varobject */
    (destructor)iconvcodec_dealloc, /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    iconvcodec_repr,                /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
             Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    iconvcodec_doc,                 /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iterext */
    iconvcodec_methods,             /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    PyType_GenericAlloc,            /* tp_alloc */
    iconvcodec_new,                 /* tp_new */
    PyObject_GC_Del,                /* tp_free */
};

static struct PyMethodDef _iconv_codec_methods[] = {
    {NULL, NULL},
};

void
init_iconv_codec(void)
{
    PyObject *m;

    m = Py_InitModule("_iconv_codec", _iconv_codec_methods);

    PyModule_AddStringConstant(m, "__version__", (char*)__version__);
    PyModule_AddObject(m, "iconvcodec", (PyObject *)(&iconvcodec_Type));
    PyModule_AddStringConstant(m, "internal_encoding", UNICODE_ENCODING);

    if (PyErr_Occurred())
        Py_FatalError("can't initialize the _iconv_codec module");
}

/*
 * ex: ts=8 sts=4 et
 * $Id$
 */
