
/* Compile an expression node to intermediate code */

/* XXX TO DO:
   XXX add __doc__ attribute == co_doc to code object attributes?
   XXX   (it's currently the first item of the co_const tuple)
   XXX Generate simple jump for break/return outside 'try...finally'
   XXX Allow 'continue' inside try-finally
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
code_getattr(PyCodeObject *co, char *name)
{
	return PyMember_Get((char *)co, code_memberlist, name);
}

static void
code_dealloc(PyCodeObject *co)
{
	Py_XDECREF(co->co_code);
	Py_XDECREF(co->co_consts);
	Py_XDECREF(co->co_names);
	Py_XDECREF(co->co_varnames);
	Py_XDECREF(co->co_filename);
	Py_XDECREF(co->co_name);
	Py_XDECREF(co->co_lnotab);
	PyObject_DEL(co);
}

static PyObject *
code_repr(PyCodeObject *co)
{
	char buf[500];
	int lineno = -1;
	char *filename = "???";
	char *name = "???";

	if (co->co_firstlineno != 0)
		lineno = co->co_firstlineno;
	if (co->co_filename && PyString_Check(co->co_filename))
		filename = PyString_AsString(co->co_filename);
	if (co->co_name && PyString_Check(co->co_name))
		name = PyString_AsString(co->co_name);
	sprintf(buf, "<code object %.100s at %p, file \"%.300s\", line %d>",
		name, co, filename, lineno);
	return PyString_FromString(buf);
}

static int
code_compare(PyCodeObject *co, PyCodeObject *cp)
{
	int cmp;
	cmp = PyObject_Compare(co->co_name, cp->co_name);
	if (cmp) return cmp;
	cmp = co->co_argcount - cp->co_argcount;
	if (cmp) return cmp;
	cmp = co->co_nlocals - cp->co_nlocals;
	if (cmp) return cmp;
	cmp = co->co_flags - cp->co_flags;
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_code, cp->co_code);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_consts, cp->co_consts);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_names, cp->co_names);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_varnames, cp->co_varnames);
	return cmp;
}

static long
code_hash(PyCodeObject *co)
{
	long h, h0, h1, h2, h3, h4;
	h0 = PyObject_Hash(co->co_name);
	if (h0 == -1) return -1;
	h1 = PyObject_Hash(co->co_code);
	if (h1 == -1) return -1;
	h2 = PyObject_Hash(co->co_consts);
	if (h2 == -1) return -1;
	h3 = PyObject_Hash(co->co_names);
	if (h3 == -1) return -1;
	h4 = PyObject_Hash(co->co_varnames);
	if (h4 == -1) return -1;
	h = h0 ^ h1 ^ h2 ^ h3 ^ h4 ^
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
PyCode_New(int argcount, int nlocals, int stacksize, int flags,
	   PyObject *code, PyObject *consts, PyObject *names,
	   PyObject *varnames, PyObject *filename, PyObject *name,
	   int firstlineno, PyObject *lnotab)
{
	PyCodeObject *co;
	int i;
	PyBufferProcs *pb;
	/* Check argument types */
	if (argcount < 0 || nlocals < 0 ||
	    code == NULL ||
	    consts == NULL || !PyTuple_Check(consts) ||
	    names == NULL || !PyTuple_Check(names) ||
	    varnames == NULL || !PyTuple_Check(varnames) ||
	    name == NULL || !PyString_Check(name) ||
	    filename == NULL || !PyString_Check(filename) ||
		lnotab == NULL || !PyString_Check(lnotab)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	pb = code->ob_type->tp_as_buffer;
	if (pb == NULL ||
	    pb->bf_getreadbuffer == NULL ||
	    pb->bf_getsegcount == NULL ||
	    (*pb->bf_getsegcount)(code, NULL) != 1)
	{
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
		if (strspn(p, NAME_CHARS)
		    != (size_t)PyString_Size(v))
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
		co->co_code = code;
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
	PyObject *c_code;	/* string */
	PyObject *c_consts;	/* list of objects */
	PyObject *c_const_dict; /* inverse of c_consts */
	PyObject *c_names;	/* list of strings (names) */
	PyObject *c_name_dict;  /* inverse of c_names */
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
	int c_tmpname;		/* temporary local name counter */
};


/* Error message including line number */

static void
com_error(struct compiling *c, PyObject *exc, char *msg)
{
	PyObject *v, *tb, *tmp;
	c->c_errors++;
	if (c->c_lineno <= 1) {
		/* Unknown line number or single interactive command */
		PyErr_SetString(exc, msg);
		return;
	}
	v = PyString_FromString(msg);
	if (v == NULL)
		return; /* MemoryError, too bad */
	PyErr_SetObject(exc, v);
	Py_DECREF(v);

	/* add attributes for the line number and filename for the error */
	PyErr_Fetch(&exc, &v, &tb);
	PyErr_NormalizeException(&exc, &v, &tb);
	tmp = PyInt_FromLong(c->c_lineno);
	if (tmp == NULL)
		PyErr_Clear();
	else {
		if (PyObject_SetAttrString(v, "lineno", tmp))
			PyErr_Clear();
		Py_DECREF(tmp);
	}
	if (c->c_filename != NULL) {
		tmp = PyString_FromString(c->c_filename);
		if (tmp == NULL)
			PyErr_Clear();
		else {
			if (PyObject_SetAttrString(v, "filename", tmp))
				PyErr_Clear();
			Py_DECREF(tmp);
		}
	}
	PyErr_Restore(exc, v, tb);
}


/* Interface to the block stack */

static void
block_push(struct compiling *c, int type)
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
block_pop(struct compiling *c, int type)
{
	if (c->c_nblocks > 0)
		c->c_nblocks--;
	if (c->c_block[c->c_nblocks] != type && c->c_errors == 0) {
		com_error(c, PyExc_SystemError, "bad block pop");
	}
}


/* Prototype forward declarations */

static int com_init(struct compiling *, char *);
static void com_free(struct compiling *);
static void com_push(struct compiling *, int);
static void com_pop(struct compiling *, int);
static void com_done(struct compiling *);
static void com_node(struct compiling *, struct _node *);
static void com_factor(struct compiling *, struct _node *);
static void com_addbyte(struct compiling *, int);
static void com_addint(struct compiling *, int);
static void com_addoparg(struct compiling *, int, int);
static void com_addfwref(struct compiling *, int, int *);
static void com_backpatch(struct compiling *, int);
static int com_add(struct compiling *, PyObject *, PyObject *, PyObject *);
static int com_addconst(struct compiling *, PyObject *);
static int com_addname(struct compiling *, PyObject *);
static void com_addopname(struct compiling *, int, node *);
static void com_list(struct compiling *, node *, int);
static void com_list_iter(struct compiling *, node *, node *, char *);
static int com_argdefs(struct compiling *, node *);
static int com_newlocal(struct compiling *, char *);
static void com_assign(struct compiling *, node *, int, node *);
static void com_assign_name(struct compiling *, node *, int);
static PyCodeObject *icompile(struct _node *, struct compiling *);
static PyCodeObject *jcompile(struct _node *, char *,
			      struct compiling *);
static PyObject *parsestrplus(node *);
static PyObject *parsestr(char *);
static node *get_rawdocstring(node *);

static int
com_init(struct compiling *c, char *filename)
{
	memset((void *)c, '\0', sizeof(struct compiling));
	if ((c->c_code = PyString_FromStringAndSize((char *)NULL,
						    1000)) == NULL)
		goto fail;
	if ((c->c_consts = PyList_New(0)) == NULL)
		goto fail;
	if ((c->c_const_dict = PyDict_New()) == NULL)
		goto fail;
	if ((c->c_names = PyList_New(0)) == NULL)
		goto fail;
	if ((c->c_name_dict = PyDict_New()) == NULL)
		goto fail;
	if ((c->c_globals = PyDict_New()) == NULL)
		goto fail;
	if ((c->c_locals = PyDict_New()) == NULL)
		goto fail;
	if ((c->c_varnames = PyList_New(0)) == NULL)
		goto fail;
	if ((c->c_lnotab = PyString_FromStringAndSize((char *)NULL,
						      1000)) == NULL)
		goto fail;
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
	c->c_tmpname = 0;
	return 1;
	
  fail:
	com_free(c);
 	return 0;
}

static void
com_free(struct compiling *c)
{
	Py_XDECREF(c->c_code);
	Py_XDECREF(c->c_consts);
	Py_XDECREF(c->c_const_dict);
	Py_XDECREF(c->c_names);
	Py_XDECREF(c->c_name_dict);
	Py_XDECREF(c->c_globals);
	Py_XDECREF(c->c_locals);
	Py_XDECREF(c->c_varnames);
	Py_XDECREF(c->c_lnotab);
}

static void
com_push(struct compiling *c, int n)
{
	c->c_stacklevel += n;
	if (c->c_stacklevel > c->c_maxstacklevel)
		c->c_maxstacklevel = c->c_stacklevel;
}

static void
com_pop(struct compiling *c, int n)
{
	if (c->c_stacklevel < n) {
		/* fprintf(stderr,
			"%s:%d: underflow! nexti=%d, level=%d, n=%d\n",
			c->c_filename, c->c_lineno,
			c->c_nexti, c->c_stacklevel, n); */
		c->c_stacklevel = 0;
	}
	else
		c->c_stacklevel -= n;
}

static void
com_done(struct compiling *c)
{
	if (c->c_code != NULL)
		_PyString_Resize(&c->c_code, c->c_nexti);
	if (c->c_lnotab != NULL)
		_PyString_Resize(&c->c_lnotab, c->c_lnotab_next);
}

static void
com_addbyte(struct compiling *c, int byte)
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
com_addint(struct compiling *c, int x)
{
	com_addbyte(c, x & 0xff);
	com_addbyte(c, x >> 8); /* XXX x should be positive */
}

static void
com_add_lnotab(struct compiling *c, int addr, int line)
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
com_set_lineno(struct compiling *c, int lineno)
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
com_addoparg(struct compiling *c, int op, int arg)
{
	int extended_arg = arg >> 16;
	if (op == SET_LINENO) {
		com_set_lineno(c, arg);
		if (Py_OptimizeFlag)
			return;
	}
	if (extended_arg){
		com_addbyte(c, EXTENDED_ARG);
		com_addint(c, extended_arg);
		arg &= 0xffff;
	}
	com_addbyte(c, op);
	com_addint(c, arg);
}

static void
com_addfwref(struct compiling *c, int op, int *p_anchor)
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
com_backpatch(struct compiling *c, int anchor)
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
		dist >>= 8;
		code[anchor+1] = dist;
		dist >>= 8;
		if (dist) {
			com_error(c, PyExc_SystemError,
				  "com_backpatch: offset too large");
			break;
		}
		if (!prev)
			break;
		anchor -= prev;
	}
}

