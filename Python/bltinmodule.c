/* Built-in functions */

#include "Python.h"
#include "Python-ast.h"

#include "node.h"
#include "code.h"
#include "eval.h"

#include <ctype.h>

#ifdef RISCOS
#include "unixstuff.h"
#endif

/* The default encoding used by the platform file system APIs
   Can remain NULL for all platforms that don't have such a concept
*/
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
const char *Py_FileSystemDefaultEncoding = "mbcs";
#elif defined(__APPLE__)
const char *Py_FileSystemDefaultEncoding = "utf-8";
#else
const char *Py_FileSystemDefaultEncoding = NULL; /* use default */
#endif

/* Forward */
static PyObject *filterstring(PyObject *, PyObject *);
#ifdef Py_USING_UNICODE
static PyObject *filterunicode(PyObject *, PyObject *);
#endif
static PyObject *filtertuple (PyObject *, PyObject *);

static PyObject *
builtin___import__(PyObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"name", "globals", "locals", "fromlist",
				 "level", 0};
	char *name;
	PyObject *globals = NULL;
	PyObject *locals = NULL;
	PyObject *fromlist = NULL;
	int level = -1;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|OOOi:__import__",
			kwlist, &name, &globals, &locals, &fromlist, &level))
		return NULL;
	return PyImport_ImportModuleLevel(name, globals, locals,
					  fromlist, level);
}

PyDoc_STRVAR(import_doc,
"__import__(name, globals={}, locals={}, fromlist=[], level=-1) -> module\n\
\n\
Import a module.  The globals are only used to determine the context;\n\
they are not modified.  The locals are currently unused.  The fromlist\n\
should be a list of names to emulate ``from name import ...'', or an\n\
empty list to emulate ``import name''.\n\
When importing a module from a package, note that __import__('A.B', ...)\n\
returns package A when fromlist is empty, but its submodule B when\n\
fromlist is not empty.  Level is used to determine whether to perform \n\
absolute or relative imports.  -1 is the original strategy of attempting\n\
both absolute and relative imports, 0 is absolute, a positive number\n\
is the number of parent directories to search relative to the current module.");


static PyObject *
builtin_abs(PyObject *self, PyObject *v)
{
	return PyNumber_Absolute(v);
}

PyDoc_STRVAR(abs_doc,
"abs(number) -> number\n\
\n\
Return the absolute value of the argument.");

static PyObject *
builtin_all(PyObject *self, PyObject *v)
{
	PyObject *it, *item;
	PyObject *(*iternext)(PyObject *);
	int cmp;

	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;
	iternext = *Py_TYPE(it)->tp_iternext;

	for (;;) {
		item = iternext(it);
		if (item == NULL)
			break;
		cmp = PyObject_IsTrue(item);
		Py_DECREF(item);
		if (cmp < 0) {
			Py_DECREF(it);
			return NULL;
		}
		if (cmp == 0) {
			Py_DECREF(it);
			Py_RETURN_FALSE;
		}
	}
	Py_DECREF(it);
	if (PyErr_Occurred()) {
		if (PyErr_ExceptionMatches(PyExc_StopIteration))
			PyErr_Clear();
		else
			return NULL;
	}
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(all_doc,
"all(iterable) -> bool\n\
\n\
Return True if bool(x) is True for all values x in the iterable.");

static PyObject *
builtin_any(PyObject *self, PyObject *v)
{
	PyObject *it, *item;
	PyObject *(*iternext)(PyObject *);
	int cmp;

	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;
	iternext = *Py_TYPE(it)->tp_iternext;

	for (;;) {
		item = iternext(it);
		if (item == NULL)
			break;
		cmp = PyObject_IsTrue(item);
		Py_DECREF(item);
		if (cmp < 0) {
			Py_DECREF(it);
			return NULL;
		}
		if (cmp == 1) {
			Py_DECREF(it);
			Py_RETURN_TRUE;
		}
	}
	Py_DECREF(it);
	if (PyErr_Occurred()) {
		if (PyErr_ExceptionMatches(PyExc_StopIteration))
			PyErr_Clear();
		else
			return NULL;
	}
	Py_RETURN_FALSE;
}

PyDoc_STRVAR(any_doc,
"any(iterable) -> bool\n\
\n\
Return True if bool(x) is True for any x in the iterable.");

static PyObject *
builtin_apply(PyObject *self, PyObject *args)
{
	PyObject *func, *alist = NULL, *kwdict = NULL;
	PyObject *t = NULL, *retval = NULL;

	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "apply() not supported in 3.x; "
		       "use func(*args, **kwargs)") < 0)
		return NULL;

	if (!PyArg_UnpackTuple(args, "apply", 1, 3, &func, &alist, &kwdict))
		return NULL;
	if (alist != NULL) {
		if (!PyTuple_Check(alist)) {
			if (!PySequence_Check(alist)) {
				PyErr_Format(PyExc_TypeError,
				     "apply() arg 2 expected sequence, found %s",
					     alist->ob_type->tp_name);
				return NULL;
			}
			t = PySequence_Tuple(alist);
			if (t == NULL)
				return NULL;
			alist = t;
		}
	}
	if (kwdict != NULL && !PyDict_Check(kwdict)) {
		PyErr_Format(PyExc_TypeError,
			     "apply() arg 3 expected dictionary, found %s",
			     kwdict->ob_type->tp_name);
		goto finally;
	}
	retval = PyEval_CallObjectWithKeywords(func, alist, kwdict);
  finally:
	Py_XDECREF(t);
	return retval;
}

PyDoc_STRVAR(apply_doc,
"apply(object[, args[, kwargs]]) -> value\n\
\n\
Call a callable object with positional arguments taken from the tuple args,\n\
and keyword arguments taken from the optional dictionary kwargs.\n\
Note that classes are callable, as are instances with a __call__() method.\n\
\n\
Deprecated since release 2.3. Instead, use the extended call syntax:\n\
    function(*args, **keywords).");


static PyObject *
builtin_bin(PyObject *self, PyObject *v)
{
        return PyNumber_ToBase(v, 2);
}

PyDoc_STRVAR(bin_doc,
"bin(number) -> string\n\
\n\
Return the binary representation of an integer or long integer.");


static PyObject *
builtin_callable(PyObject *self, PyObject *v)
{
	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "callable() not supported in 3.x; "
		       "use hasattr(o, '__call__')") < 0)
		return NULL;
	return PyBool_FromLong((long)PyCallable_Check(v));
}

PyDoc_STRVAR(callable_doc,
"callable(object) -> bool\n\
\n\
Return whether the object is callable (i.e., some kind of function).\n\
Note that classes are callable, as are instances with a __call__() method.");


static PyObject *
builtin_filter(PyObject *self, PyObject *args)
{
	PyObject *func, *seq, *result, *it, *arg;
	Py_ssize_t len;   /* guess for result list size */
	register Py_ssize_t j;

	if (!PyArg_UnpackTuple(args, "filter", 2, 2, &func, &seq))
		return NULL;

	/* Strings and tuples return a result of the same type. */
	if (PyString_Check(seq))
		return filterstring(func, seq);
#ifdef Py_USING_UNICODE
	if (PyUnicode_Check(seq))
		return filterunicode(func, seq);
#endif
	if (PyTuple_Check(seq))
		return filtertuple(func, seq);

	/* Pre-allocate argument list tuple. */
	arg = PyTuple_New(1);
	if (arg == NULL)
		return NULL;

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		goto Fail_arg;

	/* Guess a result list size. */
	len = _PyObject_LengthHint(seq, 8);

	/* Get a result list. */
	if (PyList_Check(seq) && seq->ob_refcnt == 1) {
		/* Eww - can modify the list in-place. */
		Py_INCREF(seq);
		result = seq;
	}
	else {
		result = PyList_New(len);
		if (result == NULL)
			goto Fail_it;
	}

	/* Build the result list. */
	j = 0;
	for (;;) {
		PyObject *item;
		int ok;

		item = PyIter_Next(it);
		if (item == NULL) {
			if (PyErr_Occurred())
				goto Fail_result_it;
			break;
		}

		if (func == (PyObject *)&PyBool_Type || func == Py_None) {
			ok = PyObject_IsTrue(item);
		}
		else {
			PyObject *good;
			PyTuple_SET_ITEM(arg, 0, item);
			good = PyObject_Call(func, arg, NULL);
			PyTuple_SET_ITEM(arg, 0, NULL);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_result_it;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			if (j < len)
				PyList_SET_ITEM(result, j, item);
			else {
				int status = PyList_Append(result, item);
				Py_DECREF(item);
				if (status < 0)
					goto Fail_result_it;
			}
			++j;
		}
		else
			Py_DECREF(item);
	}


	/* Cut back result list if len is too big. */
	if (j < len && PyList_SetSlice(result, j, len, NULL) < 0)
		goto Fail_result_it;

	Py_DECREF(it);
	Py_DECREF(arg);
	return result;

Fail_result_it:
	Py_DECREF(result);
Fail_it:
	Py_DECREF(it);
Fail_arg:
	Py_DECREF(arg);
	return NULL;
}

PyDoc_STRVAR(filter_doc,
"filter(function or None, sequence) -> list, tuple, or string\n"
"\n"
"Return those items of sequence for which function(item) is true.  If\n"
"function is None, return the items that are true.  If sequence is a tuple\n"
"or string, return the same type, else return a list.");

static PyObject *
builtin_format(PyObject *self, PyObject *args)
{
	PyObject *value;
	PyObject *format_spec = NULL;

	if (!PyArg_ParseTuple(args, "O|O:format", &value, &format_spec))
		return NULL;

	return PyObject_Format(value, format_spec);
}

PyDoc_STRVAR(format_doc,
"format(value[, format_spec]) -> string\n\
\n\
Returns value.__format__(format_spec)\n\
format_spec defaults to \"\"");

static PyObject *
builtin_chr(PyObject *self, PyObject *args)
{
	long x;
	char s[1];

	if (!PyArg_ParseTuple(args, "l:chr", &x))
		return NULL;
	if (x < 0 || x >= 256) {
		PyErr_SetString(PyExc_ValueError,
				"chr() arg not in range(256)");
		return NULL;
	}
	s[0] = (char)x;
	return PyString_FromStringAndSize(s, 1);
}

