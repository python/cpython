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

/* Compile an expression node to intermediate code */

/* XXX TO DO:
   XXX add __doc__ attribute == co_doc to code object attributes?
   XXX   (it's currently the first item of the co_const tuple)
   XXX Generate simple jump for break/return outside 'try...finally'
   XXX Allow 'continue' inside try-finally
   XXX New 1-byte opcode for loading None
   XXX New opcode for loading the initial index for a for loop
   XXX other JAR tricks?
*/

#ifndef NO_PRIVATE_NAME_MANGLING
#define PRIVATE_NAME_MANGLING
#endif

#include "Python.h"

#include "node.h"
#include "token.h"
#include "graminit.h"
#include "compile.h"
#include "opcode.h"
#include "structmember.h"

#include <ctype.h>

/* Three symbols from graminit.h are also defined in Python.h, with
   Py_ prefixes to their names.  Python.h can't include graminit.h
   (which defines too many confusing symbols), but we can check here
   that they haven't changed (which is very unlikely, but possible). */
#if Py_single_input != single_input
  #error "single_input has changed -- update Py_single_input in Python.h"
#endif
#if Py_file_input != file_input
  #error "file_input has changed -- update Py_file_input in Python.h"
#endif
#if Py_eval_input != eval_input
  #error "eval_input has changed -- update Py_eval_input in Python.h"
#endif

int Py_OptimizeFlag = 0;

#define OP_DELETE 0
#define OP_ASSIGN 1
#define OP_APPLY 2

#define OFF(x) offsetof(PyCodeObject, x)

static struct memberlist code_memberlist[] = {
	{"co_argcount",	T_INT,		OFF(co_argcount),	READONLY},
	{"co_nlocals",	T_INT,		OFF(co_nlocals),	READONLY},
	{"co_stacksize",T_INT,		OFF(co_stacksize),	READONLY},
	{"co_flags",	T_INT,		OFF(co_flags),		READONLY},
	{"co_code",	T_OBJECT,	OFF(co_code),		READONLY},
	{"co_consts",	T_OBJECT,	OFF(co_consts),		READONLY},
	{"co_names",	T_OBJECT,	OFF(co_names),		READONLY},
	{"co_varnames",	T_OBJECT,	OFF(co_varnames),	READONLY},
	{"co_filename",	T_OBJECT,	OFF(co_filename),	READONLY},
	{"co_name",	T_OBJECT,	OFF(co_name),		READONLY},
	{"co_firstlineno", T_INT,	OFF(co_firstlineno),	READONLY},
	{"co_lnotab",	T_OBJECT,	OFF(co_lnotab),		READONLY},
	{NULL}	/* Sentinel */
};

static PyObject *
code_getattr(co, name)
	PyCodeObject *co;
	char *name;
{
	return PyMember_Get((char *)co, code_memberlist, name);
}

static void
code_dealloc(co)
	PyCodeObject *co;
{
	Py_XDECREF(co->co_code);
	Py_XDECREF(co->co_consts);
	Py_XDECREF(co->co_names);
	Py_XDECREF(co->co_varnames);
	Py_XDECREF(co->co_filename);
	Py_XDECREF(co->co_name);
	Py_XDECREF(co->co_lnotab);
	PyMem_DEL(co);
}

static PyObject *
code_repr(co)
	PyCodeObject *co;
{
	char buf[500];
	int lineno = -1;
	char *p = PyString_AS_STRING(co->co_code);
	char *filename = "???";
	char *name = "???";
	if (*p == SET_LINENO)
		lineno = (p[1] & 0xff) | ((p[2] & 0xff) << 8);
	if (co->co_filename && PyString_Check(co->co_filename))
		filename = PyString_AsString(co->co_filename);
	if (co->co_name && PyString_Check(co->co_name))
		name = PyString_AsString(co->co_name);
	sprintf(buf, "<code object %.100s at %lx, file \"%.300s\", line %d>",
		name, (long)co, filename, lineno);
	return PyString_FromString(buf);
}

static int
code_compare(co, cp)
	PyCodeObject *co, *cp;
{
	int cmp;
	cmp = cp->co_argcount - cp->co_argcount;
	if (cmp) return cmp;
	cmp = cp->co_nlocals - cp->co_nlocals;
	if (cmp) return cmp;
	cmp = cp->co_flags - cp->co_flags;
	if (cmp) return cmp;
	cmp = PyObject_Compare((PyObject *)co->co_code,
			       (PyObject *)cp->co_code);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_consts, cp->co_consts);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_names, cp->co_names);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_varnames, cp->co_varnames);
	return cmp;
}

static long
code_hash(co)
	PyCodeObject *co;
{
	long h, h1, h2, h3, h4;
	h1 = PyObject_Hash((PyObject *)co->co_code);
	if (h1 == -1) return -1;
	h2 = PyObject_Hash(co->co_consts);
	if (h2 == -1) return -1;
	h3 = PyObject_Hash(co->co_names);
	if (h3 == -1) return -1;
	h4 = PyObject_Hash(co->co_varnames);
	if (h4 == -1) return -1;
	h = h1 ^ h2 ^ h3 ^ h4 ^
		co->co_argcount ^ co->co_nlocals ^ co->co_flags;
	if (h == -1) h = -2;
	return h;
}

PyTypeObject PyCode_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"code",
	sizeof(PyCodeObject),
	0,
	(destructor)code_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)code_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)code_compare, /*tp_compare*/
	(reprfunc)code_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)code_hash, /*tp_hash*/
};

#define NAME_CHARS \
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"

