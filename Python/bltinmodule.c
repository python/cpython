
/* Built-in functions */

#include "Python.h"

#include "node.h"
#include "compile.h"
#include "eval.h"

#include <ctype.h>

#ifdef RISCOS
#include "unixstuff.h"
#endif

/* The default encoding used by the platform file system APIs
   Can remain NULL for all platforms that don't have such a concept
*/
#if defined(MS_WIN32) && defined(HAVE_USABLE_WCHAR_T)
const char *Py_FileSystemDefaultEncoding = "mbcs";
#else
const char *Py_FileSystemDefaultEncoding = NULL; /* use default */
#endif

/* Forward */
static PyObject *filterstring(PyObject *, PyObject *);
static PyObject *filtertuple (PyObject *, PyObject *);

static PyObject *
builtin___import__(PyObject *self, PyObject *args)
{
	char *name;
	PyObject *globals = NULL;
	PyObject *locals = NULL;
	PyObject *fromlist = NULL;

	if (!PyArg_ParseTuple(args, "s|OOO:__import__",
			&name, &globals, &locals, &fromlist))
		return NULL;
	return PyImport_ImportModuleEx(name, globals, locals, fromlist);
}

static char import_doc[] =
"__import__(name, globals, locals, fromlist) -> module\n\
\n\
Import a module.  The globals are only used to determine the context;\n\
they are not modified.  The locals are currently unused.  The fromlist\n\
should be a list of names to emulate ``from name import ...'', or an\n\
empty list to emulate ``import name''.\n\
When importing a module from a package, note that __import__('A.B', ...)\n\
returns package A when fromlist is empty, but its submodule B when\n\
fromlist is not empty.";


static PyObject *
builtin_abs(PyObject *self, PyObject *v)
{
	return PyNumber_Absolute(v);
}

static char abs_doc[] =
"abs(number) -> number\n\
\n\
Return the absolute value of the argument.";