PyDoc_STRVAR(chr_doc,
"chr(i) -> character\n\
\n\
Return a string of one character with ordinal i; 0 <= i < 256.");


#ifdef Py_USING_UNICODE
static PyObject *
builtin_unichr(PyObject *self, PyObject *args)
{
	long x;

	if (!PyArg_ParseTuple(args, "l:unichr", &x))
		return NULL;

	return PyUnicode_FromOrdinal(x);
}

PyDoc_STRVAR(unichr_doc,
"unichr(i) -> Unicode character\n\
\n\
Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");
#endif


static PyObject *
builtin_cmp(PyObject *self, PyObject *args)
{
	PyObject *a, *b;
	int c;

	if (!PyArg_UnpackTuple(args, "cmp", 2, 2, &a, &b))
		return NULL;
	if (PyObject_Cmp(a, b, &c) < 0)
		return NULL;
	return PyInt_FromLong((long)c);
}

PyDoc_STRVAR(cmp_doc,
"cmp(x, y) -> integer\n\
\n\
Return negative if x<y, zero if x==y, positive if x>y.");


static PyObject *
builtin_coerce(PyObject *self, PyObject *args)
{
	PyObject *v, *w;
	PyObject *res;

	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "coerce() not supported in 3.x") < 0)
		return NULL;

	if (!PyArg_UnpackTuple(args, "coerce", 2, 2, &v, &w))
		return NULL;
	if (PyNumber_Coerce(&v, &w) < 0)
		return NULL;
	res = PyTuple_Pack(2, v, w);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

PyDoc_STRVAR(coerce_doc,
"coerce(x, y) -> (x1, y1)\n\
\n\
Return a tuple consisting of the two numeric arguments converted to\n\
a common type, using the same rules as used by arithmetic operations.\n\
If coercion is not possible, raise TypeError.");

static PyObject *
builtin_compile(PyObject *self, PyObject *args, PyObject *kwds)
{
	char *str;
	char *filename;
	char *startstr;
	int mode = -1;
	int dont_inherit = 0;
	int supplied_flags = 0;
	PyCompilerFlags cf;
	PyObject *result = NULL, *cmd, *tmp = NULL;
	Py_ssize_t length;
	static char *kwlist[] = {"source", "filename", "mode", "flags",
				 "dont_inherit", NULL};
	int start[] = {Py_file_input, Py_eval_input, Py_single_input};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oss|ii:compile",
					 kwlist, &cmd, &filename, &startstr,
					 &supplied_flags, &dont_inherit))
		return NULL;

	cf.cf_flags = supplied_flags;

	if (supplied_flags &
	    ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_DONT_IMPLY_DEDENT | PyCF_ONLY_AST))
	{
		PyErr_SetString(PyExc_ValueError,
				"compile(): unrecognised flags");
		return NULL;
	}
	/* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

	if (!dont_inherit) {
		PyEval_MergeCompilerFlags(&cf);
	}

	if (strcmp(startstr, "exec") == 0)
		mode = 0;
	else if (strcmp(startstr, "eval") == 0)
		mode = 1;
	else if (strcmp(startstr, "single") == 0)
		mode = 2;
	else {
		PyErr_SetString(PyExc_ValueError,
				"compile() arg 3 must be 'exec', 'eval' or 'single'");
		return NULL;
	}

	if (PyAST_Check(cmd)) {
		if (supplied_flags & PyCF_ONLY_AST) {
			Py_INCREF(cmd);
			result = cmd;
		}
		else {
			PyArena *arena;
			mod_ty mod;

			arena = PyArena_New();
			mod = PyAST_obj2mod(cmd, arena, mode);
			if (mod == NULL) {
				PyArena_Free(arena);
				return NULL;
			}
			result = (PyObject*)PyAST_Compile(mod, filename,
							  &cf, arena);
			PyArena_Free(arena);
		}
		return result;
	}

#ifdef Py_USING_UNICODE
	if (PyUnicode_Check(cmd)) {
		tmp = PyUnicode_AsUTF8String(cmd);
		if (tmp == NULL)
			return NULL;
		cmd = tmp;
		cf.cf_flags |= PyCF_SOURCE_IS_UTF8;
	}
#endif

	if (PyObject_AsReadBuffer(cmd, (const void **)&str, &length))
		goto cleanup;
	if ((size_t)length != strlen(str)) {
		PyErr_SetString(PyExc_TypeError,
				"compile() expected string without null bytes");
		goto cleanup;
	}
	result = Py_CompileStringFlags(str, filename, start[mode], &cf);
cleanup:
	Py_XDECREF(tmp);
	return result;
}

PyDoc_STRVAR(compile_doc,
"compile(source, filename, mode[, flags[, dont_inherit]]) -> code object\n\
\n\
Compile the source string (a Python module, statement or expression)\n\
into a code object that can be executed by the exec statement or eval().\n\
The filename will be used for run-time error messages.\n\
The mode must be 'exec' to compile a module, 'single' to compile a\n\
single (interactive) statement, or 'eval' to compile an expression.\n\
The flags argument, if present, controls which future statements influence\n\
the compilation of the code.\n\
The dont_inherit argument, if non-zero, stops the compilation inheriting\n\
the effects of any future statements in effect in the code calling\n\
compile; if absent or zero these statements do influence the compilation,\n\
in addition to any features explicitly specified.");

static PyObject *
builtin_dir(PyObject *self, PyObject *args)
{
	PyObject *arg = NULL;

	if (!PyArg_UnpackTuple(args, "dir", 0, 1, &arg))
		return NULL;
	return PyObject_Dir(arg);
}

PyDoc_STRVAR(dir_doc,
"dir([object]) -> list of strings\n"
"\n"
"If called without an argument, return the names in the current scope.\n"
"Else, return an alphabetized list of names comprising (some of) the attributes\n"
"of the given object, and of attributes reachable from it.\n"
"If the object supplies a method named __dir__, it will be used; otherwise\n"
"the default dir() logic is used and returns:\n"
"  for a module object: the module's attributes.\n"
"  for a class object:  its attributes, and recursively the attributes\n"
"    of its bases.\n"
"  for any other object: its attributes, its class's attributes, and\n"
"    recursively the attributes of its class's base classes.");

static PyObject *
builtin_divmod(PyObject *self, PyObject *args)
{
	PyObject *v, *w;

	if (!PyArg_UnpackTuple(args, "divmod", 2, 2, &v, &w))
		return NULL;
	return PyNumber_Divmod(v, w);
}

PyDoc_STRVAR(divmod_doc,
"divmod(x, y) -> (div, mod)\n\
\n\
Return the tuple ((x-x%y)/y, x%y).  Invariant: div*y + mod == x.");


static PyObject *
builtin_eval(PyObject *self, PyObject *args)
{
	PyObject *cmd, *result, *tmp = NULL;
	PyObject *globals = Py_None, *locals = Py_None;
	char *str;
	PyCompilerFlags cf;

	if (!PyArg_UnpackTuple(args, "eval", 1, 3, &cmd, &globals, &locals))
		return NULL;
	if (locals != Py_None && !PyMapping_Check(locals)) {
		PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
		return NULL;
	}
	if (globals != Py_None && !PyDict_Check(globals)) {
		PyErr_SetString(PyExc_TypeError, PyMapping_Check(globals) ?
			"globals must be a real dict; try eval(expr, {}, mapping)"
			: "globals must be a dict");
		return NULL;
	}
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None)
			locals = PyEval_GetLocals();
	}
	else if (locals == Py_None)
		locals = globals;

	if (globals == NULL || locals == NULL) {
		PyErr_SetString(PyExc_TypeError, 
			"eval must be given globals and locals "
			"when called without a frame");
		return NULL;
	}

	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}

	if (PyCode_Check(cmd)) {
		if (PyCode_GetNumFree((PyCodeObject *)cmd) > 0) {
			PyErr_SetString(PyExc_TypeError,
		"code object passed to eval() may not contain free variables");
			return NULL;
		}
		return PyEval_EvalCode((PyCodeObject *) cmd, globals, locals);
	}

	if (!PyString_Check(cmd) &&
	    !PyUnicode_Check(cmd)) {
		PyErr_SetString(PyExc_TypeError,
			   "eval() arg 1 must be a string or code object");
		return NULL;
	}
	cf.cf_flags = 0;

#ifdef Py_USING_UNICODE
	if (PyUnicode_Check(cmd)) {
		tmp = PyUnicode_AsUTF8String(cmd);
		if (tmp == NULL)
			return NULL;
		cmd = tmp;
		cf.cf_flags |= PyCF_SOURCE_IS_UTF8;
	}
#endif
	if (PyString_AsStringAndSize(cmd, &str, NULL)) {
		Py_XDECREF(tmp);
		return NULL;
	}
	while (*str == ' ' || *str == '\t')
		str++;

	(void)PyEval_MergeCompilerFlags(&cf);
	result = PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
	Py_XDECREF(tmp);
	return result;
}

PyDoc_STRVAR(eval_doc,
"eval(source[, globals[, locals]]) -> value\n\
\n\
Evaluate the source in the context of globals and locals.\n\
The source may be a string representing a Python expression\n\
or a code object as returned by compile().\n\
The globals must be a dictionary and locals can be any mapping,\n\
defaulting to the current globals and locals.\n\
If only globals is given, locals defaults to it.\n");


static PyObject *
builtin_execfile(PyObject *self, PyObject *args)
{
	char *filename;
	PyObject *globals = Py_None, *locals = Py_None;
	PyObject *res;
	FILE* fp = NULL;
	PyCompilerFlags cf;
	int exists;

	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "execfile() not supported in 3.x; use exec()") < 0)
		return NULL;

	if (!PyArg_ParseTuple(args, "s|O!O:execfile",
			&filename,
			&PyDict_Type, &globals,
			&locals))
		return NULL;
	if (locals != Py_None && !PyMapping_Check(locals)) {
		PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
		return NULL;
	}
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None)
			locals = PyEval_GetLocals();
	}
	else if (locals == Py_None)
		locals = globals;
	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}

	exists = 0;
	/* Test for existence or directory. */
#if defined(PLAN9)
	{
		Dir *d;

		if ((d = dirstat(filename))!=nil) {
			if(d->mode & DMDIR)
				werrstr("is a directory");
			else
				exists = 1;
			free(d);
		}
	}
