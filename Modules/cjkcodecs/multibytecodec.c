/*
 * multibytecodec.c: Common Multibyte Codec Implementation
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: multibytecodec.c,v 1.13 2004/08/19 16:57:19 perky Exp $
 */

#include "Python.h"
#include "multibytecodec.h"


typedef struct {
    const Py_UNICODE    *inbuf, *inbuf_top, *inbuf_end;
    unsigned char       *outbuf, *outbuf_end;
    PyObject            *excobj, *outobj;
} MultibyteEncodeBuffer;

typedef struct {
    const unsigned char *inbuf, *inbuf_top, *inbuf_end;
    Py_UNICODE          *outbuf, *outbuf_end;
    PyObject            *excobj, *outobj;
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
are 'ignore' and 'replace' as well as any other name registerd with\n\
codecs.register_error that is able to handle UnicodeDecodeErrors.");

PyDoc_STRVAR(MultibyteCodec_StreamReader__doc__,
"I.StreamReader(stream[, errors]) -> StreamReader instance");

PyDoc_STRVAR(MultibyteCodec_StreamWriter__doc__,
"I.StreamWriter(stream[, errors]) -> StreamWriter instance");

static char *codeckwarglist[] = {"input", "errors", NULL};
static char *streamkwarglist[] = {"stream", "errors", NULL};

static PyObject *multibytecodec_encode(MultibyteCodec *,
		MultibyteCodec_State *, const Py_UNICODE **, size_t,
		PyObject *, int);
static PyObject *mbstreamreader_create(MultibyteCodec *,
		PyObject *, const char *);
static PyObject *mbstreamwriter_create(MultibyteCodec *,
		PyObject *, const char *);

#define MBENC_RESET	MBENC_MAX<<1 /* reset after an encoding session */

static PyObject *
make_tuple(PyObject *unicode, int len)
{
	PyObject *v, *w;

	if (unicode == NULL)
		return NULL;

	v = PyTuple_New(2);
	if (v == NULL) {
		Py_DECREF(unicode);
		return NULL;
	}
	PyTuple_SET_ITEM(v, 0, unicode);

	w = PyInt_FromLong(len);
	if (w == NULL) {
		Py_DECREF(v);
		return NULL;
	}
	PyTuple_SET_ITEM(v, 1, w);

	return v;
}

static PyObject *
get_errorcallback(const char *errors)
{
	if (errors == NULL || strcmp(errors, "strict") == 0)
		return ERROR_STRICT;
	else if (strcmp(errors, "ignore") == 0)
		return ERROR_IGNORE;
	else if (strcmp(errors, "replace") == 0)
		return ERROR_REPLACE;
	else {
		return PyCodec_LookupError(errors);
	}
}

static int
expand_encodebuffer(MultibyteEncodeBuffer *buf, int esize)
{
	int orgpos, orgsize;

	orgpos = (int)((char*)buf->outbuf - PyString_AS_STRING(buf->outobj));
	orgsize = PyString_GET_SIZE(buf->outobj);
	if (_PyString_Resize(&buf->outobj, orgsize + (
	    esize < (orgsize >> 1) ? (orgsize >> 1) | 1 : esize)) == -1)
		return -1;

	buf->outbuf = (unsigned char *)PyString_AS_STRING(buf->outobj) +orgpos;
	buf->outbuf_end = (unsigned char *)PyString_AS_STRING(buf->outobj)
		+ PyString_GET_SIZE(buf->outobj);

	return 0;
}
#define REQUIRE_ENCODEBUFFER(buf, s) {					\
	if ((s) < 1 || (buf)->outbuf + (s) > (buf)->outbuf_end)		\
		if (expand_encodebuffer(buf, s) == -1)			\
			goto errorexit;					\
}