/* Handle literals and names uniformly */

static int
com_add(struct compiling *c, PyObject *list, PyObject *dict, PyObject *v)
{
	PyObject *w, *t, *np=NULL;
	long n;

	t = Py_BuildValue("(OO)", v, v->ob_type);
	if (t == NULL)
	    goto fail;
	w = PyDict_GetItem(dict, t);
	if (w != NULL) {
		n = PyInt_AsLong(w);
	} else {
		n = PyList_Size(list);
		np = PyInt_FromLong(n);
		if (np == NULL)
		    goto fail;
		if (PyList_Append(list, v) != 0)
		    goto fail;
		if (PyDict_SetItem(dict, t, np) != 0)
		    goto fail;
		Py_DECREF(np);
	}
	Py_DECREF(t);
	return n;
  fail:
	Py_XDECREF(np);
	Py_XDECREF(t);
	c->c_errors++;
	return 0;
}

static int
com_addconst(struct compiling *c, PyObject *v)
{
	return com_add(c, c->c_consts, c->c_const_dict, v);
}

static int
com_addname(struct compiling *c, PyObject *v)
{
	return com_add(c, c->c_names, c->c_name_dict, v);
}

#ifdef PRIVATE_NAME_MANGLING
static int
com_mangle(struct compiling *c, char *name, char *buffer, size_t maxlen)
{
	/* Name mangling: __private becomes _classname__private.
	   This is independent from how the name is used. */
	char *p;
	size_t nlen, plen;
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
com_addopnamestr(struct compiling *c, int op, char *name)
{
	PyObject *v;
	int i;
#ifdef PRIVATE_NAME_MANGLING
	char buffer[256];
	if (name != NULL && name[0] == '_' && name[1] == '_' &&
	    c->c_private != NULL &&
	    com_mangle(c, name, buffer, sizeof(buffer)))
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
com_addopname(struct compiling *c, int op, node *n)
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
parsenumber(struct compiling *co, char *s)
{
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
	else
#endif
	{
		PyFPE_START_PROTECT("atof", return 0)
		dx = atof(s);
		PyFPE_END_PROTECT(dx)
		return PyFloat_FromDouble(dx);
	}
}

static PyObject *
parsestr(char *s)
{
	PyObject *v;
	size_t len;
	char *buf;
	char *p;
	char *end;
	int c;
	int first = *s;
	int quote = first;
	int rawmode = 0;
	int unicode = 0;
	if (isalpha(quote) || quote == '_') {
		if (quote == 'u' || quote == 'U') {
			quote = *++s;
			unicode = 1;
		}
		if (quote == 'r' || quote == 'R') {
			quote = *++s;
			rawmode = 1;
		}
	}
	if (quote != '\'' && quote != '\"') {
		PyErr_BadInternalCall();
		return NULL;
	}
	s++;
	len = strlen(s);
	if (len > INT_MAX) {
		PyErr_SetString(PyExc_OverflowError, "string to parse is too long");
		return NULL;
	}
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
	if (unicode || Py_UnicodeFlag) {
		if (rawmode)
			return PyUnicode_DecodeRawUnicodeEscape(
				s, len, NULL);
		else
			return PyUnicode_DecodeUnicodeEscape(
				s, len, NULL);
	}
	if (rawmode || strchr(s, '\\') == NULL)
		return PyString_FromStringAndSize(s, len);
	v = PyString_FromStringAndSize((char *)NULL, len);
	if (v == NULL)
		return NULL;
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
			if (isxdigit(Py_CHARMASK(s[0])) && isxdigit(Py_CHARMASK(s[1]))) {
				unsigned int x = 0;
				c = Py_CHARMASK(*s);
				s++;
				if (isdigit(c))
					x = c - '0';
				else if (islower(c))
					x = 10 + c - 'a';
				else
					x = 10 + c - 'A';
				x = x << 4;
				c = Py_CHARMASK(*s);
				s++;
				if (isdigit(c))
					x += c - '0';
				else if (islower(c))
					x += 10 + c - 'a';
				else
					x += 10 + c - 'A';
				*p++ = x;
				break;
			}
			PyErr_SetString(PyExc_ValueError, "invalid \\x escape");
			Py_DECREF(v);
			return NULL;
		default:
			*p++ = '\\';
			*p++ = s[-1];
			break;
		}
	}
	_PyString_Resize(&v, (int)(p - buf));
	return v;
}