#elif defined(RISCOS)
	if (object_exists(filename)) {
		if (isdir(filename))
			errno = EISDIR;
		else
			exists = 1;
	}
#else	/* standard Posix */
	{
		struct stat s;
		if (stat(filename, &s) == 0) {
			if (S_ISDIR(s.st_mode))
#				if defined(PYOS_OS2) && defined(PYCC_VACPP)
					errno = EOS2ERR;
#				else
					errno = EISDIR;
#				endif
			else
				exists = 1;
		}
	}
#endif

        if (exists) {
		Py_BEGIN_ALLOW_THREADS
		fp = fopen(filename, "r" PY_STDIOTEXTMODE);
		Py_END_ALLOW_THREADS

		if (fp == NULL) {
			exists = 0;
		}
        }

	if (!exists) {
		PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
		return NULL;
	}
	cf.cf_flags = 0;
	if (PyEval_MergeCompilerFlags(&cf))
		res = PyRun_FileExFlags(fp, filename, Py_file_input, globals,
				   locals, 1, &cf);
	else
		res = PyRun_FileEx(fp, filename, Py_file_input, globals,
				   locals, 1);
	return res;
}

PyDoc_STRVAR(execfile_doc,
"execfile(filename[, globals[, locals]])\n\
\n\
Read and execute a Python script from a file.\n\
The globals and locals are dictionaries, defaulting to the current\n\
globals and locals.  If only globals is given, locals defaults to it.");


static PyObject *
builtin_getattr(PyObject *self, PyObject *args)
{
	PyObject *v, *result, *dflt = NULL;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "getattr", 2, 3, &v, &name, &dflt))
		return NULL;
#ifdef Py_USING_UNICODE
	if (PyUnicode_Check(name)) {
		name = _PyUnicode_AsDefaultEncodedString(name, NULL);
		if (name == NULL)
			return NULL;
	}
#endif

	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"getattr(): attribute name must be string");
		return NULL;
	}
	result = PyObject_GetAttr(v, name);
	if (result == NULL && dflt != NULL &&
	    PyErr_ExceptionMatches(PyExc_AttributeError))
	{
		PyErr_Clear();
		Py_INCREF(dflt);
		result = dflt;
	}
	return result;
}

PyDoc_STRVAR(getattr_doc,
"getattr(object, name[, default]) -> value\n\
\n\
Get a named attribute from an object; getattr(x, 'y') is equivalent to x.y.\n\
When a default argument is given, it is returned when the attribute doesn't\n\
exist; without it, an exception is raised in that case.");


static PyObject *
builtin_globals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetGlobals();
	Py_XINCREF(d);
	return d;
}

PyDoc_STRVAR(globals_doc,
"globals() -> dictionary\n\
\n\
Return the dictionary containing the current scope's global variables.");


static PyObject *
builtin_hasattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "hasattr", 2, 2, &v, &name))
		return NULL;
#ifdef Py_USING_UNICODE
	if (PyUnicode_Check(name)) {
		name = _PyUnicode_AsDefaultEncodedString(name, NULL);
		if (name == NULL)
			return NULL;
	}
#endif

	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_TypeError,
				"hasattr(): attribute name must be string");
		return NULL;
	}
	v = PyObject_GetAttr(v, name);
	if (v == NULL) {
		PyErr_Clear();
		Py_INCREF(Py_False);
		return Py_False;
	}
	Py_DECREF(v);
	Py_INCREF(Py_True);
	return Py_True;
}

PyDoc_STRVAR(hasattr_doc,
"hasattr(object, name) -> bool\n\
\n\
Return whether the object has an attribute with the given name.\n\
(This is done by calling getattr(object, name) and catching exceptions.)");


static PyObject *
builtin_id(PyObject *self, PyObject *v)
{
	return PyLong_FromVoidPtr(v);
}

PyDoc_STRVAR(id_doc,
"id(object) -> integer\n\
\n\
Return the identity of an object.  This is guaranteed to be unique among\n\
simultaneously existing objects.  (Hint: it's the object's memory address.)");


static PyObject *
builtin_map(PyObject *self, PyObject *args)
{
	typedef struct {
		PyObject *it;	/* the iterator object */
		int saw_StopIteration;  /* bool:  did the iterator end? */
	} sequence;

	PyObject *func, *result;
	sequence *seqs = NULL, *sqp;
	Py_ssize_t n, len;
	register int i, j;

	n = PyTuple_Size(args);
	if (n < 2) {
		PyErr_SetString(PyExc_TypeError,
				"map() requires at least two args");
		return NULL;
	}

	func = PyTuple_GetItem(args, 0);
	n--;

	if (func == Py_None) {
		if (Py_Py3kWarningFlag &&
		    PyErr_Warn(PyExc_DeprecationWarning, 
			       "map(None, ...) not supported in 3.x; "
			       "use list(...)") < 0)
			return NULL;
		if (n == 1) {
			/* map(None, S) is the same as list(S). */
			return PySequence_List(PyTuple_GetItem(args, 1));
		}
	}

	/* Get space for sequence descriptors.  Must NULL out the iterator
	 * pointers so that jumping to Fail_2 later doesn't see trash.
	 */
	if ((seqs = PyMem_NEW(sequence, n)) == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	for (i = 0; i < n; ++i) {
		seqs[i].it = (PyObject*)NULL;
		seqs[i].saw_StopIteration = 0;
	}

	/* Do a first pass to obtain iterators for the arguments, and set len
	 * to the largest of their lengths.
	 */
	len = 0;
	for (i = 0, sqp = seqs; i < n; ++i, ++sqp) {
		PyObject *curseq;
		Py_ssize_t curlen;

		/* Get iterator. */
		curseq = PyTuple_GetItem(args, i+1);
		sqp->it = PyObject_GetIter(curseq);
		if (sqp->it == NULL) {
			static char errmsg[] =
			    "argument %d to map() must support iteration";
			char errbuf[sizeof(errmsg) + 25];
			PyOS_snprintf(errbuf, sizeof(errbuf), errmsg, i+2);
			PyErr_SetString(PyExc_TypeError, errbuf);
			goto Fail_2;
		}

		/* Update len. */
		curlen = _PyObject_LengthHint(curseq, 8);
		if (curlen > len)
			len = curlen;
	}

	/* Get space for the result list. */
	if ((result = (PyObject *) PyList_New(len)) == NULL)
		goto Fail_2;

	/* Iterate over the sequences until all have stopped. */
	for (i = 0; ; ++i) {
		PyObject *alist, *item=NULL, *value;
		int numactive = 0;

		if (func == Py_None && n == 1)
			alist = NULL;
		else if ((alist = PyTuple_New(n)) == NULL)
			goto Fail_1;

		for (j = 0, sqp = seqs; j < n; ++j, ++sqp) {
			if (sqp->saw_StopIteration) {
				Py_INCREF(Py_None);
				item = Py_None;
			}
			else {
				item = PyIter_Next(sqp->it);
				if (item)
					++numactive;
				else {
					if (PyErr_Occurred()) {
						Py_XDECREF(alist);
						goto Fail_1;
					}
					Py_INCREF(Py_None);
					item = Py_None;
					sqp->saw_StopIteration = 1;
				}
			}
			if (alist)
				PyTuple_SET_ITEM(alist, j, item);
			else
				break;
		}

		if (!alist)
			alist = item;

		if (numactive == 0) {
			Py_DECREF(alist);
			break;
		}

		if (func == Py_None)
			value = alist;
		else {
			value = PyEval_CallObject(func, alist);
			Py_DECREF(alist);
			if (value == NULL)
				goto Fail_1;
		}
		if (i >= len) {
			int status = PyList_Append(result, value);
			Py_DECREF(value);
			if (status < 0)
				goto Fail_1;
		}
		else if (PyList_SetItem(result, i, value) < 0)
		 	goto Fail_1;
	}

	if (i < len && PyList_SetSlice(result, i, len, NULL) < 0)
		goto Fail_1;

	goto Succeed;

Fail_1:
	Py_DECREF(result);
Fail_2:
	result = NULL;
Succeed:
	assert(seqs);
	for (i = 0; i < n; ++i)
		Py_XDECREF(seqs[i].it);
	PyMem_DEL(seqs);
	return result;
}

PyDoc_STRVAR(map_doc,
"map(function, sequence[, sequence, ...]) -> list\n\
\n\
Return a list of the results of applying the function to the items of\n\
the argument sequence(s).  If more than one sequence is given, the\n\
function is called with an argument list consisting of the corresponding\n\
item of each sequence, substituting None for missing values when not all\n\
sequences have the same length.  If the function is None, return a list of\n\
the items of the sequence (or a list of tuples if more than one sequence).");


static PyObject *
builtin_setattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;
	PyObject *value;

	if (!PyArg_UnpackTuple(args, "setattr", 3, 3, &v, &name, &value))
		return NULL;
	if (PyObject_SetAttr(v, name, value) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(setattr_doc,
"setattr(object, name, value)\n\
\n\
Set a named attribute on an object; setattr(x, 'y', v) is equivalent to\n\
``x.y = v''.");


static PyObject *
builtin_delattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_UnpackTuple(args, "delattr", 2, 2, &v, &name))
		return NULL;
	if (PyObject_SetAttr(v, name, (PyObject *)NULL) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(delattr_doc,
"delattr(object, name)\n\
\n\
Delete a named attribute on an object; delattr(x, 'y') is equivalent to\n\
``del x.y''.");


static PyObject *
builtin_hash(PyObject *self, PyObject *v)
{
	long x;

	x = PyObject_Hash(v);
	if (x == -1)
		return NULL;
	return PyInt_FromLong(x);
}

PyDoc_STRVAR(hash_doc,
"hash(object) -> integer\n\
\n\
Return a hash value for the object.  Two objects with the same value have\n\
the same hash value.  The reverse is not necessarily true, but likely.");


static PyObject *
builtin_hex(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;
	PyObject *res;

	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_hex == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "hex() argument can't be converted to hex");
		return NULL;
	}
	res = (*nb->nb_hex)(v);
	if (res && !PyString_Check(res)) {
		PyErr_Format(PyExc_TypeError,
			     "__hex__ returned non-string (type %.200s)",
			     res->ob_type->tp_name);
		Py_DECREF(res);
		return NULL;
	}
	return res;
}

