/* ------------------------------------------------------------------------

   _codecs -- Provides access to the codec registry and the builtin
              codecs.

   This module should never be imported directly. The standard library
   module "codecs" wraps this builtin module for use within Python.

   The codec registry is accessible via:

     register(search_function) -> None

     lookup(encoding) -> (encoder, decoder, stream_reader, stream_writer)

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

#include "Python.h"

/* --- Registry ----------------------------------------------------------- */

static
PyObject *codecregister(PyObject *self, PyObject *args)
{
    PyObject *search_function;

    if (!PyArg_ParseTuple(args, "O:register", &search_function))
        goto onError;

    if (PyCodec_Register(search_function))
	goto onError;
    
    Py_INCREF(Py_None);
    return Py_None;

 onError:
    return NULL;
}

static
PyObject *codeclookup(PyObject *self, PyObject *args)
{
    char *encoding;

    if (!PyArg_ParseTuple(args, "s:lookup", &encoding))
        goto onError;

    return _PyCodec_Lookup(encoding);

 onError:
    return NULL;
}

#ifdef Py_USING_UNICODE
/* --- Helpers ------------------------------------------------------------ */

static
PyObject *codec_tuple(PyObject *unicode,
		      int len)
{
    PyObject *v,*w;
    
    if (unicode == NULL)
	return NULL;
    v = PyTuple_New(2);
    if (v == NULL) {
	Py_DECREF(unicode);
	return NULL;
    }
    PyTuple_SET_ITEM(v,0,unicode);
    w = PyInt_FromLong(len);
    if (w == NULL) {
	Py_DECREF(v);
	return NULL;
    }
    PyTuple_SET_ITEM(v,1,w);
    return v;
}

/* --- Decoder ------------------------------------------------------------ */

static PyObject *
unicode_internal_decode(PyObject *self,
			PyObject *args)
{
    PyObject *obj;
    const char *errors = NULL;
    const char *data;
    int size;
    
    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_decode",
			  &obj, &errors))
	return NULL;

    if (PyUnicode_Check(obj))
	return codec_tuple(obj, PyUnicode_GET_SIZE(obj));
    else {
	if (PyObject_AsReadBuffer(obj, (const void **)&data, &size))
	    return NULL;
	return codec_tuple(PyUnicode_FromUnicode((Py_UNICODE *)data,
						 size / sizeof(Py_UNICODE)),
			   size);
    }
}

static PyObject *
utf_7_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    int size;
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
    int size;
    const char *errors = NULL;
    
    if (!PyArg_ParseTuple(args, "t#|z:utf_8_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeUTF8(data, size, errors),
		       size);
}

static PyObject *
utf_16_decode(PyObject *self,
	    PyObject *args)
{
    const char *data;
    int size;
    const char *errors = NULL;
    int byteorder = 0;
    
    if (!PyArg_ParseTuple(args, "t#|z:utf_16_decode",
			  &data, &size, &errors))
	return NULL;
    return codec_tuple(PyUnicode_DecodeUTF16(data, size, errors, &byteorder),
		       size);
}

static PyObject *
utf_16_le_decode(PyObject *self,
		 PyObject *args)
{
    const char *data;
    int size;
    const char *errors = NULL;
    int byteorder = -1;
    
    if (!PyArg_ParseTuple(args, "t#|z:utf_16_le_decode",
			  &data, &size, &errors))
	return NULL;
    return codec_tuple(PyUnicode_DecodeUTF16(data, size, errors, &byteorder),
		       size);
}

static PyObject *
utf_16_be_decode(PyObject *self,
		 PyObject *args)
{
    const char *data;
    int size;
    const char *errors = NULL;
    int byteorder = 1;
    
    if (!PyArg_ParseTuple(args, "t#|z:utf_16_be_decode",
			  &data, &size, &errors))
	return NULL;
    return codec_tuple(PyUnicode_DecodeUTF16(data, size, errors, &byteorder),
		       size);
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
    int size;
    const char *errors = NULL;
    int byteorder = 0;
    PyObject *unicode, *tuple;
    
    if (!PyArg_ParseTuple(args, "t#|zi:utf_16_ex_decode",
			  &data, &size, &errors, &byteorder))
	return NULL;

    unicode = PyUnicode_DecodeUTF16(data, size, errors, &byteorder);
    if (unicode == NULL)
	return NULL;
    tuple = Py_BuildValue("Oii", unicode, size, byteorder);
    Py_DECREF(unicode);
    return tuple;
}

static PyObject *
unicode_escape_decode(PyObject *self,
		     PyObject *args)
{
    const char *data;
    int size;
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
    int size;
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
    int size;
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
    int size;
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
    int size;
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
    int size;
    const char *errors = NULL;
    
    if (!PyArg_ParseTuple(args, "t#|z:mbcs_decode",
			  &data, &size, &errors))
	return NULL;

    return codec_tuple(PyUnicode_DecodeMBCS(data, size, errors),
		       size);
}

#endif /* MS_WINDOWS */

/* --- Encoder ------------------------------------------------------------ */

static PyObject *
readbuffer_encode(PyObject *self,
		  PyObject *args)
{
    const char *data;
    int size;
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
    int size;
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
    int size;
    
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

/* --- Module API --------------------------------------------------------- */

static PyMethodDef _codecs_functions[] = {
    {"register",		codecregister,			METH_VARARGS},
    {"lookup",			codeclookup, 			METH_VARARGS},
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
    {"readbuffer_encode",	readbuffer_encode,		METH_VARARGS},
    {"charbuffer_encode",	charbuffer_encode,		METH_VARARGS},
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
    {"mbcs_encode", 		mbcs_encode,			METH_VARARGS},
    {"mbcs_decode", 		mbcs_decode,			METH_VARARGS},
#endif
#endif /* Py_USING_UNICODE */
    {NULL, NULL}		/* sentinel */
};

PyMODINIT_FUNC
init_codecs(void)
{
    Py_InitModule("_codecs", _codecs_functions);
}