static PyObject *
parsestrplus(node *n)
{
	PyObject *v;
	int i;
	REQ(CHILD(n, 0), STRING);
	if ((v = parsestr(STR(CHILD(n, 0)))) != NULL) {
		/* String literal concatenation */
		for (i = 1; i < NCH(n); i++) {
		    PyObject *s;
		    s = parsestr(STR(CHILD(n, i)));
		    if (s == NULL)
			goto onError;
		    if (PyString_Check(v) && PyString_Check(s)) {
			PyString_ConcatAndDel(&v, s);
			if (v == NULL)
			    goto onError;
		    }
		    else {
			PyObject *temp;
			temp = PyUnicode_Concat(v, s);
			Py_DECREF(s);
			if (temp == NULL)
			    goto onError;
			Py_DECREF(v);
			v = temp;
		    }
		}
	}
	return v;

 onError:
	Py_XDECREF(v);
	return NULL;
}

static void
com_list_for(struct compiling *c, node *n, node *e, char *t)
{
	PyObject *v;
	int anchor = 0;
	int save_begin = c->c_begin;

	/* list_iter: for v in expr [list_iter] */
	com_node(c, CHILD(n, 3)); /* expr */
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
	com_assign(c, CHILD(n, 1), OP_ASSIGN, NULL);
	c->c_loops++;
	com_list_iter(c, n, e, t);
	c->c_loops--;
	com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	c->c_begin = save_begin;
	com_backpatch(c, anchor);
	com_pop(c, 2); /* FOR_LOOP has popped these */
}  

static void
com_list_if(struct compiling *c, node *n, node *e, char *t)
{
	int anchor = 0;
	int a = 0;
	/* list_iter: 'if' test [list_iter] */
	com_addoparg(c, SET_LINENO, n->n_lineno);
	com_node(c, CHILD(n, 1));
	com_addfwref(c, JUMP_IF_FALSE, &a);
	com_addbyte(c, POP_TOP);
	com_pop(c, 1);
	com_list_iter(c, n, e, t);
	com_addfwref(c, JUMP_FORWARD, &anchor);
	com_backpatch(c, a);
	/* We jump here with an extra entry which we now pop */
	com_addbyte(c, POP_TOP);
	com_backpatch(c, anchor);
}

