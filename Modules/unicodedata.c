/* ------------------------------------------------------------------------

   unicodedata -- Provides access to the Unicode 3.0 data base.

   Data was extracted from the Unicode 3.0 UnicodeData.txt file.

   Written by Marc-Andre Lemburg (mal@lemburg.com).
   Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

   Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include "Python.h"
#include "unicodedatabase.h"

/* --- Module API --------------------------------------------------------- */

static PyObject *
unicodedata_decimal(PyObject *self,
		    PyObject *args)
{
    PyUnicodeObject *v;
    PyObject *defobj = NULL;
    long rc;

    if (!PyArg_ParseTuple(args, "O!|O:decimal",
			  &PyUnicode_Type, &v, &defobj))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    rc = Py_UNICODE_TODECIMAL(*PyUnicode_AS_UNICODE(v));
    if (rc < 0) {
	if (defobj == NULL) {
	    PyErr_SetString(PyExc_ValueError,
			    "not a decimal");
	    goto onError;
	}
	else {
	    Py_INCREF(defobj);
	    return defobj;
	}
    }
    return PyInt_FromLong(rc);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_digit(PyObject *self,
		  PyObject *args)
{
    PyUnicodeObject *v;
    PyObject *defobj = NULL;
    long rc;

    if (!PyArg_ParseTuple(args, "O!|O:digit",
			  &PyUnicode_Type, &v, &defobj))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    rc = Py_UNICODE_TODIGIT(*PyUnicode_AS_UNICODE(v));
    if (rc < 0) {
	if (defobj == NULL) {
	    PyErr_SetString(PyExc_ValueError,
			    "not a digit");
	    goto onError;
	}
	else {
	    Py_INCREF(defobj);
	    return defobj;
	}
    }
    return PyInt_FromLong(rc);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_numeric(PyObject *self,
		    PyObject *args)
{
    PyUnicodeObject *v;
    PyObject *defobj = NULL;
    double rc;

    if (!PyArg_ParseTuple(args, "O!|O:numeric",
			  &PyUnicode_Type, &v, &defobj))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    rc = Py_UNICODE_TONUMERIC(*PyUnicode_AS_UNICODE(v));
    if (rc < 0) {
	if (defobj == NULL) {
	    PyErr_SetString(PyExc_ValueError,
			    "not a numeric character");
	    goto onError;
	}
	else {
	    Py_INCREF(defobj);
	    return defobj;
	}
    }
    return PyFloat_FromDouble(rc);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_category(PyObject *self,
		     PyObject *args)
{
    PyUnicodeObject *v;
    int index;

    if (!PyArg_ParseTuple(args, "O!:category",
			  &PyUnicode_Type, &v))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    index = (int) _PyUnicode_Database_GetRecord(
        (int) *PyUnicode_AS_UNICODE(v)
        )->category;
    return PyString_FromString(_PyUnicode_CategoryNames[index]);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_bidirectional(PyObject *self,
			  PyObject *args)
{
    PyUnicodeObject *v;
    int index;

    if (!PyArg_ParseTuple(args, "O!:bidirectional",
			  &PyUnicode_Type, &v))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    index = (int) _PyUnicode_Database_GetRecord(
        (int) *PyUnicode_AS_UNICODE(v)
        )->bidirectional;
    return PyString_FromString(_PyUnicode_BidirectionalNames[index]);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_combining(PyObject *self,
		      PyObject *args)
{
    PyUnicodeObject *v;
    int value;

    if (!PyArg_ParseTuple(args, "O!:combining",
			  &PyUnicode_Type, &v))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    value = (int) _PyUnicode_Database_GetRecord(
        (int) *PyUnicode_AS_UNICODE(v)
        )->combining;
    return PyInt_FromLong(value);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_mirrored(PyObject *self,
		     PyObject *args)
{
    PyUnicodeObject *v;
    int value;

    if (!PyArg_ParseTuple(args, "O!:mirrored",
			  &PyUnicode_Type, &v))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    value = (int) _PyUnicode_Database_GetRecord(
        (int) *PyUnicode_AS_UNICODE(v)
        )->mirrored;
    return PyInt_FromLong(value);
    
 onError:
    return NULL;
}

static PyObject *
unicodedata_decomposition(PyObject *self,
		      PyObject *args)
{
    PyUnicodeObject *v;
    const char *value;

    if (!PyArg_ParseTuple(args, "O!:decomposition",
			  &PyUnicode_Type, &v))
	goto onError;
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
			"need a single Unicode character as parameter");
	goto onError;
    }
    value = _PyUnicode_Database_GetDecomposition(
        (int) *PyUnicode_AS_UNICODE(v)
        );
	return PyString_FromString(value);
    
 onError:
    return NULL;
}

/* XXX Add doc strings. */

static PyMethodDef unicodedata_functions[] = {
    {"decimal",		unicodedata_decimal,			1},
    {"digit",		unicodedata_digit,			1},
    {"numeric",		unicodedata_numeric,			1},
    {"category",	unicodedata_category,			1},
    {"bidirectional",	unicodedata_bidirectional,		1},
    {"combining",	unicodedata_combining,			1},
    {"mirrored",	unicodedata_mirrored,			1},
    {"decomposition",	unicodedata_decomposition,		1},
    {NULL, NULL}		/* sentinel */
};

DL_EXPORT(void)
initunicodedata(void)
{
    Py_InitModule("unicodedata", unicodedata_functions);
}