static int
expand_decodebuffer(MultibyteDecodeBuffer *buf, int esize)
{
	int orgpos, orgsize;

	orgpos = (int)(buf->outbuf - PyUnicode_AS_UNICODE(buf->outobj));
	orgsize = PyUnicode_GET_SIZE(buf->outobj);
	if (PyUnicode_Resize(&buf->outobj, orgsize + (
	    esize < (orgsize >> 1) ? (orgsize >> 1) | 1 : esize)) == -1)
		return -1;

	buf->outbuf = PyUnicode_AS_UNICODE(buf->outobj) + orgpos;
	buf->outbuf_end = PyUnicode_AS_UNICODE(buf->outobj)
			  + PyUnicode_GET_SIZE(buf->outobj);

	return 0;
}
#define REQUIRE_DECODEBUFFER(buf, s) {					\
	if ((s) < 1 || (buf)->outbuf + (s) > (buf)->outbuf_end)		\
		if (expand_decodebuffer(buf, s) == -1)			\
			goto errorexit;					\
}

static int
multibytecodec_encerror(MultibyteCodec *codec,
			MultibyteCodec_State *state,
			MultibyteEncodeBuffer *buf,
			PyObject *errors, int e)
{
	PyObject *retobj = NULL, *retstr = NULL, *argsobj, *tobj;
	int retstrsize, newpos;
	const char *reason;
	size_t esize;
	int start, end;

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
			esize = (size_t)(buf->inbuf_end - buf->inbuf);
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
		int r;

		for (;;) {
			size_t outleft;

			outleft = (size_t)(buf->outbuf_end - buf->outbuf);
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

	start = (int)(buf->inbuf - buf->inbuf_top);
	end = start + esize;

	/* use cached exception object if available */
	if (buf->excobj == NULL) {
		buf->excobj = PyUnicodeEncodeError_Create(codec->encoding,
				buf->inbuf_top,
				(int)(buf->inbuf_end - buf->inbuf_top),
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

	argsobj = PyTuple_New(1);
	if (argsobj == NULL)
		goto errorexit;

	PyTuple_SET_ITEM(argsobj, 0, buf->excobj);
	Py_INCREF(buf->excobj);
	retobj = PyObject_CallObject(errors, argsobj);
	Py_DECREF(argsobj);
	if (retobj == NULL)
		goto errorexit;

	if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
	    !PyUnicode_Check((tobj = PyTuple_GET_ITEM(retobj, 0))) ||
	    !PyInt_Check(PyTuple_GET_ITEM(retobj, 1))) {
		PyErr_SetString(PyExc_ValueError,
				"encoding error handler must return "
				"(unicode, int) tuple");
		goto errorexit;
	}

	{
		const Py_UNICODE *uraw = PyUnicode_AS_UNICODE(tobj);

		retstr = multibytecodec_encode(codec, state, &uraw,
				PyUnicode_GET_SIZE(tobj), ERROR_STRICT,
				MBENC_FLUSH);
		if (retstr == NULL)
			goto errorexit;
	}

	retstrsize = PyString_GET_SIZE(retstr);
	REQUIRE_ENCODEBUFFER(buf, retstrsize);

	memcpy(buf->outbuf, PyString_AS_STRING(retstr), retstrsize);
	buf->outbuf += retstrsize;

	newpos = (int)PyInt_AS_LONG(PyTuple_GET_ITEM(retobj, 1));
	if (newpos < 0)
		newpos += (int)(buf->inbuf_end - buf->inbuf_top);
	if (newpos < 0 || buf->inbuf_top + newpos > buf->inbuf_end) {
		PyErr_Format(PyExc_IndexError,
			     "position %d from error handler out of bounds",
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
			PyObject *errors, int e)
{
	PyObject *argsobj, *retobj = NULL, *retuni = NULL;
	int retunisize, newpos;
	const char *reason;
	size_t esize;
	int start, end;

	if (e > 0) {
		reason = "illegal multibyte sequence";
		esize = e;
	}
	else {
		switch (e) {
		case MBERR_TOOSMALL:
			REQUIRE_DECODEBUFFER(buf, -1);
			return 0; /* retry it */
		case MBERR_TOOFEW:
			reason = "incomplete multibyte sequence";
			esize = (size_t)(buf->inbuf_end - buf->inbuf);
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
		REQUIRE_DECODEBUFFER(buf, 1);
		*buf->outbuf++ = Py_UNICODE_REPLACEMENT_CHARACTER;
	}
	if (errors == ERROR_IGNORE || errors == ERROR_REPLACE) {
		buf->inbuf += esize;
		return 0;
	}

	start = (int)(buf->inbuf - buf->inbuf_top);
	end = start + esize;

	/* use cached exception object if available */
	if (buf->excobj == NULL) {
		buf->excobj = PyUnicodeDecodeError_Create(codec->encoding,
				(const char *)buf->inbuf_top,
				(int)(buf->inbuf_end - buf->inbuf_top),
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

	argsobj = PyTuple_New(1);
	if (argsobj == NULL)
		goto errorexit;

	PyTuple_SET_ITEM(argsobj, 0, buf->excobj);
	Py_INCREF(buf->excobj);
	retobj = PyObject_CallObject(errors, argsobj);
	Py_DECREF(argsobj);
	if (retobj == NULL)
		goto errorexit;

	if (!PyTuple_Check(retobj) || PyTuple_GET_SIZE(retobj) != 2 ||
	    !PyUnicode_Check((retuni = PyTuple_GET_ITEM(retobj, 0))) ||
	    !PyInt_Check(PyTuple_GET_ITEM(retobj, 1))) {
		PyErr_SetString(PyExc_ValueError,
				"decoding error handler must return "
				"(unicode, int) tuple");
		goto errorexit;
	}

	retunisize = PyUnicode_GET_SIZE(retuni);
	if (retunisize > 0) {
		REQUIRE_DECODEBUFFER(buf, retunisize);
		memcpy((char *)buf->outbuf, PyUnicode_AS_DATA(retuni),
				retunisize * Py_UNICODE_SIZE);
		buf->outbuf += retunisize;
	}

	newpos = (int)PyInt_AS_LONG(PyTuple_GET_ITEM(retobj, 1));
	if (newpos < 0)
		newpos += (int)(buf->inbuf_end - buf->inbuf_top);
	if (newpos < 0 || buf->inbuf_top + newpos > buf->inbuf_end) {
		PyErr_Format(PyExc_IndexError,
				"position %d from error handler out of bounds",
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
		      const Py_UNICODE **data, size_t datalen,
		      PyObject *errors, int flags)
{
	MultibyteEncodeBuffer buf;
	int finalsize, r = 0;

	if (datalen == 0)
		return PyString_FromString("");

	buf.excobj = NULL;
	buf.inbuf = buf.inbuf_top = *data;
	buf.inbuf_end = buf.inbuf_top + datalen;
	buf.outobj = PyString_FromStringAndSize(NULL, datalen * 2 + 16);
	if (buf.outobj == NULL)
		goto errorexit;
	buf.outbuf = (unsigned char *)PyString_AS_STRING(buf.outobj);
	buf.outbuf_end = buf.outbuf + PyString_GET_SIZE(buf.outobj);

	while (buf.inbuf < buf.inbuf_end) {
		size_t inleft, outleft;

		/* we don't reuse inleft and outleft here.
		 * error callbacks can relocate the cursor anywhere on buffer*/
		inleft = (size_t)(buf.inbuf_end - buf.inbuf);
		outleft = (size_t)(buf.outbuf_end - buf.outbuf);
		r = codec->encode(state, codec->config, &buf.inbuf, inleft,
				  &buf.outbuf, outleft, flags);
		*data = buf.inbuf;
		if ((r == 0) || (r == MBERR_TOOFEW && !(flags & MBENC_FLUSH)))
			break;
		else if (multibytecodec_encerror(codec, state, &buf, errors,r))
			goto errorexit;
		else if (r == MBERR_TOOFEW)
			break;
	}

	if (codec->encreset != NULL)
		for (;;) {
			size_t outleft;

			outleft = (size_t)(buf.outbuf_end - buf.outbuf);
			r = codec->encreset(state, codec->config, &buf.outbuf,
					    outleft);
			if (r == 0)
				break;
			else if (multibytecodec_encerror(codec, state,
							 &buf, errors, r))
				goto errorexit;
		}

	finalsize = (int)((char*)buf.outbuf - PyString_AS_STRING(buf.outobj));

	if (finalsize != PyString_GET_SIZE(buf.outobj))
		if (_PyString_Resize(&buf.outobj, finalsize) == -1)
			goto errorexit;

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
	int datalen;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|z:encode",
				codeckwarglist, &arg, &errors))
		return NULL;

	if (PyUnicode_Check(arg))
		ucvt = NULL;
	else {
		arg = ucvt = PyObject_Unicode(arg);
		if (arg == NULL)
			return NULL;
		else if (!PyUnicode_Check(arg)) {
			PyErr_SetString(PyExc_TypeError,
				"couldn't convert the object to unicode.");
			Py_DECREF(ucvt);
			return NULL;
		}
	}

	data = PyUnicode_AS_UNICODE(arg);
	datalen = PyUnicode_GET_SIZE(arg);

	errorcb = get_errorcallback(errors);
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

	if (errorcb > ERROR_MAX) {
		Py_DECREF(errorcb);
	}
	Py_XDECREF(ucvt);
	return make_tuple(r, datalen);

errorexit:
	if (errorcb > ERROR_MAX) {
		Py_DECREF(errorcb);
	}
	Py_XDECREF(ucvt);
	return NULL;
}

static PyObject *
MultibyteCodec_Decode(MultibyteCodecObject *self,
		      PyObject *args, PyObject *kwargs)
{
	MultibyteCodec_State state;
	MultibyteDecodeBuffer buf;
	PyObject *errorcb;
	const char *data, *errors = NULL;
	int datalen, finalsize;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|z:decode",
				codeckwarglist, &data, &datalen, &errors))
		return NULL;

	errorcb = get_errorcallback(errors);
	if (errorcb == NULL)
		return NULL;

	if (datalen == 0) {
		if (errorcb > ERROR_MAX) {
			Py_DECREF(errorcb);
		}
		return make_tuple(PyUnicode_FromUnicode(NULL, 0), 0);
	}

	buf.outobj = buf.excobj = NULL;
	buf.inbuf = buf.inbuf_top = (unsigned char *)data;
	buf.inbuf_end = buf.inbuf_top + datalen;
	buf.outobj = PyUnicode_FromUnicode(NULL, datalen);
	if (buf.outobj == NULL)
		goto errorexit;
	buf.outbuf = PyUnicode_AS_UNICODE(buf.outobj);
	buf.outbuf_end = buf.outbuf + PyUnicode_GET_SIZE(buf.outobj);

	if (self->codec->decinit != NULL &&
	    self->codec->decinit(&state, self->codec->config) != 0)
		goto errorexit;

	while (buf.inbuf < buf.inbuf_end) {
		size_t inleft, outleft;
		int r;

		inleft = (size_t)(buf.inbuf_end - buf.inbuf);
		outleft = (size_t)(buf.outbuf_end - buf.outbuf);

		r = self->codec->decode(&state, self->codec->config,
				&buf.inbuf, inleft, &buf.outbuf, outleft);
		if (r == 0)
			break;
		else if (multibytecodec_decerror(self->codec, &state,
						 &buf, errorcb, r))
			goto errorexit;
	}

	finalsize = (int)(buf.outbuf - PyUnicode_AS_UNICODE(buf.outobj));

	if (finalsize != PyUnicode_GET_SIZE(buf.outobj))
		if (PyUnicode_Resize(&buf.outobj, finalsize) == -1)
			goto errorexit;

	Py_XDECREF(buf.excobj);
	if (errorcb > ERROR_MAX) {
		Py_DECREF(errorcb);
	}
	return make_tuple(buf.outobj, datalen);

errorexit:
	if (errorcb > ERROR_MAX) {
		Py_DECREF(errorcb);
	}
	Py_XDECREF(buf.excobj);
	Py_XDECREF(buf.outobj);

	return NULL;
}

static PyObject *
MultibyteCodec_StreamReader(MultibyteCodecObject *self,
			    PyObject *args, PyObject *kwargs)
{
	PyObject *stream;
	char *errors = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|s:StreamReader",
				streamkwarglist, &stream, &errors))
		return NULL;

	return mbstreamreader_create(self->codec, stream, errors);
}

static PyObject *
MultibyteCodec_StreamWriter(MultibyteCodecObject *self,
			    PyObject *args, PyObject *kwargs)
{
	PyObject *stream;
	char *errors = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|s:StreamWriter",
				streamkwarglist, &stream, &errors))
		return NULL;

	return mbstreamwriter_create(self->codec, stream, errors);
}

static struct PyMethodDef multibytecodec_methods[] = {
	{"encode",	(PyCFunction)MultibyteCodec_Encode,
			METH_VARARGS | METH_KEYWORDS,
			MultibyteCodec_Encode__doc__},
	{"decode",	(PyCFunction)MultibyteCodec_Decode,
			METH_VARARGS | METH_KEYWORDS,
			MultibyteCodec_Decode__doc__},
	{"StreamReader",(PyCFunction)MultibyteCodec_StreamReader,
			METH_VARARGS | METH_KEYWORDS,
			MultibyteCodec_StreamReader__doc__},
	{"StreamWriter",(PyCFunction)MultibyteCodec_StreamWriter,
			METH_VARARGS | METH_KEYWORDS,
			MultibyteCodec_StreamWriter__doc__},
	{NULL,		NULL},
};

static void
multibytecodec_dealloc(MultibyteCodecObject *self)
{
	PyObject_Del(self);
}



static PyTypeObject MultibyteCodec_Type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"MultibyteCodec",		/* tp_name */
	sizeof(MultibyteCodecObject),	/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)multibytecodec_dealloc, /* tp_dealloc */
	0,				/* tp_print */
	0,	/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	0,				/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iterext */
	multibytecodec_methods,		/* tp_methods */
};

