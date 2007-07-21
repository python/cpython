#include "Python.h"
#include "code.h"
#include "structmember.h"

#define NAME_CHARS \
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"

/* all_name_chars(s): true iff all chars in s are valid NAME_CHARS */

static int
all_name_chars(unsigned char *s)
{
	static char ok_name_char[256];
	static unsigned char *name_chars = (unsigned char *)NAME_CHARS;

	if (ok_name_char[*name_chars] == 0) {
		unsigned char *p;
		for (p = name_chars; *p; p++)
			ok_name_char[*p] = 1;
	}
	while (*s) {
		if (ok_name_char[*s++] == 0)
			return 0;
	}
	return 1;
}

static void
intern_strings(PyObject *tuple)
{
	Py_ssize_t i;

	for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
		PyObject *v = PyTuple_GET_ITEM(tuple, i);
		if (v == NULL || !PyString_CheckExact(v)) {
			Py_FatalError("non-string found in code slot");
		}
		PyString_InternInPlace(&PyTuple_GET_ITEM(tuple, i));
	}
}


PyCodeObject *
PyCode_New(int argcount, int nlocals, int stacksize, int flags,
	   PyObject *code, PyObject *consts, PyObject *names,
	   PyObject *varnames, PyObject *freevars, PyObject *cellvars,
	   PyObject *filename, PyObject *name, int firstlineno,
	   PyObject *lnotab)
{
	PyCodeObject *co;
	Py_ssize_t i;
	/* Check argument types */
	if (argcount < 0 || nlocals < 0 ||
	    code == NULL ||
	    consts == NULL || !PyTuple_Check(consts) ||
	    names == NULL || !PyTuple_Check(names) ||
	    varnames == NULL || !PyTuple_Check(varnames) ||
	    freevars == NULL || !PyTuple_Check(freevars) ||
	    cellvars == NULL || !PyTuple_Check(cellvars) ||
	    name == NULL || !PyString_Check(name) ||
	    filename == NULL || !PyString_Check(filename) ||
	    lnotab == NULL || !PyString_Check(lnotab) ||
	    !PyObject_CheckReadBuffer(code)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	intern_strings(names);
	intern_strings(varnames);
	intern_strings(freevars);
	intern_strings(cellvars);
	/* Intern selected string constants */
	for (i = PyTuple_Size(consts); --i >= 0; ) {
		PyObject *v = PyTuple_GetItem(consts, i);
		if (!PyString_Check(v))
			continue;
		if (!all_name_chars((unsigned char *)PyString_AS_STRING(v)))
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
		Py_INCREF(freevars);
		co->co_freevars = freevars;
		Py_INCREF(cellvars);
		co->co_cellvars = cellvars;
		Py_INCREF(filename);
		co->co_filename = filename;
		Py_INCREF(name);
		co->co_name = name;
		co->co_firstlineno = firstlineno;
		Py_INCREF(lnotab);
		co->co_lnotab = lnotab;
                co->co_zombieframe = NULL;
	}
	return co;
}


#define OFF(x) offsetof(PyCodeObject, x)

static PyMemberDef code_memberlist[] = {
	{"co_argcount",	T_INT,		OFF(co_argcount),	READONLY},
	{"co_nlocals",	T_INT,		OFF(co_nlocals),	READONLY},
	{"co_stacksize",T_INT,		OFF(co_stacksize),	READONLY},
	{"co_flags",	T_INT,		OFF(co_flags),		READONLY},
	{"co_code",	T_OBJECT,	OFF(co_code),		READONLY},
	{"co_consts",	T_OBJECT,	OFF(co_consts),		READONLY},
	{"co_names",	T_OBJECT,	OFF(co_names),		READONLY},
	{"co_varnames",	T_OBJECT,	OFF(co_varnames),	READONLY},
	{"co_freevars",	T_OBJECT,	OFF(co_freevars),	READONLY},
	{"co_cellvars",	T_OBJECT,	OFF(co_cellvars),	READONLY},
	{"co_filename",	T_OBJECT,	OFF(co_filename),	READONLY},
	{"co_name",	T_OBJECT,	OFF(co_name),		READONLY},
	{"co_firstlineno", T_INT,	OFF(co_firstlineno),	READONLY},
	{"co_lnotab",	T_OBJECT,	OFF(co_lnotab),		READONLY},
	{NULL}	/* Sentinel */
};

/* Helper for code_new: return a shallow copy of a tuple that is
   guaranteed to contain exact strings, by converting string subclasses
   to exact strings and complaining if a non-string is found. */
static PyObject*
validate_and_copy_tuple(PyObject *tup)
{
	PyObject *newtuple;
	PyObject *item;
	Py_ssize_t i, len;

	len = PyTuple_GET_SIZE(tup);
	newtuple = PyTuple_New(len);
	if (newtuple == NULL)
		return NULL;

	for (i = 0; i < len; i++) {
		item = PyTuple_GET_ITEM(tup, i);
		if (PyString_CheckExact(item)) {
			Py_INCREF(item);
		}
		else if (!PyString_Check(item)) {
			PyErr_Format(
				PyExc_TypeError,
				"name tuples must contain only "
				"strings, not '%.500s'",
				item->ob_type->tp_name);
			Py_DECREF(newtuple);
			return NULL;
		}
		else {
			item = PyString_FromStringAndSize(
				PyString_AS_STRING(item),
				PyString_GET_SIZE(item));
			if (item == NULL) {
				Py_DECREF(newtuple);
				return NULL;
			}
		}
		PyTuple_SET_ITEM(newtuple, i, item);
	}

	return newtuple;
}

PyDoc_STRVAR(code_doc,
"code(argcount, nlocals, stacksize, flags, codestring, constants, names,\n\
      varnames, filename, name, firstlineno, lnotab[, freevars[, cellvars]])\n\
\n\
Create a code object.  Not for the faint of heart.");

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	int argcount;
	int nlocals;
	int stacksize;
	int flags;
	PyObject *co = NULL;
	PyObject *code;
	PyObject *consts;
	PyObject *names, *ournames = NULL;
	PyObject *varnames, *ourvarnames = NULL;
	PyObject *freevars = NULL, *ourfreevars = NULL;
	PyObject *cellvars = NULL, *ourcellvars = NULL;
	PyObject *filename;
	PyObject *name;
	int firstlineno;
	PyObject *lnotab;

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

	if (argcount < 0) {
		PyErr_SetString(
			PyExc_ValueError,
			"code: argcount must not be negative");
		goto cleanup;
	}

	if (nlocals < 0) {
		PyErr_SetString(
			PyExc_ValueError,
			"code: nlocals must not be negative");
		goto cleanup;
	}

	ournames = validate_and_copy_tuple(names);
	if (ournames == NULL)
		goto cleanup;
	ourvarnames = validate_and_copy_tuple(varnames);
	if (ourvarnames == NULL)
		goto cleanup;
	if (freevars)
		ourfreevars = validate_and_copy_tuple(freevars);
	else
		ourfreevars = PyTuple_New(0);
	if (ourfreevars == NULL)
		goto cleanup;
	if (cellvars)
		ourcellvars = validate_and_copy_tuple(cellvars);
	else
		ourcellvars = PyTuple_New(0);
	if (ourcellvars == NULL)
		goto cleanup;

	co = (PyObject *)PyCode_New(argcount, nlocals, stacksize, flags,
				    code, consts, ournames, ourvarnames,
				    ourfreevars, ourcellvars, filename,
				    name, firstlineno, lnotab);
  cleanup:
	Py_XDECREF(ournames);
	Py_XDECREF(ourvarnames);
	Py_XDECREF(ourfreevars);
	Py_XDECREF(ourcellvars);
	return co;
}

