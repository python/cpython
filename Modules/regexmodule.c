/*
XXX support range parameter on search
XXX support mstop parameter on search
*/


/* Regular expression objects */
/* This uses Tatu Ylonen's copyleft-free reimplementation of
   GNU regular expressions */

#include "Python.h"

#include <ctype.h>

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
reg_dealloc(regexobject *re)
{
	if (re->re_patbuf.buffer)
		free(re->re_patbuf.buffer);
	Py_XDECREF(re->re_translate);
	Py_XDECREF(re->re_lastok);
	Py_XDECREF(re->re_groupindex);
	Py_XDECREF(re->re_givenpat);
	Py_XDECREF(re->re_realpat);
	PyObject_Del(re);
}

static PyObject *
makeresult(struct re_registers *regs)
{
	PyObject *v;
	int i;
	static PyObject *filler = NULL;

	if (filler == NULL) {
		filler = Py_BuildValue("(ii)", -1, -1);
		if (filler == NULL)
			return NULL;
	}
	v = PyTuple_New(RE_NREGS);
	if (v == NULL)
		return NULL;

	for (i = 0; i < RE_NREGS; i++) {
		int lo = regs->start[i];
		int hi = regs->end[i];
		PyObject *w;
		if (lo == -1 && hi == -1) {
			w = filler;
			Py_INCREF(w);
		}
		else
			w = Py_BuildValue("(ii)", lo, hi);
		if (w == NULL || PyTuple_SetItem(v, i, w) < 0) {
			Py_DECREF(v);
			return NULL;
		}
	}
	return v;
}

static PyObject *
regobj_match(regexobject *re, PyObject *args)
{
	PyObject *argstring;
	char *buffer;
	int size;
	int offset = 0;
	int result;

	if (!PyArg_ParseTuple(args, "O|i:match", &argstring, &offset))
		return NULL;
	if (!PyArg_Parse(argstring, "t#", &buffer, &size))
		return NULL;

	if (offset < 0 || offset > size) {
		PyErr_SetString(RegexError, "match offset out of range");
		return NULL;
	}
	Py_XDECREF(re->re_lastok);
	re->re_lastok = NULL;
	result = _Py_re_match(&re->re_patbuf, (unsigned char *)buffer, size, offset,
			      &re->re_regs);
	if (result < -1) {
		/* Serious failure of some sort; if re_match didn't 
		   set an exception, raise a generic error */
	        if (!PyErr_Occurred())
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
regobj_search(regexobject *re, PyObject *args)
{
	PyObject *argstring;
	char *buffer;
	int size;
	int offset = 0;
	int range;
	int result;
	
	if (!PyArg_ParseTuple(args, "O|i:search", &argstring, &offset))
		return NULL;
	if (!PyArg_Parse(argstring, "t#:search", &buffer, &size))
		return NULL;

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
	result = _Py_re_search(&re->re_patbuf, (unsigned char *)buffer, size, offset, range,
			   &re->re_regs);
	if (result < -1) {
		/* Serious failure of some sort; if re_match didn't 
		   set an exception, raise a generic error */
	        if (!PyErr_Occurred())
	  	        PyErr_SetString(RegexError, "match failure");
		return NULL;
	}
	if (result >= 0) {
		Py_INCREF(argstring);
		re->re_lastok = argstring;
	}
	return PyInt_FromLong((long)result); /* Position of the match or -1 */
}

/* get the group from the regex where index can be a string (group name) or
   an integer index [0 .. 99]
 */
static PyObject*
group_from_index(regexobject *re, PyObject *index)
{
	int i, a, b;
	char *v;

	if (PyString_Check(index))
		if (re->re_groupindex == NULL ||
		    !(index = PyDict_GetItem(re->re_groupindex, index)))
		{
			PyErr_SetString(RegexError,
					"group() group name doesn't exist");
			return NULL;
		}

	i = PyInt_AsLong(index);
	if (i == -1 && PyErr_Occurred())
		return NULL;

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
	
	if (!(v = PyString_AsString(re->re_lastok)))
		return NULL;

	return PyString_FromStringAndSize(v+a, b-a);
}


static PyObject *
regobj_group(regexobject *re, PyObject *args)
{
	int n = PyTuple_Size(args);
	int i;
	PyObject *res = NULL;

	if (n < 0)
		return NULL;
	if (n == 0) {
		PyErr_SetString(PyExc_TypeError, "not enough arguments");
		return NULL;
	}
	if (n == 1) {
		/* return value is a single string */
		PyObject *index = PyTuple_GetItem(args, 0);
		if (!index)
			return NULL;
		
		return group_from_index(re, index);
	}

	/* return value is a tuple */
	if (!(res = PyTuple_New(n)))
		return NULL;

	for (i = 0; i < n; i++) {
		PyObject *index = PyTuple_GetItem(args, i);
		PyObject *group = NULL;

		if (!index)
			goto finally;
		if (!(group = group_from_index(re, index)))
			goto finally;
		if (PyTuple_SetItem(res, i, group) < 0)
			goto finally;
	}
	return res;

  finally:
	Py_DECREF(res);
	return NULL;
}


static struct PyMethodDef reg_methods[] = {
	{"match",	(PyCFunction)regobj_match, 1},
	{"search",	(PyCFunction)regobj_search, 1},
	{"group",	(PyCFunction)regobj_group, 1},
	{NULL,		NULL}		/* sentinel */
};



static char* members[] = {
	"last", "regs", "translate",
	"groupindex", "realpat", "givenpat",
	NULL
};


static PyObject *
regobj_getattr(regexobject *re, char *name)
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
		int i = 0;
		PyObject *list = NULL;

		/* okay, so it's unlikely this list will change that often.
		   still, it's easier to change it in just one place.
		 */
		while (members[i])
			i++;
		if (!(list = PyList_New(i)))
			return NULL;

		i = 0;
		while (members[i]) {
			PyObject* v = PyString_FromString(members[i]);
			if (!v || PyList_SetItem(list, i, v) < 0) {
				Py_DECREF(list);
				return NULL;
			}
			i++;
		}
		return list;
	}
	return Py_FindMethod(reg_methods, (PyObject *)re, name);
}

