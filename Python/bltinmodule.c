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

/* Built-in functions */

#include "Python.h"

#include "node.h"
#include "compile.h"
#include "eval.h"

#include "mymath.h"

#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Forward */
static PyObject *filterstring Py_PROTO((PyObject *, PyObject *));
static PyObject *filtertuple  Py_PROTO((PyObject *, PyObject *));
static PyObject *int_from_string Py_PROTO((PyObject *));
static PyObject *long_from_string Py_PROTO((PyObject *));
static PyObject *float_from_string Py_PROTO((PyObject *));

static PyObject *
builtin___import__(self, args)
	PyObject *self;
	PyObject *args;
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


static PyObject *
builtin_abs(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nm;

	if (!PyArg_ParseTuple(args, "O:abs", &v))
		return NULL;
	if ((nm = v->ob_type->tp_as_number) == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"abs() requires numeric argument");
		return NULL;
	}
	return (*nm->nb_absolute)(v);
}

static PyObject *
builtin_apply(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *func, *alist = NULL, *kwdict = NULL;

	if (!PyArg_ParseTuple(args, "O|OO:apply", &func, &alist, &kwdict))
		return NULL;
	if (alist != NULL && !PyTuple_Check(alist)) {
		PyErr_SetString(PyExc_TypeError,
				"apply() 2nd argument must be tuple");
		return NULL;
	}
	if (kwdict != NULL && !PyDict_Check(kwdict)) {
		PyErr_SetString(PyExc_TypeError,
			   "apply() 3rd argument must be dictionary");
		return NULL;
	}
	return PyEval_CallObjectWithKeywords(func, alist, kwdict);
}

static PyObject *
builtin_callable(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:callable", &v))
		return NULL;
	return PyInt_FromLong((long)PyCallable_Check(v));
}

static PyObject *
builtin_filter(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *func, *seq, *result;
	PySequenceMethods *sqf;
	int len;
	register int i, j;

	if (!PyArg_ParseTuple(args, "OO:filter", &func, &seq))
		return NULL;

	if (PyString_Check(seq)) {
		PyObject *r = filterstring(func, seq);
		return r;
	}

	if (PyTuple_Check(seq)) {
		PyObject *r = filtertuple(func, seq);
		return r;
	}

	if ((sqf = seq->ob_type->tp_as_sequence) == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "argument 2 to filter() must be a sequence type");
		goto Fail_2;
	}

	if ((len = (*sqf->sq_length)(seq)) < 0)
		goto Fail_2;

	if (PyList_Check(seq) && seq->ob_refcnt == 1) {
		Py_INCREF(seq);
		result = seq;
	}
	else {
		if ((result = PyList_New(len)) == NULL)
			goto Fail_2;
	}

	for (i = j = 0; ; ++i) {
		PyObject *item, *good;
		int ok;

		if ((item = (*sqf->sq_item)(seq, i)) == NULL) {
			if (i < len)
				goto Fail_1;
			if (PyErr_ExceptionMatches(PyExc_IndexError)) {
				PyErr_Clear();
				break;
			}
			goto Fail_1;
		}

		if (func == Py_None) {
			good = item;
			Py_INCREF(good);
		}
		else {
			PyObject *arg = Py_BuildValue("(O)", item);
			if (arg == NULL)
				goto Fail_1;
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
			if (j < len) {
				if (PyList_SetItem(result, j++, item) < 0)
					goto Fail_1;
			}
			else {
				j++;
				if (PyList_Append(result, item) < 0)
					goto Fail_1;
			}
		} else {
			Py_DECREF(item);
		}
	}


	if (j < len && PyList_SetSlice(result, j, len, NULL) < 0)
		goto Fail_1;

	return result;

Fail_1:
	Py_DECREF(result);
Fail_2:
	return NULL;
}

static PyObject *
builtin_chr(self, args)
	PyObject *self;
	PyObject *args;
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

static PyObject *
builtin_cmp(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *a, *b;
	long c;

	if (!PyArg_ParseTuple(args, "OO:cmp", &a, &b))
		return NULL;
	c = PyObject_Compare(a, b);
	if (c && PyErr_Occurred())
		return NULL;
	return PyInt_FromLong(c);
}

static PyObject *
builtin_coerce(self, args)
	PyObject *self;
	PyObject *args;
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

static PyObject *
builtin_compile(self, args)
	PyObject *self;
	PyObject *args;
{
	char *str;
	char *filename;
	char *startstr;
	int start;

	if (!PyArg_ParseTuple(args, "sss:compile", &str, &filename, &startstr))
		return NULL;
	if (strcmp(startstr, "exec") == 0)
		start = Py_file_input;
	else if (strcmp(startstr, "eval") == 0)
		start = Py_eval_input;
	else if (strcmp(startstr, "single") == 0)
		start = Py_single_input;
	else {
		PyErr_SetString(PyExc_ValueError,
		   "compile() mode must be 'exec' or 'eval' or 'single'");
		return NULL;
	}
	return Py_CompileString(str, filename, start);
}

#ifndef WITHOUT_COMPLEX