PyDoc_STRVAR(hex_doc,
"hex(number) -> string\n\
\n\
Return the hexadecimal representation of an integer or long integer.");


static PyObject *builtin_raw_input(PyObject *, PyObject *);

static PyObject *
builtin_input(PyObject *self, PyObject *args)
{
	PyObject *line;
	char *str;
	PyObject *res;
	PyObject *globals, *locals;
	PyCompilerFlags cf;

	line = builtin_raw_input(self, args);
	if (line == NULL)
		return line;
	if (!PyArg_Parse(line, "s;embedded '\\0' in input line", &str))
		return NULL;
	while (*str == ' ' || *str == '\t')
			str++;
	globals = PyEval_GetGlobals();
	locals = PyEval_GetLocals();
	if (PyDict_GetItemString(globals, "__builtins__") == NULL) {
		if (PyDict_SetItemString(globals, "__builtins__",
					 PyEval_GetBuiltins()) != 0)
			return NULL;
	}
	cf.cf_flags = 0;
	PyEval_MergeCompilerFlags(&cf);
	res = PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
	Py_DECREF(line);
	return res;
}

PyDoc_STRVAR(input_doc,
"input([prompt]) -> value\n\
\n\
Equivalent to eval(raw_input(prompt)).");


static PyObject *
builtin_intern(PyObject *self, PyObject *args)
{
	PyObject *s;
	if (!PyArg_ParseTuple(args, "S:intern", &s))
		return NULL;
	if (!PyString_CheckExact(s)) {
		PyErr_SetString(PyExc_TypeError,
				"can't intern subclass of string");
		return NULL;
	}
	Py_INCREF(s);
	PyString_InternInPlace(&s);
	return s;
}

PyDoc_STRVAR(intern_doc,
"intern(string) -> string\n\
\n\
``Intern'' the given string.  This enters the string in the (global)\n\
table of interned strings whose purpose is to speed up dictionary lookups.\n\
Return the string itself or the previously interned string object with the\n\
same value.");


static PyObject *
builtin_iter(PyObject *self, PyObject *args)
{
	PyObject *v, *w = NULL;

	if (!PyArg_UnpackTuple(args, "iter", 1, 2, &v, &w))
		return NULL;
	if (w == NULL)
		return PyObject_GetIter(v);
	if (!PyCallable_Check(v)) {
		PyErr_SetString(PyExc_TypeError,
				"iter(v, w): v must be callable");
		return NULL;
	}
	return PyCallIter_New(v, w);
}

PyDoc_STRVAR(iter_doc,
"iter(collection) -> iterator\n\
iter(callable, sentinel) -> iterator\n\
\n\
Get an iterator from an object.  In the first form, the argument must\n\
supply its own iterator, or be a sequence.\n\
In the second form, the callable is called until it returns the sentinel.");


static PyObject *
builtin_len(PyObject *self, PyObject *v)
{
	Py_ssize_t res;

	res = PyObject_Size(v);
	if (res < 0 && PyErr_Occurred())
		return NULL;
	return PyInt_FromSsize_t(res);
}

PyDoc_STRVAR(len_doc,
"len(object) -> integer\n\
\n\
Return the number of items of a sequence or mapping.");


static PyObject *
builtin_locals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetLocals();
	Py_XINCREF(d);
	return d;
}

PyDoc_STRVAR(locals_doc,
"locals() -> dictionary\n\
\n\
Update and return a dictionary containing the current scope's local variables.");


static PyObject *
min_max(PyObject *args, PyObject *kwds, int op)
{
	PyObject *v, *it, *item, *val, *maxitem, *maxval, *keyfunc=NULL;
	const char *name = op == Py_LT ? "min" : "max";

	if (PyTuple_Size(args) > 1)
		v = args;
	else if (!PyArg_UnpackTuple(args, (char *)name, 1, 1, &v))
		return NULL;

	if (kwds != NULL && PyDict_Check(kwds) && PyDict_Size(kwds)) {
		keyfunc = PyDict_GetItemString(kwds, "key");
		if (PyDict_Size(kwds)!=1  ||  keyfunc == NULL) {
			PyErr_Format(PyExc_TypeError,
				"%s() got an unexpected keyword argument", name);
			return NULL;
		}
		Py_INCREF(keyfunc);
	}

	it = PyObject_GetIter(v);
	if (it == NULL) {
		Py_XDECREF(keyfunc);
		return NULL;
	}

	maxitem = NULL; /* the result */
	maxval = NULL;  /* the value associated with the result */
	while (( item = PyIter_Next(it) )) {
		/* get the value from the key function */
		if (keyfunc != NULL) {
			val = PyObject_CallFunctionObjArgs(keyfunc, item, NULL);
			if (val == NULL)
				goto Fail_it_item;
		}
		/* no key function; the value is the item */
		else {
			val = item;
			Py_INCREF(val);
		}

		/* maximum value and item are unset; set them */
		if (maxval == NULL) {
			maxitem = item;
			maxval = val;
		}
		/* maximum value and item are set; update them as necessary */
		else {
			int cmp = PyObject_RichCompareBool(val, maxval, op);
			if (cmp < 0)
				goto Fail_it_item_and_val;
			else if (cmp > 0) {
				Py_DECREF(maxval);
				Py_DECREF(maxitem);
				maxval = val;
				maxitem = item;
			}
			else {
				Py_DECREF(item);
				Py_DECREF(val);
			}
		}
	}
	if (PyErr_Occurred())
		goto Fail_it;
	if (maxval == NULL) {
		PyErr_Format(PyExc_ValueError,
			     "%s() arg is an empty sequence", name);
		assert(maxitem == NULL);
	}
	else
		Py_DECREF(maxval);
	Py_DECREF(it);
	Py_XDECREF(keyfunc);
	return maxitem;

Fail_it_item_and_val:
	Py_DECREF(val);
Fail_it_item:
	Py_DECREF(item);
Fail_it:
	Py_XDECREF(maxval);
	Py_XDECREF(maxitem);
	Py_DECREF(it);
	Py_XDECREF(keyfunc);
	return NULL;
}

static PyObject *
builtin_min(PyObject *self, PyObject *args, PyObject *kwds)
{
	return min_max(args, kwds, Py_LT);
}

PyDoc_STRVAR(min_doc,
"min(iterable[, key=func]) -> value\n\
min(a, b, c, ...[, key=func]) -> value\n\
\n\
With a single iterable argument, return its smallest item.\n\
With two or more arguments, return the smallest argument.");


static PyObject *
builtin_max(PyObject *self, PyObject *args, PyObject *kwds)
{
	return min_max(args, kwds, Py_GT);
}

PyDoc_STRVAR(max_doc,
"max(iterable[, key=func]) -> value\n\
max(a, b, c, ...[, key=func]) -> value\n\
\n\
With a single iterable argument, return its largest item.\n\
With two or more arguments, return the largest argument.");


static PyObject *
builtin_oct(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;
	PyObject *res;

	if (v == NULL || (nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_oct == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "oct() argument can't be converted to oct");
		return NULL;
	}
	res = (*nb->nb_oct)(v);
	if (res && !PyString_Check(res)) {
		PyErr_Format(PyExc_TypeError,
			     "__oct__ returned non-string (type %.200s)",
			     res->ob_type->tp_name);
		Py_DECREF(res);
		return NULL;
	}
	return res;
}

PyDoc_STRVAR(oct_doc,
"oct(number) -> string\n\
\n\
Return the octal representation of an integer or long integer.");


static PyObject *
builtin_open(PyObject *self, PyObject *args, PyObject *kwds)
{
	return PyObject_Call((PyObject*)&PyFile_Type, args, kwds);
}

PyDoc_STRVAR(open_doc,
"open(name[, mode[, buffering]]) -> file object\n\
\n\
Open a file using the file() type, returns a file object.  This is the\n\
preferred way to open a file.");


static PyObject *
builtin_ord(PyObject *self, PyObject* obj)
{
	long ord;
	Py_ssize_t size;

	if (PyString_Check(obj)) {
		size = PyString_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)((unsigned char)*PyString_AS_STRING(obj));
			return PyInt_FromLong(ord);
		}
	} else if (PyBytes_Check(obj)) {
		size = PyBytes_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)((unsigned char)*PyBytes_AS_STRING(obj));
			return PyInt_FromLong(ord);
		}

#ifdef Py_USING_UNICODE
	} else if (PyUnicode_Check(obj)) {
		size = PyUnicode_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)*PyUnicode_AS_UNICODE(obj);
			return PyInt_FromLong(ord);
		}
#endif
	} else {
		PyErr_Format(PyExc_TypeError,
			     "ord() expected string of length 1, but " \
			     "%.200s found", obj->ob_type->tp_name);
		return NULL;
	}

	PyErr_Format(PyExc_TypeError,
		     "ord() expected a character, "
		     "but string of length %zd found",
		     size);
	return NULL;
}

PyDoc_STRVAR(ord_doc,
"ord(c) -> integer\n\
\n\
Return the integer ordinal of a one-character string.");


static PyObject *
builtin_pow(PyObject *self, PyObject *args)
{
	PyObject *v, *w, *z = Py_None;

	if (!PyArg_UnpackTuple(args, "pow", 2, 3, &v, &w, &z))
		return NULL;
	return PyNumber_Power(v, w, z);
}

PyDoc_STRVAR(pow_doc,
"pow(x, y[, z]) -> number\n\
\n\
With two arguments, equivalent to x**y.  With three arguments,\n\
equivalent to (x**y) % z, but may be more efficient (e.g. for longs).");