PyCodeObject *
PyCode_New(argcount, nlocals, stacksize, flags,
	      code, consts, names, varnames, filename, name,
	      firstlineno, lnotab)
	int argcount;
	int nlocals;
	int stacksize;
	int flags;
	PyObject *code;
	PyObject *consts;
	PyObject *names;
	PyObject *varnames;
	PyObject *filename;
	PyObject *name;
	int firstlineno;
	PyObject *lnotab;
{
	PyCodeObject *co;
	int i;
	/* Check argument types */
	if (argcount < 0 || nlocals < 0 ||
	    code == NULL || !PyString_Check(code) ||
	    consts == NULL || !PyTuple_Check(consts) ||
	    names == NULL || !PyTuple_Check(names) ||
	    varnames == NULL || !PyTuple_Check(varnames) ||
	    name == NULL || !PyString_Check(name) ||
	    filename == NULL || !PyString_Check(filename) ||
		lnotab == NULL || !PyString_Check(lnotab)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	/* Make sure names and varnames are all strings, & intern them */
	for (i = PyTuple_Size(names); --i >= 0; ) {
		PyObject *v = PyTuple_GetItem(names, i);
		if (v == NULL || !PyString_Check(v)) {
			PyErr_BadInternalCall();
			return NULL;
		}
		PyString_InternInPlace(&PyTuple_GET_ITEM(names, i));
	}
	for (i = PyTuple_Size(varnames); --i >= 0; ) {
		PyObject *v = PyTuple_GetItem(varnames, i);
		if (v == NULL || !PyString_Check(v)) {
			PyErr_BadInternalCall();
			return NULL;
		}
		PyString_InternInPlace(&PyTuple_GET_ITEM(varnames, i));
	}
	/* Intern selected string constants */
	for (i = PyTuple_Size(consts); --i >= 0; ) {
		PyObject *v = PyTuple_GetItem(consts, i);
		char *p;
		if (!PyString_Check(v))
			continue;
		p = PyString_AsString(v);
		if ((int)strspn(p, NAME_CHARS)
		    != PyString_Size(v))
			continue;
		PyString_InternInPlace(&PyTuple_GET_ITEM(consts, i));
	}
	co = PyObject_NEW(PyCodeObject, &PyCode_Type);
	if (co != NULL) {
		co->co_argcount = argcount;
		co->co_nlocals = nlocals;
		co->co_stacksize = stacksize;
		co->co_flags = flags;
		Py_INCREF(code);
		co->co_code = (PyStringObject *)code;
		Py_INCREF(consts);
		co->co_consts = consts;
		Py_INCREF(names);
		co->co_names = names;
		Py_INCREF(varnames);
		co->co_varnames = varnames;
		Py_INCREF(filename);
		co->co_filename = filename;
		Py_INCREF(name);
		co->co_name = name;
		co->co_firstlineno = firstlineno;
		Py_INCREF(lnotab);
		co->co_lnotab = lnotab;
	}
	return co;
}


/* Data structure used internally */

struct compiling {
	PyObject *c_code;		/* string */
	PyObject *c_consts;	/* list of objects */
	PyObject *c_names;	/* list of strings (names) */
	PyObject *c_globals;	/* dictionary (value=None) */
	PyObject *c_locals;	/* dictionary (value=localID) */
	PyObject *c_varnames;	/* list (inverse of c_locals) */
	int c_nlocals;		/* index of next local */
	int c_argcount;		/* number of top-level arguments */
	int c_flags;		/* same as co_flags */
	int c_nexti;		/* index into c_code */
	int c_errors;		/* counts errors occurred */
	int c_infunction;	/* set when compiling a function */
	int c_interactive;	/* generating code for interactive command */
	int c_loops;		/* counts nested loops */
	int c_begin;		/* begin of current loop, for 'continue' */
	int c_block[CO_MAXBLOCKS]; /* stack of block types */
	int c_nblocks;		/* current block stack level */
	char *c_filename;	/* filename of current node */
	char *c_name;		/* name of object (e.g. function) */
	int c_lineno;		/* Current line number */
	int c_stacklevel;	/* Current stack level */
	int c_maxstacklevel;	/* Maximum stack level */
	int c_firstlineno;
	PyObject *c_lnotab;	/* Table mapping address to line number */
	int c_last_addr, c_last_line, c_lnotab_next;
#ifdef PRIVATE_NAME_MANGLING
	char *c_private;	/* for private name mangling */
#endif
};


/* Error message including line number */

static void
com_error(c, exc, msg)
	struct compiling *c;
	PyObject *exc;
	char *msg;
{
	int n = strlen(msg);
	PyObject *v;
	char buffer[30];
	char *s;
	c->c_errors++;
	if (c->c_lineno <= 1) {
		/* Unknown line number or single interactive command */
		PyErr_SetString(exc, msg);
		return;
	}
	sprintf(buffer, " (line %d)", c->c_lineno);
	v = PyString_FromStringAndSize((char *)NULL, n + strlen(buffer));
	if (v == NULL)
		return; /* MemoryError, too bad */
	s = PyString_AS_STRING((PyStringObject *)v);
	strcpy(s, msg);
	strcat(s, buffer);
	PyErr_SetObject(exc, v);
	Py_DECREF(v);
}


/* Interface to the block stack */

static void
block_push(c, type)
	struct compiling *c;
	int type;
{
	if (c->c_nblocks >= CO_MAXBLOCKS) {
		com_error(c, PyExc_SystemError,
			  "too many statically nested blocks");
	}
	else {
		c->c_block[c->c_nblocks++] = type;
	}
}

static void
block_pop(c, type)
	struct compiling *c;
	int type;
{
	if (c->c_nblocks > 0)
		c->c_nblocks--;
	if (c->c_block[c->c_nblocks] != type && c->c_errors == 0) {
		com_error(c, PyExc_SystemError, "bad block pop");
	}
}


/* Prototype forward declarations */

static int com_init Py_PROTO((struct compiling *, char *));
static void com_free Py_PROTO((struct compiling *));
static void com_push Py_PROTO((struct compiling *, int));
static void com_pop Py_PROTO((struct compiling *, int));
static void com_done Py_PROTO((struct compiling *));
static void com_node Py_PROTO((struct compiling *, struct _node *));
static void com_factor Py_PROTO((struct compiling *, struct _node *));
static void com_addbyte Py_PROTO((struct compiling *, int));
static void com_addint Py_PROTO((struct compiling *, int));
static void com_addoparg Py_PROTO((struct compiling *, int, int));
static void com_addfwref Py_PROTO((struct compiling *, int, int *));
static void com_backpatch Py_PROTO((struct compiling *, int));
static int com_add Py_PROTO((struct compiling *, PyObject *, PyObject *));
static int com_addconst Py_PROTO((struct compiling *, PyObject *));
static int com_addname Py_PROTO((struct compiling *, PyObject *));
static void com_addopname Py_PROTO((struct compiling *, int, node *));
static void com_list Py_PROTO((struct compiling *, node *, int));
static int com_argdefs Py_PROTO((struct compiling *, node *));
static int com_newlocal Py_PROTO((struct compiling *, char *));
static PyCodeObject *icompile Py_PROTO((struct _node *, struct compiling *));
static PyCodeObject *jcompile Py_PROTO((struct _node *, char *,
					struct compiling *));
static PyObject *parsestrplus Py_PROTO((node *));
static PyObject *parsestr Py_PROTO((char *));

static int
com_init(c, filename)
	struct compiling *c;
	char *filename;
{
	if ((c->c_code = PyString_FromStringAndSize((char *)NULL,
						    1000)) == NULL)
		goto fail_3;
	if ((c->c_consts = PyList_New(0)) == NULL)
		goto fail_2;
	if ((c->c_names = PyList_New(0)) == NULL)
		goto fail_1;
	if ((c->c_globals = PyDict_New()) == NULL)
		goto fail_0;
	if ((c->c_locals = PyDict_New()) == NULL)
		goto fail_00;
	if ((c->c_varnames = PyList_New(0)) == NULL)
		goto fail_000;
	if ((c->c_lnotab = PyString_FromStringAndSize((char *)NULL,
						      1000)) == NULL)
		goto fail_0000;
	c->c_nlocals = 0;
	c->c_argcount = 0;
	c->c_flags = 0;
	c->c_nexti = 0;
	c->c_errors = 0;
	c->c_infunction = 0;
	c->c_interactive = 0;
	c->c_loops = 0;
	c->c_begin = 0;
	c->c_nblocks = 0;
	c->c_filename = filename;
	c->c_name = "?";
	c->c_lineno = 0;
	c->c_stacklevel = 0;
	c->c_maxstacklevel = 0;
	c->c_firstlineno = 0;
	c->c_last_addr = 0;
	c->c_last_line = 0;
	c-> c_lnotab_next = 0;
	return 1;
	
  fail_0000:
  	Py_DECREF(c->c_lnotab);
  fail_000:
  	Py_DECREF(c->c_locals);
  fail_00:
  	Py_DECREF(c->c_globals);
  fail_0:
  	Py_DECREF(c->c_names);
  fail_1:
	Py_DECREF(c->c_consts);
  fail_2:
	Py_DECREF(c->c_code);
  fail_3:
 	return 0;
}

static void
com_free(c)
	struct compiling *c;
{
	Py_XDECREF(c->c_code);
	Py_XDECREF(c->c_consts);
	Py_XDECREF(c->c_names);
	Py_XDECREF(c->c_globals);
	Py_XDECREF(c->c_locals);
	Py_XDECREF(c->c_varnames);
	Py_XDECREF(c->c_lnotab);
}

static void
com_push(c, n)
	struct compiling *c;
	int n;
{
	c->c_stacklevel += n;
	if (c->c_stacklevel > c->c_maxstacklevel)
		c->c_maxstacklevel = c->c_stacklevel;
}

static void
com_pop(c, n)
	struct compiling *c;
	int n;
{
	if (c->c_stacklevel < n) {
		fprintf(stderr,
			"%s:%d: underflow! nexti=%d, level=%d, n=%d\n",
			c->c_filename, c->c_lineno,
			c->c_nexti, c->c_stacklevel, n);
		c->c_stacklevel = 0;
	}
	else
		c->c_stacklevel -= n;
}

static void
com_done(c)
	struct compiling *c;
{
	if (c->c_code != NULL)
		_PyString_Resize(&c->c_code, c->c_nexti);
	if (c->c_lnotab != NULL)
		_PyString_Resize(&c->c_lnotab, c->c_lnotab_next);
}

static void
com_addbyte(c, byte)
	struct compiling *c;
	int byte;
{
	int len;
	/*fprintf(stderr, "%3d: %3d\n", c->c_nexti, byte);*/
	if (byte < 0 || byte > 255) {
		/*
		fprintf(stderr, "XXX compiling bad byte: %d\n", byte);
		fatal("com_addbyte: byte out of range");
		*/
		com_error(c, PyExc_SystemError,
			  "com_addbyte: byte out of range");
	}
	if (c->c_code == NULL)
		return;
	len = PyString_Size(c->c_code);
	if (c->c_nexti >= len) {
		if (_PyString_Resize(&c->c_code, len+1000) != 0) {
			c->c_errors++;
			return;
		}
	}
	PyString_AsString(c->c_code)[c->c_nexti++] = byte;
}

static void
com_addint(c, x)
	struct compiling *c;
	int x;
{
	com_addbyte(c, x & 0xff);
	com_addbyte(c, x >> 8); /* XXX x should be positive */
}

static void
com_add_lnotab(c, addr, line)
	struct compiling *c;
	int addr;
	int line;
{
	int size;
	char *p;
	if (c->c_lnotab == NULL)
		return;
	size = PyString_Size(c->c_lnotab);
	if (c->c_lnotab_next+2 > size) {
		if (_PyString_Resize(&c->c_lnotab, size + 1000) < 0) {
			c->c_errors++;
			return;
		}
	}
	p = PyString_AsString(c->c_lnotab) + c->c_lnotab_next;
	*p++ = addr;
	*p++ = line;
	c->c_lnotab_next += 2;
}

static void
com_set_lineno(c, lineno)
	struct compiling *c;
	int lineno;
{
	c->c_lineno = lineno;
	if (c->c_firstlineno == 0) {
		c->c_firstlineno = c->c_last_line = lineno;
	}
	else {
		int incr_addr = c->c_nexti - c->c_last_addr;
		int incr_line = lineno - c->c_last_line;
		while (incr_addr > 0 || incr_line > 0) {
			int trunc_addr = incr_addr;
			int trunc_line = incr_line;
			if (trunc_addr > 255)
				trunc_addr = 255;
			if (trunc_line > 255)
				trunc_line = 255;
			com_add_lnotab(c, trunc_addr, trunc_line);
			incr_addr -= trunc_addr;
			incr_line -= trunc_line;
		}
		c->c_last_addr = c->c_nexti;
		c->c_last_line = lineno;
	}
}

static void
com_addoparg(c, op, arg)
	struct compiling *c;
	int op;
	int arg;
{
	if (op == SET_LINENO) {
		com_set_lineno(c, arg);
		if (Py_OptimizeFlag)
			return;
	}
	com_addbyte(c, op);
	com_addint(c, arg);
}

static void
com_addfwref(c, op, p_anchor)
	struct compiling *c;
	int op;
	int *p_anchor;
{
	/* Compile a forward reference for backpatching */
	int here;
	int anchor;
	com_addbyte(c, op);
	here = c->c_nexti;
	anchor = *p_anchor;
	*p_anchor = here;
	com_addint(c, anchor == 0 ? 0 : here - anchor);
}

static void
com_backpatch(c, anchor)
	struct compiling *c;
	int anchor; /* Must be nonzero */
{
	unsigned char *code = (unsigned char *) PyString_AsString(c->c_code);
	int target = c->c_nexti;
	int dist;
	int prev;
	for (;;) {
		/* Make the JUMP instruction at anchor point to target */
		prev = code[anchor] + (code[anchor+1] << 8);
		dist = target - (anchor+2);
		code[anchor] = dist & 0xff;
		code[anchor+1] = dist >> 8;
		if (!prev)
			break;
		anchor -= prev;
	}
}

/* Handle literals and names uniformly */

static int
com_add(c, list, v)
	struct compiling *c;
	PyObject *list;
	PyObject *v;
{
	int n = PyList_Size(list);
	int i;
	for (i = n; --i >= 0; ) {
		PyObject *w = PyList_GetItem(list, i);
		if (v->ob_type == w->ob_type && PyObject_Compare(v, w) == 0)
			return i;
	}
	/* Check for error from PyObject_Compare */
	if (PyErr_Occurred() || PyList_Append(list, v) != 0)
		c->c_errors++;
	return n;
}

static int
com_addconst(c, v)
	struct compiling *c;
	PyObject *v;
{
	return com_add(c, c->c_consts, v);
}

static int
com_addname(c, v)
	struct compiling *c;
	PyObject *v;
{
	return com_add(c, c->c_names, v);
}

#ifdef PRIVATE_NAME_MANGLING
static int
com_mangle(c, name, buffer, maxlen)
	struct compiling *c;
	char *name;
	char *buffer;
	int maxlen;
{
	/* Name mangling: __private becomes _classname__private.
	   This is independent from how the name is used. */
	char *p;
	int nlen, plen;
	nlen = strlen(name);
	if (nlen+2 >= maxlen)
		return 0; /* Don't mangle __extremely_long_names */
	if (name[nlen-1] == '_' && name[nlen-2] == '_')
		return 0; /* Don't mangle __whatever__ */
	p = c->c_private;
	/* Strip leading underscores from class name */
	while (*p == '_')
		p++;
	if (*p == '\0')
		return 0; /* Don't mangle if class is just underscores */
	plen = strlen(p);
	if (plen + nlen >= maxlen)
		plen = maxlen-nlen-2; /* Truncate class name if too long */
	/* buffer = "_" + p[:plen] + name # i.e. 1+plen+nlen bytes */
	buffer[0] = '_';
	strncpy(buffer+1, p, plen);
	strcpy(buffer+1+plen, name);
	/* fprintf(stderr, "mangle %s -> %s\n", name, buffer); */
	return 1;
}
#endif

static void
com_addopnamestr(c, op, name)
	struct compiling *c;
	int op;
	char *name;
{
	PyObject *v;
	int i;
#ifdef PRIVATE_NAME_MANGLING
	char buffer[256];
	if (name != NULL && name[0] == '_' && name[1] == '_' &&
	    c->c_private != NULL &&
	    com_mangle(c, name, buffer, (int)sizeof(buffer)))
		name = buffer;
#endif
	if (name == NULL || (v = PyString_InternFromString(name)) == NULL) {
		c->c_errors++;
		i = 255;
	}
	else {
		i = com_addname(c, v);
		Py_DECREF(v);
	}
	/* Hack to replace *_NAME opcodes by *_GLOBAL if necessary */
	switch (op) {
	case LOAD_NAME:
	case STORE_NAME:
	case DELETE_NAME:
		if (PyDict_GetItemString(c->c_globals, name) != NULL) {
			switch (op) {
			case LOAD_NAME:   op = LOAD_GLOBAL;   break;
			case STORE_NAME:  op = STORE_GLOBAL;  break;
			case DELETE_NAME: op = DELETE_GLOBAL; break;
			}
		}
	}
	com_addoparg(c, op, i);
}

static void
com_addopname(c, op, n)
	struct compiling *c;
	int op;
	node *n;
{
	char *name;
	char buffer[1000];
	/* XXX it is possible to write this code without the 1000
	   chars on the total length of dotted names, I just can't be
	   bothered right now */
	if (TYPE(n) == STAR)
		name = "*";
	else if (TYPE(n) == dotted_name) {
		char *p = buffer;
		int i;
		name = buffer;
		for (i = 0; i < NCH(n); i += 2) {
			char *s = STR(CHILD(n, i));
			if (p + strlen(s) > buffer + (sizeof buffer) - 2) {
				com_error(c, PyExc_MemoryError,
					  "dotted_name too long");
				name = NULL;
				break;
			}
			if (p != buffer)
				*p++ = '.';
			strcpy(p, s);
			p = strchr(p, '\0');
		}
	}
	else {
		REQ(n, NAME);
		name = STR(n);
	}
	com_addopnamestr(c, op, name);
}

static PyObject *
parsenumber(co, s)
	struct compiling *co;
	char *s;
{
	extern long PyOS_strtol Py_PROTO((const char *, char **, int));
	extern unsigned long PyOS_strtoul Py_PROTO((const char *,
						    char **, int));
	extern double atof Py_PROTO((const char *));
	char *end;
	long x;
	double dx;
#ifndef WITHOUT_COMPLEX
	Py_complex c;
	int imflag;
#endif

	errno = 0;
	end = s + strlen(s) - 1;
#ifndef WITHOUT_COMPLEX
	imflag = *end == 'j' || *end == 'J';
#endif
	if (*end == 'l' || *end == 'L')
		return PyLong_FromString(s, (char **)0, 0);
	if (s[0] == '0')
		x = (long) PyOS_strtoul(s, &end, 0);
	else
		x = PyOS_strtol(s, &end, 0);
	if (*end == '\0') {
		if (errno != 0) {
			com_error(co, PyExc_OverflowError,
				  "integer literal too large");
			return NULL;
		}
		return PyInt_FromLong(x);
	}
	/* XXX Huge floats may silently fail */
#ifndef WITHOUT_COMPLEX
	if (imflag) {
		c.real = 0.;
		PyFPE_START_PROTECT("atof", return 0)
		c.imag = atof(s);
		PyFPE_END_PROTECT(c)
		return PyComplex_FromCComplex(c);
	}
	else {
#endif
		PyFPE_START_PROTECT("atof", return 0)
		dx = atof(s);
		PyFPE_END_PROTECT(dx)
		return PyFloat_FromDouble(dx);
	}
}

static PyObject *
parsestr(s)
	char *s;
{
	PyObject *v;
	int len;
	char *buf;
	char *p;
	char *end;
	int c;
	int first = *s;
	int quote = first;
	if (isalpha(quote) || quote == '_')
		quote = *++s;
	if (quote != '\'' && quote != '\"') {
		PyErr_BadInternalCall();
		return NULL;
	}
	s++;
	len = strlen(s);
	if (s[--len] != quote) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (len >= 4 && s[0] == quote && s[1] == quote) {
		s += 2;
		len -= 2;
		if (s[--len] != quote || s[--len] != quote) {
			PyErr_BadInternalCall();
			return NULL;
		}
	}
	if (first != quote || strchr(s, '\\') == NULL)
		return PyString_FromStringAndSize(s, len);
	v = PyString_FromStringAndSize((char *)NULL, len);
	p = buf = PyString_AsString(v);
	end = s + len;
	while (s < end) {
		if (*s != '\\') {
			*p++ = *s++;
			continue;
		}
		s++;
		switch (*s++) {
		/* XXX This assumes ASCII! */
		case '\n': break;
		case '\\': *p++ = '\\'; break;
		case '\'': *p++ = '\''; break;
		case '\"': *p++ = '\"'; break;
		case 'b': *p++ = '\b'; break;
		case 'f': *p++ = '\014'; break; /* FF */
		case 't': *p++ = '\t'; break;
		case 'n': *p++ = '\n'; break;
		case 'r': *p++ = '\r'; break;
		case 'v': *p++ = '\013'; break; /* VT */
		case 'a': *p++ = '\007'; break; /* BEL, not classic C */
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			c = s[-1] - '0';
			if ('0' <= *s && *s <= '7') {
				c = (c<<3) + *s++ - '0';
				if ('0' <= *s && *s <= '7')
					c = (c<<3) + *s++ - '0';
			}
			*p++ = c;
			break;
		case 'x':
			if (isxdigit(Py_CHARMASK(*s))) {
				sscanf(s, "%x", &c);
				*p++ = c;
				do {
					s++;
				} while (isxdigit(Py_CHARMASK(*s)));
				break;
			}
		/* FALLTHROUGH */
		default: *p++ = '\\'; *p++ = s[-1]; break;
		}
	}
	_PyString_Resize(&v, (int)(p - buf));
	return v;
}

static PyObject *
parsestrplus(n)
	node *n;
{
	PyObject *v;
	int i;
	REQ(CHILD(n, 0), STRING);
	if ((v = parsestr(STR(CHILD(n, 0)))) != NULL) {
		/* String literal concatenation */
		for (i = 1; i < NCH(n) && v != NULL; i++) {
			PyString_ConcatAndDel(&v, parsestr(STR(CHILD(n, i))));
		}
	}
	return v;
}

static void
com_list_constructor(c, n)
	struct compiling *c;
	node *n;
{
	int len;
	int i;
	if (TYPE(n) != testlist)
		REQ(n, exprlist);
	/* exprlist: expr (',' expr)* [',']; likewise for testlist */
	len = (NCH(n) + 1) / 2;
	for (i = 0; i < NCH(n); i += 2)
		com_node(c, CHILD(n, i));
	com_addoparg(c, BUILD_LIST, len);
	com_pop(c, len-1);
}

static void
com_dictmaker(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	/* dictmaker: test ':' test (',' test ':' value)* [','] */
	for (i = 0; i+2 < NCH(n); i += 4) {
		/* We must arrange things just right for STORE_SUBSCR.
		   It wants the stack to look like (value) (dict) (key) */
		com_addbyte(c, DUP_TOP);
		com_push(c, 1);
		com_node(c, CHILD(n, i+2)); /* value */
		com_addbyte(c, ROT_TWO);
		com_node(c, CHILD(n, i)); /* key */
		com_addbyte(c, STORE_SUBSCR);
		com_pop(c, 3);
	}
}

static void
com_atom(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	PyObject *v;
	int i;
	REQ(n, atom);
	ch = CHILD(n, 0);
	switch (TYPE(ch)) {
	case LPAR:
		if (TYPE(CHILD(n, 1)) == RPAR) {
			com_addoparg(c, BUILD_TUPLE, 0);
			com_push(c, 1);
		}
		else
			com_node(c, CHILD(n, 1));
		break;
	case LSQB:
		if (TYPE(CHILD(n, 1)) == RSQB) {
			com_addoparg(c, BUILD_LIST, 0);
			com_push(c, 1);
		}
		else
			com_list_constructor(c, CHILD(n, 1));
		break;
	case LBRACE: /* '{' [dictmaker] '}' */
		com_addoparg(c, BUILD_MAP, 0);
		com_push(c, 1);
		if (TYPE(CHILD(n, 1)) != RBRACE)
			com_dictmaker(c, CHILD(n, 1));
		break;
	case BACKQUOTE:
		com_node(c, CHILD(n, 1));
		com_addbyte(c, UNARY_CONVERT);
		break;
	case NUMBER:
		if ((v = parsenumber(c, STR(ch))) == NULL) {
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			Py_DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		break;
	case STRING:
		v = parsestrplus(n);
		if (v == NULL) {
			c->c_errors++;
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			Py_DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		break;
	case NAME:
		com_addopname(c, LOAD_NAME, ch);
		com_push(c, 1);
		break;
	default:
		/* XXX fprintf(stderr, "node type %d\n", TYPE(ch)); */
		com_error(c, PyExc_SystemError,
			  "com_atom: unexpected node type");
	}
}

static void
com_slice(c, n, op)
	struct compiling *c;
	node *n;
	int op;
{
	if (NCH(n) == 1) {
		com_addbyte(c, op);
	}
	else if (NCH(n) == 2) {
		if (TYPE(CHILD(n, 0)) != COLON) {
			com_node(c, CHILD(n, 0));
			com_addbyte(c, op+1);
		}
		else {
			com_node(c, CHILD(n, 1));
			com_addbyte(c, op+2);
		}
		com_pop(c, 1);
	}
	else {
		com_node(c, CHILD(n, 0));
		com_node(c, CHILD(n, 2));
		com_addbyte(c, op+3);
		com_pop(c, 2);
	}
}

static void
com_argument(c, n, pkeywords)
	struct compiling *c;
	node *n; /* argument */
	PyObject **pkeywords;
{
	node *m;
	REQ(n, argument); /* [test '='] test; really [keyword '='] test */
	if (NCH(n) == 1) {
		if (*pkeywords != NULL) {
			com_error(c, PyExc_SyntaxError,
				   "non-keyword arg after keyword arg");
		}
		else {
			com_node(c, CHILD(n, 0));
		}
		return;
	}
	m = n;
	do {
		m = CHILD(m, 0);
	} while (NCH(m) == 1);
	if (TYPE(m) != NAME) {
		com_error(c, PyExc_SyntaxError,
			  "keyword can't be an expression");
	}
	else {
		PyObject *v = PyString_InternFromString(STR(m));
		if (v != NULL && *pkeywords == NULL)
			*pkeywords = PyDict_New();
		if (v == NULL || *pkeywords == NULL)
			c->c_errors++;
		else {
			if (PyDict_GetItem(*pkeywords, v) != NULL)
				com_error(c, PyExc_SyntaxError,
					  "duplicate keyword argument");
			else
				if (PyDict_SetItem(*pkeywords, v, v) != 0)
					c->c_errors++;
			com_addoparg(c, LOAD_CONST, com_addconst(c, v));
			com_push(c, 1);
			Py_DECREF(v);
		}
	}
	com_node(c, CHILD(n, 2));
}

static void
com_call_function(c, n)
	struct compiling *c;
	node *n; /* EITHER arglist OR ')' */
{
	if (TYPE(n) == RPAR) {
		com_addoparg(c, CALL_FUNCTION, 0);
	}
	else {
		PyObject *keywords = NULL;
		int i, na, nk;
		REQ(n, arglist);
		na = 0;
		nk = 0;
		for (i = 0; i < NCH(n); i += 2) {
			com_argument(c, CHILD(n, i), &keywords);
			if (keywords == NULL)
				na++;
			else
				nk++;
		}
		Py_XDECREF(keywords);
		if (na > 255 || nk > 255) {
			com_error(c, PyExc_SyntaxError,
				  "more than 255 arguments");
		}
		com_addoparg(c, CALL_FUNCTION, na | (nk << 8));
		com_pop(c, na + 2*nk);
	}
}

static void
com_select_member(c, n)
	struct compiling *c;
	node *n;
{
	com_addopname(c, LOAD_ATTR, n);
}

static void
com_sliceobj(c, n)
	struct compiling *c;
	node *n;
{
	int i=0;
	int ns=2; /* number of slice arguments */
	node *ch;

	/* first argument */
	if (TYPE(CHILD(n,i)) == COLON) {
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
		i++;
	}
	else {
		com_node(c, CHILD(n,i));
		i++;
		REQ(CHILD(n,i),COLON);
		i++;
	}
	/* second argument */
	if (i < NCH(n) && TYPE(CHILD(n,i)) == test) {
		com_node(c, CHILD(n,i));
		i++;
	}
	else {
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
	}
	/* remaining arguments */
	for (; i < NCH(n); i++) {
		ns++;
		ch=CHILD(n,i);
		REQ(ch, sliceop);
		if (NCH(ch) == 1) {
			/* right argument of ':' missing */
			com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
			com_push(c, 1);
		}
		else
			com_node(c, CHILD(ch,1));
	}
	com_addoparg(c, BUILD_SLICE, ns);
	com_pop(c, 1 + (ns == 3));
}

static void
com_subscript(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	REQ(n, subscript);
	ch = CHILD(n,0);
	/* check for rubber index */
	if (TYPE(ch) == DOT && TYPE(CHILD(n,1)) == DOT) {
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_Ellipsis));
		com_push(c, 1);
	}
	else {
		/* check for slice */
		if ((TYPE(ch) == COLON || NCH(n) > 1))
			com_sliceobj(c, n);
		else {
			REQ(ch, test);
			com_node(c, ch);
		}
	}
}

static void
com_subscriptlist(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	int i, op;
	REQ(n, subscriptlist);
	/* Check to make backward compatible slice behavior for '[i:j]' */
	if (NCH(n) == 1) {
		node *sub = CHILD(n, 0); /* subscript */
		/* Make it is a simple slice.
		   Should have exactly one colon. */
        if ((TYPE(CHILD(sub, 0)) == COLON
             || (NCH(sub) > 1 && TYPE(CHILD(sub, 1)) == COLON))
            && (TYPE(CHILD(sub,NCH(sub)-1)) != sliceop))
	{
			if (assigning == OP_APPLY)
				op = SLICE;
			else
				op = ((assigning == OP_ASSIGN) ?
				      STORE_SLICE : DELETE_SLICE);
			com_slice(c, sub, op);
			if (op == STORE_SLICE)
				com_pop(c, 2);
			else if (op == DELETE_SLICE)
				com_pop(c, 1);
			return;
		}
	}
	/* Else normal subscriptlist.  Compile each subscript. */
	for (i = 0; i < NCH(n); i += 2)
		com_subscript(c, CHILD(n, i));
	/* Put multiple subscripts into a tuple */
	if (NCH(n) > 1) {
		i = (NCH(n)+1) / 2;
		com_addoparg(c, BUILD_TUPLE, i);
		com_pop(c, i-1);
	}
	if (assigning == OP_APPLY) {
		op = BINARY_SUBSCR;
		i = 1;
	}
	else if (assigning == OP_ASSIGN) {
		op = STORE_SUBSCR;
		i = 3;
	}
	else {
		op = DELETE_SUBSCR;
		i = 2;
	}
	com_addbyte(c, op);
	com_pop(c, i);
}

static void
com_apply_trailer(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, trailer);
	switch (TYPE(CHILD(n, 0))) {
	case LPAR:
		com_call_function(c, CHILD(n, 1));
		break;
	case DOT:
		com_select_member(c, CHILD(n, 1));
		break;
	case LSQB:
		com_subscriptlist(c, CHILD(n, 1), OP_APPLY);
		break;
	default:
		com_error(c, PyExc_SystemError,
			  "com_apply_trailer: unknown trailer type");
	}
}