static PyObject *
builtin_complex(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *r, *i, *tmp;
	PyNumberMethods *nbr, *nbi = NULL;
	Py_complex cr, ci;
	int own_r = 0;

	i = NULL;
	if (!PyArg_ParseTuple(args, "O|O:complex", &r, &i))
		return NULL;
	if ((nbr = r->ob_type->tp_as_number) == NULL ||
	    nbr->nb_float == NULL ||
	    (i != NULL &&
	     ((nbi = i->ob_type->tp_as_number) == NULL ||
	      nbi->nb_float == NULL))) {
		PyErr_SetString(PyExc_TypeError,
			   "complex() argument can't be converted to complex");
		return NULL;
	}
	/* XXX Hack to support classes with __complex__ method */
	if (PyInstance_Check(r)) {
		static PyObject *complexstr;
		PyObject *f;
		if (complexstr == NULL) {
			complexstr = PyString_InternFromString("__complex__");
			if (complexstr == NULL)
				return NULL;
		}
		f = PyObject_GetAttr(r, complexstr);
		if (f == NULL)
			PyErr_Clear();
		else {
			PyObject *args = Py_BuildValue("()");
			if (args == NULL)
				return NULL;
			r = PyEval_CallObject(f, args);
			Py_DECREF(args);
			if (r == NULL)
				return NULL;
			own_r = 1;
		}
	}
	if (PyComplex_Check(r)) {
		cr = ((PyComplexObject*)r)->cval;
		if (own_r)
			Py_DECREF(r);
	}
	else {
		tmp = (*nbr->nb_float)(r);
		if (own_r)
			Py_DECREF(r);
		if (tmp == NULL)
			return NULL;
		cr.real = PyFloat_AsDouble(tmp);
		Py_DECREF(tmp);
		cr.imag = 0.;
	}
	if (i == NULL) {
		ci.real = 0.;
		ci.imag = 0.;
	}
	else if (PyComplex_Check(i))
		ci = ((PyComplexObject*)i)->cval;
	else {
		tmp = (*nbi->nb_float)(i);
		if (tmp == NULL)
			return NULL;
		ci.real = PyFloat_AsDouble(tmp);
		Py_DECREF(tmp);
		ci.imag = 0.;
	}
	cr.real -= ci.imag;
	cr.imag += ci.real;
	return PyComplex_FromCComplex(cr);
}

#endif

static PyObject *
builtin_dir(self, args)
	PyObject *self;
	PyObject *args;
{
	static char *attrlist[] = {"__members__", "__methods__", NULL};
	PyObject *v = NULL, *l = NULL, *m = NULL;
	PyObject *d, *x;
	int i;
	char **s;

	if (!PyArg_ParseTuple(args, "|O:dir", &v))
		return NULL;
	if (v == NULL) {
		x = PyEval_GetLocals();
		if (x == NULL)
			goto error;
		l = PyMapping_Keys(x);
		if (l == NULL)
			goto error;
	}
	else {
		d = PyObject_GetAttrString(v, "__dict__");
		if (d == NULL)
			PyErr_Clear();
		else {
			l = PyMapping_Keys(d);
			if (l == NULL)
				PyErr_Clear();
			Py_DECREF(d);
		}
		if (l == NULL) {
			l = PyList_New(0);
			if (l == NULL)
				goto error;
		}
		for (s = attrlist; *s != NULL; s++) {
			m = PyObject_GetAttrString(v, *s);
			if (m == NULL) {
				PyErr_Clear();
				continue;
			}
			for (i = 0; ; i++) {
				x = PySequence_GetItem(m, i);
				if (x == NULL) {
					PyErr_Clear();
					break;
				}
				if (PyList_Append(l, x) != 0) {
					Py_DECREF(x);
					Py_DECREF(m);
					goto error;
				}
				Py_DECREF(x);
			}
			Py_DECREF(m);
		}
	}
	if (PyList_Sort(l) != 0)
		goto error;
	return l;
  error:
	Py_XDECREF(l);
	return NULL;
}

static PyObject *
do_divmod(v, w)
	PyObject *v, *w;
{
	PyObject *res;

	if (PyInstance_Check(v) || PyInstance_Check(w))
		return PyInstance_DoBinOp(v, w, "__divmod__", "__rdivmod__",
				     do_divmod);
	if (v->ob_type->tp_as_number == NULL ||
				w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
		    "divmod() requires numeric or class instance arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_divmod)(v, w);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

static PyObject *
builtin_divmod(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v, *w;

	if (!PyArg_ParseTuple(args, "OO:divmod", &v, &w))
		return NULL;
	return do_divmod(v, w);
}

static PyObject *
builtin_eval(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *cmd;
	PyObject *globals = Py_None, *locals = Py_None;
	char *str;

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
	if (PyCode_Check(cmd))
		return PyEval_EvalCode((PyCodeObject *) cmd, globals, locals);
	if (!PyString_Check(cmd)) {
		PyErr_SetString(PyExc_TypeError,
			   "eval() argument 1 must be string or code object");
		return NULL;
	}
	str = PyString_AsString(cmd);
	if ((int)strlen(str) != PyString_Size(cmd)) {
		PyErr_SetString(PyExc_ValueError,
			   "embedded '\\0' in string arg");
		return NULL;
	}
	while (*str == ' ' || *str == '\t')
		str++;
	return PyRun_String(str, Py_eval_input, globals, locals);
}

static PyObject *
builtin_execfile(self, args)
	PyObject *self;
	PyObject *args;
{
	char *filename;
	PyObject *globals = Py_None, *locals = Py_None;
	PyObject *res;
	FILE* fp;

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
	Py_BEGIN_ALLOW_THREADS
	fp = fopen(filename, "r");
	Py_END_ALLOW_THREADS
	if (fp == NULL) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	res = PyRun_File(fp, filename, Py_file_input, globals, locals);
	Py_BEGIN_ALLOW_THREADS
	fclose(fp);
	Py_END_ALLOW_THREADS
	return res;
}

static PyObject *
builtin_float(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nb;

	if (!PyArg_ParseTuple(args, "O:float", &v))
		return NULL;
	if (PyString_Check(v))
		return float_from_string(v);
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_float == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "float() argument can't be converted to float");
		return NULL;
	}
	return (*nb->nb_float)(v);
}