static PyTypeObject Regextype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				     /*ob_size*/
	"regex",			     /*tp_name*/
	sizeof(regexobject),		     /*tp_size*/
	0,				     /*tp_itemsize*/
	/* methods */
	(destructor)reg_dealloc,	     /*tp_dealloc*/
	0,				     /*tp_print*/
	(getattrfunc)regobj_getattr,	     /*tp_getattr*/
	0,				     /*tp_setattr*/
	0,				     /*tp_compare*/
	0,				     /*tp_repr*/
};

/* reference counting invariants:
   pattern: borrowed
   translate: borrowed
   givenpat: borrowed
   groupindex: transferred
*/
static PyObject *
newregexobject(PyObject *pattern, PyObject *translate, PyObject *givenpat, PyObject *groupindex)
{
	regexobject *re;
	char *pat;
	int size;

	if (!PyArg_Parse(pattern, "t#", &pat, &size))
		return NULL;
	
	if (translate != NULL && PyString_Size(translate) != 256) {
		PyErr_SetString(RegexError,
				"translation table must be 256 bytes");
		return NULL;
	}
	re = PyObject_New(regexobject, &Regextype);
	if (re != NULL) {
		char *error;
		re->re_patbuf.buffer = NULL;
		re->re_patbuf.allocated = 0;
		re->re_patbuf.fastmap = (unsigned char *)re->re_fastmap;
		if (translate) {
			re->re_patbuf.translate = (unsigned char *)PyString_AsString(translate);
			if (!re->re_patbuf.translate)
				goto finally;
			Py_INCREF(translate);
		}
		else
			re->re_patbuf.translate = NULL;
		re->re_translate = translate;
		re->re_lastok = NULL;
		re->re_groupindex = groupindex;
		Py_INCREF(pattern);
		re->re_realpat = pattern;
		Py_INCREF(givenpat);
		re->re_givenpat = givenpat;
		error = _Py_re_compile_pattern((unsigned char *)pat, size, &re->re_patbuf);
		if (error != NULL) {
			PyErr_SetString(RegexError, error);
			goto finally;
		}
	}
	return (PyObject *)re;
  finally:
	Py_DECREF(re);
	return NULL;
}

static PyObject *
regex_compile(PyObject *self, PyObject *args)
{
	PyObject *pat = NULL;
	PyObject *tran = NULL;

	if (!PyArg_ParseTuple(args, "S|S:compile", &pat, &tran))
		return NULL;
	return newregexobject(pat, tran, pat, NULL);
}

static PyObject *
symcomp(PyObject *pattern, PyObject *gdict)
{
	char *opat, *oend, *o, *n, *g, *v;
	int group_count = 0;
	int sz;
	int escaped = 0;
	char name_buf[128];
	PyObject *npattern;
	int require_escape = re_syntax & RE_NO_BK_PARENS ? 0 : 1;

	if (!(opat = PyString_AsString(pattern)))
		return NULL;

	if ((sz = PyString_Size(pattern)) < 0)
		return NULL;

	oend = opat + sz;
	o = opat;

	if (oend == opat) {
		Py_INCREF(pattern);
		return pattern;
	}

	if (!(npattern = PyString_FromStringAndSize((char*)NULL, sz)) ||
	    !(n = PyString_AsString(npattern)))
		return NULL;

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
				    if (group_name == NULL ||
					group_index == NULL ||
					PyDict_SetItem(gdict, group_name,
						       group_index) != 0)
				    {
					    Py_XDECREF(group_name);
					    Py_XDECREF(group_index);
					    Py_XDECREF(npattern);
					    return NULL;
				    }
				    Py_DECREF(group_name);
				    Py_DECREF(group_index);
				    ++o;     /* eat the '>' */
				    break;
				}
				if (!isalnum(Py_CHARMASK(*o)) && *o != '_') {
					o = backtrack;
					break;
				}
				*g++ = *o++;
			}
		}
		else if (*o == '[' && !escaped) {
			*n++ = *o;
			++o;		     /* eat the char following '[' */
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

	if (!(v = PyString_AsString(npattern))) {
		Py_DECREF(npattern);
		return NULL;
	}
	/* _PyString_Resize() decrements npattern on failure */
	if (_PyString_Resize(&npattern, n - v) == 0)
		return npattern;
	else {
		return NULL;
	}

}

