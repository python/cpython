
/* Module new -- create new objects of various types */

#include "Python.h"
#include "compile.h"

static char new_instance_doc[] =
"Create an instance object from (CLASS [, DICT]) without calling its\n\
__init__() method.  DICT must be a dictionary or None.";

static PyObject *
new_instance(PyObject* unused, PyObject* args)
{
	PyObject *klass;
	PyObject *dict = NULL;

	if (!PyArg_ParseTuple(args, "O!|O:instance",
			      &PyClass_Type, &klass, &dict))
		return NULL;

	if (dict == Py_None)
		dict = NULL;
	else if (dict == NULL)
		/* do nothing */;
	else if (!PyDict_Check(dict)) {
		PyErr_SetString(PyExc_TypeError,
		      "new.instance() second arg must be dictionary or None");
		return NULL;
	}
	return PyInstance_NewRaw(klass, dict);
}

static char new_im_doc[] =
"Create a instance method object from (FUNCTION, INSTANCE, CLASS).";

static PyObject *
new_instancemethod(PyObject* unused, PyObject* args)
{
	PyObject* func;
	PyObject* self;
	PyObject* classObj;

	if (!PyArg_ParseTuple(args, "OOO!:instancemethod",
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
"Create a function object from (CODE, GLOBALS, [NAME [, ARGDEFS]]).";

static PyObject *
new_function(PyObject* unused, PyObject* args)
{
	PyObject* code;
	PyObject* globals;
	PyObject* name = Py_None;
	PyObject* defaults = Py_None;
	PyFunctionObject* newfunc;

	if (!PyArg_ParseTuple(args, "O!O!|OO!:function",
			      &PyCode_Type, &code,
			      &PyDict_Type, &globals,
			      &name,
			      &PyTuple_Type, &defaults))
		return NULL;
	if (name != Py_None && !PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"arg 3 (name) must be None or string");
		return NULL;
	}

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
"Create a code object from (ARGCOUNT, NLOCALS, STACKSIZE, FLAGS, CODESTRING,\n"
"CONSTANTS, NAMES, VARNAMES, FILENAME, NAME, FIRSTLINENO, LNOTAB, FREEVARS,\n"
"CELLVARS).";

static PyObject *
new_code(PyObject* unused, PyObject* args)
{
	int argcount;
	int nlocals;
	int stacksize;
	int flags;
	PyObject* code;
	PyObject* consts;
	PyObject* names;
	PyObject* varnames;
	PyObject* freevars = NULL;
	PyObject* cellvars = NULL;
	PyObject* filename;
	PyObject* name;
	int firstlineno;
	PyObject* lnotab;
	PyBufferProcs *pb;

	if (!PyArg_ParseTuple(args, "iiiiSO!O!O!SSiS|O!O!:code",
			      &argcount, &nlocals, &stacksize, &flags,
			      &code,
			      &PyTuple_Type, &consts,
			      &PyTuple_Type, &names,
			      &PyTuple_Type, &varnames,
			      &filename, &name,
			      &firstlineno, &lnotab,
			      &PyTuple_Type, &freevars,
			      &PyTuple_Type, &cellvars))
		return NULL;

	if (freevars == NULL || cellvars == NULL) {
		PyObject *empty = PyTuple_New(0);
		if (empty == NULL)
		    return NULL;
		if (freevars == NULL) {
		    freevars = empty;
		    Py_INCREF(freevars);
		}
		if (cellvars == NULL) {
		    cellvars = empty;
		    Py_INCREF(cellvars);
		}
		Py_DECREF(empty);
	}

	pb = code->ob_type->tp_as_buffer;
	if (pb == NULL ||
	    pb->bf_getreadbuffer == NULL ||
	    pb->bf_getsegcount == NULL ||
	    (*pb->bf_getsegcount)(code, NULL) != 1)
	{
		PyErr_SetString(PyExc_TypeError,
		  "bytecode object must be a single-segment read-only buffer");
		return NULL;
	}

	return (PyObject *)PyCode_New(argcount, nlocals, stacksize, flags,
				      code, consts, names, varnames,
				      freevars, cellvars, filename, name,
				      firstlineno, lnotab); 
}

static char new_module_doc[] =
"Create a module object from (NAME).";

static PyObject *
new_module(PyObject* unused, PyObject* args)
{
	char *name;
  
	if (!PyArg_ParseTuple(args, "s:module", &name))
		return NULL;
	return PyModule_New(name);
}

static char new_class_doc[] =
"Create a class object from (NAME, BASE_CLASSES, DICT).";

static PyObject *
new_class(PyObject* unused, PyObject* args)
{
	PyObject * name;
	PyObject * classes;
	PyObject * dict;
  
	if (!PyArg_ParseTuple(args, "SO!O!:class", &name, &PyTuple_Type, &classes,
			      &PyDict_Type, &dict))
		return NULL;
	return PyClass_New(classes, dict, name);
}

static PyMethodDef new_methods[] = {
	{"instance",		new_instance,		
	 METH_VARARGS, new_instance_doc},
	{"instancemethod",	new_instancemethod,	
	 METH_VARARGS, new_im_doc},
	{"function",		new_function,		
	 METH_VARARGS, new_function_doc},
	{"code",		new_code,		
	 METH_VARARGS, new_code_doc},
	{"module",		new_module,		
	 METH_VARARGS, new_module_doc},
	{"classobj",		new_class,		
	 METH_VARARGS, new_class_doc},
	{NULL,			NULL}		/* sentinel */
};

static char new_doc[] =
"Functions to create new objects used by the interpreter.\n\
\n\
You need to know a great deal about the interpreter to use this!";

DL_EXPORT(void)
initnew(void)
{
	Py_InitModule4("new", new_methods, new_doc, (PyObject *)NULL,
		       PYTHON_API_VERSION);
}