static PyObject *
builtin_print(PyObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"sep", "end", "file", 0};
	static PyObject *dummy_args;
	PyObject *sep = NULL, *end = NULL, *file = NULL;
	int i, err;

	if (dummy_args == NULL) {
		if (!(dummy_args = PyTuple_New(0)))
			return NULL;
	}
	if (!PyArg_ParseTupleAndKeywords(dummy_args, kwds, "|OOO:print",
					 kwlist, &sep, &end, &file))
		return NULL;
	if (file == NULL || file == Py_None) {
		file = PySys_GetObject("stdout");
		/* sys.stdout may be None when FILE* stdout isn't connected */
		if (file == Py_None)
			Py_RETURN_NONE;
	}

	if (sep && sep != Py_None && !PyString_Check(sep) &&
            !PyUnicode_Check(sep)) {
		PyErr_Format(PyExc_TypeError,
			     "sep must be None, str or unicode, not %.200s",
			     sep->ob_type->tp_name);
		return NULL;
	}
	if (end && end != Py_None && !PyString_Check(end) &&
	    !PyUnicode_Check(end)) {
		PyErr_Format(PyExc_TypeError,
			     "end must be None, str or unicode, not %.200s",
			     end->ob_type->tp_name);
		return NULL;
	}

	for (i = 0; i < PyTuple_Size(args); i++) {
		if (i > 0) {
			if (sep == NULL || sep == Py_None)
				err = PyFile_WriteString(" ", file);
			else
				err = PyFile_WriteObject(sep, file,
							 Py_PRINT_RAW);
			if (err)
				return NULL;
		}
		err = PyFile_WriteObject(PyTuple_GetItem(args, i), file,
					 Py_PRINT_RAW);
		if (err)
			return NULL;
	}

	if (end == NULL || end == Py_None)
		err = PyFile_WriteString("\n", file);
	else
		err = PyFile_WriteObject(end, file, Py_PRINT_RAW);
	if (err)
		return NULL;

	Py_RETURN_NONE;
}

PyDoc_STRVAR(print_doc,
"print(value, ..., sep=' ', end='\\n', file=sys.stdout)\n\
\n\
Prints the values to a stream, or to sys.stdout by default.\n\
Optional keyword arguments:\n\
file: a file-like object (stream); defaults to the current sys.stdout.\n\
sep:  string inserted between values, default a space.\n\
end:  string appended after the last value, default a newline.");


/* Return number of items in range (lo, hi, step), when arguments are
 * PyInt or PyLong objects.  step > 0 required.  Return a value < 0 if
 * & only if the true value is too large to fit in a signed long.
 * Arguments MUST return 1 with either PyInt_Check() or
 * PyLong_Check().  Return -1 when there is an error.
 */
static long
get_len_of_range_longs(PyObject *lo, PyObject *hi, PyObject *step)
{
	/* -------------------------------------------------------------
	Algorithm is equal to that of get_len_of_range(), but it operates
	on PyObjects (which are assumed to be PyLong or PyInt objects).
	---------------------------------------------------------------*/
	long n;
	PyObject *diff = NULL;
	PyObject *one = NULL;
	PyObject *tmp1 = NULL, *tmp2 = NULL, *tmp3 = NULL;
		/* holds sub-expression evaluations */

	/* if (lo >= hi), return length of 0. */
	if (PyObject_Compare(lo, hi) >= 0)
		return 0;

	if ((one = PyLong_FromLong(1L)) == NULL)
		goto Fail;

	if ((tmp1 = PyNumber_Subtract(hi, lo)) == NULL)
		goto Fail;

	if ((diff = PyNumber_Subtract(tmp1, one)) == NULL)
		goto Fail;

	if ((tmp2 = PyNumber_FloorDivide(diff, step)) == NULL)
		goto Fail;

	if ((tmp3 = PyNumber_Add(tmp2, one)) == NULL)
		goto Fail;

	n = PyLong_AsLong(tmp3);
	if (PyErr_Occurred()) {  /* Check for Overflow */
		PyErr_Clear();
		goto Fail;
	}

	Py_DECREF(tmp3);
	Py_DECREF(tmp2);
	Py_DECREF(diff);
	Py_DECREF(tmp1);
	Py_DECREF(one);
	return n;

  Fail:
	Py_XDECREF(tmp3);
	Py_XDECREF(tmp2);
	Py_XDECREF(diff);
	Py_XDECREF(tmp1);
	Py_XDECREF(one);
	return -1;
}

/* An extension of builtin_range() that handles the case when PyLong
 * arguments are given. */
static PyObject *
handle_range_longs(PyObject *self, PyObject *args)
{
	PyObject *ilow;
	PyObject *ihigh = NULL;
	PyObject *istep = NULL;

	PyObject *curnum = NULL;
	PyObject *v = NULL;
	long bign;
	int i, n;
	int cmp_result;

	PyObject *zero = PyLong_FromLong(0);

	if (zero == NULL)
		return NULL;

	if (!PyArg_UnpackTuple(args, "range", 1, 3, &ilow, &ihigh, &istep)) {
		Py_DECREF(zero);
		return NULL;
	}

	/* Figure out which way we were called, supply defaults, and be
	 * sure to incref everything so that the decrefs at the end
	 * are correct.
	 */
	assert(ilow != NULL);
	if (ihigh == NULL) {
		/* only 1 arg -- it's the upper limit */
		ihigh = ilow;
		ilow = NULL;
	}
	assert(ihigh != NULL);
	Py_INCREF(ihigh);

	/* ihigh correct now; do ilow */
	if (ilow == NULL)
		ilow = zero;
	Py_INCREF(ilow);

	/* ilow and ihigh correct now; do istep */
	if (istep == NULL) {
		istep = PyLong_FromLong(1L);
		if (istep == NULL)
			goto Fail;
	}
	else {
		Py_INCREF(istep);
	}

	if (!PyInt_Check(ilow) && !PyLong_Check(ilow)) {
		PyErr_Format(PyExc_TypeError,
			     "range() integer start argument expected, got %s.",
			     ilow->ob_type->tp_name);
		goto Fail;
	}

	if (!PyInt_Check(ihigh) && !PyLong_Check(ihigh)) {
		PyErr_Format(PyExc_TypeError,
			     "range() integer end argument expected, got %s.",
			     ihigh->ob_type->tp_name);
		goto Fail;
	}

	if (!PyInt_Check(istep) && !PyLong_Check(istep)) {
		PyErr_Format(PyExc_TypeError,
			     "range() integer step argument expected, got %s.",
			     istep->ob_type->tp_name);
		goto Fail;
	}

	if (PyObject_Cmp(istep, zero, &cmp_result) == -1)
		goto Fail;
	if (cmp_result == 0) {
		PyErr_SetString(PyExc_ValueError,
				"range() step argument must not be zero");
		goto Fail;
	}

	if (cmp_result > 0)
		bign = get_len_of_range_longs(ilow, ihigh, istep);
	else {
		PyObject *neg_istep = PyNumber_Negative(istep);
		if (neg_istep == NULL)
			goto Fail;
		bign = get_len_of_range_longs(ihigh, ilow, neg_istep);
		Py_DECREF(neg_istep);
	}

	n = (int)bign;
	if (bign < 0 || (long)n != bign) {
		PyErr_SetString(PyExc_OverflowError,
				"range() result has too many items");
		goto Fail;
	}

	v = PyList_New(n);
	if (v == NULL)
		goto Fail;

	curnum = ilow;
	Py_INCREF(curnum);

	for (i = 0; i < n; i++) {
		PyObject *w = PyNumber_Long(curnum);
		PyObject *tmp_num;
		if (w == NULL)
			goto Fail;

		PyList_SET_ITEM(v, i, w);

		tmp_num = PyNumber_Add(curnum, istep);
		if (tmp_num == NULL)
			goto Fail;

		Py_DECREF(curnum);
		curnum = tmp_num;
	}
	Py_DECREF(ilow);
	Py_DECREF(ihigh);
	Py_DECREF(istep);
	Py_DECREF(zero);
	Py_DECREF(curnum);
	return v;

  Fail:
	Py_DECREF(ilow);
	Py_DECREF(ihigh);
	Py_XDECREF(istep);
	Py_DECREF(zero);
	Py_XDECREF(curnum);
	Py_XDECREF(v);
	return NULL;
}

/* Return number of items in range/xrange (lo, hi, step).  step > 0
 * required.  Return a value < 0 if & only if the true value is too
 * large to fit in a signed long.
 */
static long
get_len_of_range(long lo, long hi, long step)
{
	/* -------------------------------------------------------------
	If lo >= hi, the range is empty.
	Else if n values are in the range, the last one is
	lo + (n-1)*step, which must be <= hi-1.  Rearranging,
	n <= (hi - lo - 1)/step + 1, so taking the floor of the RHS gives
	the proper value.  Since lo < hi in this case, hi-lo-1 >= 0, so
	the RHS is non-negative and so truncation is the same as the
	floor.  Letting M be the largest positive long, the worst case
	for the RHS numerator is hi=M, lo=-M-1, and then
	hi-lo-1 = M-(-M-1)-1 = 2*M.  Therefore unsigned long has enough
	precision to compute the RHS exactly.
	---------------------------------------------------------------*/
	long n = 0;
	if (lo < hi) {
		unsigned long uhi = (unsigned long)hi;
		unsigned long ulo = (unsigned long)lo;
		unsigned long diff = uhi - ulo - 1;
		n = (long)(diff / (unsigned long)step + 1);
	}
	return n;
}

static PyObject *
builtin_range(PyObject *self, PyObject *args)
{
	long ilow = 0, ihigh = 0, istep = 1;
	long bign;
	int i, n;

	PyObject *v;

	if (PyTuple_Size(args) <= 1) {
		if (!PyArg_ParseTuple(args,
				"l;range() requires 1-3 int arguments",
				&ihigh)) {
			PyErr_Clear();
			return handle_range_longs(self, args);
		}
	}
	else {
		if (!PyArg_ParseTuple(args,
				"ll|l;range() requires 1-3 int arguments",
				&ilow, &ihigh, &istep)) {
			PyErr_Clear();
			return handle_range_longs(self, args);
		}
	}
	if (istep == 0) {
		PyErr_SetString(PyExc_ValueError,
				"range() step argument must not be zero");
		return NULL;
	}
	if (istep > 0)
		bign = get_len_of_range(ilow, ihigh, istep);
	else
		bign = get_len_of_range(ihigh, ilow, -istep);
	n = (int)bign;
	if (bign < 0 || (long)n != bign) {
		PyErr_SetString(PyExc_OverflowError,
				"range() result has too many items");
		return NULL;
	}
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyObject *w = PyInt_FromLong(ilow);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SET_ITEM(v, i, w);
		ilow += istep;
	}
	return v;
}