static PyObject *
builtin_getattr(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OS:getattr", &v, &name))
		return NULL;
	return PyObject_GetAttr(v, name);
}

static PyObject *
builtin_globals(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *d;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	d = PyEval_GetGlobals();
	Py_INCREF(d);
	return d;
}

static PyObject *
builtin_hasattr(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OS:hasattr", &v, &name))
		return NULL;
	v = PyObject_GetAttr(v, name);
	if (v == NULL) {
		PyErr_Clear();
		return PyInt_FromLong(0L);
	}
	Py_DECREF(v);
	return PyInt_FromLong(1L);
}

static PyObject *
builtin_id(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:id", &v))
		return NULL;
	return PyInt_FromLong((long)v);
}

static PyObject *
builtin_map(self, args)
	PyObject *self;
	PyObject *args;
{
	typedef struct {
		PyObject *seq;
		PySequenceMethods *sqf;
		int len;
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

	if ((seqs = PyMem_NEW(sequence, n)) == NULL) {
		PyErr_NoMemory();
		goto Fail_2;
	}

	for (len = 0, i = 0, sqp = seqs; i < n; ++i, ++sqp) {
		int curlen;
	
		if ((sqp->seq = PyTuple_GetItem(args, i + 1)) == NULL)
			goto Fail_2;

		if (! (sqp->sqf = sqp->seq->ob_type->tp_as_sequence)) {
			static char errmsg[] =
			    "argument %d to map() must be a sequence object";
			char errbuf[sizeof(errmsg) + 25];

			sprintf(errbuf, errmsg, i+2);
			PyErr_SetString(PyExc_TypeError, errbuf);
			goto Fail_2;
		}

		if ((curlen = sqp->len = (*sqp->sqf->sq_length)(sqp->seq)) < 0)
			goto Fail_2;

		if (curlen > len)
			len = curlen;
	}

	if ((result = (PyObject *) PyList_New(len)) == NULL)
		goto Fail_2;

	/* XXX Special case map(None, single_list) could be more efficient */
	for (i = 0; ; ++i) {
		PyObject *alist, *item=NULL, *value;
		int any = 0;

		if (func == Py_None && n == 1)
			alist = NULL;
		else {
			if ((alist = PyTuple_New(n)) == NULL)
				goto Fail_1;
		}

		for (j = 0, sqp = seqs; j < n; ++j, ++sqp) {
			if (sqp->len < 0) {
				Py_INCREF(Py_None);
				item = Py_None;
			}
			else {
				item = (*sqp->sqf->sq_item)(sqp->seq, i);
				if (item == NULL) {
					if (i < sqp->len)
						goto Fail_0;
					if (PyErr_ExceptionMatches(
						PyExc_IndexError))
					{
						PyErr_Clear();
						Py_INCREF(Py_None);
						item = Py_None;
						sqp->len = -1;
					}
					else {
						goto Fail_0;
					}
				}
				else
					any = 1;

			}
			if (!alist)
				break;
			if (PyTuple_SetItem(alist, j, item) < 0) {
				Py_DECREF(item);
				goto Fail_0;
			}
			continue;

		Fail_0:
			Py_XDECREF(alist);
			goto Fail_1;
		}

		if (!alist)
			alist = item;

		if (!any) {
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
			if (PyList_Append(result, value) < 0)
				goto Fail_1;
		}
		else {
			if (PyList_SetItem(result, i, value) < 0)
				goto Fail_1;
		}
	}

	PyMem_DEL(seqs);
	return result;

Fail_1:
	Py_DECREF(result);
Fail_2:
	if (seqs) PyMem_DEL(seqs);
	return NULL;
}

static PyObject *
builtin_setattr(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyObject *name;
	PyObject *value;

	if (!PyArg_ParseTuple(args, "OSO:setattr", &v, &name, &value))
		return NULL;
	if (PyObject_SetAttr(v, name, value) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
builtin_delattr(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyObject *name;

	if (!PyArg_ParseTuple(args, "OS:delattr", &v, &name))
		return NULL;
	if (PyObject_SetAttr(v, name, (PyObject *)NULL) != 0)
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
builtin_hash(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	long x;

	if (!PyArg_ParseTuple(args, "O:hash", &v))
		return NULL;
	x = PyObject_Hash(v);
	if (x == -1)
		return NULL;
	return PyInt_FromLong(x);
}

static PyObject *
builtin_hex(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nb;

	if (!PyArg_ParseTuple(args, "O:hex", &v))
		return NULL;
	
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_hex == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "hex() argument can't be converted to hex");
		return NULL;
	}
	return (*nb->nb_hex)(v);
}

static PyObject *builtin_raw_input Py_PROTO((PyObject *, PyObject *));

static PyObject *
builtin_input(self, args)
	PyObject *self;
	PyObject *args;
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

static PyObject *
builtin_intern(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *s;
	if (!PyArg_ParseTuple(args, "S", &s))
		return NULL;
	Py_INCREF(s);
	PyString_InternInPlace(&s);
	return s;
}

static PyObject *
builtin_int(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nb;

	if (!PyArg_ParseTuple(args, "O:int", &v))
		return NULL;
	if (PyString_Check(v))
		return int_from_string(v);
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_int == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "int() argument can't be converted to int");
		return NULL;
	}
	return (*nb->nb_int)(v);
}