static void
com_power(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, power);
	com_atom(c, CHILD(n, 0));
	for (i = 1; i < NCH(n); i++) {
		if (TYPE(CHILD(n, i)) == DOUBLESTAR) {
			com_factor(c, CHILD(n, i+1));
			com_addbyte(c, BINARY_POWER);
			com_pop(c, 1);
			break;
		}
		else
			com_apply_trailer(c, CHILD(n, i));
	}
}

static void
com_factor(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, factor);
	if (TYPE(CHILD(n, 0)) == PLUS) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_POSITIVE);
	}
	else if (TYPE(CHILD(n, 0)) == MINUS) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_NEGATIVE);
	}
	else if (TYPE(CHILD(n, 0)) == TILDE) {
		com_factor(c, CHILD(n, 1));
		com_addbyte(c, UNARY_INVERT);
	}
	else {
		com_power(c, CHILD(n, 0));
	}
}

static void
com_term(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, term);
	com_factor(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_factor(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case STAR:
			op = BINARY_MULTIPLY;
			break;
		case SLASH:
			op = BINARY_DIVIDE;
			break;
		case PERCENT:
			op = BINARY_MODULO;
			break;
		default:
			com_error(c, PyExc_SystemError,
				  "com_term: operator not *, / or %");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static void
com_arith_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, arith_expr);
	com_term(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_term(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case PLUS:
			op = BINARY_ADD;
			break;
		case MINUS:
			op = BINARY_SUBTRACT;
			break;
		default:
			com_error(c, PyExc_SystemError,
				  "com_arith_expr: operator not + or -");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static void
com_shift_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, shift_expr);
	com_arith_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_arith_expr(c, CHILD(n, i));
		switch (TYPE(CHILD(n, i-1))) {
		case LEFTSHIFT:
			op = BINARY_LSHIFT;
			break;
		case RIGHTSHIFT:
			op = BINARY_RSHIFT;
			break;
		default:
			com_error(c, PyExc_SystemError,
				  "com_shift_expr: operator not << or >>");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static void
com_and_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, and_expr);
	com_shift_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_shift_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == AMPER) {
			op = BINARY_AND;
		}
		else {
			com_error(c, PyExc_SystemError,
				  "com_and_expr: operator not &");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static void
com_xor_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, xor_expr);
	com_and_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_and_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == CIRCUMFLEX) {
			op = BINARY_XOR;
		}
		else {
			com_error(c, PyExc_SystemError,
				  "com_xor_expr: operator not ^");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static void
com_expr(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int op;
	REQ(n, expr);
	com_xor_expr(c, CHILD(n, 0));
	for (i = 2; i < NCH(n); i += 2) {
		com_xor_expr(c, CHILD(n, i));
		if (TYPE(CHILD(n, i-1)) == VBAR) {
			op = BINARY_OR;
		}
		else {
			com_error(c, PyExc_SystemError,
				  "com_expr: expr operator not |");
			op = 255;
		}
		com_addbyte(c, op);
		com_pop(c, 1);
	}
}

static enum cmp_op
cmp_type(n)
	node *n;
{
	REQ(n, comp_op);
	/* comp_op: '<' | '>' | '=' | '>=' | '<=' | '<>' | '!=' | '=='
	          | 'in' | 'not' 'in' | 'is' | 'is' not' */
	if (NCH(n) == 1) {
		n = CHILD(n, 0);
		switch (TYPE(n)) {
		case LESS:	return LT;
		case GREATER:	return GT;
		case EQEQUAL:			/* == */
		case EQUAL:	return EQ;
		case LESSEQUAL:	return LE;
		case GREATEREQUAL: return GE;
		case NOTEQUAL:	return NE;	/* <> or != */
		case NAME:	if (strcmp(STR(n), "in") == 0) return IN;
				if (strcmp(STR(n), "is") == 0) return IS;
		}
	}
	else if (NCH(n) == 2) {
		switch (TYPE(CHILD(n, 0))) {
		case NAME:	if (strcmp(STR(CHILD(n, 1)), "in") == 0)
					return NOT_IN;
				if (strcmp(STR(CHILD(n, 0)), "is") == 0)
					return IS_NOT;
		}
	}
	return BAD;
}

static void
com_comparison(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	enum cmp_op op;
	int anchor;
	REQ(n, comparison); /* comparison: expr (comp_op expr)* */
	com_expr(c, CHILD(n, 0));
	if (NCH(n) == 1)
		return;
	
	/****************************************************************
	   The following code is generated for all but the last
	   comparison in a chain:
	   
	   label:	on stack:	opcode:		jump to:
	   
			a		<code to load b>
			a, b		DUP_TOP
			a, b, b		ROT_THREE
			b, a, b		COMPARE_OP
			b, 0-or-1	JUMP_IF_FALSE	L1
			b, 1		POP_TOP
			b		
	
	   We are now ready to repeat this sequence for the next
	   comparison in the chain.
	   
	   For the last we generate:
	   
	   		b		<code to load c>
	   		b, c		COMPARE_OP
	   		0-or-1		
	   
	   If there were any jumps to L1 (i.e., there was more than one
	   comparison), we generate:
	   
	   		0-or-1		JUMP_FORWARD	L2
	   L1:		b, 0		ROT_TWO
	   		0, b		POP_TOP
	   		0
	   L2:		0-or-1
	****************************************************************/
	
	anchor = 0;
	
	for (i = 2; i < NCH(n); i += 2) {
		com_expr(c, CHILD(n, i));
		if (i+2 < NCH(n)) {
			com_addbyte(c, DUP_TOP);
			com_push(c, 1);
			com_addbyte(c, ROT_THREE);
		}
		op = cmp_type(CHILD(n, i-1));
		if (op == BAD) {
			com_error(c, PyExc_SystemError,
				  "com_comparison: unknown comparison op");
		}
		com_addoparg(c, COMPARE_OP, op);
		com_pop(c, 1);
		if (i+2 < NCH(n)) {
			com_addfwref(c, JUMP_IF_FALSE, &anchor);
			com_addbyte(c, POP_TOP);
			com_pop(c, 1);
		}
	}
	
	if (anchor) {
		int anchor2 = 0;
		com_addfwref(c, JUMP_FORWARD, &anchor2);
		com_backpatch(c, anchor);
		com_addbyte(c, ROT_TWO);
		com_addbyte(c, POP_TOP);
		com_backpatch(c, anchor2);
	}
}

static void
com_not_test(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, not_test); /* 'not' not_test | comparison */
	if (NCH(n) == 1) {
		com_comparison(c, CHILD(n, 0));
	}
	else {
		com_not_test(c, CHILD(n, 1));
		com_addbyte(c, UNARY_NOT);
	}
}

