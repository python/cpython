/* This module provides the suite of standard class-based exceptions for
 * Python's builtin module.  This is a complete C implementation of what,
 * in Python 1.5.2, was contained in the exceptions.py module.  The problem
 * there was that if exceptions.py could not be imported for some reason,
 * the entire interpreter would abort.
 *
 * By moving the exceptions into C and statically linking, we can guarantee
 * that the standard exceptions will always be available.
 *
 * history:
 * 98-08-19    fl   created (for pyexe)
 * 00-02-08    fl   updated for 1.5.2
 * 26-May-2000 baw  vetted for Python 1.6
 *
 * written by Fredrik Lundh
 * modifications, additions, cleanups, and proofreading by Barry Warsaw
 *
 * Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
 */

#include "Python.h"
#include "osdefs.h"

/* Caution:  MS Visual C++ 6 errors if a single string literal exceeds
 * 2Kb.  So the module docstring has been broken roughly in half, using
 * compile-time literal concatenation.
 */

/* NOTE: If the exception class hierarchy changes, don't forget to update
 * Doc/lib/libexcs.tex!
 */

PyDoc_STRVAR(module__doc__,
"Python's standard exception class hierarchy.\n\
\n\
Before Python 1.5, the standard exceptions were all simple string objects.\n\
In Python 1.5, the standard exceptions were converted to classes organized\n\
into a relatively flat hierarchy.  String-based standard exceptions were\n\
optional, or used as a fallback if some problem occurred while importing\n\
the exception module.  With Python 1.6, optional string-based standard\n\
exceptions were removed (along with the -X command line flag).\n\
\n\
The class exceptions were implemented in such a way as to be almost\n\
completely backward compatible.  Some tricky uses of IOError could\n\
potentially have broken, but by Python 1.6, all of these should have\n\
been fixed.  As of Python 1.6, the class-based standard exceptions are\n\
now implemented in C, and are guaranteed to exist in the Python\n\
interpreter.\n\
\n\
Here is a rundown of the class hierarchy.  The classes found here are\n\
inserted into both the exceptions module and the `built-in' module.  It is\n\
recommended that user defined class based exceptions be derived from the\n\
`Exception' class, although this is currently not enforced.\n"
	/* keep string pieces "small" */
"\n\
Exception\n\
 |\n\
 +-- SystemExit\n\
 +-- StopIteration\n\
 +-- StandardError\n\
 |    |\n\
 |    +-- KeyboardInterrupt\n\
 |    +-- ImportError\n\
 |    +-- EnvironmentError\n\
 |    |    |\n\
 |    |    +-- IOError\n\
 |    |    +-- OSError\n\
 |    |         |\n\
 |    |         +-- WindowsError\n\
 |    |         +-- VMSError\n\
 |    |\n\
 |    +-- EOFError\n\
 |    +-- RuntimeError\n\
 |    |    |\n\
 |    |    +-- NotImplementedError\n\
 |    |\n\
 |    +-- NameError\n\
 |    |    |\n\
 |    |    +-- UnboundLocalError\n\
 |    |\n\
 |    +-- AttributeError\n\
 |    +-- SyntaxError\n\
 |    |    |\n\
 |    |    +-- IndentationError\n\
 |    |         |\n\
 |    |         +-- TabError\n\
 |    |\n\
 |    +-- TypeError\n\
 |    +-- AssertionError\n\
 |    +-- LookupError\n\
 |    |    |\n\
 |    |    +-- IndexError\n\
 |    |    +-- KeyError\n\
 |    |\n\
 |    +-- ArithmeticError\n\
 |    |    |\n\
 |    |    +-- OverflowError\n\
 |    |    +-- ZeroDivisionError\n\
 |    |    +-- FloatingPointError\n\
 |    |\n\
 |    +-- ValueError\n\
 |    |    |\n\
 |    |    +-- UnicodeError\n\
 |    |        |\n\
 |    |        +-- UnicodeEncodeError\n\
 |    |        +-- UnicodeDecodeError\n\
 |    |        +-- UnicodeTranslateError\n\
 |    |\n\
 |    +-- ReferenceError\n\
 |    +-- SystemError\n\
 |    +-- MemoryError\n\
 |\n\
 +---Warning\n\
      |\n\
      +-- UserWarning\n\
      +-- DeprecationWarning\n\
      +-- PendingDeprecationWarning\n\
      +-- SyntaxWarning\n\
      +-- OverflowWarning\n\
      +-- RuntimeWarning\n\
      +-- FutureWarning"
);


/* Helper function for populating a dictionary with method wrappers. */
static int
populate_methods(PyObject *klass, PyObject *dict, PyMethodDef *methods)
{
    PyObject *module;
    int status = -1;

    if (!methods)
	return 0;

    module = PyString_FromString("exceptions");
    if (!module)
	return 0;
    while (methods->ml_name) {
	/* get a wrapper for the built-in function */
	PyObject *func = PyCFunction_NewEx(methods, NULL, module);
	PyObject *meth;

	if (!func)
	    goto status;

	/* turn the function into an unbound method */
	if (!(meth = PyMethod_New(func, NULL, klass))) {
	    Py_DECREF(func);
	    goto status;
	}

	/* add method to dictionary */
	status = PyDict_SetItemString(dict, methods->ml_name, meth);
	Py_DECREF(meth);
	Py_DECREF(func);

	/* stop now if an error occurred, otherwise do the next method */
	if (status)
	    goto status;

	methods++;
    }
    status = 0;
 status:
    Py_DECREF(module);
    return status;
}



/* This function is used to create all subsequent exception classes. */
static int
make_class(PyObject **klass, PyObject *base,
	   char *name, PyMethodDef *methods,
	   char *docstr)
{
    PyObject *dict = PyDict_New();
    PyObject *str = NULL;
    int status = -1;

    if (!dict)
	return -1;

    /* If an error occurs from here on, goto finally instead of explicitly
     * returning NULL.
     */

    if (docstr) {
	if (!(str = PyString_FromString(docstr)))
	    goto finally;
	if (PyDict_SetItemString(dict, "__doc__", str))
	    goto finally;
    }

    if (!(*klass = PyErr_NewException(name, base, dict)))
	goto finally;

    if (populate_methods(*klass, dict, methods)) {
	Py_DECREF(*klass);
	*klass = NULL;
	goto finally;
    }

    status = 0;

  finally:
    Py_XDECREF(dict);
    Py_XDECREF(str);
    return status;
}


/* Use this for *args signatures, otherwise just use PyArg_ParseTuple() */
static PyObject *
get_self(PyObject *args)
{
    PyObject *self = PyTuple_GetItem(args, 0);
    if (!self) {
	/* Watch out for being called to early in the bootstrapping process */
	if (PyExc_TypeError) {
	    PyErr_SetString(PyExc_TypeError,
	     "unbound method must be called with instance as first argument");
	}
        return NULL;
    }
    return self;
}