PyDoc_STRVAR(range_doc,
"range([start,] stop[, step]) -> list of integers\n\
\n\
Return a list containing an arithmetic progression of integers.\n\
range(i, j) returns [i, i+1, i+2, ..., j-1]; start (!) defaults to 0.\n\
When step is given, it specifies the increment (or decrement).\n\
For example, range(4) returns [0, 1, 2, 3].  The end point is omitted!\n\
These are exactly the valid indices for a list of 4 elements.");


static PyObject *
builtin_raw_input(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *fin = PySys_GetObject("stdin");
	PyObject *fout = PySys_GetObject("stdout");

	if (!PyArg_UnpackTuple(args, "[raw_]input", 0, 1, &v))
		return NULL;

	if (fin == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "[raw_]input: lost sys.stdin");
		return NULL;
	}
	if (fout == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "[raw_]input: lost sys.stdout");
		return NULL;
	}
	if (PyFile_SoftSpace(fout, 0)) {
		if (PyFile_WriteString(" ", fout) != 0)
			return NULL;
	}
	if (PyFile_AsFile(fin) && PyFile_AsFile(fout)
            && isatty(fileno(PyFile_AsFile(fin)))
            && isatty(fileno(PyFile_AsFile(fout)))) {
		PyObject *po;
		char *prompt;
		char *s;
		PyObject *result;
		if (v != NULL) {
			po = PyObject_Str(v);
			if (po == NULL)
				return NULL;
			prompt = PyString_AsString(po);
			if (prompt == NULL)
				return NULL;
		}
		else {
			po = NULL;
			prompt = "";
		}
		s = PyOS_Readline(PyFile_AsFile(fin), PyFile_AsFile(fout),
                                  prompt);
		Py_XDECREF(po);
		if (s == NULL) {
			if (!PyErr_Occurred())
				PyErr_SetNone(PyExc_KeyboardInterrupt);
			return NULL;
		}
		if (*s == '\0') {
			PyErr_SetNone(PyExc_EOFError);
			result = NULL;
		}
		else { /* strip trailing '\n' */
			size_t len = strlen(s);
			if (len > PY_SSIZE_T_MAX) {
				PyErr_SetString(PyExc_OverflowError,
						"[raw_]input: input too long");
				result = NULL;
			}
			else {
				result = PyString_FromStringAndSize(s, len-1);
			}
		}
		PyMem_FREE(s);
		return result;
	}
	if (v != NULL) {
		if (PyFile_WriteObject(v, fout, Py_PRINT_RAW) != 0)
			return NULL;
	}
	return PyFile_GetLine(fin, -1);
}

PyDoc_STRVAR(raw_input_doc,
"raw_input([prompt]) -> string\n\
\n\
Read a string from standard input.  The trailing newline is stripped.\n\
If the user hits EOF (Unix: Ctl-D, Windows: Ctl-Z+Return), raise EOFError.\n\
On Unix, GNU readline is used if enabled.  The prompt string, if given,\n\
is printed without a trailing newline before reading.");


static PyObject *
builtin_reduce(PyObject *self, PyObject *args)
{
	PyObject *seq, *func, *result = NULL, *it;

	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "reduce() not supported in 3.x; "
		       "use functools.reduce()") < 0)
		return NULL;

	if (!PyArg_UnpackTuple(args, "reduce", 2, 3, &func, &seq, &result))
		return NULL;
	if (result != NULL)
		Py_INCREF(result);

	it = PyObject_GetIter(seq);
	if (it == NULL) {
		PyErr_SetString(PyExc_TypeError,
		    "reduce() arg 2 must support iteration");
		Py_XDECREF(result);
		return NULL;
	}

	if ((args = PyTuple_New(2)) == NULL)
		goto Fail;

	for (;;) {
		PyObject *op2;

		if (args->ob_refcnt > 1) {
			Py_DECREF(args);
			if ((args = PyTuple_New(2)) == NULL)
				goto Fail;
		}

		op2 = PyIter_Next(it);
		if (op2 == NULL) {
			if (PyErr_Occurred())
				goto Fail;
 			break;
		}

		if (result == NULL)
			result = op2;
		else {
			PyTuple_SetItem(args, 0, result);
			PyTuple_SetItem(args, 1, op2);
			if ((result = PyEval_CallObject(func, args)) == NULL)
				goto Fail;
		}
	}

	Py_DECREF(args);

	if (result == NULL)
		PyErr_SetString(PyExc_TypeError,
			   "reduce() of empty sequence with no initial value");

	Py_DECREF(it);
	return result;

Fail:
	Py_XDECREF(args);
	Py_XDECREF(result);
	Py_DECREF(it);
	return NULL;
}

PyDoc_STRVAR(reduce_doc,
"reduce(function, sequence[, initial]) -> value\n\
\n\
Apply a function of two arguments cumulatively to the items of a sequence,\n\
from left to right, so as to reduce the sequence to a single value.\n\
For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates\n\
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items\n\
of the sequence in the calculation, and serves as a default when the\n\
sequence is empty.");


static PyObject *
builtin_reload(PyObject *self, PyObject *v)
{
	if (Py_Py3kWarningFlag &&
	    PyErr_Warn(PyExc_DeprecationWarning, 
		       "reload() not supported in 3.x; use imp.reload()") < 0)
		return NULL;

	return PyImport_ReloadModule(v);
}

PyDoc_STRVAR(reload_doc,
"reload(module) -> module\n\
\n\
Reload the module.  The module must have been successfully imported before.");


static PyObject *
builtin_repr(PyObject *self, PyObject *v)
{
	return PyObject_Repr(v);
}

PyDoc_STRVAR(repr_doc,
"repr(object) -> string\n\
\n\
Return the canonical string representation of the object.\n\
For most object types, eval(repr(object)) == object.");


static PyObject *
builtin_round(PyObject *self, PyObject *args, PyObject *kwds)
{
	double number;
	double f;
	int ndigits = 0;
	int i;
	static char *kwlist[] = {"number", "ndigits", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "d|i:round",
		kwlist, &number, &ndigits))
		return NULL;
	f = 1.0;
	i = abs(ndigits);
	while  (--i >= 0)
		f = f*10.0;
	if (ndigits < 0)
		number /= f;
	else
		number *= f;
	if (number >= 0.0)
		number = floor(number + 0.5);
	else
		number = ceil(number - 0.5);
	if (ndigits < 0)
		number *= f;
	else
		number /= f;
	return PyFloat_FromDouble(number);
}

PyDoc_STRVAR(round_doc,
"round(number[, ndigits]) -> floating point number\n\
\n\
Round a number to a given precision in decimal digits (default 0 digits).\n\
This always returns a floating point number.  Precision may be negative.");

static PyObject *
builtin_sorted(PyObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *newlist, *v, *seq, *compare=NULL, *keyfunc=NULL, *newargs;
	PyObject *callable;
	static char *kwlist[] = {"iterable", "cmp", "key", "reverse", 0};
	int reverse;

	/* args 1-4 should match listsort in Objects/listobject.c */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOi:sorted",
		kwlist, &seq, &compare, &keyfunc, &reverse))
		return NULL;

	newlist = PySequence_List(seq);
	if (newlist == NULL)
		return NULL;

	callable = PyObject_GetAttrString(newlist, "sort");
	if (callable == NULL) {
		Py_DECREF(newlist);
		return NULL;
	}

	newargs = PyTuple_GetSlice(args, 1, 4);
	if (newargs == NULL) {
		Py_DECREF(newlist);
		Py_DECREF(callable);
		return NULL;
	}

	v = PyObject_Call(callable, newargs, kwds);
	Py_DECREF(newargs);
	Py_DECREF(callable);
	if (v == NULL) {
		Py_DECREF(newlist);
		return NULL;
	}
	Py_DECREF(v);
	return newlist;
}

PyDoc_STRVAR(sorted_doc,
"sorted(iterable, cmp=None, key=None, reverse=False) --> new sorted list");

static PyObject *
builtin_vars(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *d;

	if (!PyArg_UnpackTuple(args, "vars", 0, 1, &v))
		return NULL;
	if (v == NULL) {
		d = PyEval_GetLocals();
		if (d == NULL) {
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_SystemError,
						"vars(): no locals!?");
		}
		else
			Py_INCREF(d);
	}
	else {
		d = PyObject_GetAttrString(v, "__dict__");
		if (d == NULL) {
			PyErr_SetString(PyExc_TypeError,
			    "vars() argument must have __dict__ attribute");
			return NULL;
		}
	}
	return d;
}

PyDoc_STRVAR(vars_doc,
"vars([object]) -> dictionary\n\
\n\
Without arguments, equivalent to locals().\n\
With an argument, equivalent to object.__dict__.");