static PyObject *
builtin_len(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	long len;
	PyTypeObject *tp;

	if (!PyArg_ParseTuple(args, "O:len", &v))
		return NULL;
	tp = v->ob_type;
	if (tp->tp_as_sequence != NULL) {
		len = (*tp->tp_as_sequence->sq_length)(v);
	}
	else if (tp->tp_as_mapping != NULL) {
		len = (*tp->tp_as_mapping->mp_length)(v);
	}
	else {
		PyErr_SetString(PyExc_TypeError, "len() of unsized object");
		return NULL;
	}
	if (len < 0)
		return NULL;
	else
		return PyInt_FromLong(len);
}

static PyObject *
builtin_list(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PySequenceMethods *sqf;

	if (!PyArg_ParseTuple(args, "O:list", &v))
		return NULL;
	if ((sqf = v->ob_type->tp_as_sequence) != NULL) {
		int n = (*sqf->sq_length)(v);
		int i;
		PyObject *l;
		if (n < 0)
			return NULL;
		l = PyList_New(n);
		if (l == NULL)
			return NULL;
		for (i = 0; i < n; i++) {
			PyObject *item = (*sqf->sq_item)(v, i);
			if (item == NULL) {
				Py_DECREF(l);
				l = NULL;
				break;
			}
			PyList_SetItem(l, i, item);
		}
		/* XXX Should support indefinite-length sequences */
		return l;
	}
	PyErr_SetString(PyExc_TypeError, "list() argument must be a sequence");
	return NULL;
}


static PyObject *
builtin_slice(self, args)
     PyObject *self;
     PyObject *args;
{
  PyObject *start, *stop, *step;

  start = stop = step = NULL;

  if (!PyArg_ParseTuple(args, "O|OO:slice", &start, &stop, &step))
    return NULL;

  /*This swapping of stop and start is to maintain compatibility with
    the range builtin.*/
  if (stop == NULL) {
    stop = start;
    start = NULL;
  }
  return PySlice_New(start, stop, step);
}

static PyObject *
builtin_locals(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *d;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	d = PyEval_GetLocals();
	Py_INCREF(d);
	return d;
}

static PyObject *
builtin_long(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nb;
	
	if (!PyArg_ParseTuple(args, "O:long", &v))
		return NULL;
	if (PyString_Check(v))
		return long_from_string(v);
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_long == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "long() argument can't be converted to long");
		return NULL;
	}
	return (*nb->nb_long)(v);
}

static PyObject *
min_max(args, sign)
	PyObject *args;
	int sign;
{
	int i;
	PyObject *v, *w, *x;
	PySequenceMethods *sq;

	if (PyTuple_Size(args) > 1)
		v = args;
	else if (!PyArg_ParseTuple(args, "O:min/max", &v))
		return NULL;
	sq = v->ob_type->tp_as_sequence;
	if (sq == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"min() or max() of non-sequence");
		return NULL;
	}
	w = NULL;
	for (i = 0; ; i++) {
		x = (*sq->sq_item)(v, i); /* Implies INCREF */
		if (x == NULL) {
			if (PyErr_ExceptionMatches(PyExc_IndexError)) {
				PyErr_Clear();
				break;
			}
			Py_XDECREF(w);
			return NULL;
		}
		if (w == NULL)
			w = x;
		else {
			int c = PyObject_Compare(x, w);
			if (c && PyErr_Occurred()) {
				Py_DECREF(x);
				Py_XDECREF(w);
				return NULL;
			}
			if (c * sign > 0) {
				Py_DECREF(w);
				w = x;
			}
			else
				Py_DECREF(x);
		}
	}
	if (w == NULL)
		PyErr_SetString(PyExc_ValueError,
				"min() or max() of empty sequence");
	return w;
}

static PyObject *
builtin_min(self, v)
	PyObject *self;
	PyObject *v;
{
	return min_max(v, -1);
}

static PyObject *
builtin_max(self, v)
	PyObject *self;
	PyObject *v;
{
	return min_max(v, 1);
}

static PyObject *
builtin_oct(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PyNumberMethods *nb;

	if (!PyArg_ParseTuple(args, "O:oct", &v))
		return NULL;
	if (v == NULL || (nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_oct == NULL) {
		PyErr_SetString(PyExc_TypeError,
			   "oct() argument can't be converted to oct");
		return NULL;
	}
	return (*nb->nb_oct)(v);
}

static PyObject *
builtin_open(self, args)
	PyObject *self;
	PyObject *args;
{
	char *name;
	char *mode = "r";
	int bufsize = -1;
	PyObject *f;

	if (!PyArg_ParseTuple(args, "s|si:open", &name, &mode, &bufsize))
		return NULL;
	f = PyFile_FromString(name, mode);
	if (f != NULL)
		PyFile_SetBufSize(f, bufsize);
	return f;
}

static PyObject *
builtin_ord(self, args)
	PyObject *self;
	PyObject *args;
{
	char c;

	if (!PyArg_ParseTuple(args, "c:ord", &c))
		return NULL;
	return PyInt_FromLong((long)(c & 0xff));
}

static PyObject *
do_pow(v, w)
	PyObject *v, *w;
{
	PyObject *res;
	if (PyInstance_Check(v) || PyInstance_Check(w))
		return PyInstance_DoBinOp(v, w, "__pow__", "__rpow__", do_pow);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"pow() requires numeric arguments");
		return NULL;
	}
	if (
#ifndef WITHOUT_COMPLEX
            !PyComplex_Check(v) && 
#endif
            PyFloat_Check(w) && PyFloat_AsDouble(v) < 0.0) {
		if (!PyErr_Occurred())
		    PyErr_SetString(PyExc_ValueError,
				    "negative number to float power");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_power)(v, w, Py_None);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

