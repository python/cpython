/* ------------------------------------------------------------------------

   _codecs -- Provides access to the codec registry and the builtin
              codecs.

   This module should never be imported directly. The standard library
   module "codecs" wraps this builtin module for use within Python.

   The codec registry is accessible via:

     register(search_function) -> None

     lookup(encoding) -> CodecInfo object

   The builtin Unicode codecs use the following interface:

     <encoding>_encode(Unicode_object[,errors='strict']) ->
     	(string object, bytes consumed)

     <encoding>_decode(char_buffer_obj[,errors='strict']) ->
        (Unicode object, bytes consumed)

   <encoding>_encode() interfaces also accept non-Unicode object as
   input. The objects are then converted to Unicode using
   PyUnicode_FromObject() prior to applying the conversion.

   These <encoding>s are available: utf_8, unicode_escape,
   raw_unicode_escape, unicode_internal, latin_1, ascii (7-bit),
   mbcs (on win32).


Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

/* --- Registry ----------------------------------------------------------- */

PyDoc_STRVAR(register__doc__,
"register(search_function)\n\
\n\
Register a codec search function. Search functions are expected to take\n\
one argument, the encoding name in all lower case letters, and return\n\
a tuple of functions (encoder, decoder, stream_reader, stream_writer)\n\
(or a CodecInfo object).");

static
PyObject *codec_register(PyObject *self, PyObject *search_function)
{
    if (PyCodec_Register(search_function))
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(lookup__doc__,
"lookup(encoding) -> CodecInfo\n\
\n\
Looks up a codec tuple in the Python codec registry and returns\n\
a tuple of function (or a CodecInfo object).");

static
PyObject *codec_lookup(PyObject *self, PyObject *args)
{
    char *encoding;

    if (!PyArg_ParseTuple(args, "s:lookup", &encoding))
        return NULL;

    return _PyCodec_Lookup(encoding);
}

PyDoc_STRVAR(encode__doc__,
"encode(obj, [encoding[,errors]]) -> object\n\
\n\
Encodes obj using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore', 'replace' and\n\
'xmlcharrefreplace' as well as any other name registered with\n\
codecs.register_error that can handle ValueErrors.");

static PyObject *
codec_encode(PyObject *self, PyObject *args)
{
    const char *encoding = NULL;
    const char *errors = NULL;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O|ss:encode", &v, &encoding, &errors))
        return NULL;

#ifdef Py_USING_UNICODE
    if (encoding == NULL)
	encoding = PyUnicode_GetDefaultEncoding();
#else
    if (encoding == NULL) {
	PyErr_SetString(PyExc_ValueError, "no encoding specified");
	return NULL;
    }
#endif

    /* Encode via the codec registry */
    return PyCodec_Encode(v, encoding, errors);
}

PyDoc_STRVAR(decode__doc__,
"decode(obj, [encoding[,errors]]) -> object\n\
\n\
Decodes obj using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore' and 'replace'\n\
as well as any other name registerd with codecs.register_error that is\n\
able to handle ValueErrors.");

static PyObject *
codec_decode(PyObject *self, PyObject *args)
{
    const char *encoding = NULL;
    const char *errors = NULL;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O|ss:decode", &v, &encoding, &errors))
        return NULL;

#ifdef Py_USING_UNICODE
    if (encoding == NULL)
	encoding = PyUnicode_GetDefaultEncoding();
#else
    if (encoding == NULL) {
	PyErr_SetString(PyExc_ValueError, "no encoding specified");
	return NULL;
    }
#endif

    /* Decode via the codec registry */
    return PyCodec_Decode(v, encoding, errors);
}

/* --- Helpers ------------------------------------------------------------ */

static
PyObject *codec_tuple(PyObject *unicode,
		      Py_ssize_t len)
{
    PyObject *v;
    if (unicode == NULL)
        return NULL;
    v = Py_BuildValue("On", unicode, len);
    Py_DECREF(unicode);
    return v;
}

/* --- String codecs ------------------------------------------------------ */
static PyObject *
escape_decode(PyObject *self,
	      PyObject *args)
{
    const char *errors = NULL;
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "s#|z:escape_decode",
			  &data, &size, &errors))
	return NULL;
    return codec_tuple(PyString_DecodeEscape(data, size, errors, 0, NULL),
		       size);
}