static void
code_dealloc(PyCodeObject *co)
{
	Py_XDECREF(co->co_code);
	Py_XDECREF(co->co_consts);
	Py_XDECREF(co->co_names);
	Py_XDECREF(co->co_varnames);
	Py_XDECREF(co->co_freevars);
	Py_XDECREF(co->co_cellvars);
	Py_XDECREF(co->co_filename);
	Py_XDECREF(co->co_name);
	Py_XDECREF(co->co_lnotab);
        if (co->co_zombieframe != NULL)
                PyObject_GC_Del(co->co_zombieframe);
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
		filename = PyString_AS_STRING(co->co_filename);
	if (co->co_name && PyString_Check(co->co_name))
		name = PyString_AS_STRING(co->co_name);
	PyOS_snprintf(buf, sizeof(buf),
		      "<code object %.100s at %p, file \"%.300s\", line %d>",
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
	if (cmp) goto normalize;
	cmp = co->co_nlocals - cp->co_nlocals;
	if (cmp) goto normalize;
	cmp = co->co_flags - cp->co_flags;
	if (cmp) goto normalize;
	cmp = co->co_firstlineno - cp->co_firstlineno;
	if (cmp) goto normalize;
	cmp = PyObject_Compare(co->co_code, cp->co_code);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_consts, cp->co_consts);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_names, cp->co_names);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_varnames, cp->co_varnames);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_freevars, cp->co_freevars);
	if (cmp) return cmp;
	cmp = PyObject_Compare(co->co_cellvars, cp->co_cellvars);
	return cmp;

 normalize:
	if (cmp > 0)
		return 1;
	else if (cmp < 0)
		return -1;
	else
		return 0;
}

