/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module new -- create new objects of various types */

#include "Python.h"
#include "compile.h"

static char new_instance_doc[] =
"Create an instance object from (CLASS, DICT) without calling its __init__().";

static PyObject *
new_instance(unused, args)
	PyObject* unused;
	PyObject* args;
{
	PyObject* klass;
	PyObject *dict;
	PyInstanceObject *inst;
	if (!PyArg_ParseTuple(args, "O!O!",
			      &PyClass_Type, &klass,
			      &PyDict_Type, &dict))
		return NULL;
	inst = PyObject_NEW(PyInstanceObject, &PyInstance_Type);
	if (inst == NULL)
		return NULL;
	Py_INCREF(klass);
	Py_INCREF(dict);
	inst->in_class = (PyClassObject *)klass;
	inst->in_dict = dict;
	return (PyObject *)inst;
}

static char new_im_doc[] =
"Create a instance method object from (FUNCTION, INSTANCE, CLASS).";

static PyObject *
new_instancemethod(unused, args)
	PyObject* unused;
	PyObject* args;
{
	PyObject* func;
	PyObject* self;
	PyObject* classObj;

	if (!PyArg_ParseTuple(args, "OOO!",
			      &func,
			      &self,
			      &PyClass_Type, &classObj))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError,
				"first argument must be callable");
		return NULL;
	}
	if (self == Py_None)
		self = NULL;
	else if (!PyInstance_Check(self)) {
		PyErr_SetString(PyExc_TypeError,
				"second argument must be instance or None");
		return NULL;
	}
	return PyMethod_New(func, self, classObj);
}

static char new_function_doc[] =
"Create a function object from (CODE, GLOBALS, [NAME, ARGDEFS]).";

static PyObject *
new_function(unused, args)
	PyObject* unused;
	PyObject* args;
{
	PyObject* code;
	PyObject* globals;
	PyObject* name = Py_None;
	PyObject* defaults = Py_None;
	PyFunctionObject* newfunc;

	if (!PyArg_ParseTuple(args, "O!O!|SO!",
			      &PyCode_Type, &code,
			      &PyDict_Type, &globals,
			      &name,
			      &PyTuple_Type, &defaults))
		return NULL;

	newfunc = (PyFunctionObject *)PyFunction_New(code, globals);
	if (newfunc == NULL)
		return NULL;

	if (name != Py_None) {
		Py_XINCREF(name);
		Py_XDECREF(newfunc->func_name);
		newfunc->func_name = name;
	}
	if (defaults != Py_None) {
		Py_XINCREF(defaults);
		Py_XDECREF(newfunc->func_defaults);
		newfunc->func_defaults  = defaults;
	}

	return (PyObject *)newfunc;
}

static char new_code_doc[] =
"Create a code object from (ARGCOUNT, NLOCALS, STACKSIZE, FLAGS, CODESTRING, CONSTANTS, NAMES, VARNAMES, FILENAME, NAME, FIRSTLINENO, LNOTAB).";

static PyObject *
new_code(unused, args)
	PyObject* unused;
	PyObject* args;
{
	int argcount;
	int nlocals;
	int stacksize;
	int flags;
	PyObject* code;
	PyObject* consts;
	PyObject* names;
	PyObject* varnames;
	PyObject* filename;
	PyObject* name;
	int firstlineno;
	PyObject* lnotab;
  
	if (!PyArg_ParseTuple(args, "iiiiSO!O!O!SSiS",
			      &argcount, &nlocals, &stacksize, &flags,
			      &code,
			      &PyTuple_Type, &consts,
			      &PyTuple_Type, &names,
			      &PyTuple_Type, &varnames,
			      &filename, &name,
			      &firstlineno, &lnotab))
		return NULL;
	return (PyObject *)PyCode_New(argcount, nlocals, stacksize, flags,
				      code, consts, names, varnames,
				      filename, name, firstlineno, lnotab);
}

static char new_module_doc[] =
"Create a module object from (NAME).";

static PyObject *
new_module(unused, args)
	PyObject* unused;
	PyObject* args;
{
	char *name;
  
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	return PyModule_New(name);
}

static char new_class_doc[] =
"Create a class object from (NAME, BASE_CLASSES, DICT).";

static PyObject *
new_class(unused, args)
	PyObject* unused;
	PyObject* args;
{
	PyObject * name;
	PyObject * classes;
	PyObject * dict;
  
	if (!PyArg_ParseTuple(args, "SO!O!", &name, &PyTuple_Type, &classes,
			      &PyDict_Type, &dict))
		return NULL;
	return PyClass_New(classes, dict, name);
}

static PyMethodDef new_methods[] = {
	{"instance",		new_instance,		1, new_instance_doc},
	{"instancemethod",	new_instancemethod,	1, new_im_doc},
	{"function",		new_function,		1, new_function_doc},
	{"code",		new_code,		1, new_code_doc},
	{"module",		new_module,		1, new_module_doc},
	{"classobj",		new_class,		1, new_class_doc},
	{NULL,			NULL}		/* sentinel */
};

char new_doc[] =
"Functions to create new objects used by the interpreter.\n\
\n\
You need to know a great deal about the interpreter to use this!";

void
initnew()
{
	Py_InitModule4("new", new_methods, new_doc, (PyObject *)NULL,
		       PYTHON_API_VERSION);
}