static PyObject *
mbstreamreader_iread(MultibyteStreamReaderObject *self,
		     const char *method, int sizehint)
{
	MultibyteDecodeBuffer buf;
	PyObject *cres;
	int rsize, r, finalsize = 0;

	if (sizehint == 0)
		return PyUnicode_FromUnicode(NULL, 0);

	buf.outobj = buf.excobj = NULL;
	cres = NULL;

	for (;;) {
		if (sizehint < 0)
			cres = PyObject_CallMethod(self->stream,
					(char *)method, NULL);
		else
			cres = PyObject_CallMethod(self->stream,
					(char *)method, "i", sizehint);
		if (cres == NULL)
			goto errorexit;

		if (!PyString_Check(cres)) {
			PyErr_SetString(PyExc_TypeError,
					"stream function returned a "
					"non-string object");
			goto errorexit;
		}

		if (self->pendingsize > 0) {
			PyObject *ctr;
			char *ctrdata;

			rsize = PyString_GET_SIZE(cres) + self->pendingsize;
			ctr = PyString_FromStringAndSize(NULL, rsize);
			if (ctr == NULL)
				goto errorexit;
			ctrdata = PyString_AS_STRING(ctr);
			memcpy(ctrdata, self->pending, self->pendingsize);
			memcpy(ctrdata + self->pendingsize,
				PyString_AS_STRING(cres),
				PyString_GET_SIZE(cres));
			Py_DECREF(cres);
			cres = ctr;
			self->pendingsize = 0;
		}

		rsize = PyString_GET_SIZE(cres);
		buf.inbuf = buf.inbuf_top =
			(unsigned char *)PyString_AS_STRING(cres);
		buf.inbuf_end = buf.inbuf_top + rsize;
		if (buf.outobj == NULL) {
			buf.outobj = PyUnicode_FromUnicode(NULL, rsize);
			if (buf.outobj == NULL)
				goto errorexit;
			buf.outbuf = PyUnicode_AS_UNICODE(buf.outobj);
			buf.outbuf_end = buf.outbuf +
					PyUnicode_GET_SIZE(buf.outobj);
		}

		r = 0;
		if (rsize > 0)
			while (buf.inbuf < buf.inbuf_end) {
				size_t inleft, outleft;

				inleft = (size_t)(buf.inbuf_end - buf.inbuf);
				outleft = (size_t)(buf.outbuf_end -buf.outbuf);

				r = self->codec->decode(&self->state,
							self->codec->config,
							&buf.inbuf, inleft,
							&buf.outbuf, outleft);
				if (r == 0 || r == MBERR_TOOFEW)
					break;
				else if (multibytecodec_decerror(self->codec,
						&self->state, &buf,
						self->errors, r))
					goto errorexit;
			}

		if (rsize == 0 || sizehint < 0) { /* end of file */
			if (buf.inbuf < buf.inbuf_end &&
			    multibytecodec_decerror(self->codec, &self->state,
					&buf, self->errors, MBERR_TOOFEW))
				goto errorexit;
		}

		if (buf.inbuf < buf.inbuf_end) { /* pending sequence exists */
			size_t npendings;

			/* we can't assume that pendingsize is still 0 here.
			 * because this function can be called recursively
			 * from error callback */
			npendings = (size_t)(buf.inbuf_end - buf.inbuf);
			if (npendings + self->pendingsize > MAXDECPENDING) {
				PyErr_SetString(PyExc_RuntimeError,
						"pending buffer overflow");
				goto errorexit;
			}
			memcpy(self->pending + self->pendingsize, buf.inbuf,
				npendings);
			self->pendingsize += npendings;
		}

		finalsize = (int)(buf.outbuf -
				PyUnicode_AS_UNICODE(buf.outobj));
		Py_DECREF(cres);
		cres = NULL;

		if (sizehint < 0 || finalsize != 0 || rsize == 0)
			break;

		sizehint = 1; /* read 1 more byte and retry */
	}

	if (finalsize != PyUnicode_GET_SIZE(buf.outobj))
		if (PyUnicode_Resize(&buf.outobj, finalsize) == -1)
			goto errorexit;

	Py_XDECREF(cres);
	Py_XDECREF(buf.excobj);
	return buf.outobj;

errorexit:
	Py_XDECREF(cres);
	Py_XDECREF(buf.excobj);
	Py_XDECREF(buf.outobj);
	return NULL;
}