static void
com_list_iter(struct compiling *c,
	      node *p,		/* parent of list_iter node */
	      node *e,		/* element expression node */
	      char *t		/* name of result list temp local */)
{
	/* list_iter is the last child in a listmaker, list_for, or list_if */
	node *n = CHILD(p, NCH(p)-1);
	if (TYPE(n) == list_iter) {
		n = CHILD(n, 0);
		switch (TYPE(n)) {
		case list_for: 
			com_list_for(c, n, e, t);
			break;
		case list_if:
			com_list_if(c, n, e, t);
			break;
		default:
			com_error(c, PyExc_SystemError,
				  "invalid list_iter node type");
		}
	}
	else {
		com_addopnamestr(c, LOAD_NAME, t);
		com_push(c, 1);
		com_node(c, e);
		com_addoparg(c, CALL_FUNCTION, 1);
		com_addbyte(c, POP_TOP);
		com_pop(c, 2);
	}
}

static void
com_list_comprehension(struct compiling *c, node *n)
{
	/* listmaker: test list_for */
	char tmpname[12];
	sprintf(tmpname, "__%d__", ++c->c_tmpname);
	com_addoparg(c, BUILD_LIST, 0);
	com_addbyte(c, DUP_TOP); /* leave the result on the stack */
	com_push(c, 2);
	com_addopnamestr(c, LOAD_ATTR, "append");
	com_addopnamestr(c, STORE_NAME, tmpname);
	com_pop(c, 1);
	com_list_for(c, CHILD(n, 1), CHILD(n, 0), tmpname);
	com_addopnamestr(c, DELETE_NAME, tmpname);
	--c->c_tmpname;
}

static void
com_listmaker(struct compiling *c, node *n)
{
	/* listmaker: test ( list_for | (',' test)* [','] ) */
	if (NCH(n) > 1 && TYPE(CHILD(n, 1)) == list_for)
		com_list_comprehension(c, n);
	else {
		int len = 0;
		int i;
		for (i = 0; i < NCH(n); i += 2, len++)
			com_node(c, CHILD(n, i));
		com_addoparg(c, BUILD_LIST, len);
		com_pop(c, len-1);
	}
}

static void
com_dictmaker(struct compiling *c, node *n)
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
com_atom(struct compiling *c, node *n)
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
	case LSQB: /* '[' [listmaker] ']' */
		if (TYPE(CHILD(n, 1)) == RSQB) {
			com_addoparg(c, BUILD_LIST, 0);
			com_push(c, 1);
		}
		else
			com_listmaker(c, CHILD(n, 1));
		break;
	case LBRACE: /* '{' [dictmaker] '}' */
		com_addoparg(c, BUILD_MAP, 0);
		com_push(c, 1);
		if (TYPE(CHILD(n, 1)) == dictmaker)
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
com_slice(struct compiling *c, node *n, int op)
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
com_augassign_slice(struct compiling *c, node *n, int opcode, node *augn)
{
	if (NCH(n) == 1) {
		com_addbyte(c, DUP_TOP);
		com_push(c, 1);
		com_addbyte(c, SLICE);
		com_node(c, augn);
		com_addbyte(c, opcode);
		com_pop(c, 1);
		com_addbyte(c, ROT_TWO);
		com_addbyte(c, STORE_SLICE);
		com_pop(c, 2);
	} else if (NCH(n) == 2 && TYPE(CHILD(n, 0)) != COLON) {
		com_node(c, CHILD(n, 0));
		com_addoparg(c, DUP_TOPX, 2);
		com_push(c, 2);
		com_addbyte(c, SLICE+1);
		com_pop(c, 1);
		com_node(c, augn);
		com_addbyte(c, opcode);
		com_pop(c, 1);
		com_addbyte(c, ROT_THREE);
		com_addbyte(c, STORE_SLICE+1);
		com_pop(c, 3);
	} else if (NCH(n) == 2) {
		com_node(c, CHILD(n, 1));
		com_addoparg(c, DUP_TOPX, 2);
		com_push(c, 2);
		com_addbyte(c, SLICE+2);
		com_pop(c, 1);
		com_node(c, augn);
		com_addbyte(c, opcode);
		com_pop(c, 1);
		com_addbyte(c, ROT_THREE);
		com_addbyte(c, STORE_SLICE+2);
		com_pop(c, 3);
	} else {
		com_node(c, CHILD(n, 0));
		com_node(c, CHILD(n, 2));
		com_addoparg(c, DUP_TOPX, 3);
		com_push(c, 3);
		com_addbyte(c, SLICE+3);
		com_pop(c, 2);
		com_node(c, augn);
		com_addbyte(c, opcode);
		com_pop(c, 1);
		com_addbyte(c, ROT_FOUR);
		com_addbyte(c, STORE_SLICE+3);
		com_pop(c, 4);
	}
}