static long
code_hash(PyCodeObject *co)
{
	long h, h0, h1, h2, h3, h4, h5, h6;
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
	h5 = PyObject_Hash(co->co_freevars);
	if (h5 == -1) return -1;
	h6 = PyObject_Hash(co->co_cellvars);
	if (h6 == -1) return -1;
	h = h0 ^ h1 ^ h2 ^ h3 ^ h4 ^ h5 ^ h6 ^
		co->co_argcount ^ co->co_nlocals ^ co->co_flags;
	if (h == -1) h = -2;
	return h;
}

/* XXX code objects need to participate in GC? */

PyTypeObject PyCode_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"code",
	sizeof(PyCodeObject),
	0,
	(destructor)code_dealloc, 	/* tp_dealloc */
	0,				/* tp_print */
	0, 				/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc)code_compare, 		/* tp_compare */
	(reprfunc)code_repr,		/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	(hashfunc)code_hash, 		/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	code_doc,			/* tp_doc */
	0,				/* tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	0,				/* tp_methods */
	code_memberlist,		/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	0,				/* tp_alloc */
	code_new,			/* tp_new */
};

/* All about c_lnotab.

c_lnotab is an array of unsigned bytes disguised as a Python string.  In -O
mode, SET_LINENO opcodes aren't generated, and bytecode offsets are mapped
to source code line #s (when needed for tracebacks) via c_lnotab instead.
The array is conceptually a list of
    (bytecode offset increment, line number increment)
pairs.  The details are important and delicate, best illustrated by example:

    byte code offset    source code line number
        0		    1
        6		    2
       50		    7
      350                 307
      361                 308

The first trick is that these numbers aren't stored, only the increments
from one row to the next (this doesn't really work, but it's a start):

    0, 1,  6, 1,  44, 5,  300, 300,  11, 1

The second trick is that an unsigned byte can't hold negative values, or
values larger than 255, so (a) there's a deep assumption that byte code
offsets and their corresponding line #s both increase monotonically, and (b)
if at least one column jumps by more than 255 from one row to the next, more
than one pair is written to the table. In case #b, there's no way to know
from looking at the table later how many were written.  That's the delicate
part.  A user of c_lnotab desiring to find the source line number
corresponding to a bytecode address A should do something like this

    lineno = addr = 0
    for addr_incr, line_incr in c_lnotab:
        addr += addr_incr
        if addr > A:
            return lineno
        lineno += line_incr

In order for this to work, when the addr field increments by more than 255,
the line # increment in each pair generated must be 0 until the remaining addr
increment is < 256.  So, in the example above, com_set_lineno should not (as
was actually done until 2.2) expand 300, 300 to 255, 255,  45, 45, but to
255, 0,  45, 255,  0, 45.
*/

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