static PyObject*
builtin_sum(PyObject *self, PyObject *args)
{
	PyObject *seq;
	PyObject *result = NULL;
	PyObject *temp, *item, *iter;

	if (!PyArg_UnpackTuple(args, "sum", 1, 2, &seq, &result))
		return NULL;

	iter = PyObject_GetIter(seq);
	if (iter == NULL)
		return NULL;

	if (result == NULL) {
		result = PyInt_FromLong(0);
		if (result == NULL) {
			Py_DECREF(iter);
			return NULL;
		}
	} else {
		/* reject string values for 'start' parameter */
		if (PyObject_TypeCheck(result, &PyBaseString_Type)) {
			PyErr_SetString(PyExc_TypeError,
				"sum() can't sum strings [use ''.join(seq) instead]");
			Py_DECREF(iter);
			return NULL;
		}
		Py_INCREF(result);
	}

#ifndef SLOW_SUM
	/* Fast addition by keeping temporary sums in C instead of new Python objects.
           Assumes all inputs are the same type.  If the assumption fails, default
           to the more general routine.
	*/
	if (PyInt_CheckExact(result)) {
		long i_result = PyInt_AS_LONG(result);
		Py_DECREF(result);
		result = NULL;
		while(result == NULL) {
			item = PyIter_Next(iter);
			if (item == NULL) {
				Py_DECREF(iter);
				if (PyErr_Occurred())
					return NULL;
    				return PyInt_FromLong(i_result);
			}
        		if (PyInt_CheckExact(item)) {
            			long b = PyInt_AS_LONG(item);
				long x = i_result + b;
				if ((x^i_result) >= 0 || (x^b) >= 0) {
					i_result = x;
					Py_DECREF(item);
					continue;
				}
			}
			/* Either overflowed or is not an int. Restore real objects and process normally */
			result = PyInt_FromLong(i_result);
			temp = PyNumber_Add(result, item);
			Py_DECREF(result);
			Py_DECREF(item);
			result = temp;
			if (result == NULL) {
				Py_DECREF(iter);
				return NULL;
			}
		}
	}

	if (PyFloat_CheckExact(result)) {
		double f_result = PyFloat_AS_DOUBLE(result);
		Py_DECREF(result);
		result = NULL;
		while(result == NULL) {
			item = PyIter_Next(iter);
			if (item == NULL) {
				Py_DECREF(iter);
				if (PyErr_Occurred())
					return NULL;
    				return PyFloat_FromDouble(f_result);
			}
        		if (PyFloat_CheckExact(item)) {
				PyFPE_START_PROTECT("add", return 0)
				f_result += PyFloat_AS_DOUBLE(item);
				PyFPE_END_PROTECT(f_result)
				Py_DECREF(item);
				continue;
			}
        		if (PyInt_CheckExact(item)) {
				PyFPE_START_PROTECT("add", return 0)
				f_result += (double)PyInt_AS_LONG(item);
				PyFPE_END_PROTECT(f_result)
				Py_DECREF(item);
				continue;
			}
			result = PyFloat_FromDouble(f_result);
			temp = PyNumber_Add(result, item);
			Py_DECREF(result);
			Py_DECREF(item);
			result = temp;
			if (result == NULL) {
				Py_DECREF(iter);
				return NULL;
			}
		}
	}
#endif

	for(;;) {
		item = PyIter_Next(iter);
		if (item == NULL) {
			/* error, or end-of-sequence */
			if (PyErr_Occurred()) {
				Py_DECREF(result);
				result = NULL;
			}
			break;
		}
		temp = PyNumber_Add(result, item);
		Py_DECREF(result);
		Py_DECREF(item);
		result = temp;
		if (result == NULL)
			break;
	}
	Py_DECREF(iter);
	return result;
}

PyDoc_STRVAR(sum_doc,
"sum(sequence[, start]) -> value\n\
\n\
Returns the sum of a sequence of numbers (NOT strings) plus the value\n\
of parameter 'start' (which defaults to 0).  When the sequence is\n\
empty, returns start.");


static PyObject *
builtin_isinstance(PyObject *self, PyObject *args)
{
	PyObject *inst;
	PyObject *cls;
	int retval;

	if (!PyArg_UnpackTuple(args, "isinstance", 2, 2, &inst, &cls))
		return NULL;

	retval = PyObject_IsInstance(inst, cls);
	if (retval < 0)
		return NULL;
	return PyBool_FromLong(retval);
}

PyDoc_STRVAR(isinstance_doc,
"isinstance(object, class-or-type-or-tuple) -> bool\n\
\n\
Return whether an object is an instance of a class or of a subclass thereof.\n\
With a type as second argument, return whether that is the object's type.\n\
The form using a tuple, isinstance(x, (A, B, ...)), is a shortcut for\n\
isinstance(x, A) or isinstance(x, B) or ... (etc.).");


static PyObject *
builtin_issubclass(PyObject *self, PyObject *args)
{
	PyObject *derived;
	PyObject *cls;
	int retval;

	if (!PyArg_UnpackTuple(args, "issubclass", 2, 2, &derived, &cls))
		return NULL;

	retval = PyObject_IsSubclass(derived, cls);
	if (retval < 0)
		return NULL;
	return PyBool_FromLong(retval);
}

PyDoc_STRVAR(issubclass_doc,
"issubclass(C, B) -> bool\n\
\n\
Return whether class C is a subclass (i.e., a derived class) of class B.\n\
When using a tuple as the second argument issubclass(X, (A, B, ...)),\n\
is a shortcut for issubclass(X, A) or issubclass(X, B) or ... (etc.).");


static PyObject*
builtin_zip(PyObject *self, PyObject *args)
{
	PyObject *ret;
	const Py_ssize_t itemsize = PySequence_Length(args);
	Py_ssize_t i;
	PyObject *itlist;  /* tuple of iterators */
	Py_ssize_t len;	   /* guess at result length */

	if (itemsize == 0)
		return PyList_New(0);

	/* args must be a tuple */
	assert(PyTuple_Check(args));

	/* Guess at result length:  the shortest of the input lengths.
	   If some argument refuses to say, we refuse to guess too, lest
	   an argument like xrange(sys.maxint) lead us astray.*/
	len = -1;	/* unknown */
	for (i = 0; i < itemsize; ++i) {
		PyObject *item = PyTuple_GET_ITEM(args, i);
		Py_ssize_t thislen = _PyObject_LengthHint(item, -1);
		if (thislen < 0) {
			len = -1;
			break;
		}
		else if (len < 0 || thislen < len)
			len = thislen;
	}

	/* allocate result list */
	if (len < 0)
		len = 10;	/* arbitrary */
	if ((ret = PyList_New(len)) == NULL)
		return NULL;

	/* obtain iterators */
	itlist = PyTuple_New(itemsize);
	if (itlist == NULL)
		goto Fail_ret;
	for (i = 0; i < itemsize; ++i) {
		PyObject *item = PyTuple_GET_ITEM(args, i);
		PyObject *it = PyObject_GetIter(item);
		if (it == NULL) {
			if (PyErr_ExceptionMatches(PyExc_TypeError))
				PyErr_Format(PyExc_TypeError,
				    "zip argument #%zd must support iteration",
				    i+1);
			goto Fail_ret_itlist;
		}
		PyTuple_SET_ITEM(itlist, i, it);
	}

	/* build result into ret list */
	for (i = 0; ; ++i) {
		int j;
		PyObject *next = PyTuple_New(itemsize);
		if (!next)
			goto Fail_ret_itlist;

		for (j = 0; j < itemsize; j++) {
			PyObject *it = PyTuple_GET_ITEM(itlist, j);
			PyObject *item = PyIter_Next(it);
			if (!item) {
				if (PyErr_Occurred()) {
					Py_DECREF(ret);
					ret = NULL;
				}
				Py_DECREF(next);
				Py_DECREF(itlist);
				goto Done;
			}
			PyTuple_SET_ITEM(next, j, item);
		}

		if (i < len)
			PyList_SET_ITEM(ret, i, next);
		else {
			int status = PyList_Append(ret, next);
			Py_DECREF(next);
			++len;
			if (status < 0)
				goto Fail_ret_itlist;
		}
	}

Done:
	if (ret != NULL && i < len) {
		/* The list is too big. */
		if (PyList_SetSlice(ret, i, len, NULL) < 0)
			return NULL;
	}
	return ret;

Fail_ret_itlist:
	Py_DECREF(itlist);
Fail_ret:
	Py_DECREF(ret);
	return NULL;
}


PyDoc_STRVAR(zip_doc,
"zip(seq1 [, seq2 [...]]) -> [(seq1[0], seq2[0] ...), (...)]\n\
\n\
Return a list of tuples, where each tuple contains the i-th element\n\
from each of the argument sequences.  The returned list is truncated\n\
in length to the length of the shortest argument sequence.");


static PyMethodDef builtin_methods[] = {
 	{"__import__",	(PyCFunction)builtin___import__, METH_VARARGS | METH_KEYWORDS, import_doc},
 	{"abs",		builtin_abs,        METH_O, abs_doc},
 	{"all",		builtin_all,        METH_O, all_doc},
 	{"any",		builtin_any,        METH_O, any_doc},
 	{"apply",	builtin_apply,      METH_VARARGS, apply_doc},
	{"bin",		builtin_bin,	    METH_O, bin_doc},
 	{"callable",	builtin_callable,   METH_O, callable_doc},
 	{"chr",		builtin_chr,        METH_VARARGS, chr_doc},
 	{"cmp",		builtin_cmp,        METH_VARARGS, cmp_doc},
 	{"coerce",	builtin_coerce,     METH_VARARGS, coerce_doc},
 	{"compile",	(PyCFunction)builtin_compile,    METH_VARARGS | METH_KEYWORDS, compile_doc},
 	{"delattr",	builtin_delattr,    METH_VARARGS, delattr_doc},
 	{"dir",		builtin_dir,        METH_VARARGS, dir_doc},
 	{"divmod",	builtin_divmod,     METH_VARARGS, divmod_doc},
 	{"eval",	builtin_eval,       METH_VARARGS, eval_doc},
 	{"execfile",	builtin_execfile,   METH_VARARGS, execfile_doc},
 	{"filter",	builtin_filter,     METH_VARARGS, filter_doc},
 	{"format",	builtin_format,     METH_VARARGS, format_doc},
 	{"getattr",	builtin_getattr,    METH_VARARGS, getattr_doc},
 	{"globals",	(PyCFunction)builtin_globals,    METH_NOARGS, globals_doc},
 	{"hasattr",	builtin_hasattr,    METH_VARARGS, hasattr_doc},
 	{"hash",	builtin_hash,       METH_O, hash_doc},
 	{"hex",		builtin_hex,        METH_O, hex_doc},
 	{"id",		builtin_id,         METH_O, id_doc},
 	{"input",	builtin_input,      METH_VARARGS, input_doc},
 	{"intern",	builtin_intern,     METH_VARARGS, intern_doc},
 	{"isinstance",  builtin_isinstance, METH_VARARGS, isinstance_doc},
 	{"issubclass",  builtin_issubclass, METH_VARARGS, issubclass_doc},
 	{"iter",	builtin_iter,       METH_VARARGS, iter_doc},
 	{"len",		builtin_len,        METH_O, len_doc},
 	{"locals",	(PyCFunction)builtin_locals,     METH_NOARGS, locals_doc},
 	{"map",		builtin_map,        METH_VARARGS, map_doc},
 	{"max",		(PyCFunction)builtin_max,        METH_VARARGS | METH_KEYWORDS, max_doc},
 	{"min",		(PyCFunction)builtin_min,        METH_VARARGS | METH_KEYWORDS, min_doc},
 	{"oct",		builtin_oct,        METH_O, oct_doc},
 	{"open",	(PyCFunction)builtin_open,       METH_VARARGS | METH_KEYWORDS, open_doc},
 	{"ord",		builtin_ord,        METH_O, ord_doc},
 	{"pow",		builtin_pow,        METH_VARARGS, pow_doc},
 	{"print",	(PyCFunction)builtin_print,      METH_VARARGS | METH_KEYWORDS, print_doc},
 	{"range",	builtin_range,      METH_VARARGS, range_doc},
 	{"raw_input",	builtin_raw_input,  METH_VARARGS, raw_input_doc},
 	{"reduce",	builtin_reduce,     METH_VARARGS, reduce_doc},
 	{"reload",	builtin_reload,     METH_O, reload_doc},
 	{"repr",	builtin_repr,       METH_O, repr_doc},
 	{"round",	(PyCFunction)builtin_round,      METH_VARARGS | METH_KEYWORDS, round_doc},
 	{"setattr",	builtin_setattr,    METH_VARARGS, setattr_doc},
 	{"sorted",	(PyCFunction)builtin_sorted,     METH_VARARGS | METH_KEYWORDS, sorted_doc},
 	{"sum",		builtin_sum,        METH_VARARGS, sum_doc},
#ifdef Py_USING_UNICODE
 	{"unichr",	builtin_unichr,     METH_VARARGS, unichr_doc},
#endif
 	{"vars",	builtin_vars,       METH_VARARGS, vars_doc},
  	{"zip",         builtin_zip,        METH_VARARGS, zip_doc},
	{NULL,		NULL},
};