static PyObject *
builtin_pow(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v, *w, *z = Py_None, *res;
	PyObject *v1, *z1, *w2, *z2;

	if (!PyArg_ParseTuple(args, "OO|O:pow", &v, &w, &z))
		return NULL;
	if (z == Py_None)
		return do_pow(v, w);
	/* XXX The ternary version doesn't do class instance coercions */
	if (PyInstance_Check(v))
		return v->ob_type->tp_as_number->nb_power(v, w, z);
	if (v->ob_type->tp_as_number == NULL ||
	    z->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"pow() requires numeric arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = NULL;
	v1 = v;
	z1 = z;
	if (PyNumber_Coerce(&v1, &z1) != 0)
		goto error2;
	w2 = w;
	z2 = z1;
 	if (PyNumber_Coerce(&w2, &z2) != 0)
		goto error1;
	res = (*v1->ob_type->tp_as_number->nb_power)(v1, w2, z2);
	Py_DECREF(w2);
	Py_DECREF(z2);
 error1:
	Py_DECREF(v1);
	Py_DECREF(z1);
 error2:
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

static PyObject *
builtin_range(self, args)
	PyObject *self;
	PyObject *args;
{
	long ilow = 0, ihigh = 0, istep = 1;
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
		PyErr_SetString(PyExc_ValueError, "zero step for range()");
		return NULL;
	}
	/* XXX ought to check overflow of subtraction */
	if (istep > 0)
		n = (ihigh - ilow + istep - 1) / istep;
	else
		n = (ihigh - ilow + istep + 1) / istep;
	if (n < 0)
		n = 0;
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyObject *w = PyInt_FromLong(ilow);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i, w);
		ilow += istep;
	}
	return v;
}

static PyObject *
builtin_xrange(self, args)
	PyObject *self;
	PyObject *args;
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
		PyErr_SetString(PyExc_ValueError, "zero step for xrange()");
		return NULL;
	}
	/* XXX ought to check overflow of subtraction */
	if (istep > 0)
		n = (ihigh - ilow + istep - 1) / istep;
	else
		n = (ihigh - ilow + istep + 1) / istep;
	if (n < 0)
		n = 0;
	return PyRange_New(ilow, n, istep, 1);
}

extern char *PyOS_Readline Py_PROTO((char *));

static PyObject *
builtin_raw_input(self, args)
	PyObject *self;
	PyObject *args;
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
			result = PyString_FromStringAndSize(s, strlen(s)-1);
		}
		free(s);
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