/* Notes on bootstrapping the exception classes.
 *
 * First thing we create is the base class for all exceptions, called
 * appropriately enough: Exception.  Creation of this class makes no
 * assumptions about the existence of any other exception class -- except
 * for TypeError, which can conditionally exist.
 *
 * Next, StandardError is created (which is quite simple) followed by
 * TypeError, because the instantiation of other exceptions can potentially
 * throw a TypeError.  Once these exceptions are created, all the others
 * can be created in any order.  See the static exctable below for the
 * explicit bootstrap order.
 *
 * All classes after Exception can be created using PyErr_NewException().
 */

PyDoc_STRVAR(Exception__doc__, "Common base class for all exceptions.");


static PyObject *
Exception__init__(PyObject *self, PyObject *args)
{
    int status;

    if (!(self = get_self(args)))
	return NULL;

    /* set args attribute */
    /* XXX size is only a hint */
    args = PySequence_GetSlice(args, 1, PySequence_Size(args));
    if (!args)
        return NULL;
    status = PyObject_SetAttrString(self, "args", args);
    Py_DECREF(args);
    if (status < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
Exception__str__(PyObject *self, PyObject *args)
{
    PyObject *out;

    if (!PyArg_ParseTuple(args, "O:__str__", &self))
        return NULL;

    args = PyObject_GetAttrString(self, "args");
    if (!args)
        return NULL;

    switch (PySequence_Size(args)) {
    case 0:
        out = PyString_FromString("");
        break;
    case 1:
    {
	PyObject *tmp = PySequence_GetItem(args, 0);
	if (tmp) {
	    out = PyObject_Str(tmp);
	    Py_DECREF(tmp);
	}
	else
	    out = NULL;
	break;
    }
    case -1:
        PyErr_Clear();
        /* Fall through */
    default:
        out = PyObject_Str(args);
        break;
    }

    Py_DECREF(args);
    return out;
}


static PyObject *
Exception__getitem__(PyObject *self, PyObject *args)
{
    PyObject *out;
    PyObject *index;

    if (!PyArg_ParseTuple(args, "OO:__getitem__", &self, &index))
        return NULL;

    args = PyObject_GetAttrString(self, "args");
    if (!args)
        return NULL;

    out = PyObject_GetItem(args, index);
    Py_DECREF(args);
    return out;
}


static PyMethodDef
Exception_methods[] = {
    /* methods for the Exception class */
    { "__getitem__", Exception__getitem__, METH_VARARGS},
    { "__str__",     Exception__str__, METH_VARARGS},
    { "__init__",    Exception__init__, METH_VARARGS},
    { NULL, NULL }
};


static int
make_Exception(char *modulename)
{
    PyObject *dict = PyDict_New();
    PyObject *str = NULL;
    PyObject *name = NULL;
    int status = -1;

    if (!dict)
	return -1;

    /* If an error occurs from here on, goto finally instead of explicitly
     * returning NULL.
     */

    if (!(str = PyString_FromString(modulename)))
	goto finally;
    if (PyDict_SetItemString(dict, "__module__", str))
	goto finally;
    Py_DECREF(str);
    if (!(str = PyString_FromString(Exception__doc__)))
	goto finally;
    if (PyDict_SetItemString(dict, "__doc__", str))
	goto finally;

    if (!(name = PyString_FromString("Exception")))
	goto finally;

    if (!(PyExc_Exception = PyClass_New(NULL, dict, name)))
	goto finally;

    /* Now populate the dictionary with the method suite */
    if (populate_methods(PyExc_Exception, dict, Exception_methods))
	/* Don't need to reclaim PyExc_Exception here because that'll
	 * happen during interpreter shutdown.
	 */
	goto finally;

    status = 0;

  finally:
    Py_XDECREF(dict);
    Py_XDECREF(str);
    Py_XDECREF(name);
    return status;
}



PyDoc_STRVAR(StandardError__doc__,
"Base class for all standard Python exceptions.");

PyDoc_STRVAR(TypeError__doc__, "Inappropriate argument type.");

PyDoc_STRVAR(StopIteration__doc__, "Signal the end from iterator.next().");



PyDoc_STRVAR(SystemExit__doc__, "Request to exit from the interpreter.");


static PyObject *
SystemExit__init__(PyObject *self, PyObject *args)
{
    PyObject *code;
    int status;

    if (!(self = get_self(args)))
	return NULL;

    /* Set args attribute. */
    if (!(args = PySequence_GetSlice(args, 1, PySequence_Size(args))))
        return NULL;

    status = PyObject_SetAttrString(self, "args", args);
    if (status < 0) {
	Py_DECREF(args);
        return NULL;
    }

    /* set code attribute */
    switch (PySequence_Size(args)) {
    case 0:
        Py_INCREF(Py_None);
        code = Py_None;
        break;
    case 1:
        code = PySequence_GetItem(args, 0);
        break;
    case -1:
        PyErr_Clear();
        /* Fall through */
    default:
        Py_INCREF(args);
        code = args;
        break;
    }

    status = PyObject_SetAttrString(self, "code", code);
    Py_DECREF(code);
    Py_DECREF(args);
    if (status < 0)
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef SystemExit_methods[] = {
    { "__init__", SystemExit__init__, METH_VARARGS},
    {NULL, NULL}
};



PyDoc_STRVAR(KeyboardInterrupt__doc__, "Program interrupted by user.");

PyDoc_STRVAR(ImportError__doc__,
"Import can't find module, or can't find name in module.");



PyDoc_STRVAR(EnvironmentError__doc__, "Base class for I/O related errors.");


static PyObject *
EnvironmentError__init__(PyObject *self, PyObject *args)
{
    PyObject *item0 = NULL;
    PyObject *item1 = NULL;
    PyObject *item2 = NULL;
    PyObject *subslice = NULL;
    PyObject *rtnval = NULL;

    if (!(self = get_self(args)))
	return NULL;

    if (!(args = PySequence_GetSlice(args, 1, PySequence_Size(args))))
	return NULL;

    if (PyObject_SetAttrString(self, "args", args) ||
	PyObject_SetAttrString(self, "errno", Py_None) ||
	PyObject_SetAttrString(self, "strerror", Py_None) ||
	PyObject_SetAttrString(self, "filename", Py_None))
    {
	goto finally;
    }

    switch (PySequence_Size(args)) {
    case 3:
	/* Where a function has a single filename, such as open() or some
	 * of the os module functions, PyErr_SetFromErrnoWithFilename() is
	 * called, giving a third argument which is the filename.  But, so
	 * that old code using in-place unpacking doesn't break, e.g.:
	 *
	 * except IOError, (errno, strerror):
	 *
	 * we hack args so that it only contains two items.  This also
	 * means we need our own __str__() which prints out the filename
	 * when it was supplied.
	 */
	item0 = PySequence_GetItem(args, 0);
	item1 = PySequence_GetItem(args, 1);
	item2 = PySequence_GetItem(args, 2);
	if (!item0 || !item1 || !item2)
	    goto finally;

	if (PyObject_SetAttrString(self, "errno", item0) ||
	    PyObject_SetAttrString(self, "strerror", item1) ||
	    PyObject_SetAttrString(self, "filename", item2))
	{
	    goto finally;
	}

	subslice = PySequence_GetSlice(args, 0, 2);
	if (!subslice || PyObject_SetAttrString(self, "args", subslice))
	    goto finally;
	break;

    case 2:
	/* Used when PyErr_SetFromErrno() is called and no filename
	 * argument is given.
	 */
	item0 = PySequence_GetItem(args, 0);
	item1 = PySequence_GetItem(args, 1);
	if (!item0 || !item1)
	    goto finally;

	if (PyObject_SetAttrString(self, "errno", item0) ||
	    PyObject_SetAttrString(self, "strerror", item1))
	{
	    goto finally;
	}
	break;

    case -1:
	PyErr_Clear();
	break;
    }

    Py_INCREF(Py_None);
    rtnval = Py_None;

  finally:
    Py_DECREF(args);
    Py_XDECREF(item0);
    Py_XDECREF(item1);
    Py_XDECREF(item2);
    Py_XDECREF(subslice);
    return rtnval;
}


static PyObject *
EnvironmentError__str__(PyObject *self, PyObject *args)
{
    PyObject *originalself = self;
    PyObject *filename;
    PyObject *serrno;
    PyObject *strerror;
    PyObject *rtnval = NULL;

    if (!PyArg_ParseTuple(args, "O:__str__", &self))
	return NULL;

    filename = PyObject_GetAttrString(self, "filename");
    serrno = PyObject_GetAttrString(self, "errno");
    strerror = PyObject_GetAttrString(self, "strerror");
    if (!filename || !serrno || !strerror)
	goto finally;

    if (filename != Py_None) {
	PyObject *fmt = PyString_FromString("[Errno %s] %s: %s");
	PyObject *repr = PyObject_Repr(filename);
	PyObject *tuple = PyTuple_New(3);

	if (!fmt || !repr || !tuple) {
	    Py_XDECREF(fmt);
	    Py_XDECREF(repr);
	    Py_XDECREF(tuple);
	    goto finally;
	}

	PyTuple_SET_ITEM(tuple, 0, serrno);
	PyTuple_SET_ITEM(tuple, 1, strerror);
	PyTuple_SET_ITEM(tuple, 2, repr);

	rtnval = PyString_Format(fmt, tuple);

	Py_DECREF(fmt);
	Py_DECREF(tuple);
	/* already freed because tuple owned only reference */
	serrno = NULL;
	strerror = NULL;
    }
    else if (PyObject_IsTrue(serrno) && PyObject_IsTrue(strerror)) {
	PyObject *fmt = PyString_FromString("[Errno %s] %s");
	PyObject *tuple = PyTuple_New(2);

	if (!fmt || !tuple) {
	    Py_XDECREF(fmt);
	    Py_XDECREF(tuple);
	    goto finally;
	}

	PyTuple_SET_ITEM(tuple, 0, serrno);
	PyTuple_SET_ITEM(tuple, 1, strerror);

	rtnval = PyString_Format(fmt, tuple);

	Py_DECREF(fmt);
	Py_DECREF(tuple);
	/* already freed because tuple owned only reference */
	serrno = NULL;
	strerror = NULL;
    }
    else
	/* The original Python code said:
	 *
	 *   return StandardError.__str__(self)
	 *
	 * but there is no StandardError__str__() function; we happen to
	 * know that's just a pass through to Exception__str__().
	 */
	rtnval = Exception__str__(originalself, args);

  finally:
    Py_XDECREF(filename);
    Py_XDECREF(serrno);
    Py_XDECREF(strerror);
    return rtnval;
}


static
PyMethodDef EnvironmentError_methods[] = {
    {"__init__", EnvironmentError__init__, METH_VARARGS},
    {"__str__",  EnvironmentError__str__, METH_VARARGS},
    {NULL, NULL}
};




PyDoc_STRVAR(IOError__doc__, "I/O operation failed.");

PyDoc_STRVAR(OSError__doc__, "OS system call failed.");

#ifdef MS_WINDOWS
PyDoc_STRVAR(WindowsError__doc__, "MS-Windows OS system call failed.");
#endif /* MS_WINDOWS */

#ifdef __VMS
static char
VMSError__doc__[] = "OpenVMS OS system call failed.";
#endif

PyDoc_STRVAR(EOFError__doc__, "Read beyond end of file.");

PyDoc_STRVAR(RuntimeError__doc__, "Unspecified run-time error.");

PyDoc_STRVAR(NotImplementedError__doc__,
"Method or function hasn't been implemented yet.");

PyDoc_STRVAR(NameError__doc__, "Name not found globally.");

PyDoc_STRVAR(UnboundLocalError__doc__,
"Local name referenced but not bound to a value.");

PyDoc_STRVAR(AttributeError__doc__, "Attribute not found.");



PyDoc_STRVAR(SyntaxError__doc__, "Invalid syntax.");


static int
SyntaxError__classinit__(PyObject *klass)
{
    int retval = 0;
    PyObject *emptystring = PyString_FromString("");

    /* Additional class-creation time initializations */
    if (!emptystring ||
	PyObject_SetAttrString(klass, "msg", emptystring) ||
	PyObject_SetAttrString(klass, "filename", Py_None) ||
	PyObject_SetAttrString(klass, "lineno", Py_None) ||
	PyObject_SetAttrString(klass, "offset", Py_None) ||
	PyObject_SetAttrString(klass, "text", Py_None) ||
	PyObject_SetAttrString(klass, "print_file_and_line", Py_None))
    {
	retval = -1;
    }
    Py_XDECREF(emptystring);
    return retval;
}


static PyObject *
SyntaxError__init__(PyObject *self, PyObject *args)
{
    PyObject *rtnval = NULL;
    int lenargs;

    if (!(self = get_self(args)))
	return NULL;

    if (!(args = PySequence_GetSlice(args, 1, PySequence_Size(args))))
	return NULL;

    if (PyObject_SetAttrString(self, "args", args))
	goto finally;

    lenargs = PySequence_Size(args);
    if (lenargs >= 1) {
	PyObject *item0 = PySequence_GetItem(args, 0);
	int status;

	if (!item0)
	    goto finally;
	status = PyObject_SetAttrString(self, "msg", item0);
	Py_DECREF(item0);
	if (status)
	    goto finally;
    }
    if (lenargs == 2) {
	PyObject *info = PySequence_GetItem(args, 1);
	PyObject *filename = NULL, *lineno = NULL;
	PyObject *offset = NULL, *text = NULL;
	int status = 1;

	if (!info)
	    goto finally;

	filename = PySequence_GetItem(info, 0);
	if (filename != NULL) {
	    lineno = PySequence_GetItem(info, 1);
	    if (lineno != NULL) {
		offset = PySequence_GetItem(info, 2);
		if (offset != NULL) {
		    text = PySequence_GetItem(info, 3);
		    if (text != NULL) {
			status =
			    PyObject_SetAttrString(self, "filename", filename)
			    || PyObject_SetAttrString(self, "lineno", lineno)
			    || PyObject_SetAttrString(self, "offset", offset)
			    || PyObject_SetAttrString(self, "text", text);
			Py_DECREF(text);
		    }
		    Py_DECREF(offset);
		}
		Py_DECREF(lineno);
	    }
	    Py_DECREF(filename);
	}
	Py_DECREF(info);

	if (status)
	    goto finally;
    }
    Py_INCREF(Py_None);
    rtnval = Py_None;

  finally:
    Py_DECREF(args);
    return rtnval;
}


/* This is called "my_basename" instead of just "basename" to avoid name
   conflicts with glibc; basename is already prototyped if _GNU_SOURCE is
   defined, and Python does define that. */
static char *
my_basename(char *name)
{
	char *cp = name;
	char *result = name;

	if (name == NULL)
		return "???";
	while (*cp != '\0') {
		if (*cp == SEP)
			result = cp + 1;
		++cp;
	}
	return result;
}


static PyObject *
SyntaxError__str__(PyObject *self, PyObject *args)
{
    PyObject *msg;
    PyObject *str;
    PyObject *filename, *lineno, *result;

    if (!PyArg_ParseTuple(args, "O:__str__", &self))
	return NULL;

    if (!(msg = PyObject_GetAttrString(self, "msg")))
	return NULL;

    str = PyObject_Str(msg);
    Py_DECREF(msg);
    result = str;

    /* XXX -- do all the additional formatting with filename and
       lineno here */

    if (str != NULL && PyString_Check(str)) {
	int have_filename = 0;
	int have_lineno = 0;
	char *buffer = NULL;

	if ((filename = PyObject_GetAttrString(self, "filename")) != NULL)
	    have_filename = PyString_Check(filename);
	else
	    PyErr_Clear();

	if ((lineno = PyObject_GetAttrString(self, "lineno")) != NULL)
	    have_lineno = PyInt_Check(lineno);
	else
	    PyErr_Clear();

	if (have_filename || have_lineno) {
	    int bufsize = PyString_GET_SIZE(str) + 64;
	    if (have_filename)
		bufsize += PyString_GET_SIZE(filename);

	    buffer = PyMem_MALLOC(bufsize);
	    if (buffer != NULL) {
		if (have_filename && have_lineno)
		    PyOS_snprintf(buffer, bufsize, "%s (%s, line %ld)",
				  PyString_AS_STRING(str),
				  my_basename(PyString_AS_STRING(filename)),
				  PyInt_AsLong(lineno));
		else if (have_filename)
		    PyOS_snprintf(buffer, bufsize, "%s (%s)",
				  PyString_AS_STRING(str),
				  my_basename(PyString_AS_STRING(filename)));
		else if (have_lineno)
		    PyOS_snprintf(buffer, bufsize, "%s (line %ld)",
				  PyString_AS_STRING(str),
				  PyInt_AsLong(lineno));

		result = PyString_FromString(buffer);
		PyMem_FREE(buffer);

		if (result == NULL)
		    result = str;
		else
		    Py_DECREF(str);
	    }
	}
	Py_XDECREF(filename);
	Py_XDECREF(lineno);
    }
    return result;
}


static PyMethodDef SyntaxError_methods[] = {
    {"__init__", SyntaxError__init__, METH_VARARGS},
    {"__str__",  SyntaxError__str__, METH_VARARGS},
    {NULL, NULL}
};


static PyObject *
KeyError__str__(PyObject *self, PyObject *args)
{
    PyObject *argsattr;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O:__str__", &self))
	return NULL;

    if (!(argsattr = PyObject_GetAttrString(self, "args")))
	return NULL;

    /* If args is a tuple of exactly one item, apply repr to args[0].
       This is done so that e.g. the exception raised by {}[''] prints
         KeyError: ''
       rather than the confusing
         KeyError
       alone.  The downside is that if KeyError is raised with an explanatory
       string, that string will be displayed in quotes.  Too bad.
       If args is anything else, use the default Exception__str__().
    */
    if (PyTuple_Check(argsattr) && PyTuple_GET_SIZE(argsattr) == 1) {
	PyObject *key = PyTuple_GET_ITEM(argsattr, 0);
	result = PyObject_Repr(key);
    }
    else
	result = Exception__str__(self, args);

    Py_DECREF(argsattr);
    return result;
}

static PyMethodDef KeyError_methods[] = {
    {"__str__",  KeyError__str__, METH_VARARGS},
    {NULL, NULL}
};


#ifdef Py_USING_UNICODE
static
int get_int(PyObject *exc, const char *name, int *value)
{
    PyObject *attr = PyObject_GetAttrString(exc, (char *)name);

    if (!attr)
	return -1;
    if (!PyInt_Check(attr)) {
	PyErr_Format(PyExc_TypeError, "%.200s attribute must be int", name);
	Py_DECREF(attr);
	return -1;
    }
    *value = PyInt_AS_LONG(attr);
    Py_DECREF(attr);
    return 0;
}


static
int set_int(PyObject *exc, const char *name, int value)
{
    PyObject *obj = PyInt_FromLong(value);
    int result;

    if (!obj)
	return -1;
    result = PyObject_SetAttrString(exc, (char *)name, obj);
    Py_DECREF(obj);
    return result;
}


static
PyObject *get_string(PyObject *exc, const char *name)
{
    PyObject *attr = PyObject_GetAttrString(exc, (char *)name);

    if (!attr)
	return NULL;
    if (!PyString_Check(attr)) {
	PyErr_Format(PyExc_TypeError, "%.200s attribute must be str", name);
	Py_DECREF(attr);
	return NULL;
    }
    return attr;
}


static
int set_string(PyObject *exc, const char *name, const char *value)
{
    PyObject *obj = PyString_FromString(value);
    int result;

    if (!obj)
	return -1;
    result = PyObject_SetAttrString(exc, (char *)name, obj);
    Py_DECREF(obj);
    return result;
}


static
PyObject *get_unicode(PyObject *exc, const char *name)
{
    PyObject *attr = PyObject_GetAttrString(exc, (char *)name);

    if (!attr)
	return NULL;
    if (!PyUnicode_Check(attr)) {
	PyErr_Format(PyExc_TypeError, "%.200s attribute must be unicode", name);
	Py_DECREF(attr);
	return NULL;
    }
    return attr;
}

PyObject * PyUnicodeEncodeError_GetEncoding(PyObject *exc)
{
    return get_string(exc, "encoding");
}

PyObject * PyUnicodeDecodeError_GetEncoding(PyObject *exc)
{
    return get_string(exc, "encoding");
}

PyObject *PyUnicodeEncodeError_GetObject(PyObject *exc)
{
    return get_unicode(exc, "object");
}

PyObject *PyUnicodeDecodeError_GetObject(PyObject *exc)
{
    return get_string(exc, "object");
}

PyObject *PyUnicodeTranslateError_GetObject(PyObject *exc)
{
    return get_unicode(exc, "object");
}

int PyUnicodeEncodeError_GetStart(PyObject *exc, int *start)
{
    if (!get_int(exc, "start", start)) {
	PyObject *object = PyUnicodeEncodeError_GetObject(exc);
	int size;
	if (!object)
	    return -1;
	size = PyUnicode_GET_SIZE(object);
	if (*start<0)
	    *start = 0;
	if (*start>=size)
	    *start = size-1;
	Py_DECREF(object);
	return 0;
    }
    return -1;
}


int PyUnicodeDecodeError_GetStart(PyObject *exc, int *start)
{
    if (!get_int(exc, "start", start)) {
	PyObject *object = PyUnicodeDecodeError_GetObject(exc);
	int size;
	if (!object)
	    return -1;
	size = PyString_GET_SIZE(object);
	if (*start<0)
	    *start = 0;
	if (*start>=size)
	    *start = size-1;
	Py_DECREF(object);
	return 0;
    }
    return -1;
}


int PyUnicodeTranslateError_GetStart(PyObject *exc, int *start)
{
    return PyUnicodeEncodeError_GetStart(exc, start);
}


int PyUnicodeEncodeError_SetStart(PyObject *exc, int start)
{
    return set_int(exc, "start", start);
}


int PyUnicodeDecodeError_SetStart(PyObject *exc, int start)
{
    return set_int(exc, "start", start);
}


int PyUnicodeTranslateError_SetStart(PyObject *exc, int start)
{
    return set_int(exc, "start", start);
}


int PyUnicodeEncodeError_GetEnd(PyObject *exc, int *end)
{
    if (!get_int(exc, "end", end)) {
	PyObject *object = PyUnicodeEncodeError_GetObject(exc);
	int size;
	if (!object)
	    return -1;
	size = PyUnicode_GET_SIZE(object);
	if (*end<1)
	    *end = 1;
	if (*end>size)
	    *end = size;
	Py_DECREF(object);
	return 0;
    }
    return -1;
}


int PyUnicodeDecodeError_GetEnd(PyObject *exc, int *end)
{
    if (!get_int(exc, "end", end)) {
	PyObject *object = PyUnicodeDecodeError_GetObject(exc);
	int size;
	if (!object)
	    return -1;
	size = PyString_GET_SIZE(object);
	if (*end<1)
	    *end = 1;
	if (*end>size)
	    *end = size;
	Py_DECREF(object);
	return 0;
    }
    return -1;
}


int PyUnicodeTranslateError_GetEnd(PyObject *exc, int *start)
{
    return PyUnicodeEncodeError_GetEnd(exc, start);
}


int PyUnicodeEncodeError_SetEnd(PyObject *exc, int end)
{
    return set_int(exc, "end", end);
}


int PyUnicodeDecodeError_SetEnd(PyObject *exc, int end)
{
    return set_int(exc, "end", end);
}


int PyUnicodeTranslateError_SetEnd(PyObject *exc, int end)
{
    return set_int(exc, "end", end);
}


PyObject *PyUnicodeEncodeError_GetReason(PyObject *exc)
{
    return get_string(exc, "reason");
}


PyObject *PyUnicodeDecodeError_GetReason(PyObject *exc)
{
    return get_string(exc, "reason");
}


PyObject *PyUnicodeTranslateError_GetReason(PyObject *exc)
{
    return get_string(exc, "reason");
}


int PyUnicodeEncodeError_SetReason(PyObject *exc, const char *reason)
{
    return set_string(exc, "reason", reason);
}


int PyUnicodeDecodeError_SetReason(PyObject *exc, const char *reason)
{
    return set_string(exc, "reason", reason);
}


int PyUnicodeTranslateError_SetReason(PyObject *exc, const char *reason)
{
    return set_string(exc, "reason", reason);
}


static PyObject *
UnicodeError__init__(PyObject *self, PyObject *args, PyTypeObject *objecttype)
{
    PyObject *rtnval = NULL;
    PyObject *encoding;
    PyObject *object;
    PyObject *start;
    PyObject *end;
    PyObject *reason;

    if (!(self = get_self(args)))
	return NULL;

    if (!(args = PySequence_GetSlice(args, 1, PySequence_Size(args))))
	return NULL;

    if (!PyArg_ParseTuple(args, "O!O!O!O!O!",
	&PyString_Type, &encoding,
	objecttype, &object,
	&PyInt_Type, &start,
	&PyInt_Type, &end,
	&PyString_Type, &reason))
	goto finally;

    if (PyObject_SetAttrString(self, "args", args))
	goto finally;

    if (PyObject_SetAttrString(self, "encoding", encoding))
	goto finally;
    if (PyObject_SetAttrString(self, "object", object))
	goto finally;
    if (PyObject_SetAttrString(self, "start", start))
	goto finally;
    if (PyObject_SetAttrString(self, "end", end))
	goto finally;
    if (PyObject_SetAttrString(self, "reason", reason))
	goto finally;

    Py_INCREF(Py_None);
    rtnval = Py_None;

  finally:
    Py_DECREF(args);
    return rtnval;
}


static PyObject *
UnicodeEncodeError__init__(PyObject *self, PyObject *args)
{
    return UnicodeError__init__(self, args, &PyUnicode_Type);
}

static PyObject *
UnicodeEncodeError__str__(PyObject *self, PyObject *arg)
{
    PyObject *encodingObj = NULL;
    PyObject *objectObj = NULL;
    int start;
    int end;
    PyObject *reasonObj = NULL;
    char buffer[1000];
    PyObject *result = NULL;

    self = arg;

    if (!(encodingObj = PyUnicodeEncodeError_GetEncoding(self)))
	goto error;

    if (!(objectObj = PyUnicodeEncodeError_GetObject(self)))
	goto error;

    if (PyUnicodeEncodeError_GetStart(self, &start))
	goto error;

    if (PyUnicodeEncodeError_GetEnd(self, &end))
	goto error;

    if (!(reasonObj = PyUnicodeEncodeError_GetReason(self)))
	goto error;

    if (end==start+1) {
	int badchar = (int)PyUnicode_AS_UNICODE(objectObj)[start];
	char *format;
	if (badchar <= 0xff)
	   format = "'%.400s' codec can't encode character u'\\x%02x' in position %d: %.400s";
	else if (badchar <= 0xffff)
	   format = "'%.400s' codec can't encode character u'\\u%04x' in position %d: %.400s";
	else
	   format = "'%.400s' codec can't encode character u'\\U%08x' in position %d: %.400s";
	PyOS_snprintf(buffer, sizeof(buffer),
	    format,
	    PyString_AS_STRING(encodingObj),
	    badchar,
	    start,
	    PyString_AS_STRING(reasonObj)
	);
    }
    else {
	PyOS_snprintf(buffer, sizeof(buffer),
	    "'%.400s' codec can't encode characters in position %d-%d: %.400s",
	    PyString_AS_STRING(encodingObj),
	    start,
	    end-1,
	    PyString_AS_STRING(reasonObj)
	);
    }
    result = PyString_FromString(buffer);

error:
    Py_XDECREF(reasonObj);
    Py_XDECREF(objectObj);
    Py_XDECREF(encodingObj);
    return result;
}

static PyMethodDef UnicodeEncodeError_methods[] = {
    {"__init__", UnicodeEncodeError__init__, METH_VARARGS},
    {"__str__",  UnicodeEncodeError__str__, METH_O},
    {NULL, NULL}
};


PyObject * PyUnicodeEncodeError_Create(
	const char *encoding, const Py_UNICODE *object, int length,
	int start, int end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeEncodeError, "su#iis",
	encoding, object, length, start, end, reason);
}


static PyObject *
UnicodeDecodeError__init__(PyObject *self, PyObject *args)
{
    return UnicodeError__init__(self, args, &PyString_Type);
}

static PyObject *
UnicodeDecodeError__str__(PyObject *self, PyObject *arg)
{
    PyObject *encodingObj = NULL;
    PyObject *objectObj = NULL;
    int start;
    int end;
    PyObject *reasonObj = NULL;
    char buffer[1000];
    PyObject *result = NULL;

    self = arg;

    if (!(encodingObj = PyUnicodeDecodeError_GetEncoding(self)))
	goto error;

    if (!(objectObj = PyUnicodeDecodeError_GetObject(self)))
	goto error;

    if (PyUnicodeDecodeError_GetStart(self, &start))
	goto error;

    if (PyUnicodeDecodeError_GetEnd(self, &end))
	goto error;

    if (!(reasonObj = PyUnicodeDecodeError_GetReason(self)))
	goto error;

    if (end==start+1) {
	PyOS_snprintf(buffer, sizeof(buffer),
	    "'%.400s' codec can't decode byte 0x%02x in position %d: %.400s",
	    PyString_AS_STRING(encodingObj),
	    ((int)PyString_AS_STRING(objectObj)[start])&0xff,
	    start,
	    PyString_AS_STRING(reasonObj)
	);
    }
    else {
	PyOS_snprintf(buffer, sizeof(buffer),
	    "'%.400s' codec can't decode bytes in position %d-%d: %.400s",
	    PyString_AS_STRING(encodingObj),
	    start,
	    end-1,
	    PyString_AS_STRING(reasonObj)
	);
    }
    result = PyString_FromString(buffer);

error:
    Py_XDECREF(reasonObj);
    Py_XDECREF(objectObj);
    Py_XDECREF(encodingObj);
    return result;
}

static PyMethodDef UnicodeDecodeError_methods[] = {
    {"__init__", UnicodeDecodeError__init__, METH_VARARGS},
    {"__str__",  UnicodeDecodeError__str__, METH_O},
    {NULL, NULL}
};


PyObject * PyUnicodeDecodeError_Create(
	const char *encoding, const char *object, int length,
	int start, int end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeDecodeError, "ss#iis",
	encoding, object, length, start, end, reason);
}


static PyObject *
UnicodeTranslateError__init__(PyObject *self, PyObject *args)
{
    PyObject *rtnval = NULL;
    PyObject *object;
    PyObject *start;
    PyObject *end;
    PyObject *reason;

    if (!(self = get_self(args)))
	return NULL;

    if (!(args = PySequence_GetSlice(args, 1, PySequence_Size(args))))
	return NULL;

    if (!PyArg_ParseTuple(args, "O!O!O!O!",
	&PyUnicode_Type, &object,
	&PyInt_Type, &start,
	&PyInt_Type, &end,
	&PyString_Type, &reason))
	goto finally;

    if (PyObject_SetAttrString(self, "args", args))
	goto finally;

    if (PyObject_SetAttrString(self, "object", object))
	goto finally;
    if (PyObject_SetAttrString(self, "start", start))
	goto finally;
    if (PyObject_SetAttrString(self, "end", end))
	goto finally;
    if (PyObject_SetAttrString(self, "reason", reason))
	goto finally;

    Py_INCREF(Py_None);
    rtnval = Py_None;

  finally:
    Py_DECREF(args);
    return rtnval;
}


static PyObject *
UnicodeTranslateError__str__(PyObject *self, PyObject *arg)
{
    PyObject *objectObj = NULL;
    int start;
    int end;
    PyObject *reasonObj = NULL;
    char buffer[1000];
    PyObject *result = NULL;

    self = arg;

    if (!(objectObj = PyUnicodeTranslateError_GetObject(self)))
	goto error;

    if (PyUnicodeTranslateError_GetStart(self, &start))
	goto error;

    if (PyUnicodeTranslateError_GetEnd(self, &end))
	goto error;

    if (!(reasonObj = PyUnicodeTranslateError_GetReason(self)))
	goto error;

    if (end==start+1) {
	int badchar = (int)PyUnicode_AS_UNICODE(objectObj)[start];
	char *format;
	if (badchar <= 0xff)
	   format = "can't translate character u'\\x%02x' in position %d: %.400s";
	else if (badchar <= 0xffff)
	   format = "can't translate character u'\\u%04x' in position %d: %.400s";
	else
	   format = "can't translate character u'\\U%08x' in position %d: %.400s";
	PyOS_snprintf(buffer, sizeof(buffer),
	    format,
	    badchar,
	    start,
	    PyString_AS_STRING(reasonObj)
	);
    }
    else {
	PyOS_snprintf(buffer, sizeof(buffer),
	    "can't translate characters in position %d-%d: %.400s",
	    start,
	    end-1,
	    PyString_AS_STRING(reasonObj)
	);
    }
    result = PyString_FromString(buffer);

error:
    Py_XDECREF(reasonObj);
    Py_XDECREF(objectObj);
    return result;
}

static PyMethodDef UnicodeTranslateError_methods[] = {
    {"__init__", UnicodeTranslateError__init__, METH_VARARGS},
    {"__str__",  UnicodeTranslateError__str__, METH_O},
    {NULL, NULL}
};


PyObject * PyUnicodeTranslateError_Create(
	const Py_UNICODE *object, int length,
	int start, int end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeTranslateError, "u#iis",
	object, length, start, end, reason);
}
#endif