static PyObject *
builtin_apply(PyObject *self, PyObject *args)
{
	PyObject *func, *alist = NULL, *kwdict = NULL;
	PyObject *t = NULL, *retval = NULL;

	if (!PyArg_ParseTuple(args, "O|OO:apply", &func, &alist, &kwdict))
		return NULL;
	if (alist != NULL) {
		if (!PyTuple_Check(alist)) {
			if (!PySequence_Check(alist)) {
				PyErr_Format(PyExc_TypeError,
				     "apply() arg 2 expect sequence, found %s",
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

static char apply_doc[] =
"apply(object[, args[, kwargs]]) -> value\n\
\n\
Call a callable object with positional arguments taken from the tuple args,\n\
and keyword arguments taken from the optional dictionary kwargs.\n\
Note that classes are callable, as are instances with a __call__() method.";


static PyObject *
builtin_buffer(PyObject *self, PyObject *args)
{
	PyObject *ob;
	int offset = 0;
	int size = Py_END_OF_BUFFER;

	if ( !PyArg_ParseTuple(args, "O|ii:buffer", &ob, &offset, &size) )
	    return NULL;
	return PyBuffer_FromObject(ob, offset, size);
}

static char buffer_doc[] =
"buffer(object [, offset[, size]]) -> object\n\
\n\
Create a new buffer object which references the given object.\n\
The buffer will reference a slice of the target object from the\n\
start of the object (or at the specified offset). The slice will\n\
extend to the end of the target object (or with the specified size).";


static PyObject *
builtin_callable(PyObject *self, PyObject *v)
{
	return PyInt_FromLong((long)PyCallable_Check(v));
}

static char callable_doc[] =
"callable(object) -> Boolean\n\
\n\
Return whether the object is callable (i.e., some kind of function).\n\
Note that classes are callable, as are instances with a __call__() method.";


static PyObject *
builtin_filter(PyObject *self, PyObject *args)
{
	PyObject *func, *seq, *result, *it;
	int len;   /* guess for result list size */
	register int j;

	if (!PyArg_ParseTuple(args, "OO:filter", &func, &seq))
		return NULL;

	/* Strings and tuples return a result of the same type. */
	if (PyString_Check(seq))
		return filterstring(func, seq);
	if (PyTuple_Check(seq))
		return filtertuple(func, seq);

	/* Get iterator. */
	it = PyObject_GetIter(seq);
	if (it == NULL)
		return NULL;

	/* Guess a result list size. */
	len = -1;   /* unknown */
	if (PySequence_Check(seq) &&
	    seq->ob_type->tp_as_sequence->sq_length) {
		len = PySequence_Size(seq);
		if (len < 0)
			PyErr_Clear();
	}
	if (len < 0)
		len = 8;  /* arbitrary */

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
		PyObject *item, *good;
		int ok;

		item = PyIter_Next(it);
		if (item == NULL) {
			if (PyErr_Occurred())
				goto Fail_result_it;
			break;
		}

		if (func == Py_None) {
			good = item;
			Py_INCREF(good);
		}
		else {
			PyObject *arg = Py_BuildValue("(O)", item);
			if (arg == NULL) {
				Py_DECREF(item);
				goto Fail_result_it;
			}
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL) {
				Py_DECREF(item);
				goto Fail_result_it;
			}
		}
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
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
	return result;

Fail_result_it:
	Py_DECREF(result);
Fail_it:
	Py_DECREF(it);
	return NULL;
}

static char filter_doc[] =
"filter(function, sequence) -> list\n\
\n\
Return a list containing those items of sequence for which function(item)\n\
is true.  If function is None, return a list of items that are true.";


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

static char chr_doc[] =
"chr(i) -> character\n\
\n\
Return a string of one character with ordinal i; 0 <= i < 256.";


#ifdef Py_USING_UNICODE
static PyObject *
builtin_unichr(PyObject *self, PyObject *args)
{
	long x;
	Py_UNICODE s[2];

	if (!PyArg_ParseTuple(args, "l:unichr", &x))
		return NULL;

#ifdef Py_UNICODE_WIDE
	if (x < 0 || x > 0x10ffff) {
		PyErr_SetString(PyExc_ValueError,
				"unichr() arg not in range(0x110000) "
				"(wide Python build)");
		return NULL;
	}
#else
	if (x < 0 || x > 0xffff) {
		PyErr_SetString(PyExc_ValueError,
				"unichr() arg not in range(0x10000) "
				"(narrow Python build)");
		return NULL;
	}
#endif

	if (x <= 0xffff) {
		/* UCS-2 character */
		s[0] = (Py_UNICODE) x;
		return PyUnicode_FromUnicode(s, 1);
	}
	else {
#ifndef Py_UNICODE_WIDE
		/* UCS-4 character.  store as two surrogate characters */
		x -= 0x10000L;
		s[0] = 0xD800 + (Py_UNICODE) (x >> 10);
		s[1] = 0xDC00 + (Py_UNICODE) (x & 0x03FF);
		return PyUnicode_FromUnicode(s, 2);
#else
		s[0] = (Py_UNICODE)x;
		return PyUnicode_FromUnicode(s, 1);
#endif
	}
}

static char unichr_doc[] =
"unichr(i) -> Unicode character\n\
\n\
Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.";
#endif


static PyObject *
builtin_cmp(PyObject *self, PyObject *args)
{
	PyObject *a, *b;
	int c;

	if (!PyArg_ParseTuple(args, "OO:cmp", &a, &b))
		return NULL;
	if (PyObject_Cmp(a, b, &c) < 0)
		return NULL;
	return PyInt_FromLong((long)c);
}

static char cmp_doc[] =
"cmp(x, y) -> integer\n\
\n\
Return negative if x<y, zero if x==y, positive if x>y.";


static PyObject *
builtin_coerce(PyObject *self, PyObject *args)
{
	PyObject *v, *w;
	PyObject *res;

	if (!PyArg_ParseTuple(args, "OO:coerce", &v, &w))
		return NULL;
	if (PyNumber_Coerce(&v, &w) < 0)
		return NULL;
	res = Py_BuildValue("(OO)", v, w);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

static char coerce_doc[] =
"coerce(x, y) -> None or (x1, y1)\n\
\n\
When x and y can be coerced to values of the same type, return a tuple\n\
containing the coerced values.  When they can't be coerced, return None.";


static PyObject *
builtin_compile(PyObject *self, PyObject *args)
{
	char *str;
	char *filename;
	char *startstr;
	int start;
	int dont_inherit = 0;
	int supplied_flags = 0;
	PyCompilerFlags cf;

	if (!PyArg_ParseTuple(args, "sss|ii:compile", &str, &filename, 
			      &startstr, &supplied_flags, &dont_inherit))
		return NULL;

	if (strcmp(startstr, "exec") == 0)
		start = Py_file_input;
	else if (strcmp(startstr, "eval") == 0)
		start = Py_eval_input;
	else if (strcmp(startstr, "single") == 0)
		start = Py_single_input;
	else {
		PyErr_SetString(PyExc_ValueError,
		   "compile() arg 3 must be 'exec' or 'eval' or 'single'");
		return NULL;
	}

	if (supplied_flags & ~(PyCF_MASK | PyCF_MASK_OBSOLETE)) {
		PyErr_SetString(PyExc_ValueError,
				"compile(): unrecognised flags");
		return NULL;
	}
	/* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

	cf.cf_flags = supplied_flags;
	if (!dont_inherit) {
		PyEval_MergeCompilerFlags(&cf);
	}
	return Py_CompileStringFlags(str, filename, start, &cf);
}

static char compile_doc[] =
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
in addition to any features explicitly specified.";

static PyObject *
builtin_dir(PyObject *self, PyObject *args)
{
	PyObject *arg = NULL;

	if (!PyArg_ParseTuple(args, "|O:dir", &arg))
		return NULL;
	return PyObject_Dir(arg);
}

static char dir_doc[] =
"dir([object]) -> list of strings\n"
"\n"
"Return an alphabetized list of names comprising (some of) the attributes\n"
"of the given object, and of attributes reachable from it:\n"
"\n"
"No argument:  the names in the current scope.\n"
"Module object:  the module attributes.\n"
"Type or class object:  its attributes, and recursively the attributes of\n"
"    its bases.\n"
"Otherwise:  its attributes, its class's attributes, and recursively the\n"
"    attributes of its class's base classes.";

static PyObject *
builtin_divmod(PyObject *self, PyObject *args)
{
	PyObject *v, *w;

	if (!PyArg_ParseTuple(args, "OO:divmod", &v, &w))
		return NULL;
	return PyNumber_Divmod(v, w);
}

static char divmod_doc[] =
"divmod(x, y) -> (div, mod)\n\
\n\
Return the tuple ((x-x%y)/y, x%y).  Invariant: div*y + mod == x.";


static PyObject *
builtin_eval(PyObject *self, PyObject *args)
{
	PyObject *cmd;
	PyObject *globals = Py_None, *locals = Py_None;
	char *str;
	PyCompilerFlags cf;

	if (!PyArg_ParseTuple(args, "O|O!O!:eval",
			&cmd,
			&PyDict_Type, &globals,
			&PyDict_Type, &locals))
		return NULL;
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
	if (PyString_AsStringAndSize(cmd, &str, NULL))
		return NULL;
	while (*str == ' ' || *str == '\t')
		str++;

	cf.cf_flags = 0;
	(void)PyEval_MergeCompilerFlags(&cf);
	return PyRun_StringFlags(str, Py_eval_input, globals, locals, &cf);
}

static char eval_doc[] =
"eval(source[, globals[, locals]]) -> value\n\
\n\
Evaluate the source in the context of globals and locals.\n\
The source may be a string representing a Python expression\n\
or a code object as returned by compile().\n\
The globals and locals are dictionaries, defaulting to the current\n\
globals and locals.  If only globals is given, locals defaults to it.";


static PyObject *
builtin_execfile(PyObject *self, PyObject *args)
{
	char *filename;
	PyObject *globals = Py_None, *locals = Py_None;
	PyObject *res;
	FILE* fp = NULL;
	PyCompilerFlags cf;
	int exists;
#ifndef RISCOS
	struct stat s;
#endif

	if (!PyArg_ParseTuple(args, "s|O!O!:execfile",
			&filename,
			&PyDict_Type, &globals,
			&PyDict_Type, &locals))
		return NULL;
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
#ifndef RISCOS
	if (!stat(filename, &s)) {
		if (S_ISDIR(s.st_mode))
#if defined(PYOS_OS2) && defined(PYCC_VACPP)
			errno = EOS2ERR;
#else
			errno = EISDIR;
#endif
		else
			exists = 1;
	}
#else
	if (object_exists(filename)) {
		if (isdir(filename))
			errno = EISDIR;
		else
			exists = 1;
	}
#endif /* RISCOS */

        if (exists) {
		Py_BEGIN_ALLOW_THREADS
		fp = fopen(filename, "r");
		Py_END_ALLOW_THREADS

		if (fp == NULL) {
			exists = 0;
		}
        }

	if (!exists) {
		PyErr_SetFromErrno(PyExc_IOError);
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

static char execfile_doc[] =
"execfile(filename[, globals[, locals]])\n\
\n\
Read and execute a Python script from a file.\n\
The globals and locals are dictionaries, defaulting to the current\n\
globals and locals.  If only globals is given, locals defaults to it.";


static PyObject *
builtin_getattr(PyObject *self, PyObject *args)
{
	PyObject *v, *result, *dflt = NULL;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OO|O:getattr", &v, &name, &dflt))
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
				"attribute name must be string");
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

static char getattr_doc[] =
"getattr(object, name[, default]) -> value\n\
\n\
Get a named attribute from an object; getattr(x, 'y') is equivalent to x.y.\n\
When a default argument is given, it is returned when the attribute doesn't\n\
exist; without it, an exception is raised in that case.";


static PyObject *
builtin_globals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetGlobals();
	Py_INCREF(d);
	return d;
}

static char globals_doc[] =
"globals() -> dictionary\n\
\n\
Return the dictionary containing the current scope's global variables.";


static PyObject *
builtin_hasattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OO:hasattr", &v, &name))
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
				"attribute name must be string");
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

static char hasattr_doc[] =
"hasattr(object, name) -> Boolean\n\
\n\
Return whether the object has an attribute with the given name.\n\
(This is done by calling getattr(object, name) and catching exceptions.)";


static PyObject *
builtin_id(PyObject *self, PyObject *v)
{
	return PyLong_FromVoidPtr(v);
}

static char id_doc[] =
"id(object) -> integer\n\
\n\
Return the identity of an object.  This is guaranteed to be unique among\n\
simultaneously existing objects.  (Hint: it's the object's memory address.)";


static PyObject *
builtin_map(PyObject *self, PyObject *args)
{
	typedef struct {
		PyObject *it;	/* the iterator object */
		int saw_StopIteration;  /* bool:  did the iterator end? */
	} sequence;

	PyObject *func, *result;
	sequence *seqs = NULL, *sqp;
	int n, len;
	register int i, j;

	n = PyTuple_Size(args);
	if (n < 2) {
		PyErr_SetString(PyExc_TypeError,
				"map() requires at least two args");
		return NULL;
	}

	func = PyTuple_GetItem(args, 0);
	n--;

	if (func == Py_None && n == 1) {
		/* map(None, S) is the same as list(S). */
		return PySequence_List(PyTuple_GetItem(args, 1));
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
		int curlen;

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
		curlen = -1;  /* unknown */
		if (PySequence_Check(curseq) &&
		    curseq->ob_type->tp_as_sequence->sq_length) {
			curlen = PySequence_Size(curseq);
			if (curlen < 0)
				PyErr_Clear();
		}
		if (curlen < 0)
			curlen = 8;  /* arbitrary */
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

static char map_doc[] =
"map(function, sequence[, sequence, ...]) -> list\n\
\n\
Return a list of the results of applying the function to the items of\n\
the argument sequence(s).  If more than one sequence is given, the\n\
function is called with an argument list consisting of the corresponding\n\
item of each sequence, substituting None for missing values when not all\n\
sequences have the same length.  If the function is None, return a list of\n\
the items of the sequence (or a list of tuples if more than one sequence).";


static PyObject *
builtin_setattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;
	PyObject *value;

	if (!PyArg_ParseTuple(args, "OOO:setattr", &v, &name, &value))
		return NULL;
	if (PyObject_SetAttr(v, name, value) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char setattr_doc[] =
"setattr(object, name, value)\n\
\n\
Set a named attribute on an object; setattr(x, 'y', v) is equivalent to\n\
``x.y = v''.";


static PyObject *
builtin_delattr(PyObject *self, PyObject *args)
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OO:delattr", &v, &name))
		return NULL;
	if (PyObject_SetAttr(v, name, (PyObject *)NULL) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char delattr_doc[] =
"delattr(object, name)\n\
\n\
Delete a named attribute on an object; delattr(x, 'y') is equivalent to\n\
``del x.y''.";


static PyObject *
builtin_hash(PyObject *self, PyObject *v)
{
	long x;

	x = PyObject_Hash(v);
	if (x == -1)
		return NULL;
	return PyInt_FromLong(x);
}

static char hash_doc[] =
"hash(object) -> integer\n\
\n\
Return a hash value for the object.  Two objects with the same value have\n\
the same hash value.  The reverse is not necessarily true, but likely.";


static PyObject *
builtin_hex(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;

	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_hex == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "hex() argument can't be converted to hex");
		return NULL;
	}
	return (*nb->nb_hex)(v);
}

static char hex_doc[] =
"hex(number) -> string\n\
\n\
Return the hexadecimal representation of an integer or long integer.";


static PyObject *builtin_raw_input(PyObject *, PyObject *);

static PyObject *
builtin_input(PyObject *self, PyObject *args)
{
	PyObject *line;
	char *str;
	PyObject *res;
	PyObject *globals, *locals;

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
	res = PyRun_String(str, Py_eval_input, globals, locals);
	Py_DECREF(line);
	return res;
}

static char input_doc[] =
"input([prompt]) -> value\n\
\n\
Equivalent to eval(raw_input(prompt)).";


static PyObject *
builtin_intern(PyObject *self, PyObject *args)
{
	PyObject *s;
	if (!PyArg_ParseTuple(args, "S:intern", &s))
		return NULL;
	Py_INCREF(s);
	PyString_InternInPlace(&s);
	return s;
}

static char intern_doc[] =
"intern(string) -> string\n\
\n\
``Intern'' the given string.  This enters the string in the (global)\n\
table of interned strings whose purpose is to speed up dictionary lookups.\n\
Return the string itself or the previously interned string object with the\n\
same value.";


static PyObject *
builtin_iter(PyObject *self, PyObject *args)
{
	PyObject *v, *w = NULL;

	if (!PyArg_ParseTuple(args, "O|O:iter", &v, &w))
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

static char iter_doc[] =
"iter(collection) -> iterator\n\
iter(callable, sentinel) -> iterator\n\
\n\
Get an iterator from an object.  In the first form, the argument must\n\
supply its own iterator, or be a sequence.\n\
In the second form, the callable is called until it returns the sentinel.";


static PyObject *
builtin_len(PyObject *self, PyObject *v)
{
	long res;

	res = PyObject_Size(v);
	if (res < 0 && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(res);
}

static char len_doc[] =
"len(object) -> integer\n\
\n\
Return the number of items of a sequence or mapping.";


static PyObject *
builtin_slice(PyObject *self, PyObject *args)
{
	PyObject *start, *stop, *step;

	start = stop = step = NULL;

	if (!PyArg_ParseTuple(args, "O|OO:slice", &start, &stop, &step))
		return NULL;

	/* This swapping of stop and start is to maintain similarity with
	   range(). */
	if (stop == NULL) {
		stop = start;
		start = NULL;
	}
	return PySlice_New(start, stop, step);
}

static char slice_doc[] =
"slice([start,] stop[, step]) -> slice object\n\
\n\
Create a slice object.  This is used for slicing by the Numeric extensions.";


static PyObject *
builtin_locals(PyObject *self)
{
	PyObject *d;

	d = PyEval_GetLocals();
	Py_INCREF(d);
	return d;
}

static char locals_doc[] =
"locals() -> dictionary\n\
\n\
Return the dictionary containing the current scope's local variables.";


static PyObject *
min_max(PyObject *args, int op)
{
	PyObject *v, *w, *x, *it;

	if (PyTuple_Size(args) > 1)
		v = args;
	else if (!PyArg_ParseTuple(args, "O:min/max", &v))
		return NULL;
	
	it = PyObject_GetIter(v);
	if (it == NULL)
		return NULL;

	w = NULL;  /* the result */
	for (;;) {
		x = PyIter_Next(it);
		if (x == NULL) {
			if (PyErr_Occurred()) {
				Py_XDECREF(w);
				Py_DECREF(it);
				return NULL;
			}
			break;
		}

		if (w == NULL)
			w = x;
		else {
			int cmp = PyObject_RichCompareBool(x, w, op);
			if (cmp > 0) {
				Py_DECREF(w);
				w = x;
			}
			else if (cmp < 0) {
				Py_DECREF(x);
				Py_DECREF(w);
				Py_DECREF(it);
				return NULL;
			}
			else
				Py_DECREF(x);
		}
	}
	if (w == NULL)
		PyErr_SetString(PyExc_ValueError,
				"min() or max() arg is an empty sequence");
	Py_DECREF(it);
	return w;
}

static PyObject *
builtin_min(PyObject *self, PyObject *v)
{
	return min_max(v, Py_LT);
}

static char min_doc[] =
"min(sequence) -> value\n\
min(a, b, c, ...) -> value\n\
\n\
With a single sequence argument, return its smallest item.\n\
With two or more arguments, return the smallest argument.";


static PyObject *
builtin_max(PyObject *self, PyObject *v)
{
	return min_max(v, Py_GT);
}

static char max_doc[] =
"max(sequence) -> value\n\
max(a, b, c, ...) -> value\n\
\n\
With a single sequence argument, return its largest item.\n\
With two or more arguments, return the largest argument.";


static PyObject *
builtin_oct(PyObject *self, PyObject *v)
{
	PyNumberMethods *nb;

	if (v == NULL || (nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_oct == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "oct() argument can't be converted to oct");
		return NULL;
	}
	return (*nb->nb_oct)(v);
}

static char oct_doc[] =
"oct(number) -> string\n\
\n\
Return the octal representation of an integer or long integer.";


static PyObject *
builtin_ord(PyObject *self, PyObject* obj)
{
	long ord;
	int size;

	if (PyString_Check(obj)) {
		size = PyString_GET_SIZE(obj);
		if (size == 1) {
			ord = (long)((unsigned char)*PyString_AS_STRING(obj));
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
		     "but string of length %d found",
		     size);
	return NULL;
}

static char ord_doc[] =
"ord(c) -> integer\n\
\n\
Return the integer ordinal of a one-character string.";


static PyObject *
builtin_pow(PyObject *self, PyObject *args)
{
	PyObject *v, *w, *z = Py_None;

	if (!PyArg_ParseTuple(args, "OO|O:pow", &v, &w, &z))
		return NULL;
	return PyNumber_Power(v, w, z);
}

static char pow_doc[] =
"pow(x, y[, z]) -> number\n\
\n\
With two arguments, equivalent to x**y.  With three arguments,\n\
equivalent to (x**y) % z, but may be more efficient (e.g. for longs).";


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
				&ihigh))
			return NULL;
	}
	else {
		if (!PyArg_ParseTuple(args,
				"ll|l;range() requires 1-3 int arguments",
				&ilow, &ihigh, &istep))
			return NULL;
	}
	if (istep == 0) {
		PyErr_SetString(PyExc_ValueError, "range() arg 3 must not be zero");
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

static char range_doc[] =
"range([start,] stop[, step]) -> list of integers\n\
\n\
Return a list containing an arithmetic progression of integers.\n\
range(i, j) returns [i, i+1, i+2, ..., j-1]; start (!) defaults to 0.\n\
When step is given, it specifies the increment (or decrement).\n\
For example, range(4) returns [0, 1, 2, 3].  The end point is omitted!\n\
These are exactly the valid indices for a list of 4 elements.";


static PyObject *
builtin_xrange(PyObject *self, PyObject *args)
{
	long ilow = 0, ihigh = 0, istep = 1;
	long n;

	if (PyTuple_Size(args) <= 1) {
		if (!PyArg_ParseTuple(args,
				"l;xrange() requires 1-3 int arguments",
				&ihigh))
			return NULL;
	}
	else {
		if (!PyArg_ParseTuple(args,
				"ll|l;xrange() requires 1-3 int arguments",
				&ilow, &ihigh, &istep))
			return NULL;
	}
	if (istep == 0) {
		PyErr_SetString(PyExc_ValueError, "xrange() arg 3 must not be zero");
		return NULL;
	}
	if (istep > 0)
		n = get_len_of_range(ilow, ihigh, istep);
	else
		n = get_len_of_range(ihigh, ilow, -istep);
	if (n < 0) {
		PyErr_SetString(PyExc_OverflowError,
				"xrange() result has too many items");
		return NULL;
	}
	return PyRange_New(ilow, n, istep, 1);
}

static char xrange_doc[] =
"xrange([start,] stop[, step]) -> xrange object\n\
\n\
Like range(), but instead of returning a list, returns an object that\n\
generates the numbers in the range on demand.  This is slightly slower\n\
than range() but more memory efficient.";


static PyObject *
builtin_raw_input(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *f;

	if (!PyArg_ParseTuple(args, "|O:[raw_]input", &v))
		return NULL;
	if (PyFile_AsFile(PySys_GetObject("stdin")) == stdin &&
	    PyFile_AsFile(PySys_GetObject("stdout")) == stdout &&
	    isatty(fileno(stdin)) && isatty(fileno(stdout))) {
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
		s = PyOS_Readline(prompt);
		Py_XDECREF(po);
		if (s == NULL) {
			PyErr_SetNone(PyExc_KeyboardInterrupt);
			return NULL;
		}
		if (*s == '\0') {
			PyErr_SetNone(PyExc_EOFError);
			result = NULL;
		}
		else { /* strip trailing '\n' */
			size_t len = strlen(s);
			if (len > INT_MAX) {
				PyErr_SetString(PyExc_OverflowError, "input too long");
				result = NULL;
			}
			else {
				result = PyString_FromStringAndSize(s, (int)(len-1));
			}
		}
		PyMem_FREE(s);
		return result;
	}
	if (v != NULL) {
		f = PySys_GetObject("stdout");
		if (f == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
			return NULL;
		}
		if (Py_FlushLine() != 0 ||
		    PyFile_WriteObject(v, f, Py_PRINT_RAW) != 0)
			return NULL;
	}
	f = PySys_GetObject("stdin");
	if (f == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "lost sys.stdin");
		return NULL;
	}
	return PyFile_GetLine(f, -1);
}

static char raw_input_doc[] =
"raw_input([prompt]) -> string\n\
\n\
Read a string from standard input.  The trailing newline is stripped.\n\
If the user hits EOF (Unix: Ctl-D, Windows: Ctl-Z+Return), raise EOFError.\n\
On Unix, GNU readline is used if enabled.  The prompt string, if given,\n\
is printed without a trailing newline before reading.";


static PyObject *
builtin_reduce(PyObject *self, PyObject *args)
{
	PyObject *seq, *func, *result = NULL, *it;

	if (!PyArg_ParseTuple(args, "OO|O:reduce", &func, &seq, &result))
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

static char reduce_doc[] =
"reduce(function, sequence[, initial]) -> value\n\
\n\
Apply a function of two arguments cumulatively to the items of a sequence,\n\
from left to right, so as to reduce the sequence to a single value.\n\
For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates\n\
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items\n\
of the sequence in the calculation, and serves as a default when the\n\
sequence is empty.";


static PyObject *
builtin_reload(PyObject *self, PyObject *v)
{
	return PyImport_ReloadModule(v);
}

static char reload_doc[] =
"reload(module) -> module\n\
\n\
Reload the module.  The module must have been successfully imported before.";


static PyObject *
builtin_repr(PyObject *self, PyObject *v)
{
	return PyObject_Repr(v);
}

static char repr_doc[] =
"repr(object) -> string\n\
\n\
Return the canonical string representation of the object.\n\
For most object types, eval(repr(object)) == object.";


static PyObject *
builtin_round(PyObject *self, PyObject *args)
{
	double x;
	double f;
	int ndigits = 0;
	int i;

	if (!PyArg_ParseTuple(args, "d|i:round", &x, &ndigits))
			return NULL;
	f = 1.0;
	i = abs(ndigits);
	while  (--i >= 0)
		f = f*10.0;
	if (ndigits < 0)
		x /= f;
	else
		x *= f;
	if (x >= 0.0)
		x = floor(x + 0.5);
	else
		x = ceil(x - 0.5);
	if (ndigits < 0)
		x *= f;
	else
		x /= f;
	return PyFloat_FromDouble(x);
}

static char round_doc[] =
"round(number[, ndigits]) -> floating point number\n\
\n\
Round a number to a given precision in decimal digits (default 0 digits).\n\
This always returns a floating point number.  Precision may be negative.";


static PyObject *
builtin_vars(PyObject *self, PyObject *args)
{
	PyObject *v = NULL;
	PyObject *d;

	if (!PyArg_ParseTuple(args, "|O:vars", &v))
		return NULL;
	if (v == NULL) {
		d = PyEval_GetLocals();
		if (d == NULL) {
			if (!PyErr_Occurred())
				PyErr_SetString(PyExc_SystemError,
						"no locals!?");
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

static char vars_doc[] =
"vars([object]) -> dictionary\n\
\n\
Without arguments, equivalent to locals().\n\
With an argument, equivalent to object.__dict__.";

static PyObject *
builtin_isinstance(PyObject *self, PyObject *args)
{
	PyObject *inst;
	PyObject *cls;
	int retval;

	if (!PyArg_ParseTuple(args, "OO:isinstance", &inst, &cls))
		return NULL;

	retval = PyObject_IsInstance(inst, cls);
	if (retval < 0)
		return NULL;
	return PyInt_FromLong(retval);
}

static char isinstance_doc[] =
"isinstance(object, class-or-type-or-tuple) -> Boolean\n\
\n\
Return whether an object is an instance of a class or of a subclass thereof.\n\
With a type as second argument, return whether that is the object's type.\n\
The form using a tuple, isinstance(x, (A, B, ...)), is a shortcut for\n\
isinstance(x, A) or isinstance(x, B) or ... (etc.).";


static PyObject *
builtin_issubclass(PyObject *self, PyObject *args)
{
	PyObject *derived;
	PyObject *cls;
	int retval;

	if (!PyArg_ParseTuple(args, "OO:issubclass", &derived, &cls))
		return NULL;

	retval = PyObject_IsSubclass(derived, cls);
	if (retval < 0)
		return NULL;
	return PyInt_FromLong(retval);
}

static char issubclass_doc[] =
"issubclass(C, B) -> Boolean\n\
\n\
Return whether class C is a subclass (i.e., a derived class) of class B.";


static PyObject*
builtin_zip(PyObject *self, PyObject *args)
{
	PyObject *ret;
	int itemsize = PySequence_Length(args);
	int i;
	PyObject *itlist;  /* tuple of iterators */

	if (itemsize < 1) {
		PyErr_SetString(PyExc_TypeError,
				"zip() requires at least one sequence");
		return NULL;
	}
	/* args must be a tuple */
	assert(PyTuple_Check(args));

	/* allocate result list */
	if ((ret = PyList_New(0)) == NULL)
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
				    "zip argument #%d must support iteration",
				    i+1);
			goto Fail_ret_itlist;
		}
		PyTuple_SET_ITEM(itlist, i, it);
	}

	/* build result into ret list */
	for (;;) {
		int status;
		PyObject *next = PyTuple_New(itemsize);
		if (!next)
			goto Fail_ret_itlist;

		for (i = 0; i < itemsize; i++) {
			PyObject *it = PyTuple_GET_ITEM(itlist, i);
			PyObject *item = PyIter_Next(it);
			if (!item) {
				if (PyErr_Occurred()) {
					Py_DECREF(ret);
					ret = NULL;
				}
				Py_DECREF(next);
				Py_DECREF(itlist);
				return ret;
			}
			PyTuple_SET_ITEM(next, i, item);
		}

		status = PyList_Append(ret, next);
		Py_DECREF(next);
		if (status < 0)
			goto Fail_ret_itlist;
	}

Fail_ret_itlist:
	Py_DECREF(itlist);
Fail_ret:
	Py_DECREF(ret);
	return NULL;
}


static char zip_doc[] =
"zip(seq1 [, seq2 [...]]) -> [(seq1[0], seq2[0] ...), (...)]\n\
\n\
Return a list of tuples, where each tuple contains the i-th element\n\
from each of the argument sequences.  The returned list is truncated\n\
in length to the length of the shortest argument sequence.";


static PyMethodDef builtin_methods[] = {
 	{"__import__",	builtin___import__, METH_VARARGS, import_doc},
 	{"abs",		builtin_abs,        METH_O, abs_doc},
 	{"apply",	builtin_apply,      METH_VARARGS, apply_doc},
 	{"buffer",	builtin_buffer,     METH_VARARGS, buffer_doc},
 	{"callable",	builtin_callable,   METH_O, callable_doc},
 	{"chr",		builtin_chr,        METH_VARARGS, chr_doc},
 	{"cmp",		builtin_cmp,        METH_VARARGS, cmp_doc},
 	{"coerce",	builtin_coerce,     METH_VARARGS, coerce_doc},
 	{"compile",	builtin_compile,    METH_VARARGS, compile_doc},
 	{"delattr",	builtin_delattr,    METH_VARARGS, delattr_doc},
 	{"dir",		builtin_dir,        METH_VARARGS, dir_doc},
 	{"divmod",	builtin_divmod,     METH_VARARGS, divmod_doc},
 	{"eval",	builtin_eval,       METH_VARARGS, eval_doc},
 	{"execfile",	builtin_execfile,   METH_VARARGS, execfile_doc},
 	{"filter",	builtin_filter,     METH_VARARGS, filter_doc},
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
 	{"max",		builtin_max,        METH_VARARGS, max_doc},
 	{"min",		builtin_min,        METH_VARARGS, min_doc},
 	{"oct",		builtin_oct,        METH_O, oct_doc},
 	{"ord",		builtin_ord,        METH_O, ord_doc},
 	{"pow",		builtin_pow,        METH_VARARGS, pow_doc},
 	{"range",	builtin_range,      METH_VARARGS, range_doc},
 	{"raw_input",	builtin_raw_input,  METH_VARARGS, raw_input_doc},
 	{"reduce",	builtin_reduce,     METH_VARARGS, reduce_doc},
 	{"reload",	builtin_reload,     METH_O, reload_doc},
 	{"repr",	builtin_repr,       METH_O, repr_doc},
 	{"round",	builtin_round,      METH_VARARGS, round_doc},
 	{"setattr",	builtin_setattr,    METH_VARARGS, setattr_doc},
 	{"slice",       builtin_slice,      METH_VARARGS, slice_doc},
#ifdef Py_USING_UNICODE
 	{"unichr",	builtin_unichr,     METH_VARARGS, unichr_doc},
#endif
 	{"vars",	builtin_vars,       METH_VARARGS, vars_doc},
 	{"xrange",	builtin_xrange,     METH_VARARGS, xrange_doc},
  	{"zip",         builtin_zip,        METH_VARARGS, zip_doc},
	{NULL,		NULL},
};

static char builtin_doc[] =
"Built-in functions, exceptions, and other objects.\n\
\n\
Noteworthy: None is the `nil' object; Ellipsis represents `...' in slices.";

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

#define SETBUILTIN(NAME, OBJECT) \
	if (PyDict_SetItemString(dict, NAME, (PyObject *)OBJECT) < 0) \
		return NULL

	SETBUILTIN("None",		Py_None);
	SETBUILTIN("Ellipsis",		Py_Ellipsis);
	SETBUILTIN("NotImplemented",	Py_NotImplemented);
	SETBUILTIN("classmethod",	&PyClassMethod_Type);
#ifndef WITHOUT_COMPLEX
	SETBUILTIN("complex",		&PyComplex_Type);
#endif
	SETBUILTIN("dict",		&PyDict_Type);
	SETBUILTIN("float",		&PyFloat_Type);
	SETBUILTIN("property",		&PyProperty_Type);
	SETBUILTIN("int",		&PyInt_Type);
	SETBUILTIN("list",		&PyList_Type);
	SETBUILTIN("long",		&PyLong_Type);
	SETBUILTIN("object",		&PyBaseObject_Type);
	SETBUILTIN("staticmethod",	&PyStaticMethod_Type);
	SETBUILTIN("str",		&PyString_Type);
	SETBUILTIN("super",		&PySuper_Type);
	SETBUILTIN("tuple",		&PyTuple_Type);
	SETBUILTIN("type",		&PyType_Type);

	/* Note that open() is just an alias of file(). */
	SETBUILTIN("open",		&PyFile_Type);
	SETBUILTIN("file",		&PyFile_Type);
#ifdef Py_USING_UNICODE
	SETBUILTIN("unicode",		&PyUnicode_Type);
#endif
	debug = PyInt_FromLong(Py_OptimizeFlag == 0);
	if (PyDict_SetItemString(dict, "__debug__", debug) < 0) {
		Py_XDECREF(debug);
		return NULL;
	}
	Py_XDECREF(debug);

	return mod;
#undef SETBUILTIN
}

/* Helper for filter(): filter a tuple through a function */

static PyObject *
filtertuple(PyObject *func, PyObject *tuple)
{
	PyObject *result;
	register int i, j;
	int len = PyTuple_Size(tuple);

	if (len == 0) {
		Py_INCREF(tuple);
		return tuple;
	}

	if ((result = PyTuple_New(len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *good;
		int ok;

		if ((item = PyTuple_GetItem(tuple, i)) == NULL)
			goto Fail_1;
		if (func == Py_None) {
			Py_INCREF(item);
			good = item;
		}
		else {
			PyObject *arg = Py_BuildValue("(O)", item);
			if (arg == NULL)
				goto Fail_1;
			good = PyEval_CallObject(func, arg);
			Py_DECREF(arg);
			if (good == NULL)
				goto Fail_1;
		}
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
		if (ok) {
			Py_INCREF(item);
			if (PyTuple_SetItem(result, j++, item) < 0)
				goto Fail_1;
		}
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
	register int i, j;
	int len = PyString_Size(strobj);

	if (func == Py_None) {
		/* No character is ever false -- share input string */
		Py_INCREF(strobj);
		return strobj;
	}
	if ((result = PyString_FromStringAndSize(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		PyObject *item, *arg, *good;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		arg = Py_BuildValue("(O)", item);
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
		if (ok)
			PyString_AS_STRING((PyStringObject *)result)[j++] =
				PyString_AS_STRING((PyStringObject *)item)[0];
		Py_DECREF(item);
	}

	if (j < len && _PyString_Resize(&result, j) < 0)
		return NULL;

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}