static void
com_argument(struct compiling *c, node *n, PyObject **pkeywords)
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
com_call_function(struct compiling *c, node *n)
{
	if (TYPE(n) == RPAR) {
		com_addoparg(c, CALL_FUNCTION, 0);
	}
	else {
		PyObject *keywords = NULL;
		int i, na, nk;
		int lineno = n->n_lineno;
		int star_flag = 0;
		int starstar_flag = 0;
		int opcode;
		REQ(n, arglist);
		na = 0;
		nk = 0;
		for (i = 0; i < NCH(n); i += 2) {
			node *ch = CHILD(n, i);
			if (TYPE(ch) == STAR ||
			    TYPE(ch) == DOUBLESTAR)
			  break;
			if (ch->n_lineno != lineno) {
				lineno = ch->n_lineno;
				com_addoparg(c, SET_LINENO, lineno);
			}
			com_argument(c, ch, &keywords);
			if (keywords == NULL)
				na++;
			else
				nk++;
		}
		Py_XDECREF(keywords);
		while (i < NCH(n)) {
		    node *tok = CHILD(n, i);
		    node *ch = CHILD(n, i+1);
		    i += 3;
		    switch (TYPE(tok)) {
		    case STAR:       star_flag = 1;     break;
		    case DOUBLESTAR: starstar_flag = 1;	break;
		    }
		    com_node(c, ch);
		}
		if (na > 255 || nk > 255) {
			com_error(c, PyExc_SyntaxError,
				  "more than 255 arguments");
		}
		if (star_flag || starstar_flag)
		    opcode = CALL_FUNCTION_VAR - 1 + 
			star_flag + (starstar_flag << 1);
		else
		    opcode = CALL_FUNCTION;
		com_addoparg(c, opcode, na | (nk << 8));
		com_pop(c, na + 2*nk + star_flag + starstar_flag);
	}
}

static void
com_select_member(struct compiling *c, node *n)
{
	com_addopname(c, LOAD_ATTR, n);
}

static void
com_sliceobj(struct compiling *c, node *n)
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
com_subscript(struct compiling *c, node *n)
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
com_subscriptlist(struct compiling *c, node *n, int assigning, node *augn)
{
	int i, op;
	REQ(n, subscriptlist);
	/* Check to make backward compatible slice behavior for '[i:j]' */
	if (NCH(n) == 1) {
		node *sub = CHILD(n, 0); /* subscript */
		/* 'Basic' slice, should have exactly one colon. */
		if ((TYPE(CHILD(sub, 0)) == COLON
		     || (NCH(sub) > 1 && TYPE(CHILD(sub, 1)) == COLON))
		    && (TYPE(CHILD(sub,NCH(sub)-1)) != sliceop))
		{
			switch (assigning) {
			case OP_DELETE:
				op = DELETE_SLICE;
				break;
			case OP_ASSIGN:
				op = STORE_SLICE;
				break;
			case OP_APPLY:
				op = SLICE;
				break;
			default:
				com_augassign_slice(c, sub, assigning, augn);
				return;
			}
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
	switch (assigning) {
	case OP_DELETE:
		op = DELETE_SUBSCR;
		i = 2;
		break;
	default:
	case OP_ASSIGN:
		op = STORE_SUBSCR;
		i = 3;
		break;
	case OP_APPLY:
		op = BINARY_SUBSCR;
		i = 1;
		break;
	}
	if (assigning > OP_APPLY) {
		com_addoparg(c, DUP_TOPX, 2);
		com_push(c, 2);
		com_addbyte(c, BINARY_SUBSCR);
		com_pop(c, 1);
		com_node(c, augn);
		com_addbyte(c, assigning);
		com_pop(c, 1);
		com_addbyte(c, ROT_THREE);
	}
	com_addbyte(c, op);
	com_pop(c, i);
}

static void
com_apply_trailer(struct compiling *c, node *n)
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
		com_subscriptlist(c, CHILD(n, 1), OP_APPLY, NULL);
		break;
	default:
		com_error(c, PyExc_SystemError,
			  "com_apply_trailer: unknown trailer type");
	}
}

static void
com_power(struct compiling *c, node *n)
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
com_factor(struct compiling *c, node *n)
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
com_term(struct compiling *c, node *n)
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
com_arith_expr(struct compiling *c, node *n)
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
com_shift_expr(struct compiling *c, node *n)
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
com_and_expr(struct compiling *c, node *n)
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
com_xor_expr(struct compiling *c, node *n)
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
com_expr(struct compiling *c, node *n)
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
cmp_type(node *n)
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
com_comparison(struct compiling *c, node *n)
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
com_not_test(struct compiling *c, node *n)
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
com_and_test(struct compiling *c, node *n)
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
com_test(struct compiling *c, node *n)
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
com_list(struct compiling *c, node *n, int toplevel)
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


static void
com_augassign_attr(struct compiling *c, node *n, int opcode, node *augn)
{
	com_addbyte(c, DUP_TOP);
	com_push(c, 1);
	com_addopname(c, LOAD_ATTR, n);
	com_node(c, augn);
	com_addbyte(c, opcode);
	com_pop(c, 1);
	com_addbyte(c, ROT_TWO);
	com_addopname(c, STORE_ATTR, n);
	com_pop(c, 2);
}

static void
com_assign_attr(struct compiling *c, node *n, int assigning)
{
	com_addopname(c, assigning ? STORE_ATTR : DELETE_ATTR, n);
	com_pop(c, assigning ? 2 : 1);
}

static void
com_assign_trailer(struct compiling *c, node *n, int assigning, node *augn)
{
	REQ(n, trailer);
	switch (TYPE(CHILD(n, 0))) {
	case LPAR: /* '(' [exprlist] ')' */
		com_error(c, PyExc_SyntaxError,
			  "can't assign to function call");
		break;
	case DOT: /* '.' NAME */
		if (assigning > OP_APPLY)
			com_augassign_attr(c, CHILD(n, 1), assigning, augn);
		else
			com_assign_attr(c, CHILD(n, 1), assigning);
		break;
	case LSQB: /* '[' subscriptlist ']' */
		com_subscriptlist(c, CHILD(n, 1), assigning, augn);
		break;
	default:
		com_error(c, PyExc_SystemError, "unknown trailer type");
	}
}