static void
com_and_test(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int anchor;
	REQ(n, and_test); /* not_test ('and' not_test)* */
	anchor = 0;
	i = 0;
	for (;;) {
		com_not_test(c, CHILD(n, i));
		if ((i += 2) >= NCH(n))
			break;
		com_addfwref(c, JUMP_IF_FALSE, &anchor);
		com_addbyte(c, POP_TOP);
		com_pop(c, 1);
	}
	if (anchor)
		com_backpatch(c, anchor);
}

static void
com_test(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, test); /* and_test ('or' and_test)* | lambdef */
	if (NCH(n) == 1 && TYPE(CHILD(n, 0)) == lambdef) {
		PyObject *v;
		int i;
		int ndefs = com_argdefs(c, CHILD(n, 0));
		v = (PyObject *) icompile(CHILD(n, 0), c);
		if (v == NULL) {
			c->c_errors++;
			i = 255;
		}
		else {
			i = com_addconst(c, v);
			Py_DECREF(v);
		}
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		com_addoparg(c, MAKE_FUNCTION, ndefs);
		com_pop(c, ndefs);
	}
	else {
		int anchor = 0;
		int i = 0;
		for (;;) {
			com_and_test(c, CHILD(n, i));
			if ((i += 2) >= NCH(n))
				break;
			com_addfwref(c, JUMP_IF_TRUE, &anchor);
			com_addbyte(c, POP_TOP);
			com_pop(c, 1);
		}
		if (anchor)
			com_backpatch(c, anchor);
	}
}

static void
com_list(c, n, toplevel)
	struct compiling *c;
	node *n;
	int toplevel; /* If nonzero, *always* build a tuple */
{
	/* exprlist: expr (',' expr)* [',']; likewise for testlist */
	if (NCH(n) == 1 && !toplevel) {
		com_node(c, CHILD(n, 0));
	}
	else {
		int i;
		int len;
		len = (NCH(n) + 1) / 2;
		for (i = 0; i < NCH(n); i += 2)
			com_node(c, CHILD(n, i));
		com_addoparg(c, BUILD_TUPLE, len);
		com_pop(c, len-1);
	}
}


/* Begin of assignment compilation */