static PyObject *
mbstreamreader_read(MultibyteStreamReaderObject *self, PyObject *args)
{
	PyObject *sizeobj = NULL;
	long size;

	if (!PyArg_ParseTuple(args, "|O:read", &sizeobj))
		return NULL;

	if (sizeobj == Py_None || sizeobj == NULL)
		size = -1;
	else if (PyInt_Check(sizeobj))
		size = PyInt_AsLong(sizeobj);
	else {
		PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
		return NULL;
	}

	return mbstreamreader_iread(self, "read", size);
}

static PyObject *
mbstreamreader_readline(MultibyteStreamReaderObject *self, PyObject *args)
{
	PyObject *sizeobj = NULL;
	long size;

	if (!PyArg_ParseTuple(args, "|O:readline", &sizeobj))
		return NULL;

	if (sizeobj == Py_None || sizeobj == NULL)
		size = -1;
	else if (PyInt_Check(sizeobj))
		size = PyInt_AsLong(sizeobj);
	else {
		PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
		return NULL;
	}

	return mbstreamreader_iread(self, "readline", size);
}

static PyObject *
mbstreamreader_readlines(MultibyteStreamReaderObject *self, PyObject *args)
{
	PyObject *sizehintobj = NULL, *r, *sr;
	long sizehint;

	if (!PyArg_ParseTuple(args, "|O:readlines", &sizehintobj))
		return NULL;

	if (sizehintobj == Py_None || sizehintobj == NULL)
		sizehint = -1;
	else if (PyInt_Check(sizehintobj))
		sizehint = PyInt_AsLong(sizehintobj);
	else {
		PyErr_SetString(PyExc_TypeError, "arg 1 must be an integer");
		return NULL;
	}

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

	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef mbstreamreader_methods[] = {
	{"read",	(PyCFunction)mbstreamreader_read,
			METH_VARARGS, NULL},
	{"readline",	(PyCFunction)mbstreamreader_readline,
			METH_VARARGS, NULL},
	{"readlines",	(PyCFunction)mbstreamreader_readlines,
			METH_VARARGS, NULL},
	{"reset",	(PyCFunction)mbstreamreader_reset,
			METH_NOARGS, NULL},
	{NULL,		NULL},
};

static void
mbstreamreader_dealloc(MultibyteStreamReaderObject *self)
{
	if (self->errors > ERROR_MAX) {
		Py_DECREF(self->errors);
	}
	Py_DECREF(self->stream);
	PyObject_Del(self);
}



static PyTypeObject MultibyteStreamReader_Type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"MultibyteStreamReader",	/* tp_name */
	sizeof(MultibyteStreamReaderObject), /* tp_basicsize */
	0,				/* tp_itemsize */
	/*  methods  */
	(destructor)mbstreamreader_dealloc, /* tp_dealloc */
	0,				/* tp_print */
	0,	/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	0,				/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iterext */
	mbstreamreader_methods,		/* tp_methods */
};

