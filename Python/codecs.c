/* ------------------------------------------------------------------------

   Python Codec Registry and support functions

Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include "Python.h"
#include <ctype.h>

/* --- Globals ------------------------------------------------------------ */

static PyObject *_PyCodec_SearchPath;
static PyObject *_PyCodec_SearchCache;

/* Flag used for lazy import of the standard encodings package */
static int import_encodings_called = 0;

/* --- Codec Registry ----------------------------------------------------- */

/* Import the standard encodings package which will register the first
   codec search function. 

   This is done in a lazy way so that the Unicode implementation does
   not downgrade startup time of scripts not needing it.

   ImportErrors are silently ignored by this function. Only one try is
   made.

*/

static
int import_encodings(void)
{
    PyObject *mod;
    
    import_encodings_called = 1;
    mod = PyImport_ImportModule("encodings");
    if (mod == NULL) {
	if (PyErr_ExceptionMatches(PyExc_ImportError)) {
	    /* Ignore ImportErrors... this is done so that
	       distributions can disable the encodings package. Note
	       that other errors are not masked, e.g. SystemErrors
	       raised to inform the user of an error in the Python
	       configuration are still reported back to the user. */
	    PyErr_Clear();
	    return 0;
	}
	return -1;
    }
    Py_DECREF(mod);
    return 0;
}

int PyCodec_Register(PyObject *search_function)
{
    if (!import_encodings_called) {
	if (import_encodings())
	    goto onError;
    }
    if (search_function == NULL) {
	PyErr_BadArgument();
	goto onError;
    }
    if (!PyCallable_Check(search_function)) {
	PyErr_SetString(PyExc_TypeError,
			"argument must be callable");
	goto onError;
    }
    return PyList_Append(_PyCodec_SearchPath, search_function);

 onError:
    return -1;
}

/* Convert a string to a normalized Python string: all characters are
   converted to lower case, spaces are replaced with underscores. */

static
PyObject *normalizestring(const char *string)
{
    register size_t i;
    size_t len = strlen(string);
    char *p;
    PyObject *v;
    
	if (len > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError, "string is too large");
		return NULL;
	}
	
    v = PyString_FromStringAndSize(NULL, (int)len);
    if (v == NULL)
	return NULL;
    p = PyString_AS_STRING(v);
    for (i = 0; i < len; i++) {
        register char ch = string[i];
        if (ch == ' ')
            ch = '-';
        else
            ch = tolower(ch);
	p[i] = ch;
    }
    return v;
}

/* Lookup the given encoding and return a tuple providing the codec
   facilities.

   The encoding string is looked up converted to all lower-case
   characters. This makes encodings looked up through this mechanism
   effectively case-insensitive.

   If no codec is found, a LookupError is set and NULL returned. 

   As side effect, this tries to load the encodings package, if not
   yet done. This is part of the lazy load strategy for the encodings
   package.

*/

PyObject *_PyCodec_Lookup(const char *encoding)
{
    PyObject *result, *args = NULL, *v;
    int i, len;

    if (encoding == NULL) {
	PyErr_BadArgument();
	goto onError;
    }
    if (_PyCodec_SearchCache == NULL || 
	_PyCodec_SearchPath == NULL) {
	PyErr_SetString(PyExc_SystemError,
			"codec module not properly initialized");
	goto onError;
    }
    if (!import_encodings_called) {
	if (import_encodings())
	    goto onError;
    }

    /* Convert the encoding to a normalized Python string: all
       characters are converted to lower case, spaces and hyphens are
       replaced with underscores. */
    v = normalizestring(encoding);
    if (v == NULL)
	goto onError;
    PyString_InternInPlace(&v);

    /* First, try to lookup the name in the registry dictionary */
    result = PyDict_GetItem(_PyCodec_SearchCache, v);
    if (result != NULL) {
	Py_INCREF(result);
	Py_DECREF(v);
	return result;
    }
    
    /* Next, scan the search functions in order of registration */
    args = PyTuple_New(1);
    if (args == NULL)
	goto onError;
    PyTuple_SET_ITEM(args,0,v);

    len = PyList_Size(_PyCodec_SearchPath);
    if (len < 0)
	goto onError;
    if (len == 0) {
	PyErr_SetString(PyExc_LookupError,
			"no codec search functions registered: "
			"can't find encoding");
	goto onError;
    }

    for (i = 0; i < len; i++) {
	PyObject *func;
	
	func = PyList_GetItem(_PyCodec_SearchPath, i);
	if (func == NULL)
	    goto onError;
	result = PyEval_CallObject(func, args);
	if (result == NULL)
	    goto onError;
	if (result == Py_None) {
	    Py_DECREF(result);
	    continue;
	}
	if (!PyTuple_Check(result) || PyTuple_GET_SIZE(result) != 4) {
	    PyErr_SetString(PyExc_TypeError,
			    "codec search functions must return 4-tuples");
	    Py_DECREF(result);
	    goto onError;
	}
	break;
    }
    if (i == len) {
	/* XXX Perhaps we should cache misses too ? */
	PyErr_SetString(PyExc_LookupError,
			"unknown encoding");
	goto onError;
    }

    /* Cache and return the result */
    PyDict_SetItem(_PyCodec_SearchCache, v, result);
    Py_DECREF(args);
    return result;

 onError:
    Py_XDECREF(args);
    return NULL;
}