/* Exception doc strings */

PyDoc_STRVAR(AssertionError__doc__, "Assertion failed.");

PyDoc_STRVAR(LookupError__doc__, "Base class for lookup errors.");

PyDoc_STRVAR(IndexError__doc__, "Sequence index out of range.");

PyDoc_STRVAR(KeyError__doc__, "Mapping key not found.");

PyDoc_STRVAR(ArithmeticError__doc__, "Base class for arithmetic errors.");

PyDoc_STRVAR(OverflowError__doc__, "Result too large to be represented.");

PyDoc_STRVAR(ZeroDivisionError__doc__,
"Second argument to a division or modulo operation was zero.");

PyDoc_STRVAR(FloatingPointError__doc__, "Floating point operation failed.");

PyDoc_STRVAR(ValueError__doc__,
"Inappropriate argument value (of correct type).");

PyDoc_STRVAR(UnicodeError__doc__, "Unicode related error.");

#ifdef Py_USING_UNICODE
PyDoc_STRVAR(UnicodeEncodeError__doc__, "Unicode encoding error.");

PyDoc_STRVAR(UnicodeDecodeError__doc__, "Unicode decoding error.");

PyDoc_STRVAR(UnicodeTranslateError__doc__, "Unicode translation error.");
#endif

PyDoc_STRVAR(SystemError__doc__,
"Internal error in the Python interpreter.\n\
\n\
Please report this to the Python maintainer, along with the traceback,\n\
the Python version, and the hardware/OS platform and version.");