static void com_assign_name Py_PROTO((struct compiling *, node *, int));
static void com_assign Py_PROTO((struct compiling *, node *, int));

static void
com_assign_attr(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	com_addopname(c, assigning ? STORE_ATTR : DELETE_ATTR, n);
	com_pop(c, assigning ? 2 : 1);
}

static void
com_assign_trailer(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	REQ(n, trailer);
	switch (TYPE(CHILD(n, 0))) {
	case LPAR: /* '(' [exprlist] ')' */
		com_error(c, PyExc_SyntaxError,
			  "can't assign to function call");
		break;
	case DOT: /* '.' NAME */
		com_assign_attr(c, CHILD(n, 1), assigning);
		break;
	case LSQB: /* '[' subscriptlist ']' */
		com_subscriptlist(c, CHILD(n, 1), assigning);
		break;
	default:
		com_error(c, PyExc_SystemError, "unknown trailer type");
	}
}

static void
com_assign_tuple(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	int i;
	if (TYPE(n) != testlist)
		REQ(n, exprlist);
	if (assigning) {
		i = (NCH(n)+1)/2;
		com_addoparg(c, UNPACK_TUPLE, i);
		com_push(c, i-1);
	}
	for (i = 0; i < NCH(n); i += 2)
		com_assign(c, CHILD(n, i), assigning);
}

static void
com_assign_list(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	int i;
	if (assigning) {
		i = (NCH(n)+1)/2;
		com_addoparg(c, UNPACK_LIST, i);
		com_push(c, i-1);
	}
	for (i = 0; i < NCH(n); i += 2)
		com_assign(c, CHILD(n, i), assigning);
}

static void
com_assign_name(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	REQ(n, NAME);
	com_addopname(c, assigning ? STORE_NAME : DELETE_NAME, n);
	if (assigning)
		com_pop(c, 1);
}

static void
com_assign(c, n, assigning)
	struct compiling *c;
	node *n;
	int assigning;
{
	/* Loop to avoid trivial recursion */
	for (;;) {
		switch (TYPE(n)) {
		
		case exprlist:
		case testlist:
			if (NCH(n) > 1) {
				com_assign_tuple(c, n, assigning);
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case test:
		case and_test:
		case not_test:
		case comparison:
		case expr:
		case xor_expr:
		case and_expr:
		case shift_expr:
		case arith_expr:
		case term:
		case factor:
			if (NCH(n) > 1) {
				com_error(c, PyExc_SyntaxError,
					  "can't assign to operator");
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case power: /* atom trailer* ('**' power)* */
/* ('+'|'-'|'~') factor | atom trailer* */
			if (TYPE(CHILD(n, 0)) != atom) {
				com_error(c, PyExc_SyntaxError,
					  "can't assign to operator");
				return;
			}
			if (NCH(n) > 1) { /* trailer or exponent present */
				int i;
				com_node(c, CHILD(n, 0));
				for (i = 1; i+1 < NCH(n); i++) {
					if (TYPE(CHILD(n, i)) == DOUBLESTAR) {
						com_error(c, PyExc_SyntaxError,
						  "can't assign to operator");
						return;
					}
					com_apply_trailer(c, CHILD(n, i));
				} /* NB i is still alive */
				com_assign_trailer(c,
						CHILD(n, i), assigning);
				return;
			}
			n = CHILD(n, 0);
			break;
		
		case atom:
			switch (TYPE(CHILD(n, 0))) {
			case LPAR:
				n = CHILD(n, 1);
				if (TYPE(n) == RPAR) {
					/* XXX Should allow () = () ??? */
					com_error(c, PyExc_SyntaxError,
						  "can't assign to ()");
					return;
				}
				break;
			case LSQB:
				n = CHILD(n, 1);
				if (TYPE(n) == RSQB) {
					com_error(c, PyExc_SyntaxError,
						  "can't assign to []");
					return;
				}
				com_assign_list(c, n, assigning);
				return;
			case NAME:
				com_assign_name(c, CHILD(n, 0), assigning);
				return;
			default:
				com_error(c, PyExc_SyntaxError,
					  "can't assign to literal");
				return;
			}
			break;

		case lambdef:
			com_error(c, PyExc_SyntaxError,
				  "can't assign to lambda");
			return;
		
		default:
			/* XXX fprintf(stderr, "node type %d\n", TYPE(n)); */
			com_error(c, PyExc_SystemError,
				  "com_assign: bad node");
			return;
		
		}
	}
}

/* Forward */ static node *get_rawdocstring Py_PROTO((node *));

static void
com_expr_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, expr_stmt); /* testlist ('=' testlist)* */
	/* Forget it if we have just a doc string here */
	if (!c->c_interactive && NCH(n) == 1 && get_rawdocstring(n) != NULL)
		return;
	com_node(c, CHILD(n, NCH(n)-1));
	if (NCH(n) == 1) {
		if (c->c_interactive)
			com_addbyte(c, PRINT_EXPR);
		else
			com_addbyte(c, POP_TOP);
		com_pop(c, 1);
	}
	else {
		int i;
		for (i = 0; i < NCH(n)-2; i+=2) {
			if (i+2 < NCH(n)-2) {
				com_addbyte(c, DUP_TOP);
				com_push(c, 1);
			}
			com_assign(c, CHILD(n, i), OP_ASSIGN);
		}
	}
}

static void
com_assert_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int a = 0, b = 0;
	int i;
	REQ(n, assert_stmt); /* 'assert' test [',' test] */
	/* Generate code like for
	   
	   if __debug__:
	      if not <test>:
	         raise AssertionError [, <message>]

	   where <message> is the second test, if present.
	*/
	if (Py_OptimizeFlag)
		return;
	com_addopnamestr(c, LOAD_GLOBAL, "__debug__");
	com_push(c, 1);
	com_addfwref(c, JUMP_IF_FALSE, &a);
	com_addbyte(c, POP_TOP);
	com_pop(c, 1);
	com_node(c, CHILD(n, 1));
	com_addfwref(c, JUMP_IF_TRUE, &b);
	com_addbyte(c, POP_TOP);
	com_pop(c, 1);
	/* Raise that exception! */
	com_addopnamestr(c, LOAD_GLOBAL, "AssertionError");
	com_push(c, 1);
	i = NCH(n)/2; /* Either 2 or 4 */
	if (i > 1)
		com_node(c, CHILD(n, 3));
	com_addoparg(c, RAISE_VARARGS, i);
	com_pop(c, i);
	/* The interpreter does not fall through */
	/* All jumps converge here */
	com_backpatch(c, a);
	com_backpatch(c, b);
	com_addbyte(c, POP_TOP);
}

static void
com_print_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, print_stmt); /* 'print' (test ',')* [test] */
	for (i = 1; i < NCH(n); i += 2) {
		com_node(c, CHILD(n, i));
		com_addbyte(c, PRINT_ITEM);
		com_pop(c, 1);
	}
	if (TYPE(CHILD(n, NCH(n)-1)) != COMMA)
		com_addbyte(c, PRINT_NEWLINE);
		/* XXX Alternatively, LOAD_CONST '\n' and then PRINT_ITEM */
}

static void
com_return_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, return_stmt); /* 'return' [testlist] */
	if (!c->c_infunction) {
		com_error(c, PyExc_SyntaxError, "'return' outside function");
	}
	if (NCH(n) < 2) {
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
	}
	else
		com_node(c, CHILD(n, 1));
	com_addbyte(c, RETURN_VALUE);
	com_pop(c, 1);
}

static void
com_raise_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, raise_stmt); /* 'raise' test [',' test [',' test]] */
	com_node(c, CHILD(n, 1));
	if (NCH(n) > 3) {
		com_node(c, CHILD(n, 3));
		if (NCH(n) > 5)
			com_node(c, CHILD(n, 5));
	}
	i = NCH(n)/2;
	com_addoparg(c, RAISE_VARARGS, i);
	com_pop(c, i);
}

static void
com_import_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, import_stmt);
	/* 'import' dotted_name (',' dotted_name)* |
	   'from' dotted_name 'import' ('*' | NAME (',' NAME)*) */
	if (STR(CHILD(n, 0))[0] == 'f') {
		/* 'from' dotted_name 'import' ... */
		REQ(CHILD(n, 1), dotted_name);
		com_addopname(c, IMPORT_NAME, CHILD(n, 1));
		com_push(c, 1);
		for (i = 3; i < NCH(n); i += 2)
			com_addopname(c, IMPORT_FROM, CHILD(n, i));
		com_addbyte(c, POP_TOP);
		com_pop(c, 1);
	}
	else {
		/* 'import' ... */
		for (i = 1; i < NCH(n); i += 2) {
			REQ(CHILD(n, i), dotted_name);
			com_addopname(c, IMPORT_NAME, CHILD(n, i));
			com_push(c, 1);
			com_addopname(c, STORE_NAME, CHILD(CHILD(n, i), 0));
			com_pop(c, 1);
		}
	}
}

static void
com_global_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, global_stmt);
	/* 'global' NAME (',' NAME)* */
	for (i = 1; i < NCH(n); i += 2) {
		char *s = STR(CHILD(n, i));
#ifdef PRIVATE_NAME_MANGLING
		char buffer[256];
		if (s != NULL && s[0] == '_' && s[1] == '_' &&
		    c->c_private != NULL &&
		    com_mangle(c, s, buffer, (int)sizeof(buffer)))
			s = buffer;
#endif
		if (PyDict_GetItemString(c->c_locals, s) != NULL) {
			com_error(c, PyExc_SyntaxError,
				  "name is local and global");
		}
		else if (PyDict_SetItemString(c->c_globals, s, Py_None) != 0)
			c->c_errors++;
	}
}

static int
com_newlocal_o(c, nameval)
	struct compiling *c;
	PyObject *nameval;
{
	int i;
	PyObject *ival;
	if (PyList_Size(c->c_varnames) != c->c_nlocals) {
		/* This is usually caused by an error on a previous call */
		if (c->c_errors == 0) {
			com_error(c, PyExc_SystemError,
				  "mixed up var name/index");
		}
		return 0;
	}
	ival = PyInt_FromLong(i = c->c_nlocals++);
	if (ival == NULL)
		c->c_errors++;
	else if (PyDict_SetItem(c->c_locals, nameval, ival) != 0)
		c->c_errors++;
	else if (PyList_Append(c->c_varnames, nameval) != 0)
		c->c_errors++;
	Py_XDECREF(ival);
	return i;
}

static int
com_addlocal_o(c, nameval)
	struct compiling *c;
	PyObject *nameval;
{
	PyObject *ival =  PyDict_GetItem(c->c_locals, nameval);
	if (ival != NULL)
		return PyInt_AsLong(ival);
	return com_newlocal_o(c, nameval);
}

static int
com_newlocal(c, name)
	struct compiling *c;
	char *name;
{
	PyObject *nameval = PyString_InternFromString(name);
	int i;
	if (nameval == NULL) {
		c->c_errors++;
		return 0;
	}
	i = com_newlocal_o(c, nameval);
	Py_DECREF(nameval);
	return i;
}

static void
com_exec_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, exec_stmt);
	/* exec_stmt: 'exec' expr ['in' expr [',' expr]] */
	com_node(c, CHILD(n, 1));
	if (NCH(n) >= 4)
		com_node(c, CHILD(n, 3));
	else {
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
	}
	if (NCH(n) >= 6)
		com_node(c, CHILD(n, 5));
	else {
		com_addbyte(c, DUP_TOP);
		com_push(c, 1);
	}
	com_addbyte(c, EXEC_STMT);
	com_pop(c, 3);
}