static
PyObject *args_tuple(PyObject *object,
		     const char *errors)
{
    PyObject *args;
    
    args = PyTuple_New(1 + (errors != NULL));
    if (args == NULL)
	return NULL;
    Py_INCREF(object);
    PyTuple_SET_ITEM(args,0,object);
    if (errors) {
	PyObject *v;
	
	v = PyString_FromString(errors);
	if (v == NULL) {
	    Py_DECREF(args);
	    return NULL;
	}
	PyTuple_SET_ITEM(args, 1, v);
    }
    return args;
}

/* Build a codec by calling factory(stream[,errors]) or just
   factory(errors) depending on whether the given parameters are
   non-NULL. */

static
PyObject *build_stream_codec(PyObject *factory,
			     PyObject *stream,
			     const char *errors)
{
    PyObject *args, *codec;

    args = args_tuple(stream, errors);
    if (args == NULL)
	return NULL;
    
    codec = PyEval_CallObject(factory, args);
    Py_DECREF(args);
    return codec;
}

/* Convenience APIs to query the Codec registry. 
   
   All APIs return a codec object with incremented refcount.
   
 */

PyObject *PyCodec_Encoder(const char *encoding)
{
    PyObject *codecs;
    PyObject *v;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
	goto onError;
    v = PyTuple_GET_ITEM(codecs,0);
    Py_INCREF(v);
    return v;

 onError:
    return NULL;
}

PyObject *PyCodec_Decoder(const char *encoding)
{
    PyObject *codecs;
    PyObject *v;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
	goto onError;
    v = PyTuple_GET_ITEM(codecs,1);
    Py_INCREF(v);
    return v;

 onError:
    return NULL;
}

PyObject *PyCodec_StreamReader(const char *encoding,
			       PyObject *stream,
			       const char *errors)
{
    PyObject *codecs;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
	goto onError;
    return build_stream_codec(PyTuple_GET_ITEM(codecs,2),stream,errors);

 onError:
    return NULL;
}

PyObject *PyCodec_StreamWriter(const char *encoding,
			       PyObject *stream,
			       const char *errors)
{
    PyObject *codecs;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
	goto onError;
    return build_stream_codec(PyTuple_GET_ITEM(codecs,3),stream,errors);

 onError:
    return NULL;
}

/* Encode an object (e.g. an Unicode object) using the given encoding
   and return the resulting encoded object (usually a Python string).

   errors is passed to the encoder factory as argument if non-NULL. */

PyObject *PyCodec_Encode(PyObject *object,
			 const char *encoding,
			 const char *errors)
{
    PyObject *encoder = NULL;
    PyObject *args = NULL, *result;
    PyObject *v;

    encoder = PyCodec_Encoder(encoding);
    if (encoder == NULL)
	goto onError;

    args = args_tuple(object, errors);
    if (args == NULL)
	goto onError;
    
    result = PyEval_CallObject(encoder,args);
    if (result == NULL)
	goto onError;

    if (!PyTuple_Check(result) || 
	PyTuple_GET_SIZE(result) != 2) {
	PyErr_SetString(PyExc_TypeError,
			"encoder must return a tuple (object,integer)");
	goto onError;
    }
    v = PyTuple_GET_ITEM(result,0);
    Py_INCREF(v);
    /* We don't check or use the second (integer) entry. */

    Py_DECREF(args);
    Py_DECREF(encoder);
    Py_DECREF(result);
    return v;
	
 onError:
    Py_XDECREF(args);
    Py_XDECREF(encoder);
    return NULL;
}

/* Decode an object (usually a Python string) using the given encoding
   and return an equivalent object (e.g. an Unicode object).

   errors is passed to the decoder factory as argument if non-NULL. */

PyObject *PyCodec_Decode(PyObject *object,
			 const char *encoding,
			 const char *errors)
{
    PyObject *decoder = NULL;
    PyObject *args = NULL, *result = NULL;
    PyObject *v;

    decoder = PyCodec_Decoder(encoding);
    if (decoder == NULL)
	goto onError;

    args = args_tuple(object, errors);
    if (args == NULL)
	goto onError;
    
    result = PyEval_CallObject(decoder,args);
    if (result == NULL)
	goto onError;
    if (!PyTuple_Check(result) || 
	PyTuple_GET_SIZE(result) != 2) {
	PyErr_SetString(PyExc_TypeError,
			"decoder must return a tuple (object,integer)");
	goto onError;
    }
    v = PyTuple_GET_ITEM(result,0);
    Py_INCREF(v);
    /* We don't check or use the second (integer) entry. */

    Py_DECREF(args);
    Py_DECREF(decoder);
    Py_DECREF(result);
    return v;
	
 onError:
    Py_XDECREF(args);
    Py_XDECREF(decoder);
    Py_XDECREF(result);
    return NULL;
}

void _PyCodecRegistry_Init(void)
{
    if (_PyCodec_SearchPath == NULL)
	_PyCodec_SearchPath = PyList_New(0);
    if (_PyCodec_SearchCache == NULL)
	_PyCodec_SearchCache = PyDict_New();
    if (_PyCodec_SearchPath == NULL || 
	_PyCodec_SearchCache == NULL)
	Py_FatalError("can't initialize codec registry");
}

void _PyCodecRegistry_Fini(void)
{
    Py_XDECREF(_PyCodec_SearchPath);
    _PyCodec_SearchPath = NULL;
    Py_XDECREF(_PyCodec_SearchCache);
    _PyCodec_SearchCache = NULL;
}