static int
mbstreamwriter_iwrite(MultibyteStreamWriterObject *self,
		      PyObject *unistr)
{
	PyObject *wr, *ucvt, *r = NULL;
	Py_UNICODE *inbuf, *inbuf_end, *inbuf_tmp = NULL;
	int datalen;

	if (PyUnicode_Check(unistr))
		ucvt = NULL;
	else {
		unistr = ucvt = PyObject_Unicode(unistr);
		if (unistr == NULL)
			return -1;
		else if (!PyUnicode_Check(unistr)) {
			PyErr_SetString(PyExc_TypeError,
				"couldn't convert the object to unicode.");
			Py_DECREF(ucvt);
			return -1;
		}
	}

	datalen = PyUnicode_GET_SIZE(unistr);
	if (datalen == 0) {
		Py_XDECREF(ucvt);
		return 0;
	}

	if (self->pendingsize > 0) {
		inbuf_tmp = PyMem_New(Py_UNICODE, datalen + self->pendingsize);
		if (inbuf_tmp == NULL)
			goto errorexit;
		memcpy(inbuf_tmp, self->pending,
			Py_UNICODE_SIZE * self->pendingsize);
		memcpy(inbuf_tmp + self->pendingsize,
			PyUnicode_AS_UNICODE(unistr),
			Py_UNICODE_SIZE * datalen);
		datalen += self->pendingsize;
		self->pendingsize = 0;
		inbuf = inbuf_tmp;
	}
	else
		inbuf = (Py_UNICODE *)PyUnicode_AS_UNICODE(unistr);

	inbuf_end = inbuf + datalen;

	r = multibytecodec_encode(self->codec, &self->state,
			(const Py_UNICODE **)&inbuf, datalen, self->errors, 0);
	if (r == NULL)
		goto errorexit;

	if (inbuf < inbuf_end) {
		self->pendingsize = (int)(inbuf_end - inbuf);
		if (self->pendingsize > MAXENCPENDING) {
			self->pendingsize = 0;
			PyErr_SetString(PyExc_RuntimeError,
					"pending buffer overflow");
			goto errorexit;
		}
		memcpy(self->pending, inbuf,
			self->pendingsize * Py_UNICODE_SIZE);
	}

	wr = PyObject_CallMethod(self->stream, "write", "O", r);
	if (wr == NULL)
		goto errorexit;

	if (inbuf_tmp != NULL)
		PyMem_Del(inbuf_tmp);
	Py_DECREF(r);
	Py_DECREF(wr);
	Py_XDECREF(ucvt);
	return 0;

errorexit:
	if (inbuf_tmp != NULL)
		PyMem_Del(inbuf_tmp);
	Py_XDECREF(r);
	Py_XDECREF(ucvt);
	return -1;
}