PyDoc_STRVAR(ReferenceError__doc__,
"Weak ref proxy used after referent went away.");

PyDoc_STRVAR(MemoryError__doc__, "Out of memory.");

PyDoc_STRVAR(IndentationError__doc__, "Improper indentation.");

PyDoc_STRVAR(TabError__doc__, "Improper mixture of spaces and tabs.");

/* Warning category docstrings */

PyDoc_STRVAR(Warning__doc__, "Base class for warning categories.");

PyDoc_STRVAR(UserWarning__doc__,
"Base class for warnings generated by user code.");

PyDoc_STRVAR(DeprecationWarning__doc__,
"Base class for warnings about deprecated features.");

PyDoc_STRVAR(PendingDeprecationWarning__doc__,
"Base class for warnings about features which will be deprecated "
"in the future.");

PyDoc_STRVAR(SyntaxWarning__doc__,
"Base class for warnings about dubious syntax.");

PyDoc_STRVAR(OverflowWarning__doc__,
"Base class for warnings about numeric overflow.  Won't exist in Python 2.5.");

PyDoc_STRVAR(RuntimeWarning__doc__,
"Base class for warnings about dubious runtime behavior.");

PyDoc_STRVAR(FutureWarning__doc__,
"Base class for warnings about constructs that will change semantically "
"in the future.");



