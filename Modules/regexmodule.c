/*
XXX support range parameter on search
XXX support mstop parameter on search
*/

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Regular expression objects */
/* This uses Tatu Ylonen's copyleft-free reimplementation of
   GNU regular expressions */

#include "Python.h"

#include "regexpr.h"

static PyObject *RegexError;	/* Exception */	

typedef struct {
	PyObject_HEAD
	struct re_pattern_buffer re_patbuf; /* The compiled expression */
	struct re_registers re_regs; /* The registers from the last match */
	char re_fastmap[256];	/* Storage for fastmap */
	PyObject *re_translate;	/* String object for translate table */
	PyObject *re_lastok;	/* String object last matched/searched */
	PyObject *re_groupindex;	/* Group name to index dictionary */
	PyObject *re_givenpat;	/* Pattern with symbolic groups */
	PyObject *re_realpat;	/* Pattern without symbolic groups */
} regexobject;

/* Regex object methods */

static void
reg_dealloc(re)
	regexobject *re;
{
	PyMem_XDEL(re->re_patbuf.buffer);
	Py_XDECREF(re->re_translate);
	Py_XDECREF(re->re_lastok);
	Py_XDECREF(re->re_groupindex);
	Py_XDECREF(re->re_givenpat);
	Py_XDECREF(re->re_realpat);
	PyMem_DEL(re);
}

static PyObject *
makeresult(regs)
	struct re_registers *regs;
{
	PyObject *v = PyTuple_New(RE_NREGS);
	if (v != NULL) {
		int i;
		for (i = 0; i < RE_NREGS; i++) {
			PyObject *w;
			w = Py_BuildValue("(ii)", regs->start[i], regs->end[i]);
			if (w == NULL) {
				Py_XDECREF(v);
				v = NULL;
				break;
			}
			PyTuple_SetItem(v, i, w);
		}
	}
	return v;
}

static PyObject *
reg_match(re, args)
	regexobject *re;
	PyObject *args;
{
	PyObject *argstring;
	char *buffer;
	int size;
	int offset;
	int result;
	if (PyArg_Parse(args, "S", &argstring)) {
		offset = 0;
	}
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(Si)", &argstring, &offset))
			return NULL;
	}
	buffer = PyString_AsString(argstring);
	size = PyString_Size(argstring);
	if (offset < 0 || offset > size) {
		PyErr_SetString(RegexError, "match offset out of range");
		return NULL;
	}
	Py_XDECREF(re->re_lastok);
	re->re_lastok = NULL;
	result = re_match(&re->re_patbuf, buffer, size, offset, &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		PyErr_SetString(RegexError, "match failure");
		return NULL;
	}
	if (result >= 0) {
		Py_INCREF(argstring);
		re->re_lastok = argstring;
	}
	return PyInt_FromLong((long)result); /* Length of the match or -1 */
}

static PyObject *
reg_search(re, args)
	regexobject *re;
	PyObject *args;
{
	PyObject *argstring;
	char *buffer;
	int size;
	int offset;
	int range;
	int result;
	
	if (PyArg_Parse(args, "S", &argstring)) {
		offset = 0;
	}
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(Si)", &argstring, &offset))
			return NULL;
	}
	buffer = PyString_AsString(argstring);
	size = PyString_Size(argstring);
	if (offset < 0 || offset > size) {
		PyErr_SetString(RegexError, "search offset out of range");
		return NULL;
	}
	/* NB: In Emacs 18.57, the documentation for re_search[_2] and
	   the implementation don't match: the documentation states that
	   |range| positions are tried, while the code tries |range|+1
	   positions.  It seems more productive to believe the code! */
	range = size - offset;
	Py_XDECREF(re->re_lastok);
	re->re_lastok = NULL;
	result = re_search(&re->re_patbuf, buffer, size, offset, range,
			   &re->re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		PyErr_SetString(RegexError, "match failure");
		return NULL;
	}
	if (result >= 0) {
		Py_INCREF(argstring);
		re->re_lastok = argstring;
	}
	return PyInt_FromLong((long)result); /* Position of the match or -1 */
}