static PyObject *
escape_encode(PyObject *self,
	      PyObject *args)
{
	PyObject *str;
	const char *errors = NULL;
	char *buf;
	Py_ssize_t len;

	if (!PyArg_ParseTuple(args, "O!|z:escape_encode",
			      &PyString_Type, &str, &errors))
		return NULL;

	str = PyString_Repr(str, 0);
	if (!str)
		return NULL;

	/* The string will be quoted. Unquote, similar to unicode-escape. */
	buf = PyString_AS_STRING (str);
	len = PyString_GET_SIZE (str);
	memmove(buf, buf+1, len-2);
	if (_PyString_Resize(&str, len-2) < 0)
		return NULL;
	
	return codec_tuple(str, PyString_Size(str));
}

#ifdef Py_USING_UNICODE
/* --- Decoder ------------------------------------------------------------ */

static PyObject *
unicode_internal_decode(PyObject *self,
			PyObject *args)
{
    PyObject *obj;
    const char *errors = NULL;
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_decode",
			  &obj, &errors))
	return NULL;

    if (PyUnicode_Check(obj)) {
	Py_INCREF(obj);
	return codec_tuple(obj, PyUnicode_GET_SIZE(obj));
    }
    else {
	if (PyObject_AsReadBuffer(obj, (const void **)&data, &size))
	    return NULL;

	return codec_tuple(_PyUnicode_DecodeUnicodeInternal(data, size, errors),
			   size);
    }
}

static PyObject *
utf_7_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:utf_7_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeUTF7(data, size, errors),
		       size);
}

static PyObject *
utf_8_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "t#|zi:utf_8_decode",
			  &data, &size, &errors, &final))
	return NULL;
    if (size < 0) {
	    PyErr_SetString(PyExc_ValueError, "negative argument");
	    return 0;
    }
    consumed = size;
	
    decoded = PyUnicode_DecodeUTF8Stateful(data, size, errors,
					   final ? NULL : &consumed);
    if (decoded == NULL)
	return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_16_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "t#|zi:utf_16_decode",
			  &data, &size, &errors, &final))
	return NULL;
    if (size < 0) {
	    PyErr_SetString(PyExc_ValueError, "negative argument");
	    return 0;
    }
    consumed = size; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(data, size, errors, &byteorder,
					    final ? NULL : &consumed);
    if (decoded == NULL)
	return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_16_le_decode(PyObject *self,
		 PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = -1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "t#|zi:utf_16_le_decode",
			  &data, &size, &errors, &final))
	return NULL;

    if (size < 0) {
          PyErr_SetString(PyExc_ValueError, "negative argument");
          return 0;
    }
    consumed = size; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(data, size, errors,
	&byteorder, final ? NULL : &consumed);
    if (decoded == NULL)
	return NULL;
    return codec_tuple(decoded, consumed);

}

static PyObject *
utf_16_be_decode(PyObject *self,
		 PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = 1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "t#|zi:utf_16_be_decode",
			  &data, &size, &errors, &final))
	return NULL;
    if (size < 0) {
          PyErr_SetString(PyExc_ValueError, "negative argument");
          return 0;
    }
    consumed = size; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(data, size, errors,
	&byteorder, final ? NULL : &consumed);
    if (decoded == NULL)
	return NULL;
    return codec_tuple(decoded, consumed);
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-16 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/

static PyObject *
utf_16_ex_decode(PyObject *self,
		 PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    int byteorder = 0;
    PyObject *unicode, *tuple;
    int final = 0;
    Py_ssize_t consumed;

    if (!PyArg_ParseTuple(args, "t#|zii:utf_16_ex_decode",
			  &data, &size, &errors, &byteorder, &final))
	return NULL;
    if (size < 0) {
	    PyErr_SetString(PyExc_ValueError, "negative argument");
	    return 0;
    }
    consumed = size; /* This is overwritten unless final is true. */
    unicode = PyUnicode_DecodeUTF16Stateful(data, size, errors, &byteorder,
					    final ? NULL : &consumed);
    if (unicode == NULL)
	return NULL;
    tuple = Py_BuildValue("Oni", unicode, consumed, byteorder);
    Py_DECREF(unicode);
    return tuple;
}

static PyObject *
unicode_escape_decode(PyObject *self,
		     PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:unicode_escape_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeUnicodeEscape(data, size, errors),
		       size);
}

static PyObject *
raw_unicode_escape_decode(PyObject *self,
			PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:raw_unicode_escape_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeRawUnicodeEscape(data, size, errors),
		       size);
}

static PyObject *
latin_1_decode(PyObject *self,
	       PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:latin_1_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeLatin1(data, size, errors),
		       size);
}

static PyObject *
ascii_decode(PyObject *self,
	     PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:ascii_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeASCII(data, size, errors),
		       size);
}