static PyObject *
mbstreamwriter_write(MultibyteStreamWriterObject *self, PyObject *args)
{
	PyObject *strobj;

	if (!PyArg_ParseTuple(args, "O:write", &strobj))
		return NULL;

	if (mbstreamwriter_iwrite(self, strobj))
		return NULL;
	else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

static PyObject *
mbstreamwriter_writelines(MultibyteStreamWriterObject *self, PyObject *args)
{
	PyObject *lines, *strobj;
	int i, r;

	if (!PyArg_ParseTuple(args, "O:writelines", &lines))
		return NULL;

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

	Py_INCREF(Py_None);
	return Py_None;
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

	if (PyString_Size(pwrt) > 0) {
		PyObject *wr;
		wr = PyObject_CallMethod(self->stream, "write", "O", pwrt);
		if (wr == NULL) {
			Py_DECREF(pwrt);
			return NULL;
		}
	}
	Py_DECREF(pwrt);

	Py_INCREF(Py_None);
	return Py_None;
}

static void
mbstreamwriter_dealloc(MultibyteStreamWriterObject *self)
{
	if (self->errors > ERROR_MAX) {
		Py_DECREF(self->errors);
	}
	Py_DECREF(self->stream);
	PyObject_Del(self);
}

static struct PyMethodDef mbstreamwriter_methods[] = {
	{"write",	(PyCFunction)mbstreamwriter_write,
			METH_VARARGS, NULL},
	{"writelines",	(PyCFunction)mbstreamwriter_writelines,
			METH_VARARGS, NULL},
	{"reset",	(PyCFunction)mbstreamwriter_reset,
			METH_NOARGS, NULL},
	{NULL,		NULL},
};



static PyTypeObject MultibyteStreamWriter_Type = {
	PyObject_HEAD_INIT(NULL)
	0,				/* ob_size */
	"MultibyteStreamWriter",	/* tp_name */
	sizeof(MultibyteStreamWriterObject), /* tp_basicsize */
	0,				/* tp_itemsize */
	/*  methods  */
	(destructor)mbstreamwriter_dealloc, /* tp_dealloc */
	0,				/* tp_print */
	0,	/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	0,				/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iterext */
	mbstreamwriter_methods,		/* tp_methods */
};

static PyObject *
__create_codec(PyObject *ignore, PyObject *arg)
{
	MultibyteCodecObject *self;
	MultibyteCodec *codec;

	if (!PyCObject_Check(arg)) {
		PyErr_SetString(PyExc_ValueError, "argument type invalid");
		return NULL;
	}

	codec = PyCObject_AsVoidPtr(arg);
	if (codec->codecinit != NULL && codec->codecinit(codec->config) != 0)
		return NULL;

	self = PyObject_New(MultibyteCodecObject, &MultibyteCodec_Type);
	if (self == NULL)
		return NULL;
	self->codec = codec;

	return (PyObject *)self;
}

static PyObject *
mbstreamreader_create(MultibyteCodec *codec,
		      PyObject *stream, const char *errors)
{
	MultibyteStreamReaderObject *self;

	self = PyObject_New(MultibyteStreamReaderObject,
			&MultibyteStreamReader_Type);
	if (self == NULL)
		return NULL;

	self->codec = codec;
	self->stream = stream;
	Py_INCREF(stream);
	self->pendingsize = 0;
	self->errors = get_errorcallback(errors);
	if (self->errors == NULL)
		goto errorexit;
	if (self->codec->decinit != NULL &&
	    self->codec->decinit(&self->state, self->codec->config) != 0)
		goto errorexit;

	return (PyObject *)self;

errorexit:
	Py_XDECREF(self);
	return NULL;
}

static PyObject *
mbstreamwriter_create(MultibyteCodec *codec,
		      PyObject *stream, const char *errors)
{
	MultibyteStreamWriterObject *self;

	self = PyObject_New(MultibyteStreamWriterObject,
			&MultibyteStreamWriter_Type);
	if (self == NULL)
		return NULL;

	self->codec = codec;
	self->stream = stream;
	Py_INCREF(stream);
	self->pendingsize = 0;
	self->errors = get_errorcallback(errors);
	if (self->errors == NULL)
		goto errorexit;
	if (self->codec->encinit != NULL &&
	    self->codec->encinit(&self->state, self->codec->config) != 0)
		goto errorexit;

	return (PyObject *)self;

errorexit:
	Py_XDECREF(self);
	return NULL;
}

static struct PyMethodDef __methods[] = {
	{"__create_codec", (PyCFunction)__create_codec, METH_O},
	{NULL, NULL},
};

void
init_multibytecodec(void)
{
	Py_InitModule("_multibytecodec", __methods);

	if (PyErr_Occurred())
		Py_FatalError("can't initialize the _multibytecodec module");
}