static PyObject *
reg_group(re, args)
	regexobject *re;
	PyObject *args;
{
	int i, a, b;
	if (args != NULL && PyTuple_Check(args)) {
		int n = PyTuple_Size(args);
		PyObject *res = PyTuple_New(n);
		if (res == NULL)
			return NULL;
		for (i = 0; i < n; i++) {
			PyObject *v = reg_group(re, PyTuple_GetItem(args, i));
			if (v == NULL) {
				Py_DECREF(res);
				return NULL;
			}
			PyTuple_SetItem(res, i, v);
		}
		return res;
	}
	if (!PyArg_Parse(args, "i", &i)) {
		PyObject *n;
		PyErr_Clear();
		if (!PyArg_Parse(args, "S", &n))
			return NULL;
		else {
			PyObject *index;
			if (re->re_groupindex == NULL)
				index = NULL;
			else
				index = PyDict_GetItem(re->re_groupindex, n);
			if (index == NULL) {
				PyErr_SetString(RegexError, "group() group name doesn't exist");
				return NULL;
			}
			i = PyInt_AsLong(index);
		}
	}
	if (i < 0 || i >= RE_NREGS) {
		PyErr_SetString(RegexError, "group() index out of range");
		return NULL;
	}
	if (re->re_lastok == NULL) {
		PyErr_SetString(RegexError,
		    "group() only valid after successful match/search");
		return NULL;
	}
	a = re->re_regs.start[i];
	b = re->re_regs.end[i];
	if (a < 0 || b < 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromStringAndSize(PyString_AsString(re->re_lastok)+a, b-a);
}

static struct PyMethodDef reg_methods[] = {
	{"match",	(PyCFunction)reg_match},
	{"search",	(PyCFunction)reg_search},
	{"group",	(PyCFunction)reg_group},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
reg_getattr(re, name)
	regexobject *re;
	char *name;
{
	if (strcmp(name, "regs") == 0) {
		if (re->re_lastok == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		return makeresult(&re->re_regs);
	}
	if (strcmp(name, "last") == 0) {
		if (re->re_lastok == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_INCREF(re->re_lastok);
		return re->re_lastok;
	}
	if (strcmp(name, "translate") == 0) {
		if (re->re_translate == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_INCREF(re->re_translate);
		return re->re_translate;
	}
	if (strcmp(name, "groupindex") == 0) {
		if (re->re_groupindex == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_INCREF(re->re_groupindex);
		return re->re_groupindex;
	}
	if (strcmp(name, "realpat") == 0) {
		if (re->re_realpat == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_INCREF(re->re_realpat);
		return re->re_realpat;
	}
	if (strcmp(name, "givenpat") == 0) {
		if (re->re_givenpat == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		Py_INCREF(re->re_givenpat);
		return re->re_givenpat;
	}
	if (strcmp(name, "__members__") == 0) {
		PyObject *list = PyList_New(6);
		if (list) {
			PyList_SetItem(list, 0, PyString_FromString("last"));
			PyList_SetItem(list, 1, PyString_FromString("regs"));
			PyList_SetItem(list, 2, PyString_FromString("translate"));
			PyList_SetItem(list, 3, PyString_FromString("groupindex"));
			PyList_SetItem(list, 4, PyString_FromString("realpat"));
			PyList_SetItem(list, 5, PyString_FromString("givenpat"));
			if (PyErr_Occurred()) {
				Py_DECREF(list);
				list = NULL;
			}
		}
		return list;
	}
	return Py_FindMethod(reg_methods, (PyObject *)re, name);
}

static PyTypeObject Regextype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"regex",		/*tp_name*/
	sizeof(regexobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)reg_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)reg_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static PyObject *
newregexobject(pattern, translate, givenpat, groupindex)
	PyObject *pattern;
	PyObject *translate;
	PyObject *givenpat;
	PyObject *groupindex;
{
	regexobject *re;
	char *pat = PyString_AsString(pattern);
	int size = PyString_Size(pattern);

	if (translate != NULL && PyString_Size(translate) != 256) {
		PyErr_SetString(RegexError,
			   "translation table must be 256 bytes");
		return NULL;
	}
	re = PyObject_NEW(regexobject, &Regextype);
	if (re != NULL) {
		char *error;
		re->re_patbuf.buffer = NULL;
		re->re_patbuf.allocated = 0;
		re->re_patbuf.fastmap = re->re_fastmap;
		if (translate)
			re->re_patbuf.translate = PyString_AsString(translate);
		else
			re->re_patbuf.translate = NULL;
		Py_XINCREF(translate);
		re->re_translate = translate;
		re->re_lastok = NULL;
		re->re_groupindex = groupindex;
		Py_INCREF(pattern);
		re->re_realpat = pattern;
		Py_INCREF(givenpat);
		re->re_givenpat = givenpat;
		error = re_compile_pattern(pat, size, &re->re_patbuf);
		if (error != NULL) {
			PyErr_SetString(RegexError, error);
			Py_DECREF(re);
			re = NULL;
		}
	}
	return (PyObject *)re;
}

static PyObject *
regex_compile(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *pat = NULL;
	PyObject *tran = NULL;
	if (!PyArg_Parse(args, "S", &pat)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(SS)", &pat, &tran))
			return NULL;
	}
	return newregexobject(pat, tran, pat, NULL);
}

static PyObject *
symcomp(pattern, gdict)
	PyObject *pattern;
	PyObject *gdict;
{
	char *opat = PyString_AsString(pattern);
	char *oend = opat + PyString_Size(pattern);
	int group_count = 0;
	int escaped = 0;
	char *o = opat;
	char *n;
	char name_buf[128];
	char *g;
	PyObject *npattern;
	int require_escape = re_syntax & RE_NO_BK_PARENS ? 0 : 1;

	if (oend == opat) {
		Py_INCREF(pattern);
		return pattern;
	}

	npattern = PyString_FromStringAndSize((char*)NULL, PyString_Size(pattern));
	if (npattern == NULL)
		return NULL;
	n = PyString_AsString(npattern);

	while (o < oend) {
		if (*o == '(' && escaped == require_escape) {
			char *backtrack;
			escaped = 0;
			++group_count;
			*n++ = *o;
			if (++o >= oend || *o != '<')
				continue;
			/* *o == '<' */
			if (o+1 < oend && *(o+1) == '>')
				continue;
			backtrack = o;
			g = name_buf;
			for (++o; o < oend;) {
				if (*o == '>') {
					PyObject *group_name = NULL;
					PyObject *group_index = NULL;
					*g++ = '\0';
					group_name = PyString_FromString(name_buf);
					group_index = PyInt_FromLong(group_count);
					if (group_name == NULL || group_index == NULL
					    || PyDict_SetItem(gdict, group_name, group_index) != 0) {
						Py_XDECREF(group_name);
						Py_XDECREF(group_index);
						Py_XDECREF(npattern);
						return NULL;
					}
					++o; /* eat the '>' */
					break;
				}
				if (!isalnum(Py_CHARMASK(*o)) && *o != '_') {
					o = backtrack;
					break;
				}
				*g++ = *o++;
			}
		}
		if (*o == '[' && !escaped) {
			*n++ = *o;
			++o;	/* eat the char following '[' */
			*n++ = *o;
			while (o < oend && *o != ']') {
				++o;
				*n++ = *o;
			}
			if (o < oend)
				++o;
		}
		else if (*o == '\\') {
			escaped = 1;
			*n++ = *o;
			++o;
		}
		else {
			escaped = 0;
			*n++ = *o;
			++o;
		}
	}

	if (_PyString_Resize(&npattern, n - PyString_AsString(npattern)) == 0)
		return npattern;
	else {
		return NULL;
	}

}

static PyObject *
regex_symcomp(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *pattern;
	PyObject *tran = NULL;
	PyObject *gdict = NULL;
	PyObject *npattern;
	if (!PyArg_Parse(args, "S", &pattern)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(SS)", &pattern, &tran))
			return NULL;
	}
	gdict = PyDict_New();
	if (gdict == NULL
	    || (npattern = symcomp(pattern, gdict)) == NULL) {
		Py_DECREF(gdict);
		Py_DECREF(pattern);
		return NULL;
	}
	return newregexobject(npattern, tran, pattern, gdict);
}


static PyObject *cache_pat;
static PyObject *cache_prog;

static int
update_cache(pat)
	PyObject *pat;
{
	if (pat != cache_pat) {
		Py_XDECREF(cache_pat);
		cache_pat = NULL;
		Py_XDECREF(cache_prog);
		cache_prog = regex_compile((PyObject *)NULL, pat);
		if (cache_prog == NULL)
			return -1;
		cache_pat = pat;
		Py_INCREF(cache_pat);
	}
	return 0;
}

static PyObject *
regex_match(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *pat, *string;
	if (!PyArg_Parse(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;
	return reg_match((regexobject *)cache_prog, string);
}

static PyObject *
regex_search(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *pat, *string;
	if (!PyArg_Parse(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;
	return reg_search((regexobject *)cache_prog, string);
}

static PyObject *
regex_set_syntax(self, args)
	PyObject *self, *args;
{
	int syntax;
	if (!PyArg_Parse(args, "i", &syntax))
		return NULL;
	syntax = re_set_syntax(syntax);
	return PyInt_FromLong((long)syntax);
}

static struct PyMethodDef regex_global_methods[] = {
	{"compile",	regex_compile, 0},
	{"symcomp",	regex_symcomp, 0},
	{"match",	regex_match, 0},
	{"search",	regex_search, 0},
	{"set_syntax",	regex_set_syntax, 0},
	{NULL,		NULL}		/* sentinel */
};

void
initregex()
{
	PyObject *m, *d, *v;
	
	m = Py_InitModule("regex", regex_global_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize regex.error exception */
	RegexError = PyString_FromString("regex.error");
	if (RegexError == NULL || PyDict_SetItemString(d, "error", RegexError) != 0)
		Py_FatalError("can't define regex.error");

	/* Initialize regex.casefold constant */
	v = PyString_FromStringAndSize((char *)NULL, 256);
	if (v != NULL) {
		int i;
		char *s = PyString_AsString(v);
		for (i = 0; i < 256; i++) {
			if (isupper(i))
				s[i] = tolower(i);
			else
				s[i] = i;
		}
		PyDict_SetItemString(d, "casefold", v);
		Py_DECREF(v);
	}
}