PyDoc_STRVAR(builtin_doc,
"Built-in functions, exceptions, and other objects.\n\
\n\
Noteworthy: None is the `nil' object; Ellipsis represents `...' in slices.");

PyObject *
_PyBuiltin_Init(void)
{
	PyObject *mod, *dict, *debug;
	mod = Py_InitModule4("__builtin__", builtin_methods,
			     builtin_doc, (PyObject *)NULL,
			     PYTHON_API_VERSION);
	if (mod == NULL)
		return NULL;
	dict = PyModule_GetDict(mod);

#ifdef Py_TRACE_REFS
	/* __builtin__ exposes a number of statically allocated objects
	 * that, before this code was added in 2.3, never showed up in
	 * the list of "all objects" maintained by Py_TRACE_REFS.  As a
	 * result, programs leaking references to None and False (etc)
	 * couldn't be diagnosed by examining sys.getobjects(0).
	 */
#define ADD_TO_ALL(OBJECT) _Py_AddToAllObjects((PyObject *)(OBJECT), 0)
#else
#define ADD_TO_ALL(OBJECT) (void)0
#endif

#define SETBUILTIN(NAME, OBJECT) \
	if (PyDict_SetItemString(dict, NAME, (PyObject *)OBJECT) < 0)	\
		return NULL;						\
	ADD_TO_ALL(OBJECT)

	SETBUILTIN("None",		Py_None);
	SETBUILTIN("Ellipsis",		Py_Ellipsis);
	SETBUILTIN("NotImplemented",	Py_NotImplemented);
	SETBUILTIN("False",		Py_False);
	SETBUILTIN("True",		Py_True);
	SETBUILTIN("basestring",	&PyBaseString_Type);
	SETBUILTIN("bool",		&PyBool_Type);
	/*	SETBUILTIN("memoryview",        &PyMemoryView_Type); */
	SETBUILTIN("bytearray",		&PyBytes_Type);
	SETBUILTIN("bytes",		&PyString_Type);
	SETBUILTIN("buffer",		&PyBuffer_Type);
	SETBUILTIN("classmethod",	&PyClassMethod_Type);
#ifndef WITHOUT_COMPLEX
	SETBUILTIN("complex",		&PyComplex_Type);
#endif
	SETBUILTIN("dict",		&PyDict_Type);
 	SETBUILTIN("enumerate",		&PyEnum_Type);
	SETBUILTIN("file",		&PyFile_Type);
	SETBUILTIN("float",		&PyFloat_Type);
	SETBUILTIN("frozenset",		&PyFrozenSet_Type);
	SETBUILTIN("property",		&PyProperty_Type);
	SETBUILTIN("int",		&PyInt_Type);
	SETBUILTIN("list",		&PyList_Type);
	SETBUILTIN("long",		&PyLong_Type);
	SETBUILTIN("object",		&PyBaseObject_Type);
	SETBUILTIN("reversed",		&PyReversed_Type);
	SETBUILTIN("set",		&PySet_Type);
	SETBUILTIN("slice",		&PySlice_Type);
	SETBUILTIN("staticmethod",	&PyStaticMethod_Type);
	SETBUILTIN("str",		&PyString_Type);
	SETBUILTIN("super",		&PySuper_Type);
	SETBUILTIN("tuple",		&PyTuple_Type);
	SETBUILTIN("type",		&PyType_Type);
	SETBUILTIN("xrange",		&PyRange_Type);
#ifdef Py_USING_UNICODE
	SETBUILTIN("unicode",		&PyUnicode_Type);
#endif
	debug = PyBool_FromLong(Py_OptimizeFlag == 0);
	if (PyDict_SetItemString(dict, "__debug__", debug) < 0) {
		Py_XDECREF(debug);
		return NULL;
	}
	Py_XDECREF(debug);

	return mod;
#undef ADD_TO_ALL
#undef SETBUILTIN
}

/* Helper for filter(): filter a tuple through a function */

static PyObject *
filtertuple(PyObject *func, PyObject *tuple)
{
	PyObject *result;
	Py_ssize_t i, j;
	Py_ssize_t len = PyTuple_Size(tuple);

	if (len == 0) {
		if (PyTuple_CheckExact(tuple))
			Py_INCREF(tuple);
		else
			tuple = PyTuple_New(0);
		return tuple;
	}

	if ((result = PyTuple_New(len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *good;
		int ok;

		if (tuple->ob_type->tp_as_sequence &&
		    tuple->ob_type->tp_as_sequence->sq_item) {
			item = tuple->ob_type->tp_as_sequence->sq_item(tuple, i);
			if (item == NULL)
				goto Fail_1;
		} else {
			PyErr_SetString(PyExc_TypeError, "filter(): unsubscriptable tuple");
			goto Fail_1;
		}
		if (func == Py_None) {
			Py_INCREF(item);
			good = item;
		}
		else {
			PyObject *arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
		}
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
		if (ok) {
			if (PyTuple_SetItem(result, j++, item) < 0)
				goto Fail_1;
		}
		else
			Py_DECREF(item);
	}

	if (_PyTuple_Resize(&result, j) < 0)
		return NULL;

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}


/* Helper for filter(): filter a string through a function */

static PyObject *
filterstring(PyObject *func, PyObject *strobj)
{
	PyObject *result;
	Py_ssize_t i, j;
	Py_ssize_t len = PyString_Size(strobj);
	Py_ssize_t outlen = len;

	if (func == Py_None) {
		/* If it's a real string we can return the original,
		 * as no character is ever false and __getitem__
		 * does return this character. If it's a subclass
		 * we must go through the __getitem__ loop */
		if (PyString_CheckExact(strobj)) {
			Py_INCREF(strobj);
			return strobj;
		}
	}
	if ((result = PyString_FromStringAndSize(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		if (func==Py_None) {
			ok = 1;
		} else {
			PyObject *arg, *good;
			arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			Py_ssize_t reslen;
			if (!PyString_Check(item)) {
				PyErr_SetString(PyExc_TypeError, "can't filter str to str:"
					" __getitem__ returned different type");
				Py_DECREF(item);
				goto Fail_1;
			}
			reslen = PyString_GET_SIZE(item);
			if (reslen == 1) {
				PyString_AS_STRING(result)[j++] =
					PyString_AS_STRING(item)[0];
			} else {
				/* do we need more space? */
				Py_ssize_t need = j + reslen + len-i-1;
				if (need > outlen) {
					/* overallocate, to avoid reallocations */
					if (need<2*outlen)
						need = 2*outlen;
					if (_PyString_Resize(&result, need)) {
						Py_DECREF(item);
						return NULL;
					}
					outlen = need;
				}
				memcpy(
					PyString_AS_STRING(result) + j,
					PyString_AS_STRING(item),
					reslen
				);
				j += reslen;
			}
		}
		Py_DECREF(item);
	}

	if (j < outlen)
		_PyString_Resize(&result, j);

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}

#ifdef Py_USING_UNICODE
/* Helper for filter(): filter a Unicode object through a function */

static PyObject *
filterunicode(PyObject *func, PyObject *strobj)
{
	PyObject *result;
	register Py_ssize_t i, j;
	Py_ssize_t len = PyUnicode_GetSize(strobj);
	Py_ssize_t outlen = len;

	if (func == Py_None) {
		/* If it's a real string we can return the original,
		 * as no character is ever false and __getitem__
		 * does return this character. If it's a subclass
		 * we must go through the __getitem__ loop */
		if (PyUnicode_CheckExact(strobj)) {
			Py_INCREF(strobj);
			return strobj;
		}
	}
	if ((result = PyUnicode_FromUnicode(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *arg, *good;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		if (func == Py_None) {
			ok = 1;
		} else {
			arg = PyTuple_Pack(1, item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_1;
			}
			ok = PyObject_IsTrue(good);
			Py_DECREF(good);
		}
		if (ok) {
			Py_ssize_t reslen;
			if (!PyUnicode_Check(item)) {
				PyErr_SetString(PyExc_TypeError,
				"can't filter unicode to unicode:"
				" __getitem__ returned different type");
				Py_DECREF(item);
				goto Fail_1;
			}
			reslen = PyUnicode_GET_SIZE(item);
			if (reslen == 1)
				PyUnicode_AS_UNICODE(result)[j++] =
					PyUnicode_AS_UNICODE(item)[0];
			else {
				/* do we need more space? */
				Py_ssize_t need = j + reslen + len - i - 1;
				if (need > outlen) {
					/* overallocate,
					   to avoid reallocations */
					if (need < 2 * outlen)
						need = 2 * outlen;
					if (PyUnicode_Resize(
						&result, need) < 0) {
						Py_DECREF(item);
						goto Fail_1;
					}
					outlen = need;
				}
				memcpy(PyUnicode_AS_UNICODE(result) + j,
				       PyUnicode_AS_UNICODE(item),
				       reslen*sizeof(Py_UNICODE));
				j += reslen;
			}
		}
		Py_DECREF(item);
	}

	if (j < outlen)
		PyUnicode_Resize(&result, j);

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}
#endif