/* 
   Check whether the current instruction is at the start of a line.

 */

	/* The theory of SET_LINENO-less tracing.

	   In a nutshell, we use the co_lnotab field of the code object
	   to tell when execution has moved onto a different line.

	   As mentioned above, the basic idea is so set things up so
	   that

	         *instr_lb <= frame->f_lasti < *instr_ub

	   is true so long as execution does not change lines.

	   This is all fairly simple.  Digging the information out of
	   co_lnotab takes some work, but is conceptually clear.

	   Somewhat harder to explain is why we don't *always* call the
	   line trace function when the above test fails.

	   Consider this code:

	   1: def f(a):
	   2:     if a:
	   3:        print 1
	   4:     else:
	   5:        print 2

	   which compiles to this:

	   2           0 LOAD_FAST                0 (a)
		       3 JUMP_IF_FALSE            9 (to 15)
		       6 POP_TOP

	   3           7 LOAD_CONST               1 (1)
		      10 PRINT_ITEM
		      11 PRINT_NEWLINE
		      12 JUMP_FORWARD             6 (to 21)
		 >>   15 POP_TOP

	   5          16 LOAD_CONST               2 (2)
		      19 PRINT_ITEM
		      20 PRINT_NEWLINE
		 >>   21 LOAD_CONST               0 (None)
		      24 RETURN_VALUE

	   If 'a' is false, execution will jump to instruction at offset
	   15 and the co_lnotab will claim that execution has moved to
	   line 3.  This is at best misleading.  In this case we could
	   associate the POP_TOP with line 4, but that doesn't make
	   sense in all cases (I think).

	   What we do is only call the line trace function if the co_lnotab
	   indicates we have jumped to the *start* of a line, i.e. if the
	   current instruction offset matches the offset given for the
	   start of a line by the co_lnotab.

	   This also takes care of the situation where 'a' is true.
	   Execution will jump from instruction offset 12 to offset 21.
	   Then the co_lnotab would imply that execution has moved to line
	   5, which is again misleading.

	   Why do we set f_lineno when tracing?  Well, consider the code
	   above when 'a' is true.  If stepping through this with 'n' in
	   pdb, you would stop at line 1 with a "call" type event, then
	   line events on lines 2 and 3, then a "return" type event -- but
	   you would be shown line 5 during this event.  This is a change
	   from the behaviour in 2.2 and before, and I've found it
	   confusing in practice.  By setting and using f_lineno when
	   tracing, one can report a line number different from that
	   suggested by f_lasti on this one occasion where it's desirable.
	*/


int 
PyCode_CheckLineNumber(PyCodeObject* co, int lasti, PyAddrPair *bounds)
{
        int size, addr, line;
        unsigned char* p;

        p = (unsigned char*)PyString_AS_STRING(co->co_lnotab);
        size = PyString_GET_SIZE(co->co_lnotab) / 2;

        addr = 0;
        line = co->co_firstlineno;
        assert(line > 0);

        /* possible optimization: if f->f_lasti == instr_ub
           (likely to be a common case) then we already know
           instr_lb -- if we stored the matching value of p
           somwhere we could skip the first while loop. */

        /* see comments in compile.c for the description of
           co_lnotab.  A point to remember: increments to p
           should come in pairs -- although we don't care about
           the line increments here, treating them as byte
           increments gets confusing, to say the least. */

        bounds->ap_lower = 0;
        while (size > 0) {
                if (addr + *p > lasti)
                        break;
                addr += *p++;
                if (*p) 
                        bounds->ap_lower = addr;
                line += *p++;
                --size;
        }

        /* If lasti and addr don't match exactly, we don't want to
           change the lineno slot on the frame or execute a trace
           function.  Return -1 instead.
        */
        if (addr != lasti)
                line = -1;
        
        if (size > 0) {
                while (--size >= 0) {
                        addr += *p++;
                        if (*p++)
                                break;
                }
                bounds->ap_upper = addr;
        }
        else {
                bounds->ap_upper = INT_MAX;
        }

        return line;
}