static PyObject *
charmap_decode(PyObject *self,
	       PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "t#|zO:charmap_decode",
			  &data, &size, &errors, &mapping))
	return NULL;
    if (mapping == Py_None)
	mapping = NULL;

    return codec_tuple(PyUnicode_DecodeCharmap(data, size, mapping, errors),
		       size);
}

#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)

static PyObject *
mbcs_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    Py_ssize_t size, consumed;
    const char *errors = NULL;
    int final = 0;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "t#|zi:mbcs_decode",
			  &data, &size, &errors, &final))
	return NULL;

    decoded = PyUnicode_DecodeMBCSStateful(
	data, size, errors, final ? NULL : &consumed);
    if (!decoded)
	return NULL;
    return codec_tuple(decoded, final ? size : consumed);
}

#endif /* MS_WINDOWS */

/* --- Encoder ------------------------------------------------------------ */

static PyObject *
readbuffer_encode(PyObject *self,
		  PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "s#|z:readbuffer_encode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyString_FromStringAndSize(data, size),
		       size);
}

static PyObject *
charbuffer_encode(PyObject *self,
		  PyObject *args)
{
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "t#|z:charbuffer_encode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyString_FromStringAndSize(data, size),
		       size);
}

static PyObject *
unicode_internal_encode(PyObject *self,
			PyObject *args)
{
    PyObject *obj;
    const char *errors = NULL;
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_encode",
			  &obj, &errors))
	return NULL;

    if (PyUnicode_Check(obj)) {
	data = PyUnicode_AS_DATA(obj);
	size = PyUnicode_GET_DATA_SIZE(obj);
	return codec_tuple(PyString_FromStringAndSize(data, size),
			   size);
    }
    else {
	if (PyObject_AsReadBuffer(obj, (const void **)&data, &size))
	    return NULL;
	return codec_tuple(PyString_FromStringAndSize(data, size),
			   size);
    }
}