/* module global functions */
static PyMethodDef functions[] = {
    /* Sentinel */
    {NULL, NULL}
};



/* Global C API defined exceptions */

PyObject *PyExc_Exception;
PyObject *PyExc_StopIteration;
PyObject *PyExc_StandardError;
PyObject *PyExc_ArithmeticError;
PyObject *PyExc_LookupError;

PyObject *PyExc_AssertionError;
PyObject *PyExc_AttributeError;
PyObject *PyExc_EOFError;
PyObject *PyExc_FloatingPointError;
PyObject *PyExc_EnvironmentError;
PyObject *PyExc_IOError;
PyObject *PyExc_OSError;
PyObject *PyExc_ImportError;
PyObject *PyExc_IndexError;
PyObject *PyExc_KeyError;
PyObject *PyExc_KeyboardInterrupt;
PyObject *PyExc_MemoryError;
PyObject *PyExc_NameError;
PyObject *PyExc_OverflowError;
PyObject *PyExc_RuntimeError;
PyObject *PyExc_NotImplementedError;
PyObject *PyExc_SyntaxError;
PyObject *PyExc_IndentationError;
PyObject *PyExc_TabError;
PyObject *PyExc_ReferenceError;
PyObject *PyExc_SystemError;
PyObject *PyExc_SystemExit;
PyObject *PyExc_UnboundLocalError;
PyObject *PyExc_UnicodeError;
PyObject *PyExc_UnicodeEncodeError;
PyObject *PyExc_UnicodeDecodeError;
PyObject *PyExc_UnicodeTranslateError;
PyObject *PyExc_TypeError;
PyObject *PyExc_ValueError;
PyObject *PyExc_ZeroDivisionError;
#ifdef MS_WINDOWS
PyObject *PyExc_WindowsError;
#endif
#ifdef __VMS
PyObject *PyExc_VMSError;
#endif