static int
is_constant_false(c, n)
	struct compiling *c;
	node *n;
{
	PyObject *v;
	int i;

  /* Label to avoid tail recursion */
  next:
	switch (TYPE(n)) {

	case suite:
		if (NCH(n) == 1) {
			n = CHILD(n, 0);
			goto next;
		}
		/* Fall through */
	case file_input:
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt) {
				n = ch;
				goto next;
			}
		}
		break;

	case stmt:
	case simple_stmt:
	case small_stmt:
		n = CHILD(n, 0);
		goto next;

	case expr_stmt:
	case testlist:
	case test:
	case and_test:
	case not_test:
	case comparison:
	case expr:
	case xor_expr:
	case and_expr:
	case shift_expr:
	case arith_expr:
	case term:
	case factor:
	case power:
	case atom:
		if (NCH(n) == 1) {
			n = CHILD(n, 0);
			goto next;
		}
		break;

	case NAME:
		if (Py_OptimizeFlag && strcmp(STR(n), "__debug__") == 0)
			return 1;
		break;

	case NUMBER:
		v = parsenumber(c, STR(n));
		if (v == NULL) {
			PyErr_Clear();
			break;
		}
		i = PyObject_IsTrue(v);
		Py_DECREF(v);
		return i == 0;

	case STRING:
		v = parsestr(STR(n));
		if (v == NULL) {
			PyErr_Clear();
			break;
		}
		i = PyObject_IsTrue(v);
		Py_DECREF(v);
		return i == 0;

	}
	return 0;
}

static void
com_if_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	int anchor = 0;
	REQ(n, if_stmt);
	/*'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite] */
	for (i = 0; i+3 < NCH(n); i+=4) {
		int a = 0;
		node *ch = CHILD(n, i+1);
		if (is_constant_false(c, ch))
			continue;
		if (i > 0)
			com_addoparg(c, SET_LINENO, ch->n_lineno);
		com_node(c, ch);
		com_addfwref(c, JUMP_IF_FALSE, &a);
		com_addbyte(c, POP_TOP);
		com_pop(c, 1);
		com_node(c, CHILD(n, i+3));
		com_addfwref(c, JUMP_FORWARD, &anchor);
		com_backpatch(c, a);
		/* We jump here with an extra entry which we now pop */
		com_addbyte(c, POP_TOP);
	}
	if (i+2 < NCH(n))
		com_node(c, CHILD(n, i+2));
	if (anchor)
		com_backpatch(c, anchor);
}

static void
com_while_stmt(c, n)
	struct compiling *c;
	node *n;
{
	int break_anchor = 0;
	int anchor = 0;
	int save_begin = c->c_begin;
	REQ(n, while_stmt); /* 'while' test ':' suite ['else' ':' suite] */
	com_addfwref(c, SETUP_LOOP, &break_anchor);
	block_push(c, SETUP_LOOP);
	c->c_begin = c->c_nexti;
	com_addoparg(c, SET_LINENO, n->n_lineno);
	com_node(c, CHILD(n, 1));
	com_addfwref(c, JUMP_IF_FALSE, &anchor);
	com_addbyte(c, POP_TOP);
	com_pop(c, 1);
	c->c_loops++;
	com_node(c, CHILD(n, 3));
	c->c_loops--;
	com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	c->c_begin = save_begin;
	com_backpatch(c, anchor);
	/* We jump here with one entry more on the stack */
	com_addbyte(c, POP_TOP);
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_LOOP);
	if (NCH(n) > 4)
		com_node(c, CHILD(n, 6));
	com_backpatch(c, break_anchor);
}

static void
com_for_stmt(c, n)
	struct compiling *c;
	node *n;
{
	PyObject *v;
	int break_anchor = 0;
	int anchor = 0;
	int save_begin = c->c_begin;
	REQ(n, for_stmt);
	/* 'for' exprlist 'in' exprlist ':' suite ['else' ':' suite] */
	com_addfwref(c, SETUP_LOOP, &break_anchor);
	block_push(c, SETUP_LOOP);
	com_node(c, CHILD(n, 3));
	v = PyInt_FromLong(0L);
	if (v == NULL)
		c->c_errors++;
	com_addoparg(c, LOAD_CONST, com_addconst(c, v));
	com_push(c, 1);
	Py_XDECREF(v);
	c->c_begin = c->c_nexti;
	com_addoparg(c, SET_LINENO, n->n_lineno);
	com_addfwref(c, FOR_LOOP, &anchor);
	com_push(c, 1);
	com_assign(c, CHILD(n, 1), OP_ASSIGN);
	c->c_loops++;
	com_node(c, CHILD(n, 5));
	c->c_loops--;
	com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	c->c_begin = save_begin;
	com_backpatch(c, anchor);
	com_pop(c, 2); /* FOR_LOOP has popped these */
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_LOOP);
	if (NCH(n) > 8)
		com_node(c, CHILD(n, 8));
	com_backpatch(c, break_anchor);
}

/* Code generated for "try: S finally: Sf" is as follows:
   
		SETUP_FINALLY	L
		<code for S>
		POP_BLOCK
		LOAD_CONST	<nil>
	L:	<code for Sf>
		END_FINALLY
   
   The special instructions use the block stack.  Each block
   stack entry contains the instruction that created it (here
   SETUP_FINALLY), the level of the value stack at the time the
   block stack entry was created, and a label (here L).
   
   SETUP_FINALLY:
	Pushes the current value stack level and the label
	onto the block stack.
   POP_BLOCK:
	Pops en entry from the block stack, and pops the value
	stack until its level is the same as indicated on the
	block stack.  (The label is ignored.)
   END_FINALLY:
	Pops a variable number of entries from the *value* stack
	and re-raises the exception they specify.  The number of
	entries popped depends on the (pseudo) exception type.
   
   The block stack is unwound when an exception is raised:
   when a SETUP_FINALLY entry is found, the exception is pushed
   onto the value stack (and the exception condition is cleared),
   and the interpreter jumps to the label gotten from the block
   stack.
   
   Code generated for "try: S except E1, V1: S1 except E2, V2: S2 ...":
   (The contents of the value stack is shown in [], with the top
   at the right; 'tb' is trace-back info, 'val' the exception's
   associated value, and 'exc' the exception.)
   
   Value stack		Label	Instruction	Argument
   []				SETUP_EXCEPT	L1
   []				<code for S>
   []				POP_BLOCK
   []				JUMP_FORWARD	L0
   
   [tb, val, exc]	L1:	DUP				)
   [tb, val, exc, exc]		<evaluate E1>			)
   [tb, val, exc, exc, E1]	COMPARE_OP	EXC_MATCH	) only if E1
   [tb, val, exc, 1-or-0]	JUMP_IF_FALSE	L2		)
   [tb, val, exc, 1]		POP				)
   [tb, val, exc]		POP
   [tb, val]			<assign to V1>	(or POP if no V1)
   [tb]				POP
   []				<code for S1>
   				JUMP_FORWARD	L0
   
   [tb, val, exc, 0]	L2:	POP
   [tb, val, exc]		DUP
   .............................etc.......................

   [tb, val, exc, 0]	Ln+1:	POP
   [tb, val, exc]	   	END_FINALLY	# re-raise exception
   
   []			L0:	<next statement>
   
   Of course, parts are not generated if Vi or Ei is not present.
*/

static void
com_try_except(c, n)
	struct compiling *c;
	node *n;
{
	int except_anchor = 0;
	int end_anchor = 0;
	int else_anchor = 0;
	int i;
	node *ch;

	com_addfwref(c, SETUP_EXCEPT, &except_anchor);
	block_push(c, SETUP_EXCEPT);
	com_node(c, CHILD(n, 2));
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_EXCEPT);
	com_addfwref(c, JUMP_FORWARD, &else_anchor);
	com_backpatch(c, except_anchor);
	for (i = 3;
	     i < NCH(n) && TYPE(ch = CHILD(n, i)) == except_clause;
	     i += 3) {
		/* except_clause: 'except' [expr [',' var]] */
		if (except_anchor == 0) {
			com_error(c, PyExc_SyntaxError,
				  "default 'except:' must be last");
			break;
		}
		except_anchor = 0;
		com_push(c, 3); /* tb, val, exc pushed by exception */
		com_addoparg(c, SET_LINENO, ch->n_lineno);
		if (NCH(ch) > 1) {
			com_addbyte(c, DUP_TOP);
			com_push(c, 1);
			com_node(c, CHILD(ch, 1));
			com_addoparg(c, COMPARE_OP, EXC_MATCH);
			com_pop(c, 1);
			com_addfwref(c, JUMP_IF_FALSE, &except_anchor);
			com_addbyte(c, POP_TOP);
			com_pop(c, 1);
		}
		com_addbyte(c, POP_TOP);
		com_pop(c, 1);
		if (NCH(ch) > 3)
			com_assign(c, CHILD(ch, 3), OP_ASSIGN);
		else {
			com_addbyte(c, POP_TOP);
			com_pop(c, 1);
		}
		com_addbyte(c, POP_TOP);
		com_pop(c, 1);
		com_node(c, CHILD(n, i+2));
		com_addfwref(c, JUMP_FORWARD, &end_anchor);
		if (except_anchor) {
			com_backpatch(c, except_anchor);
			/* We come in with [tb, val, exc, 0] on the
			   stack; one pop and it's the same as
			   expected at the start of the loop */
			com_addbyte(c, POP_TOP);
		}
	}
	/* We actually come in here with [tb, val, exc] but the
	   END_FINALLY will zap those and jump around.
	   The c_stacklevel does not reflect them so we need not pop
	   anything. */
	com_addbyte(c, END_FINALLY);
	com_backpatch(c, else_anchor);
	if (i < NCH(n))
		com_node(c, CHILD(n, i+2));
	com_backpatch(c, end_anchor);
}

static void
com_try_finally(c, n)
	struct compiling *c;
	node *n;
{
	int finally_anchor = 0;
	node *ch;

	com_addfwref(c, SETUP_FINALLY, &finally_anchor);
	block_push(c, SETUP_FINALLY);
	com_node(c, CHILD(n, 2));
	com_addbyte(c, POP_BLOCK);
	block_pop(c, SETUP_FINALLY);
	block_push(c, END_FINALLY);
	com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
	/* While the generated code pushes only one item,
	   the try-finally handling can enter here with
	   up to three items.  OK, here are the details:
	   3 for an exception, 2 for RETURN, 1 for BREAK. */
	com_push(c, 3);
	com_backpatch(c, finally_anchor);
	ch = CHILD(n, NCH(n)-1);
	com_addoparg(c, SET_LINENO, ch->n_lineno);
	com_node(c, ch);
	com_addbyte(c, END_FINALLY);
	block_pop(c, END_FINALLY);
	com_pop(c, 3); /* Matches the com_push above */
}

static void
com_try_stmt(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, try_stmt);
	/* 'try' ':' suite (except_clause ':' suite)+ ['else' ':' suite]
	 | 'try' ':' suite 'finally' ':' suite */
	if (TYPE(CHILD(n, 3)) != except_clause)
		com_try_finally(c, n);
	else
		com_try_except(c, n);
}