static PyObject *
utf_7_encode(PyObject *self,
	    PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_7_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUTF7(PyUnicode_AS_UNICODE(str),
					 PyUnicode_GET_SIZE(str),
					 0,
					 0,
					 errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_8_encode(PyObject *self,
	    PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_8_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(str),
					 PyUnicode_GET_SIZE(str),
					 errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-16 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

static PyObject *
utf_16_encode(PyObject *self,
	    PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    int byteorder = 0;

    if (!PyArg_ParseTuple(args, "O|zi:utf_16_encode",
			  &str, &errors, &byteorder))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(str),
					  PyUnicode_GET_SIZE(str),
					  errors,
					  byteorder),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_16_le_encode(PyObject *self,
		 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_16_le_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(str),
					     PyUnicode_GET_SIZE(str),
					     errors,
					     -1),
		       PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_16_be_encode(PyObject *self,
		 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_16_be_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(str),
					  PyUnicode_GET_SIZE(str),
					  errors,
					  +1),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
unicode_escape_encode(PyObject *self,
		     PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:unicode_escape_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeUnicodeEscape(PyUnicode_AS_UNICODE(str),
						  PyUnicode_GET_SIZE(str)),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
raw_unicode_escape_encode(PyObject *self,
			PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:raw_unicode_escape_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeRawUnicodeEscape(
			       PyUnicode_AS_UNICODE(str),
			       PyUnicode_GET_SIZE(str)),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
latin_1_encode(PyObject *self,
	       PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:latin_1_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeLatin1(
			       PyUnicode_AS_UNICODE(str),
			       PyUnicode_GET_SIZE(str),
			       errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
ascii_encode(PyObject *self,
	     PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:ascii_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeASCII(
			       PyUnicode_AS_UNICODE(str),
			       PyUnicode_GET_SIZE(str),
			       errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
charmap_encode(PyObject *self,
	     PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "O|zO:charmap_encode",
			  &str, &errors, &mapping))
	return NULL;
    if (mapping == Py_None)
	mapping = NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeCharmap(
			       PyUnicode_AS_UNICODE(str),
			       PyUnicode_GET_SIZE(str),
			       mapping,
			       errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

static PyObject*
charmap_build(PyObject *self, PyObject *args)
{
    PyObject *map;
    if (!PyArg_ParseTuple(args, "U:charmap_build", &map))
        return NULL;
    return PyUnicode_BuildEncodingMap(map);
}

#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)

static PyObject *
mbcs_encode(PyObject *self,
	    PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:mbcs_encode",
			  &str, &errors))
	return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return NULL;
    v = codec_tuple(PyUnicode_EncodeMBCS(
			       PyUnicode_AS_UNICODE(str),
			       PyUnicode_GET_SIZE(str),
			       errors),
		    PyUnicode_GET_SIZE(str));
    Py_DECREF(str);
    return v;
}

#endif /* MS_WINDOWS */
#endif /* Py_USING_UNICODE */

/* --- Error handler registry --------------------------------------------- */

PyDoc_STRVAR(register_error__doc__,
"register_error(errors, handler)\n\
\n\
Register the specified error handler under the name\n\
errors. handler must be a callable object, that\n\
will be called with an exception instance containing\n\
information about the location of the encoding/decoding\n\
error and must return a (replacement, new position) tuple.");

static PyObject *register_error(PyObject *self, PyObject *args)
{
    const char *name;
    PyObject *handler;

    if (!PyArg_ParseTuple(args, "sO:register_error",
			  &name, &handler))
	return NULL;
    if (PyCodec_RegisterError(name, handler))
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(lookup_error__doc__,
"lookup_error(errors) -> handler\n\
\n\
Return the error handler for the specified error handling name\n\
or raise a LookupError, if no handler exists under this name.");

static PyObject *lookup_error(PyObject *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:lookup_error",
			  &name))
	return NULL;
    return PyCodec_LookupError(name);
}

/* --- Module API --------------------------------------------------------- */

static PyMethodDef _codecs_functions[] = {
    {"register",		codec_register,			METH_O,
        register__doc__},
    {"lookup",			codec_lookup, 			METH_VARARGS,
        lookup__doc__},
    {"encode",			codec_encode,			METH_VARARGS,
	encode__doc__},
    {"decode",			codec_decode,			METH_VARARGS,
	decode__doc__},
    {"escape_encode",		escape_encode,			METH_VARARGS},
    {"escape_decode",		escape_decode,			METH_VARARGS},
#ifdef Py_USING_UNICODE
    {"utf_8_encode",		utf_8_encode,			METH_VARARGS},
    {"utf_8_decode",		utf_8_decode,			METH_VARARGS},
    {"utf_7_encode",		utf_7_encode,			METH_VARARGS},
    {"utf_7_decode",		utf_7_decode,			METH_VARARGS},
    {"utf_16_encode",		utf_16_encode,			METH_VARARGS},
    {"utf_16_le_encode",	utf_16_le_encode,		METH_VARARGS},
    {"utf_16_be_encode",	utf_16_be_encode,		METH_VARARGS},
    {"utf_16_decode",		utf_16_decode,			METH_VARARGS},
    {"utf_16_le_decode",	utf_16_le_decode,		METH_VARARGS},
    {"utf_16_be_decode",	utf_16_be_decode,		METH_VARARGS},
    {"utf_16_ex_decode",	utf_16_ex_decode,		METH_VARARGS},
    {"unicode_escape_encode",	unicode_escape_encode,		METH_VARARGS},
    {"unicode_escape_decode",	unicode_escape_decode,		METH_VARARGS},
    {"unicode_internal_encode",	unicode_internal_encode,	METH_VARARGS},
    {"unicode_internal_decode",	unicode_internal_decode,	METH_VARARGS},
    {"raw_unicode_escape_encode", raw_unicode_escape_encode,	METH_VARARGS},
    {"raw_unicode_escape_decode", raw_unicode_escape_decode,	METH_VARARGS},
    {"latin_1_encode", 		latin_1_encode,			METH_VARARGS},
    {"latin_1_decode", 		latin_1_decode,			METH_VARARGS},
    {"ascii_encode", 		ascii_encode,			METH_VARARGS},
    {"ascii_decode", 		ascii_decode,			METH_VARARGS},
    {"charmap_encode", 		charmap_encode,			METH_VARARGS},
    {"charmap_decode", 		charmap_decode,			METH_VARARGS},
    {"charmap_build", 		charmap_build,			METH_VARARGS},
    {"readbuffer_encode",	readbuffer_encode,		METH_VARARGS},
    {"charbuffer_encode",	charbuffer_encode,		METH_VARARGS},
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
    {"mbcs_encode", 		mbcs_encode,			METH_VARARGS},
    {"mbcs_decode", 		mbcs_decode,			METH_VARARGS},
#endif
#endif /* Py_USING_UNICODE */
    {"register_error", 		register_error,			METH_VARARGS,
        register_error__doc__},
    {"lookup_error", 		lookup_error,			METH_VARARGS,
        lookup_error__doc__},
    {NULL, NULL}		/* sentinel */
};

PyMODINIT_FUNC
init_codecs(void)
{
    Py_InitModule("_codecs", _codecs_functions);
}