static PyObject *
builtin_reduce(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *seq, *func, *result = NULL;
	PySequenceMethods *sqf;
	register int i;

	if (!PyArg_ParseTuple(args, "OO|O:reduce", &func, &seq, &result))
		return NULL;
	if (result != NULL)
		Py_INCREF(result);

	if ((sqf = seq->ob_type->tp_as_sequence) == NULL) {
		PyErr_SetString(PyExc_TypeError,
		    "2nd argument to reduce() must be a sequence object");
		return NULL;
	}

	if ((args = PyTuple_New(2)) == NULL)
		goto Fail;

	for (i = 0; ; ++i) {
		PyObject *op2;

		if (args->ob_refcnt > 1) {
			Py_DECREF(args);
			if ((args = PyTuple_New(2)) == NULL)
				goto Fail;
		}

		if ((op2 = (*sqf->sq_item)(seq, i)) == NULL) {
			if (PyErr_ExceptionMatches(PyExc_IndexError)) {
				PyErr_Clear();
				break;
			}
			goto Fail;
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
			   "reduce of empty sequence with no initial value");

	return result;

Fail:
	Py_XDECREF(args);
	Py_XDECREF(result);
	return NULL;
}

static PyObject *
builtin_reload(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:reload", &v))
		return NULL;
	return PyImport_ReloadModule(v);
}

static PyObject *
builtin_repr(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:repr", &v))
		return NULL;
	return PyObject_Repr(v);
}

static PyObject *
builtin_round(self, args)
	PyObject *self;
	PyObject *args;
{
	double x;
	double f;
	int ndigits = 0;
	int i;

	if (!PyArg_ParseTuple(args, "d|i:round", &x, &ndigits))
			return NULL;
	f = 1.0;
	for (i = ndigits; --i >= 0; )
		f = f*10.0;
	for (i = ndigits; ++i <= 0; )
		f = f*0.1;
	if (x >= 0.0)
		return PyFloat_FromDouble(floor(x*f + 0.5) / f);
	else
		return PyFloat_FromDouble(ceil(x*f - 0.5) / f);
}

static PyObject *
builtin_str(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:str", &v))
		return NULL;
	return PyObject_Str(v);
}

static PyObject *
builtin_tuple(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;
	PySequenceMethods *sqf;

	if (!PyArg_ParseTuple(args, "O:tuple", &v))
		return NULL;
	if (PyTuple_Check(v)) {
		Py_INCREF(v);
		return v;
	}
	if (PyList_Check(v))
		return PyList_AsTuple(v);
	if (PyString_Check(v)) {
		int n = PyString_Size(v);
		PyObject *t = PyTuple_New(n);
		if (t != NULL) {
			int i;
			char *p = PyString_AsString(v);
			for (i = 0; i < n; i++) {
				PyObject *item =
					PyString_FromStringAndSize(p+i, 1);
				if (item == NULL) {
					Py_DECREF(t);
					t = NULL;
					break;
				}
				PyTuple_SetItem(t, i, item);
			}
		}
		return t;
	}
	/* Generic sequence object */
	if ((sqf = v->ob_type->tp_as_sequence) != NULL) {
		int n = (*sqf->sq_length)(v);
		int i;
		PyObject *t;
		if (n < 0)
			return NULL;
		t = PyTuple_New(n);
		if (t == NULL)
			return NULL;
		for (i = 0; i < n; i++) {
			PyObject *item = (*sqf->sq_item)(v, i);
			if (item == NULL) {
				Py_DECREF(t);
				t = NULL;
				break;
			}
			PyTuple_SetItem(t, i, item);
		}
		/* XXX Should support indefinite-length sequences */
		return t;
	}
	/* None of the above */
	PyErr_SetString(PyExc_TypeError,
			"tuple() argument must be a sequence");
	return NULL;
}

static PyObject *
builtin_type(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *v;

	if (!PyArg_ParseTuple(args, "O:type", &v))
		return NULL;
	v = (PyObject *)v->ob_type;
	Py_INCREF(v);
	return v;
}

static PyObject *
builtin_vars(self, args)
	PyObject *self;
	PyObject *args;
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

static PyObject *
builtin_isinstance(self, args)
     PyObject *self;
     PyObject *args;
{
	PyObject *inst;
	PyObject *cls;
	int retval;

	if (!PyArg_ParseTuple(args, "OO", &inst, &cls))
		return NULL;
	if (!PyClass_Check(cls)) {
		PyErr_SetString(PyExc_TypeError,
				"second argument must be a class");
		return NULL;
	}

	if (!PyInstance_Check(inst))
		retval = 0;
	else {
		PyObject *inclass =
			(PyObject*)((PyInstanceObject*)inst)->in_class;
		retval = PyClass_IsSubclass(inclass, cls);
	}
	return PyInt_FromLong(retval);
}


static PyObject *
builtin_issubclass(self, args)
     PyObject *self;
     PyObject *args;
{
	PyObject *derived;
	PyObject *cls;
	int retval;

	if (!PyArg_ParseTuple(args, "OO", &derived, &cls))
		return NULL;
	if (!PyClass_Check(derived) || !PyClass_Check(cls)) {
		PyErr_SetString(PyExc_TypeError, "arguments must be classes");
		return NULL;
	}
	/* shortcut */
	if (!(retval = (derived == cls)))
		retval = PyClass_IsSubclass(derived, cls);

	return PyInt_FromLong(retval);
}


static PyMethodDef builtin_methods[] = {
	{"__import__",	builtin___import__, 1},
	{"abs",		builtin_abs, 1},
	{"apply",	builtin_apply, 1},
	{"callable",	builtin_callable, 1},
	{"chr",		builtin_chr, 1},
	{"cmp",		builtin_cmp, 1},
	{"coerce",	builtin_coerce, 1},
	{"compile",	builtin_compile, 1},
#ifndef WITHOUT_COMPLEX
	{"complex",	builtin_complex, 1},
#endif
	{"delattr",	builtin_delattr, 1},
	{"dir",		builtin_dir, 1},
	{"divmod",	builtin_divmod, 1},
	{"eval",	builtin_eval, 1},
	{"execfile",	builtin_execfile, 1},
	{"filter",	builtin_filter, 1},
	{"float",	builtin_float, 1},
	{"getattr",	builtin_getattr, 1},
	{"globals",	builtin_globals, 1},
	{"hasattr",	builtin_hasattr, 1},
	{"hash",	builtin_hash, 1},
	{"hex",		builtin_hex, 1},
	{"id",		builtin_id, 1},
	{"input",	builtin_input, 1},
	{"intern",	builtin_intern, 1},
	{"int",		builtin_int, 1},
	{"isinstance",  builtin_isinstance, 1},
	{"issubclass",  builtin_issubclass, 1},
	{"len",		builtin_len, 1},
	{"list",	builtin_list, 1},
	{"locals",	builtin_locals, 1},
	{"long",	builtin_long, 1},
	{"map",		builtin_map, 1},
	{"max",		builtin_max, 1},
	{"min",		builtin_min, 1},
	{"oct",		builtin_oct, 1},
	{"open",	builtin_open, 1},
	{"ord",		builtin_ord, 1},
	{"pow",		builtin_pow, 1},
	{"range",	builtin_range, 1},
	{"raw_input",	builtin_raw_input, 1},
	{"reduce",	builtin_reduce, 1},
	{"reload",	builtin_reload, 1},
	{"repr",	builtin_repr, 1},
	{"round",	builtin_round, 1},
	{"setattr",	builtin_setattr, 1},
	{"slice",       builtin_slice, 1},
	{"str",		builtin_str, 1},
	{"tuple",	builtin_tuple, 1},
	{"type",	builtin_type, 1},
	{"vars",	builtin_vars, 1},
	{"xrange",	builtin_xrange, 1},
	{NULL,		NULL},
};

/* Predefined exceptions */

PyObject *PyExc_Exception;
PyObject *PyExc_StandardError;
PyObject *PyExc_NumberError;
PyObject *PyExc_LookupError;

PyObject *PyExc_AssertionError;
PyObject *PyExc_AttributeError;
PyObject *PyExc_EOFError;
PyObject *PyExc_FloatingPointError;
PyObject *PyExc_IOError;
PyObject *PyExc_ImportError;
PyObject *PyExc_IndexError;
PyObject *PyExc_KeyError;
PyObject *PyExc_KeyboardInterrupt;
PyObject *PyExc_MemoryError;
PyObject *PyExc_NameError;
PyObject *PyExc_OverflowError;
PyObject *PyExc_RuntimeError;
PyObject *PyExc_SyntaxError;
PyObject *PyExc_SystemError;
PyObject *PyExc_SystemExit;
PyObject *PyExc_TypeError;
PyObject *PyExc_ValueError;
PyObject *PyExc_ZeroDivisionError;

PyObject *PyExc_MemoryErrorInst;

static struct 
{
	char* name;
	PyObject** exc;
	int leaf_exc;
}
bltin_exc[] = {
	{"Exception",          &PyExc_Exception,          0},
	{"StandardError",      &PyExc_StandardError,      0},
	{"NumberError",        &PyExc_NumberError,        0},
	{"LookupError",        &PyExc_LookupError,        0},
	{"AssertionError",     &PyExc_AssertionError,     1},
	{"AttributeError",     &PyExc_AttributeError,     1},
	{"EOFError",           &PyExc_EOFError,           1},
	{"FloatingPointError", &PyExc_FloatingPointError, 1},
	{"IOError",            &PyExc_IOError,            1},
	{"ImportError",        &PyExc_ImportError,        1},
	{"IndexError",         &PyExc_IndexError,         1},
	{"KeyError",           &PyExc_KeyError,           1},
	{"KeyboardInterrupt",  &PyExc_KeyboardInterrupt,  1},
	{"MemoryError",        &PyExc_MemoryError,        1},
	{"NameError",          &PyExc_NameError,          1},
	{"OverflowError",      &PyExc_OverflowError,      1},
	{"RuntimeError",       &PyExc_RuntimeError,       1},
	{"SyntaxError",        &PyExc_SyntaxError,        1},
	{"SystemError",        &PyExc_SystemError,        1},
	{"SystemExit",         &PyExc_SystemExit,         1},
	{"TypeError",          &PyExc_TypeError,          1},
	{"ValueError",         &PyExc_ValueError,         1},
	{"ZeroDivisionError",  &PyExc_ZeroDivisionError,  1},
	{NULL, NULL}
};


/* import exceptions module to extract class exceptions */
static void
init_class_exc(dict)
	PyObject *dict;
{
	int i;
	PyObject *m = PyImport_ImportModule("exceptions");
	PyObject *d;
	PyObject *args;

	if (m == NULL ||
	    (d = PyModule_GetDict(m)) == NULL)
	{
		PyObject *f = PySys_GetObject("stderr");
		if (Py_VerboseFlag) {
			PyFile_WriteString(
				"'import exceptions' failed; traceback:\n", f);
			PyErr_Print();
		}
		else {
			PyFile_WriteString(
		      "'import exceptions' failed; use -v for traceback\n", f);
			PyErr_Clear();
		}
		PyFile_WriteString("defaulting to old style exceptions\n", f);
		return;
	}
	for (i = 0; bltin_exc[i].name; i++) {
		/* dig the exception out of the module */
		PyObject *exc = PyDict_GetItemString(d, bltin_exc[i].name);
		if (!exc)
		     Py_FatalError("built-in exception cannot be initialized");
		
		Py_XDECREF(*bltin_exc[i].exc);

		/* squirrel away a pointer to the exception */
		Py_INCREF(exc);
		*bltin_exc[i].exc = exc;

		/* and insert the name in the __builtin__ module */
		PyDict_SetItemString(dict, bltin_exc[i].name, exc);
	}

	/* we need one pre-allocated instance */
	args = Py_BuildValue("()");
	if (args) {
		PyExc_MemoryErrorInst =
			PyEval_CallObject(PyExc_MemoryError, args);
		Py_DECREF(args);
	}

	/* we're done with the exceptions module */
	Py_DECREF(m);

	if (PyErr_Occurred())
		Py_FatalError("can't instantiate standard exceptions");
}


static void
fini_instances()
{
	Py_XDECREF(PyExc_MemoryErrorInst);
	PyExc_MemoryErrorInst = NULL;
}


static PyObject *
newstdexception(dict, name)
	PyObject *dict;
	char *name;
{
	PyObject *v = PyString_FromString(name);
	if (v == NULL || PyDict_SetItemString(dict, name, v) != 0)
		Py_FatalError("no mem for new standard exception");
	return v;
}

static void
initerrors(dict)
	PyObject *dict;
{
	int i;
	int exccnt = 0;
	for (i = 0; bltin_exc[i].name; i++, exccnt++) {
		if (bltin_exc[i].leaf_exc)
			*bltin_exc[i].exc =
				newstdexception(dict, bltin_exc[i].name);
	}

	/* This is kind of bogus because we special case the three new
	   exceptions to be nearly forward compatible.  But this means we
	   hard code knowledge about exceptions.py into C here.  I don't
	   have a better solution, though
	*/
	PyExc_LookupError = PyTuple_New(2);
	Py_INCREF(PyExc_IndexError);
	PyTuple_SET_ITEM(PyExc_LookupError, 0, PyExc_IndexError);
	Py_INCREF(PyExc_KeyError);
	PyTuple_SET_ITEM(PyExc_LookupError, 1, PyExc_KeyError);
	PyDict_SetItemString(dict, "LookupError", PyExc_LookupError);

	PyExc_NumberError = PyTuple_New(3);
	Py_INCREF(PyExc_OverflowError);
	PyTuple_SET_ITEM(PyExc_NumberError, 0, PyExc_OverflowError);
	Py_INCREF(PyExc_ZeroDivisionError);
	PyTuple_SET_ITEM(PyExc_NumberError, 1, PyExc_ZeroDivisionError);
	Py_INCREF(PyExc_FloatingPointError);
	PyTuple_SET_ITEM(PyExc_NumberError, 2, PyExc_FloatingPointError);
	PyDict_SetItemString(dict, "NumberError", PyExc_NumberError);

	PyExc_StandardError = PyTuple_New(exccnt-1);
	for (i = 1; bltin_exc[i].name; i++) {
		PyObject *exc = *bltin_exc[i].exc;
		Py_INCREF(exc);
		PyTuple_SET_ITEM(PyExc_StandardError, i-1, exc);
	}
	PyDict_SetItemString(dict, "StandardError", PyExc_StandardError);

	/* Exception is treated differently; for now, it's == StandardError */
	PyExc_Exception = PyExc_StandardError;
	Py_INCREF(PyExc_Exception);
	PyDict_SetItemString(dict, "Exception", PyExc_Exception);
	
	if (PyErr_Occurred())
	      Py_FatalError("Could not initialize built-in string exceptions");
}

static void
finierrors()
{
	int i;
	for (i = 0; bltin_exc[i].name; i++) {
		PyObject *exc = *bltin_exc[i].exc;
		Py_XDECREF(exc);
		*bltin_exc[i].exc = NULL;
	}
}

PyObject *
_PyBuiltin_Init_1()
{
	PyObject *mod, *dict;
	mod = Py_InitModule("__builtin__", builtin_methods);
	if (mod == NULL)
		return NULL;
	dict = PyModule_GetDict(mod);
	initerrors(dict);
	if (PyDict_SetItemString(dict, "None", Py_None) < 0)
		return NULL;
	if (PyDict_SetItemString(dict, "Ellipsis", Py_Ellipsis) < 0)
		return NULL;
	if (PyDict_SetItemString(dict, "__debug__",
			  PyInt_FromLong(Py_OptimizeFlag == 0)) < 0)
		return NULL;

	return mod;
}

void
_PyBuiltin_Init_2(dict)
	PyObject *dict;
{
	/* if Python was started with -X, initialize the class exceptions */
	if (Py_UseClassExceptionsFlag)
		init_class_exc(dict);
}


void
_PyBuiltin_Fini_1()
{
	fini_instances();
}


void
_PyBuiltin_Fini_2()
{
	finierrors();
}


/* Helper for filter(): filter a tuple through a function */

static PyObject *
filtertuple(func, tuple)
	PyObject *func;
	PyObject *tuple;
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

	if (_PyTuple_Resize(&result, j, 0) < 0)
		return NULL;

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}