static node *
get_rawdocstring(n)
	node *n;
{
	int i;

  /* Label to avoid tail recursion */
  next:
	switch (TYPE(n)) {

	case suite:
		if (NCH(n) == 1) {
			n = CHILD(n, 0);
			goto next;
		}
		/* Fall through */
	case file_input:
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt) {
				n = ch;
				goto next;
			}
		}
		break;

	case stmt:
	case simple_stmt:
	case small_stmt:
		n = CHILD(n, 0);
		goto next;

	case expr_stmt:
	case testlist:
	case test:
	case and_test:
	case not_test:
	case comparison:
	case expr:
	case xor_expr:
	case and_expr:
	case shift_expr:
	case arith_expr:
	case term:
	case factor:
	case power:
		if (NCH(n) == 1) {
			n = CHILD(n, 0);
			goto next;
		}
		break;

	case atom:
		if (TYPE(CHILD(n, 0)) == STRING)
			return n;
		break;

	}
	return NULL;
}

static PyObject *
get_docstring(n)
	node *n;
{
	n = get_rawdocstring(n);
	if (n == NULL)
		return NULL;
	return parsestrplus(n);
}

static void
com_suite(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, suite);
	/* simple_stmt | NEWLINE INDENT NEWLINE* (stmt NEWLINE*)+ DEDENT */
	if (NCH(n) == 1) {
		com_node(c, CHILD(n, 0));
	}
	else {
		int i;
		for (i = 0; i < NCH(n); i++) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == stmt)
				com_node(c, ch);
		}
	}
}

/* ARGSUSED */
static void
com_continue_stmt(c, n)
	struct compiling *c;
	node *n; /* Not used, but passed for consistency */
{
	int i = c->c_nblocks;
	if (i-- > 0 && c->c_block[i] == SETUP_LOOP) {
		com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	}
	else {
		com_error(c, PyExc_SyntaxError,
			  "'continue' not properly in loop");
	}
	/* XXX Could allow it inside a 'finally' clause
	   XXX if we could pop the exception still on the stack */
}

static int
com_argdefs(c, n)
	struct compiling *c;
	node *n;
{
	int i, nch, nargs, ndefs;
	if (TYPE(n) == lambdef) {
		/* lambdef: 'lambda' [varargslist] ':' test */
		n = CHILD(n, 1);
	}
	else {
		REQ(n, funcdef); /* funcdef: 'def' NAME parameters ... */
		n = CHILD(n, 2);
		REQ(n, parameters); /* parameters: '(' [varargslist] ')' */
		n = CHILD(n, 1);
	}
	if (TYPE(n) != varargslist)
		    return 0;
	/* varargslist:
		(fpdef ['=' test] ',')* '*' ....... |
		fpdef ['=' test] (',' fpdef ['=' test])* [','] */
	nch = NCH(n);
	nargs = 0;
	ndefs = 0;
	for (i = 0; i < nch; i++) {
		int t;
		if (TYPE(CHILD(n, i)) == STAR ||
		    TYPE(CHILD(n, i)) == DOUBLESTAR)
			break;
		nargs++;
		i++;
		if (i >= nch)
			t = RPAR; /* Anything except EQUAL or COMMA */
		else
			t = TYPE(CHILD(n, i));
		if (t == EQUAL) {
			i++;
			ndefs++;
			com_node(c, CHILD(n, i));
			i++;
			if (i >= nch)
				break;
			t = TYPE(CHILD(n, i));
		}
		else {
			/* Treat "(a=1, b)" as "(a=1, b=None)" */
			if (ndefs) {
				com_addoparg(c, LOAD_CONST,
					     com_addconst(c, Py_None));
				com_push(c, 1);
				ndefs++;
			}
		}
		if (t != COMMA)
			break;
	}
	return ndefs;
}

static void
com_funcdef(c, n)
	struct compiling *c;
	node *n;
{
	PyObject *v;
	REQ(n, funcdef); /* funcdef: 'def' NAME parameters ':' suite */
	v = (PyObject *)icompile(n, c);
	if (v == NULL)
		c->c_errors++;
	else {
		int i = com_addconst(c, v);
		int ndefs = com_argdefs(c, n);
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		com_addoparg(c, MAKE_FUNCTION, ndefs);
		com_pop(c, ndefs);
		com_addopname(c, STORE_NAME, CHILD(n, 1));
		com_pop(c, 1);
		Py_DECREF(v);
	}
}

static void
com_bases(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	REQ(n, testlist);
	/* testlist: test (',' test)* [','] */
	for (i = 0; i < NCH(n); i += 2)
		com_node(c, CHILD(n, i));
	i = (NCH(n)+1) / 2;
	com_addoparg(c, BUILD_TUPLE, i);
	com_pop(c, i-1);
}

static void
com_classdef(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	PyObject *v;
	REQ(n, classdef);
	/* classdef: class NAME ['(' testlist ')'] ':' suite */
	if ((v = PyString_InternFromString(STR(CHILD(n, 1)))) == NULL) {
		c->c_errors++;
		return;
	}
	/* Push the class name on the stack */
	i = com_addconst(c, v);
	com_addoparg(c, LOAD_CONST, i);
	com_push(c, 1);
	Py_DECREF(v);
	/* Push the tuple of base classes on the stack */
	if (TYPE(CHILD(n, 2)) != LPAR) {
		com_addoparg(c, BUILD_TUPLE, 0);
		com_push(c, 1);
	}
	else
		com_bases(c, CHILD(n, 3));
	v = (PyObject *)icompile(n, c);
	if (v == NULL)
		c->c_errors++;
	else {
		i = com_addconst(c, v);
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		com_addoparg(c, MAKE_FUNCTION, 0);
		com_addoparg(c, CALL_FUNCTION, 0);
		com_addbyte(c, BUILD_CLASS);
		com_pop(c, 2);
		com_addopname(c, STORE_NAME, CHILD(n, 1));
		Py_DECREF(v);
	}
}

static void
com_node(c, n)
	struct compiling *c;
	node *n;
{
	switch (TYPE(n)) {
	
	/* Definition nodes */
	
	case funcdef:
		com_funcdef(c, n);
		break;
	case classdef:
		com_classdef(c, n);
		break;
	
	/* Trivial parse tree nodes */
	
	case stmt:
	case small_stmt:
	case flow_stmt:
		com_node(c, CHILD(n, 0));
		break;

	case simple_stmt:
		/* small_stmt (';' small_stmt)* [';'] NEWLINE */
		com_addoparg(c, SET_LINENO, n->n_lineno);
		{
			int i;
			for (i = 0; i < NCH(n)-1; i += 2)
				com_node(c, CHILD(n, i));
		}
		break;
	
	case compound_stmt:
		com_addoparg(c, SET_LINENO, n->n_lineno);
		com_node(c, CHILD(n, 0));
		break;

	/* Statement nodes */
	
	case expr_stmt:
		com_expr_stmt(c, n);
		break;
	case print_stmt:
		com_print_stmt(c, n);
		break;
	case del_stmt: /* 'del' exprlist */
		com_assign(c, CHILD(n, 1), OP_DELETE);
		break;
	case pass_stmt:
		break;
	case break_stmt:
		if (c->c_loops == 0) {
			com_error(c, PyExc_SyntaxError,
				  "'break' outside loop");
		}
		com_addbyte(c, BREAK_LOOP);
		break;
	case continue_stmt:
		com_continue_stmt(c, n);
		break;
	case return_stmt:
		com_return_stmt(c, n);
		break;
	case raise_stmt:
		com_raise_stmt(c, n);
		break;
	case import_stmt:
		com_import_stmt(c, n);
		break;
	case global_stmt:
		com_global_stmt(c, n);
		break;
	case exec_stmt:
		com_exec_stmt(c, n);
		break;
	case assert_stmt:
		com_assert_stmt(c, n);
		break;
	case if_stmt:
		com_if_stmt(c, n);
		break;
	case while_stmt:
		com_while_stmt(c, n);
		break;
	case for_stmt:
		com_for_stmt(c, n);
		break;
	case try_stmt:
		com_try_stmt(c, n);
		break;
	case suite:
		com_suite(c, n);
		break;
	
	/* Expression nodes */
	
	case testlist:
		com_list(c, n, 0);
		break;
	case test:
		com_test(c, n);
		break;
	case and_test:
		com_and_test(c, n);
		break;
	case not_test:
		com_not_test(c, n);
		break;
	case comparison:
		com_comparison(c, n);
		break;
	case exprlist:
		com_list(c, n, 0);
		break;
	case expr:
		com_expr(c, n);
		break;
	case xor_expr:
		com_xor_expr(c, n);
		break;
	case and_expr:
		com_and_expr(c, n);
		break;
	case shift_expr:
		com_shift_expr(c, n);
		break;
	case arith_expr:
		com_arith_expr(c, n);
		break;
	case term:
		com_term(c, n);
		break;
	case factor:
		com_factor(c, n);
		break;
	case power:
		com_power(c, n);
		break;
	case atom:
		com_atom(c, n);
		break;
	
	default:
		/* XXX fprintf(stderr, "node type %d\n", TYPE(n)); */
		com_error(c, PyExc_SystemError,
			  "com_node: unexpected node type");
	}
}

static void com_fplist Py_PROTO((struct compiling *, node *));

static void
com_fpdef(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, fpdef); /* fpdef: NAME | '(' fplist ')' */
	if (TYPE(CHILD(n, 0)) == LPAR)
		com_fplist(c, CHILD(n, 1));
	else {
		com_addoparg(c, STORE_FAST, com_newlocal(c, STR(CHILD(n, 0))));
		com_pop(c, 1);
	}
}

static void
com_fplist(c, n)
	struct compiling *c;
	node *n;
{
	REQ(n, fplist); /* fplist: fpdef (',' fpdef)* [','] */
	if (NCH(n) == 1) {
		com_fpdef(c, CHILD(n, 0));
	}
	else {
		int i = (NCH(n)+1)/2;
		com_addoparg(c, UNPACK_TUPLE, i);
		com_push(c, i-1);
		for (i = 0; i < NCH(n); i += 2)
			com_fpdef(c, CHILD(n, i));
	}
}

static void
com_arglist(c, n)
	struct compiling *c;
	node *n;
{
	int nch, i;
	int complex = 0;
	char nbuf[10];
	REQ(n, varargslist);
	/* varargslist:
		(fpdef ['=' test] ',')* (fpdef ['=' test] | '*' .....) */
	nch = NCH(n);
	/* Enter all arguments in table of locals */
	for (i = 0; i < nch; i++) {
		node *ch = CHILD(n, i);
		node *fp;
		char *name;
		if (TYPE(ch) == STAR || TYPE(ch) == DOUBLESTAR)
			break;
		REQ(ch, fpdef); /* fpdef: NAME | '(' fplist ')' */
		fp = CHILD(ch, 0);
		if (TYPE(fp) == NAME)
			name = STR(fp);
		else {
			name = nbuf;
			sprintf(nbuf, ".%d", i);
			complex = 1;
		}
		com_newlocal(c, name);
		c->c_argcount++;
		if (++i >= nch)
			break;
		ch = CHILD(n, i);
		if (TYPE(ch) == EQUAL)
			i += 2;
		else
			REQ(ch, COMMA);
	}
	/* Handle *arguments */
	if (i < nch) {
		node *ch;
		ch = CHILD(n, i);
		if (TYPE(ch) != DOUBLESTAR) {
			REQ(ch, STAR);
			ch = CHILD(n, i+1);
			if (TYPE(ch) == NAME) {
				c->c_flags |= CO_VARARGS;
				i += 3;
				com_newlocal(c, STR(ch));
			}
		}
	}
	/* Handle **keywords */
	if (i < nch) {
		node *ch;
		ch = CHILD(n, i);
		if (TYPE(ch) != DOUBLESTAR) {
			REQ(ch, STAR);
			ch = CHILD(n, i+1);
			REQ(ch, STAR);
			ch = CHILD(n, i+2);
		}
		else
			ch = CHILD(n, i+1);
		REQ(ch, NAME);
		c->c_flags |= CO_VARKEYWORDS;
		com_newlocal(c, STR(ch));
	}
	if (complex) {
		/* Generate code for complex arguments only after
		   having counted the simple arguments */
		int ilocal = 0;
		for (i = 0; i < nch; i++) {
			node *ch = CHILD(n, i);
			node *fp;
			if (TYPE(ch) == STAR || TYPE(ch) == DOUBLESTAR)
				break;
			REQ(ch, fpdef); /* fpdef: NAME | '(' fplist ')' */
			fp = CHILD(ch, 0);
			if (TYPE(fp) != NAME) {
				com_addoparg(c, LOAD_FAST, ilocal);
				com_push(c, 1);
				com_fpdef(c, ch);
			}
			ilocal++;
			if (++i >= nch)
				break;
			ch = CHILD(n, i);
			if (TYPE(ch) == EQUAL)
				i += 2;
			else
				REQ(ch, COMMA);
		}
	}
}