/* Pre-computed MemoryError instance.  Best to create this as early as
 * possibly and not wait until a MemoryError is actually raised!
 */
PyObject *PyExc_MemoryErrorInst;

/* Predefined warning categories */
PyObject *PyExc_Warning;
PyObject *PyExc_UserWarning;
PyObject *PyExc_DeprecationWarning;
PyObject *PyExc_PendingDeprecationWarning;
PyObject *PyExc_SyntaxWarning;
/* PyExc_OverflowWarning should be removed for Python 2.5 */
PyObject *PyExc_OverflowWarning;
PyObject *PyExc_RuntimeWarning;
PyObject *PyExc_FutureWarning;



/* mapping between exception names and their PyObject ** */
static struct {
    char *name;
    PyObject **exc;
    PyObject **base;			     /* NULL == PyExc_StandardError */
    char *docstr;
    PyMethodDef *methods;
    int (*classinit)(PyObject *);
} exctable[] = {
 /*
  * The first three classes MUST appear in exactly this order
  */
 {"Exception", &PyExc_Exception},
 {"StopIteration", &PyExc_StopIteration, &PyExc_Exception,
  StopIteration__doc__},
 {"StandardError", &PyExc_StandardError, &PyExc_Exception,
  StandardError__doc__},
 {"TypeError", &PyExc_TypeError, 0, TypeError__doc__},
 /*
  * The rest appear in depth-first order of the hierarchy
  */
 {"SystemExit", &PyExc_SystemExit, &PyExc_Exception, SystemExit__doc__,
  SystemExit_methods},
 {"KeyboardInterrupt",  &PyExc_KeyboardInterrupt, 0, KeyboardInterrupt__doc__},
 {"ImportError",        &PyExc_ImportError,       0, ImportError__doc__},
 {"EnvironmentError",   &PyExc_EnvironmentError,  0, EnvironmentError__doc__,
  EnvironmentError_methods},
 {"IOError", &PyExc_IOError, &PyExc_EnvironmentError, IOError__doc__},
 {"OSError", &PyExc_OSError, &PyExc_EnvironmentError, OSError__doc__},
#ifdef MS_WINDOWS
 {"WindowsError", &PyExc_WindowsError, &PyExc_OSError,
  WindowsError__doc__},
#endif /* MS_WINDOWS */
#ifdef __VMS
 {"VMSError", &PyExc_VMSError, &PyExc_OSError,
  VMSError__doc__},
#endif
 {"EOFError",     &PyExc_EOFError,     0, EOFError__doc__},
 {"RuntimeError", &PyExc_RuntimeError, 0, RuntimeError__doc__},
 {"NotImplementedError", &PyExc_NotImplementedError,
  &PyExc_RuntimeError, NotImplementedError__doc__},
 {"NameError",    &PyExc_NameError,    0, NameError__doc__},
 {"UnboundLocalError", &PyExc_UnboundLocalError, &PyExc_NameError,
  UnboundLocalError__doc__},
 {"AttributeError",     &PyExc_AttributeError, 0, AttributeError__doc__},
 {"SyntaxError",        &PyExc_SyntaxError,    0, SyntaxError__doc__,
  SyntaxError_methods, SyntaxError__classinit__},
 {"IndentationError",   &PyExc_IndentationError, &PyExc_SyntaxError,
  IndentationError__doc__},
 {"TabError",   &PyExc_TabError, &PyExc_IndentationError,
  TabError__doc__},
 {"AssertionError",     &PyExc_AssertionError, 0, AssertionError__doc__},
 {"LookupError",        &PyExc_LookupError,    0, LookupError__doc__},
 {"IndexError",         &PyExc_IndexError,     &PyExc_LookupError,
  IndexError__doc__},
 {"KeyError",           &PyExc_KeyError,       &PyExc_LookupError,
  KeyError__doc__, KeyError_methods},
 {"ArithmeticError",    &PyExc_ArithmeticError, 0, ArithmeticError__doc__},
 {"OverflowError",      &PyExc_OverflowError,     &PyExc_ArithmeticError,
  OverflowError__doc__},
 {"ZeroDivisionError",  &PyExc_ZeroDivisionError,  &PyExc_ArithmeticError,
  ZeroDivisionError__doc__},
 {"FloatingPointError", &PyExc_FloatingPointError, &PyExc_ArithmeticError,
  FloatingPointError__doc__},
 {"ValueError",   &PyExc_ValueError,  0, ValueError__doc__},
 {"UnicodeError", &PyExc_UnicodeError, &PyExc_ValueError, UnicodeError__doc__},
#ifdef Py_USING_UNICODE
 {"UnicodeEncodeError", &PyExc_UnicodeEncodeError, &PyExc_UnicodeError,
  UnicodeEncodeError__doc__, UnicodeEncodeError_methods},
 {"UnicodeDecodeError", &PyExc_UnicodeDecodeError, &PyExc_UnicodeError,
  UnicodeDecodeError__doc__, UnicodeDecodeError_methods},
 {"UnicodeTranslateError", &PyExc_UnicodeTranslateError, &PyExc_UnicodeError,
  UnicodeTranslateError__doc__, UnicodeTranslateError_methods},
#endif
 {"ReferenceError",  &PyExc_ReferenceError, 0, ReferenceError__doc__},
 {"SystemError",  &PyExc_SystemError, 0, SystemError__doc__},
 {"MemoryError",  &PyExc_MemoryError, 0, MemoryError__doc__},
 /* Warning categories */
 {"Warning", &PyExc_Warning, &PyExc_Exception, Warning__doc__},
 {"UserWarning", &PyExc_UserWarning, &PyExc_Warning, UserWarning__doc__},
 {"DeprecationWarning", &PyExc_DeprecationWarning, &PyExc_Warning,
  DeprecationWarning__doc__},
 {"PendingDeprecationWarning", &PyExc_PendingDeprecationWarning, &PyExc_Warning,
  PendingDeprecationWarning__doc__},
 {"SyntaxWarning", &PyExc_SyntaxWarning, &PyExc_Warning, SyntaxWarning__doc__},
 /* OverflowWarning should be removed for Python 2.5 */
 {"OverflowWarning", &PyExc_OverflowWarning, &PyExc_Warning,
  OverflowWarning__doc__},
 {"RuntimeWarning", &PyExc_RuntimeWarning, &PyExc_Warning,
  RuntimeWarning__doc__},
 {"FutureWarning", &PyExc_FutureWarning, &PyExc_Warning,
  FutureWarning__doc__},
 /* Sentinel */
 {NULL}
};