/* Helper for filter(): filter a string through a function */

static PyObject *
filterstring(func, strobj)
	PyObject *func;
	PyObject *strobj;
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
		Py_DECREF(item);
		if (arg == NULL)
			goto Fail_1;
		good = PyEval_CallObject(func, arg);
		Py_DECREF(arg);
		if (good == NULL)
			goto Fail_1;
		ok = PyObject_IsTrue(good);
		Py_DECREF(good);
		if (ok)
			PyString_AS_STRING((PyStringObject *)result)[j++] =
				PyString_AS_STRING((PyStringObject *)item)[0];
	}

	if (j < len && _PyString_Resize(&result, j) < 0)
		return NULL;

	return result;

Fail_1:
	Py_DECREF(result);
	return NULL;
}

/* Copied with modifications from stropmodule.c: atoi,atof.atol */

static PyObject *
int_from_string(v)
	PyObject *v;
{
	extern long PyOS_strtol Py_PROTO((const char *, char **, int));
	char *s, *end;
	long x;
	char buffer[256]; /* For errors */

	if (!PyArg_Parse(v, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for int()");
		return NULL;
	}
	errno = 0;
	x = PyOS_strtol(s, &end, 10);
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for int(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "int() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyInt_FromLong(x);
}

static PyObject *
long_from_string(v)
	PyObject *v;
{
	char *s, *end;
	PyObject *x;
	char buffer[256]; /* For errors */

	if (!PyArg_Parse(v, "s", &s))
		return NULL;

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for long()");
		return NULL;
	}
	x = PyLong_FromString(s, &end, 10);
	if (x == NULL)
		return NULL;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for long(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		Py_DECREF(x);
		return NULL;
	}
	return x;
}

static PyObject *
float_from_string(v)
	PyObject *v;
{
	extern double strtod Py_PROTO((const char *, char **));
	char *s, *end;
	double x;
	char buffer[256]; /* For errors */

	if (!PyArg_Parse(v, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for float()");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("float_from_string", return 0)
	x = strtod(s, &end);
	PyFPE_END_PROTECT(x)
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for float(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "float() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyFloat_FromDouble(x);
}