static void
com_file_input(c, n)
	struct compiling *c;
	node *n;
{
	int i;
	PyObject *doc;
	REQ(n, file_input); /* (NEWLINE | stmt)* ENDMARKER */
	doc = get_docstring(n);
	if (doc != NULL) {
		int i = com_addconst(c, doc);
		Py_DECREF(doc);
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		com_addopnamestr(c, STORE_NAME, "__doc__");
		com_pop(c, 1);
	}
	for (i = 0; i < NCH(n); i++) {
		node *ch = CHILD(n, i);
		if (TYPE(ch) != ENDMARKER && TYPE(ch) != NEWLINE)
			com_node(c, ch);
	}
}

/* Top-level compile-node interface */

static void
compile_funcdef(c, n)
	struct compiling *c;
	node *n;
{
	PyObject *doc;
	node *ch;
	REQ(n, funcdef); /* funcdef: 'def' NAME parameters ':' suite */
	c->c_name = STR(CHILD(n, 1));
	doc = get_docstring(CHILD(n, 4));
	if (doc != NULL) {
		(void) com_addconst(c, doc);
		Py_DECREF(doc);
	}
	else
		(void) com_addconst(c, Py_None); /* No docstring */
	ch = CHILD(n, 2); /* parameters: '(' [varargslist] ')' */
	ch = CHILD(ch, 1); /* ')' | varargslist */
	if (TYPE(ch) == varargslist)
		com_arglist(c, ch);
	c->c_infunction = 1;
	com_node(c, CHILD(n, 4));
	c->c_infunction = 0;
	com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
	com_push(c, 1);
	com_addbyte(c, RETURN_VALUE);
	com_pop(c, 1);
}

static void
compile_lambdef(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	REQ(n, lambdef); /* lambdef: 'lambda' [varargslist] ':' test */
	c->c_name = "<lambda>";

	ch = CHILD(n, 1);
	(void) com_addconst(c, Py_None); /* No docstring */
	if (TYPE(ch) == varargslist) {
		com_arglist(c, ch);
		ch = CHILD(n, 3);
	}
	else
		ch = CHILD(n, 2);
	com_node(c, ch);
	com_addbyte(c, RETURN_VALUE);
	com_pop(c, 1);
}

static void
compile_classdef(c, n)
	struct compiling *c;
	node *n;
{
	node *ch;
	PyObject *doc;
	REQ(n, classdef);
	/* classdef: 'class' NAME ['(' testlist ')'] ':' suite */
	c->c_name = STR(CHILD(n, 1));
#ifdef PRIVATE_NAME_MANGLING
	c->c_private = c->c_name;
#endif
	ch = CHILD(n, NCH(n)-1); /* The suite */
	doc = get_docstring(ch);
	if (doc != NULL) {
		int i = com_addconst(c, doc);
		Py_DECREF(doc);
		com_addoparg(c, LOAD_CONST, i);
		com_push(c, 1);
		com_addopnamestr(c, STORE_NAME, "__doc__");
		com_pop(c, 1);
	}
	else
		(void) com_addconst(c, Py_None);
	com_node(c, ch);
	com_addbyte(c, LOAD_LOCALS);
	com_push(c, 1);
	com_addbyte(c, RETURN_VALUE);
	com_pop(c, 1);
}

static void
compile_node(c, n)
	struct compiling *c;
	node *n;
{
	com_addoparg(c, SET_LINENO, n->n_lineno);
	
	switch (TYPE(n)) {
	
	case single_input: /* One interactive command */
		/* NEWLINE | simple_stmt | compound_stmt NEWLINE */
		c->c_interactive++;
		n = CHILD(n, 0);
		if (TYPE(n) != NEWLINE)
			com_node(c, n);
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
		com_addbyte(c, RETURN_VALUE);
		com_pop(c, 1);
		c->c_interactive--;
		break;
	
	case file_input: /* A whole file, or built-in function exec() */
		com_file_input(c, n);
		com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
		com_push(c, 1);
		com_addbyte(c, RETURN_VALUE);
		com_pop(c, 1);
		break;
	
	case eval_input: /* Built-in function input() */
		com_node(c, CHILD(n, 0));
		com_addbyte(c, RETURN_VALUE);
		com_pop(c, 1);
		break;
	
	case lambdef: /* anonymous function definition */
		compile_lambdef(c, n);
		break;

	case funcdef: /* A function definition */
		compile_funcdef(c, n);
		break;
	
	case classdef: /* A class definition */
		compile_classdef(c, n);
		break;
	
	default:
		/* XXX fprintf(stderr, "node type %d\n", TYPE(n)); */
		com_error(c, PyExc_SystemError,
			  "compile_node: unexpected node type");
	}
}

/* Optimization for local variables in functions (and *only* functions).

   This replaces all LOAD_NAME, STORE_NAME and DELETE_NAME
   instructions that refer to local variables with LOAD_FAST etc.
   The latter instructions are much faster because they don't need to
   look up the variable name in a dictionary.

   To find all local variables, we check all STORE_NAME, IMPORT_FROM
   and DELETE_NAME instructions.  This yields all local variables,
   function definitions, class definitions and import statements.
   Argument names have already been entered into the list by the
   special processing for the argument list.

   All remaining LOAD_NAME instructions must refer to non-local (global
   or builtin) variables, so are replaced by LOAD_GLOBAL.

   There are two problems:  'from foo import *' and 'exec' may introduce
   local variables that we can't know while compiling.  If this is the
   case, we can still optimize bona fide locals (since those
   statements will be surrounded by fast_2_locals() and
   locals_2_fast()), but we can't change LOAD_NAME to LOAD_GLOBAL.

   NB: this modifies the string object c->c_code!  */

static void
optimize(c)
	struct compiling *c;
{
	unsigned char *next_instr, *cur_instr;
	int opcode;
	int oparg = 0;
	PyObject *name;
	PyObject *error_type, *error_value, *error_traceback;
	
#define NEXTOP()	(*next_instr++)
#define NEXTARG()	(next_instr += 2, (next_instr[-1]<<8) + next_instr[-2])
#define GETITEM(v, i)	(PyList_GetItem((v), (i)))
#define GETNAMEOBJ(i)	(GETITEM(c->c_names, (i)))
	
	PyErr_Fetch(&error_type, &error_value, &error_traceback);

	c->c_flags |= CO_OPTIMIZED;
	
	next_instr = (unsigned char *) PyString_AsString(c->c_code);
	for (;;) {
		opcode = NEXTOP();
		if (opcode == STOP_CODE)
			break;
		if (HAS_ARG(opcode))
			oparg = NEXTARG();
		switch (opcode) {
		case STORE_NAME:
		case DELETE_NAME:
		case IMPORT_FROM:
			com_addlocal_o(c, GETNAMEOBJ(oparg));
			break;
		case EXEC_STMT:
			c->c_flags &= ~CO_OPTIMIZED;
			break;
		}
	}
	
	if (PyDict_GetItemString(c->c_locals, "*") != NULL)
		c->c_flags &= ~CO_OPTIMIZED;
	
	next_instr = (unsigned char *) PyString_AsString(c->c_code);
	for (;;) {
		cur_instr = next_instr;
		opcode = NEXTOP();
		if (opcode == STOP_CODE)
			break;
		if (HAS_ARG(opcode))
			oparg = NEXTARG();
		if (opcode == LOAD_NAME ||
		    opcode == STORE_NAME ||
		    opcode == DELETE_NAME) {
			PyObject *v;
			int i;
			name = GETNAMEOBJ(oparg);
			v = PyDict_GetItem(c->c_locals, name);
			if (v == NULL) {
				PyErr_Clear();
				if (opcode == LOAD_NAME &&
				    (c->c_flags&CO_OPTIMIZED))
					cur_instr[0] = LOAD_GLOBAL;
				continue;
			}
			i = PyInt_AsLong(v);
			switch (opcode) {
			case LOAD_NAME: cur_instr[0] = LOAD_FAST; break;
			case STORE_NAME: cur_instr[0] = STORE_FAST; break;
			case DELETE_NAME: cur_instr[0] = DELETE_FAST; break;
			}
			cur_instr[1] = i & 0xff;
			cur_instr[2] = (i>>8) & 0xff;
		}
	}

	if (c->c_errors == 0)
		PyErr_Restore(error_type, error_value, error_traceback);
}

PyCodeObject *
PyNode_Compile(n, filename)
	node *n;
	char *filename;
{
	return jcompile(n, filename, NULL);
}

static PyCodeObject *
icompile(n, base)
	node *n;
	struct compiling *base;
{
	return jcompile(n, base->c_filename, base);
}

static PyCodeObject *
jcompile(n, filename, base)
	node *n;
	char *filename;
	struct compiling *base;
{
	struct compiling sc;
	PyCodeObject *co;
	if (!com_init(&sc, filename))
		return NULL;
#ifdef PRIVATE_NAME_MANGLING
	if (base)
		sc.c_private = base->c_private;
	else
		sc.c_private = NULL;
#endif
	compile_node(&sc, n);
	com_done(&sc);
	if ((TYPE(n) == funcdef || TYPE(n) == lambdef) && sc.c_errors == 0) {
		optimize(&sc);
		sc.c_flags |= CO_NEWLOCALS;
	}
	else if (TYPE(n) == classdef)
		sc.c_flags |= CO_NEWLOCALS;
	co = NULL;
	if (sc.c_errors == 0) {
		PyObject *consts, *names, *varnames, *filename, *name;
		consts = PyList_AsTuple(sc.c_consts);
		names = PyList_AsTuple(sc.c_names);
		varnames = PyList_AsTuple(sc.c_varnames);
		filename = PyString_InternFromString(sc.c_filename);
		name = PyString_InternFromString(sc.c_name);
		if (!PyErr_Occurred())
			co = PyCode_New(sc.c_argcount,
					   sc.c_nlocals,
					   sc.c_maxstacklevel,
					   sc.c_flags,
					   sc.c_code,
					   consts,
					   names,
					   varnames,
					   filename,
					   name,
					   sc.c_firstlineno,
					   sc.c_lnotab);
		Py_XDECREF(consts);
		Py_XDECREF(names);
		Py_XDECREF(varnames);
		Py_XDECREF(filename);
		Py_XDECREF(name);
	}
	com_free(&sc);
	return co;
}

int
PyCode_Addr2Line(co, addrq)
	PyCodeObject *co;
	int addrq;
{
	int size = PyString_Size(co->co_lnotab) / 2;
	char *p = PyString_AsString(co->co_lnotab);
	int line = co->co_firstlineno;
	int addr = 0;
	while (--size >= 0) {
		addr += *p++;
		if (addr > addrq)
			break;
		line += *p++;
	}
	return line;
}