void
_PyExc_Init(void)
{
    char *modulename = "exceptions";
    int modnamesz = strlen(modulename);
    int i;
    PyObject *me, *mydict, *bltinmod, *bdict, *doc, *args;

    me = Py_InitModule(modulename, functions);
    if (me == NULL)
	goto err;
    mydict = PyModule_GetDict(me);
    if (mydict == NULL)
	goto err;
    bltinmod = PyImport_ImportModule("__builtin__");
    if (bltinmod == NULL)
	goto err;
    bdict = PyModule_GetDict(bltinmod);
    if (bdict == NULL)
	goto err;
    doc = PyString_FromString(module__doc__);
    if (doc == NULL)
	goto err;

    i = PyDict_SetItemString(mydict, "__doc__", doc);
    Py_DECREF(doc);
    if (i < 0) {
 err:
	Py_FatalError("exceptions bootstrapping error.");
	return;
    }

    /* This is the base class of all exceptions, so make it first. */
    if (make_Exception(modulename) ||
	PyDict_SetItemString(mydict, "Exception", PyExc_Exception) ||
	PyDict_SetItemString(bdict, "Exception", PyExc_Exception))
    {
	Py_FatalError("Base class `Exception' could not be created.");
    }

    /* Now we can programmatically create all the remaining exceptions.
     * Remember to start the loop at 1 to skip Exceptions.
     */
    for (i=1; exctable[i].name; i++) {
	int status;
	char *cname = PyMem_NEW(char, modnamesz+strlen(exctable[i].name)+2);
	PyObject *base;

	(void)strcpy(cname, modulename);
	(void)strcat(cname, ".");
	(void)strcat(cname, exctable[i].name);

	if (exctable[i].base == 0)
	    base = PyExc_StandardError;
	else
	    base = *exctable[i].base;

	status = make_class(exctable[i].exc, base, cname,
			    exctable[i].methods,
			    exctable[i].docstr);

	PyMem_DEL(cname);

	if (status)
	    Py_FatalError("Standard exception classes could not be created.");

	if (exctable[i].classinit) {
	    status = (*exctable[i].classinit)(*exctable[i].exc);
	    if (status)
		Py_FatalError("An exception class could not be initialized.");
	}

	/* Now insert the class into both this module and the __builtin__
	 * module.
	 */
	if (PyDict_SetItemString(mydict, exctable[i].name, *exctable[i].exc) ||
	    PyDict_SetItemString(bdict, exctable[i].name, *exctable[i].exc))
	{
	    Py_FatalError("Module dictionary insertion problem.");
	}
    }

    /* Now we need to pre-allocate a MemoryError instance */
    args = PyTuple_New(0);
    if (!args ||
	!(PyExc_MemoryErrorInst = PyEval_CallObject(PyExc_MemoryError, args)))
    {
	Py_FatalError("Cannot pre-allocate MemoryError instance\n");
    }
    Py_DECREF(args);

    /* We're done with __builtin__ */
    Py_DECREF(bltinmod);
}


void
_PyExc_Fini(void)
{
    int i;

    Py_XDECREF(PyExc_MemoryErrorInst);
    PyExc_MemoryErrorInst = NULL;

    for (i=0; exctable[i].name; i++) {
	/* clear the class's dictionary, freeing up circular references
	 * between the class and its methods.
	 */
	PyObject* cdict = PyObject_GetAttrString(*exctable[i].exc, "__dict__");
	PyDict_Clear(cdict);
	Py_DECREF(cdict);

	/* Now decref the exception class */
	Py_XDECREF(*exctable[i].exc);
	*exctable[i].exc = NULL;
    }
}