static void
com_assign_sequence(struct compiling *c, node *n, int assigning)
{
	int i;
	if (TYPE(n) != testlist && TYPE(n) != listmaker)
		REQ(n, exprlist);
	if (assigning) {
		i = (NCH(n)+1)/2;
		com_addoparg(c, UNPACK_SEQUENCE, i);
		com_push(c, i-1);
	}
	for (i = 0; i < NCH(n); i += 2)
		com_assign(c, CHILD(n, i), assigning, NULL);
}

static void
com_augassign_name(struct compiling *c, node *n, int opcode, node *augn)
{
	REQ(n, NAME);
	com_addopname(c, LOAD_NAME, n);
	com_push(c, 1);
	com_node(c, augn);
	com_addbyte(c, opcode);
	com_pop(c, 1);
	com_assign_name(c, n, OP_ASSIGN);
}

static void
com_assign_name(struct compiling *c, node *n, int assigning)
{
	REQ(n, NAME);
	com_addopname(c, assigning ? STORE_NAME : DELETE_NAME, n);
	if (assigning)
		com_pop(c, 1);
}

static void
com_assign(struct compiling *c, node *n, int assigning, node *augn)
{
	/* Loop to avoid trivial recursion */
	for (;;) {
		switch (TYPE(n)) {
		
		case exprlist:
		case testlist:
			if (NCH(n) > 1) {
				if (assigning > OP_APPLY) {
					com_error(c, PyExc_SyntaxError,
						  "augmented assign to tuple not possible");
					return;
				}
				com_assign_sequence(c, n, assigning);
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
						CHILD(n, i), assigning, augn);
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
				if (assigning > OP_APPLY) {
					com_error(c, PyExc_SyntaxError,
						  "augmented assign to tuple not possible");
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
				if (assigning > OP_APPLY) {
					com_error(c, PyExc_SyntaxError,
						  "augmented assign to list not possible");
					return;
				}
				com_assign_sequence(c, n, assigning);
				return;
			case NAME:
				if (assigning > OP_APPLY)
					com_augassign_name(c, CHILD(n, 0),
							   assigning, augn);
				else
					com_assign_name(c, CHILD(n, 0),
							assigning);
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

static void
com_augassign(struct compiling *c, node *n)
{
	int opcode;

	switch (STR(CHILD(CHILD(n, 1), 0))[0]) {
	case '+': opcode = INPLACE_ADD; break;
	case '-': opcode = INPLACE_SUBTRACT; break;
	case '/': opcode = INPLACE_DIVIDE; break;
	case '%': opcode = INPLACE_MODULO; break;
	case '<': opcode = INPLACE_LSHIFT; break;
	case '>': opcode = INPLACE_RSHIFT; break;
	case '&': opcode = INPLACE_AND; break;
	case '^': opcode = INPLACE_XOR; break;
	case '|': opcode = INPLACE_OR; break;
	case '*':
		if (STR(CHILD(CHILD(n, 1), 0))[1] == '*')
			opcode = INPLACE_POWER;
		else
			opcode = INPLACE_MULTIPLY;
		break;
	default:
		com_error(c, PyExc_SystemError, "com_augassign: bad operator");
		return;
	}
	com_assign(c, CHILD(n, 0), opcode, CHILD(n, 2));
}

static void
com_expr_stmt(struct compiling *c, node *n)
{
	REQ(n, expr_stmt);
	/* testlist (('=' testlist)* | augassign testlist) */
	/* Forget it if we have just a doc string here */
	if (!c->c_interactive && NCH(n) == 1 && get_rawdocstring(n) != NULL)
		return;
 	if (NCH(n) == 1) {
		com_node(c, CHILD(n, NCH(n)-1));
		if (c->c_interactive)
			com_addbyte(c, PRINT_EXPR);
		else
			com_addbyte(c, POP_TOP);
		com_pop(c, 1);
	}
	else if (TYPE(CHILD(n,1)) == augassign)
		com_augassign(c, n);
	else {
		int i;
		com_node(c, CHILD(n, NCH(n)-1));
		for (i = 0; i < NCH(n)-2; i+=2) {
			if (i+2 < NCH(n)-2) {
				com_addbyte(c, DUP_TOP);
				com_push(c, 1);
			}
			com_assign(c, CHILD(n, i), OP_ASSIGN, NULL);
		}
	}
}

static void
com_assert_stmt(struct compiling *c, node *n)
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
com_print_stmt(struct compiling *c, node *n)
{
	int i = 1;
	node* stream = NULL;

	REQ(n, print_stmt); /* 'print' (test ',')* [test] */

	/* are we using the extended print form? */
	if (NCH(n) >= 2 && TYPE(CHILD(n, 1)) == RIGHTSHIFT) {
		stream = CHILD(n, 2);
		com_node(c, stream);
		/* stack: [...] => [... stream] */
		com_push(c, 1);
		if (NCH(n) > 3 && TYPE(CHILD(n, 3)) == COMMA)
			i = 4;
		else
			i = 3;
	}
	for (; i < NCH(n); i += 2) {
		if (stream != NULL) {
			com_addbyte(c, DUP_TOP);
			/* stack: [stream] => [stream stream] */
			com_push(c, 1);
			com_node(c, CHILD(n, i));
			/* stack: [stream stream] => [stream stream obj] */
			com_addbyte(c, ROT_TWO);
			/* stack: [stream stream obj] => [stream obj stream] */
			com_addbyte(c, PRINT_ITEM_TO);
			/* stack: [stream obj stream] => [stream] */
			com_pop(c, 2);
		}
		else {
			com_node(c, CHILD(n, i));
			/* stack: [...] => [... obj] */
			com_addbyte(c, PRINT_ITEM);
			com_pop(c, 1);
		}
	}
	/* XXX Alternatively, LOAD_CONST '\n' and then PRINT_ITEM */
	if (TYPE(CHILD(n, NCH(n)-1)) == COMMA) {
		if (stream != NULL) {
			/* must pop the extra stream object off the stack */
			com_addbyte(c, POP_TOP);
			/* stack: [... stream] => [...] */
			com_pop(c, 1);
		}
	}
	else {
		if (stream != NULL) {
			/* this consumes the last stream object on stack */
			com_addbyte(c, PRINT_NEWLINE_TO);
			/* stack: [... stream] => [...] */
			com_pop(c, 1);
		}
		else
			com_addbyte(c, PRINT_NEWLINE);
	}
}

static void
com_return_stmt(struct compiling *c, node *n)
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
com_raise_stmt(struct compiling *c, node *n)
{
	int i;
	REQ(n, raise_stmt); /* 'raise' [test [',' test [',' test]]] */
	if (NCH(n) > 1) {
		com_node(c, CHILD(n, 1));
		if (NCH(n) > 3) {
			com_node(c, CHILD(n, 3));
			if (NCH(n) > 5)
				com_node(c, CHILD(n, 5));
		}
	}
	i = NCH(n)/2;
	com_addoparg(c, RAISE_VARARGS, i);
	com_pop(c, i);
}

static void
com_from_import(struct compiling *c, node *n)
{
	com_addopname(c, IMPORT_FROM, CHILD(n, 0));
	com_push(c, 1);
	if (NCH(n) > 1) {
		if (strcmp(STR(CHILD(n, 1)), "as") != 0) {
			com_error(c, PyExc_SyntaxError, "invalid syntax");
			return;
		}
		com_addopname(c, STORE_NAME, CHILD(n, 2));
	} else
		com_addopname(c, STORE_NAME, CHILD(n, 0));
	com_pop(c, 1);
}

static void
com_import_stmt(struct compiling *c, node *n)
{
	int i;
	REQ(n, import_stmt);
	/* 'import' dotted_name (',' dotted_name)* |
	   'from' dotted_name 'import' ('*' | NAME (',' NAME)*) */
	if (STR(CHILD(n, 0))[0] == 'f') {
		PyObject *tup;
		/* 'from' dotted_name 'import' ... */
		REQ(CHILD(n, 1), dotted_name);
		
		if (TYPE(CHILD(n, 3)) == STAR) {
			tup = Py_BuildValue("(s)", "*");
		} else {
			tup = PyTuple_New((NCH(n) - 2)/2);
			for (i = 3; i < NCH(n); i += 2) {
				PyTuple_SET_ITEM(tup, (i-3)/2, 
					PyString_FromString(STR(
							CHILD(CHILD(n, i), 0))));
			}
		}
		com_addoparg(c, LOAD_CONST, com_addconst(c, tup));
		Py_DECREF(tup);
		com_push(c, 1);
		com_addopname(c, IMPORT_NAME, CHILD(n, 1));
		if (TYPE(CHILD(n, 3)) == STAR) 
			com_addbyte(c, IMPORT_STAR);
		else {
			for (i = 3; i < NCH(n); i += 2) 
				com_from_import(c, CHILD(n, i));
			com_addbyte(c, POP_TOP);
		}
		com_pop(c, 1);
	}
	else {
		/* 'import' ... */
		for (i = 1; i < NCH(n); i += 2) {
			node *subn = CHILD(n, i);
			REQ(subn, dotted_as_name);
			com_addoparg(c, LOAD_CONST, com_addconst(c, Py_None));
			com_push(c, 1);
			com_addopname(c, IMPORT_NAME, CHILD(subn, 0));
			if (NCH(subn) > 1) {
				int j;
				if (strcmp(STR(CHILD(subn, 1)), "as") != 0) {
					com_error(c, PyExc_SyntaxError,
						  "invalid syntax");
					return;
				}
				for (j=2 ; j < NCH(CHILD(subn, 0)); j += 2)
					com_addopname(c, LOAD_ATTR,
						      CHILD(CHILD(subn, 0), j));
				com_addopname(c, STORE_NAME, CHILD(subn, 2));
			} else
				com_addopname(c, STORE_NAME,
					      CHILD(CHILD(subn, 0),0));
			com_pop(c, 1);
		}
	}
}

static void
com_global_stmt(struct compiling *c, node *n)
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
		    com_mangle(c, s, buffer, sizeof(buffer)))
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
com_newlocal_o(struct compiling *c, PyObject *nameval)
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
com_addlocal_o(struct compiling *c, PyObject *nameval)
{
	PyObject *ival =  PyDict_GetItem(c->c_locals, nameval);
	if (ival != NULL)
		return PyInt_AsLong(ival);
	return com_newlocal_o(c, nameval);
}

static int
com_newlocal(struct compiling *c, char *name)
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
com_exec_stmt(struct compiling *c, node *n)
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
is_constant_false(struct compiling *c, node *n)
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
com_if_stmt(struct compiling *c, node *n)
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
com_while_stmt(struct compiling *c, node *n)
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
com_for_stmt(struct compiling *c, node *n)
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
	com_assign(c, CHILD(n, 1), OP_ASSIGN, NULL);
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
com_try_except(struct compiling *c, node *n)
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
			com_assign(c, CHILD(ch, 3), OP_ASSIGN, NULL);
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
com_try_finally(struct compiling *c, node *n)
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
com_try_stmt(struct compiling *c, node *n)
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
get_rawdocstring(node *n)
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
get_docstring(node *n)
{
	/* Don't generate doc-strings if run with -OO */
	if (Py_OptimizeFlag > 1)
		return NULL;
	n = get_rawdocstring(n);
	if (n == NULL)
		return NULL;
	return parsestrplus(n);
}

static void
com_suite(struct compiling *c, node *n)
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
com_continue_stmt(struct compiling *c, node *n)
{
	int i = c->c_nblocks;
	if (i-- > 0 && c->c_block[i] == SETUP_LOOP) {
		com_addoparg(c, JUMP_ABSOLUTE, c->c_begin);
	}
	else if (i <= 0) {
		/* at the outer level */
		com_error(c, PyExc_SyntaxError,
			  "'continue' not properly in loop");
	}
	else {
		int j;
		for (j = 0; j <= i; ++j) {
			if (c->c_block[j] == SETUP_LOOP)
				break;
		}
		if (j < i+1) {
			/* there is a loop, but something interferes */
			for (++j; j <= i; ++j) {
				if (c->c_block[i] == SETUP_EXCEPT
				    || c->c_block[i] == SETUP_FINALLY) {
					com_error(c, PyExc_SyntaxError,
			       "'continue' not supported inside 'try' clause");
					return;
				}
			}
		}
		com_error(c, PyExc_SyntaxError,
			  "'continue' not properly in loop");
	}
	/* XXX Could allow it inside a 'finally' clause
	   XXX if we could pop the exception still on the stack */
}

static int
com_argdefs(struct compiling *c, node *n)
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
			/* Treat "(a=1, b)" as an error */
			if (ndefs)
				com_error(c, PyExc_SyntaxError,
			    "non-default argument follows default argument");
		}
		if (t != COMMA)
			break;
	}
	return ndefs;
}