static PyObject *
regex_symcomp(PyObject *self, PyObject *args)
{
	PyObject *pattern;
	PyObject *tran = NULL;
	PyObject *gdict = NULL;
	PyObject *npattern;
	PyObject *retval = NULL;

	if (!PyArg_ParseTuple(args, "S|S:symcomp", &pattern, &tran))
		return NULL;

	gdict = PyDict_New();
	if (gdict == NULL || (npattern = symcomp(pattern, gdict)) == NULL) {
		Py_DECREF(gdict);
		Py_DECREF(pattern);
		return NULL;
	}
	retval = newregexobject(npattern, tran, pattern, gdict);
	Py_DECREF(npattern);
	return retval;
}


static PyObject *cache_pat;
static PyObject *cache_prog;

static int
update_cache(PyObject *pat)
{
	PyObject *tuple = Py_BuildValue("(O)", pat);
	int status = 0;

	if (!tuple)
		return -1;

	if (pat != cache_pat) {
		Py_XDECREF(cache_pat);
		cache_pat = NULL;
		Py_XDECREF(cache_prog);
		cache_prog = regex_compile((PyObject *)NULL, tuple);
		if (cache_prog == NULL) {
			status = -1;
			goto finally;
		}
		cache_pat = pat;
		Py_INCREF(cache_pat);
	}
  finally:
	Py_DECREF(tuple);
	return status;
}

static PyObject *
regex_match(PyObject *self, PyObject *args)
{
	PyObject *pat, *string;
	PyObject *tuple, *v;

	if (!PyArg_Parse(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;

	if (!(tuple = Py_BuildValue("(S)", string)))
		return NULL;
	v = regobj_match((regexobject *)cache_prog, tuple);
	Py_DECREF(tuple);
	return v;
}

static PyObject *
regex_search(PyObject *self, PyObject *args)
{
	PyObject *pat, *string;
	PyObject *tuple, *v;

	if (!PyArg_Parse(args, "(SS)", &pat, &string))
		return NULL;
	if (update_cache(pat) < 0)
		return NULL;

	if (!(tuple = Py_BuildValue("(S)", string)))
		return NULL;
	v = regobj_search((regexobject *)cache_prog, tuple);
	Py_DECREF(tuple);
	return v;
}

static PyObject *
regex_set_syntax(PyObject *self, PyObject *args)
{
	int syntax;
	if (!PyArg_Parse(args, "i", &syntax))
		return NULL;
	syntax = re_set_syntax(syntax);
	/* wipe the global pattern cache */
	Py_XDECREF(cache_pat);
	cache_pat = NULL;
	Py_XDECREF(cache_prog);
	cache_prog = NULL;
	return PyInt_FromLong((long)syntax);
}

static PyObject *
regex_get_syntax(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;
	return PyInt_FromLong((long)re_syntax);
}


static struct PyMethodDef regex_global_methods[] = {
	{"compile",	regex_compile, 1},
	{"symcomp",	regex_symcomp, 1},
	{"match",	regex_match, 0},
	{"search",	regex_search, 0},
	{"set_syntax",	regex_set_syntax, 0},
	{"get_syntax",  regex_get_syntax, 0},
	{NULL,		NULL}		     /* sentinel */
};

DL_EXPORT(void)
initregex(void)
{
	PyObject *m, *d, *v;
	int i;
	char *s;
	
	m = Py_InitModule("regex", regex_global_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize regex.error exception */
	v = RegexError = PyErr_NewException("regex.error", NULL, NULL);
	if (v == NULL || PyDict_SetItemString(d, "error", v) != 0)
		goto finally;
	
	/* Initialize regex.casefold constant */
	if (!(v = PyString_FromStringAndSize((char *)NULL, 256)))
		goto finally;
	
	if (!(s = PyString_AsString(v)))
		goto finally;

	for (i = 0; i < 256; i++) {
		if (isupper(i))
			s[i] = tolower(i);
		else
			s[i] = i;
	}
	if (PyDict_SetItemString(d, "casefold", v) < 0)
		goto finally;
	Py_DECREF(v);

	if (!PyErr_Occurred())
		return;
  finally:
	/* Nothing */ ;
}