static void
com_funcdef(struct compiling *c, node *n)
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
com_bases(struct compiling *c, node *n)
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
com_classdef(struct compiling *c, node *n)
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
com_node(struct compiling *c, node *n)
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
		com_assign(c, CHILD(n, 1), OP_DELETE, NULL);
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

static void com_fplist(struct compiling *, node *);

static void
com_fpdef(struct compiling *c, node *n)
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
com_fplist(struct compiling *c, node *n)
{
	REQ(n, fplist); /* fplist: fpdef (',' fpdef)* [','] */
	if (NCH(n) == 1) {
		com_fpdef(c, CHILD(n, 0));
	}
	else {
		int i = (NCH(n)+1)/2;
		com_addoparg(c, UNPACK_SEQUENCE, i);
		com_push(c, i-1);
		for (i = 0; i < NCH(n); i += 2)
			com_fpdef(c, CHILD(n, i));
	}
}

static void
com_arglist(struct compiling *c, node *n)
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
		PyObject *nameval;
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
		nameval = PyString_InternFromString(name);
		if (nameval == NULL) {
			c->c_errors++;
		}
		if (PyDict_GetItem(c->c_locals, nameval)) {
			com_error(c, PyExc_SyntaxError,
				  "duplicate argument in function definition");
		}
		com_newlocal_o(c, nameval);
		Py_DECREF(nameval);
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
com_file_input(struct compiling *c, node *n)
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
compile_funcdef(struct compiling *c, node *n)
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
compile_lambdef(struct compiling *c, node *n)
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
compile_classdef(struct compiling *c, node *n)
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
compile_node(struct compiling *c, node *n)
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
optimize(struct compiling *c)
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
	  dispatch_opcode1:
		switch (opcode) {
		case STORE_NAME:
		case DELETE_NAME:
		case IMPORT_FROM:
			com_addlocal_o(c, GETNAMEOBJ(oparg));
			break;
		case IMPORT_STAR:
		case EXEC_STMT:
			c->c_flags &= ~CO_OPTIMIZED;
			break;
		case EXTENDED_ARG:
			opcode = NEXTOP();
			oparg = oparg<<16 | NEXTARG();
			goto dispatch_opcode1;
			break;
		}
	}
	
	/* TBD: Is this still necessary ? */
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
	  dispatch_opcode2:
		if (opcode == LOAD_NAME ||
		    opcode == STORE_NAME ||
		    opcode == DELETE_NAME) {
			PyObject *v;
			int i;
			name = GETNAMEOBJ(oparg);
			v = PyDict_GetItem(c->c_locals, name);
			if (v == NULL) {
				if (opcode == LOAD_NAME &&
				    (c->c_flags&CO_OPTIMIZED))
					cur_instr[0] = LOAD_GLOBAL;
				continue;
			}
			i = PyInt_AsLong(v);
			if (i >> 16) /* too big for 2 bytes */
				continue;
			switch (opcode) {
			case LOAD_NAME: cur_instr[0] = LOAD_FAST; break;
			case STORE_NAME: cur_instr[0] = STORE_FAST; break;
			case DELETE_NAME: cur_instr[0] = DELETE_FAST; break;
			}
			cur_instr[1] = i & 0xff;
			cur_instr[2] = i >> 8;
		}
		if (opcode == EXTENDED_ARG) {
			opcode = NEXTOP();
			oparg = oparg<<16 | NEXTARG();
			goto dispatch_opcode2;
		}
	}

	if (c->c_errors == 0)
		PyErr_Restore(error_type, error_value, error_traceback);
}

PyCodeObject *
PyNode_Compile(node *n, char *filename)
{
	return jcompile(n, filename, NULL);
}

static PyCodeObject *
icompile(node *n, struct compiling *base)
{
	return jcompile(n, base->c_filename, base);
}

static PyCodeObject *
jcompile(node *n, char *filename, struct compiling *base)
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
	else if (!PyErr_Occurred()) {
		/* This could happen if someone called PyErr_Clear() after an
		   error was reported above.  That's not supposed to happen,
		   but I just plugged one case and I'm not sure there can't be
		   others.  In that case, raise SystemError so that at least
		   it gets reported instead dumping core. */
		PyErr_SetString(PyExc_SystemError, "lost syntax error");
	}
	com_free(&sc);
	return co;
}

int
PyCode_Addr2Line(PyCodeObject *co, int addrq)
{
	int size = PyString_Size(co->co_lnotab) / 2;
	unsigned char *p = (unsigned char*)PyString_AsString(co->co_lnotab);
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
