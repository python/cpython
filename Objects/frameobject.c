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

/* Frame object implementation */

#include "allobjects.h"

#include "compile.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"
#include "bltinmodule.h"

#define OFF(x) offsetof(frameobject, x)

static struct memberlist frame_memberlist[] = {
	{"f_back",	T_OBJECT,	OFF(f_back),	RO},
	{"f_code",	T_OBJECT,	OFF(f_code),	RO},
	{"f_builtins",	T_OBJECT,	OFF(f_builtins),RO},
	{"f_globals",	T_OBJECT,	OFF(f_globals),	RO},
	{"f_locals",	T_OBJECT,	OFF(f_locals),	RO},
	{"f_lasti",	T_INT,		OFF(f_lasti),	RO},
	{"f_lineno",	T_INT,		OFF(f_lineno),	RO},
	{"f_restricted",T_INT,		OFF(f_restricted),RO},
	{"f_trace",	T_OBJECT,	OFF(f_trace)},
	{NULL}	/* Sentinel */
};

static object *
frame_getattr(f, name)
	frameobject *f;
	char *name;
{
	if (strcmp(name, "f_locals") == 0)
		fast_2_locals(f);
	return getmember((char *)f, frame_memberlist, name);
}

static int
frame_setattr(f, name, value)
	frameobject *f;
	char *name;
	object *value;
{
	return setmember((char *)f, frame_memberlist, name, value);
}

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we maintain a
   separate free list of stack frames (just like integers are
   allocated in a special way -- see intobject.c).  When a stack frame
   is on the free list, only the following members have a meaning:
	ob_type		== &Frametype
	f_back		next item on free list, or NULL
	f_nlocals	number of locals
	f_stacksize	size of value stack
   Note that the value and block stacks are preserved -- this can save
   another malloc() call or two (and two free() calls as well!).
   Also note that, unlike for integers, each frame object is a
   malloc'ed object in its own right -- it is only the actual calls to
   malloc() that we are trying to save here, not the administration.
   After all, while a typical program may make millions of calls, a
   call depth of more than 20 or 30 is probably already exceptional
   unless the program contains run-away recursion.  I hope.
*/

static frameobject *free_list = NULL;

static void
frame_dealloc(f)
	frameobject *f;
{
	XDECREF(f->f_back);
	XDECREF(f->f_code);
	XDECREF(f->f_builtins);
	XDECREF(f->f_globals);
	XDECREF(f->f_locals);
	XDECREF(f->f_trace);
	f->f_back = free_list;
	free_list = f;
}

typeobject Frametype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"frame",
	sizeof(frameobject),
	0,
	(destructor)frame_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)frame_getattr, /*tp_getattr*/
	(setattrfunc)frame_setattr, /*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

frameobject *
newframeobject(back, code, globals, locals)
	frameobject *back;
	codeobject *code;
	object *globals;
	object *locals;
{
	static object *builtin_object;
	frameobject *f;
	object *builtins;
	int extras = code->co_stacksize + code->co_nlocals;

	if (builtin_object == NULL) {
		builtin_object = PyString_InternFromString("__builtins__");
		if (builtin_object == NULL)
			return NULL;
	}
	if ((back != NULL && !is_frameobject(back)) ||
	    code == NULL || !is_codeobject(code) ||
	    globals == NULL || !is_dictobject(globals) ||
	    (locals != NULL && !is_dictobject(locals))) {
		err_badcall();
		return NULL;
	}
	builtins = mappinglookup(globals, builtin_object);
	if (builtins != NULL && is_moduleobject(builtins))
		builtins = getmoduledict(builtins);
	if (builtins == NULL || !is_mappingobject(builtins)) {
		err_setstr(TypeError, "bad __builtins__ dictionary");
		return NULL;
	}
	if (free_list == NULL) {
		f = (frameobject *)
			malloc(sizeof(frameobject) + extras*sizeof(object *));
		if (f == NULL)
			return (PyFrameObject *)err_nomem();
		f->ob_type = &Frametype;
		NEWREF(f);
	}
	else {
		f = free_list;
		free_list = free_list->f_back;
		if (f->f_nlocals + f->f_stacksize < extras) {
			f = realloc(f, sizeof(frameobject) + extras*sizeof(object *));
			if (f == NULL)
				return (PyFrameObject *)err_nomem();
		}
		f->ob_type = &Frametype;
		NEWREF(f);
	}
	XINCREF(back);
	f->f_back = back;
	INCREF(code);
	f->f_code = code;
	XINCREF(builtins);
	f->f_builtins = builtins;
	INCREF(globals);
	f->f_globals = globals;
	if (code->co_flags & CO_NEWLOCALS) {
		if (code->co_flags & CO_OPTIMIZED)
			locals = NULL; /* Let fast_2_locals handle it */
		else {
			locals = newdictobject();
			if (locals == NULL) {
				DECREF(f);
				return NULL;
			}
		}
	}
	else {
		if (locals == NULL)
			locals = globals;
		INCREF(locals);
	}
	f->f_locals = locals;
	f->f_trace = NULL;

	f->f_lasti = 0;
	f->f_lineno = -1;
	f->f_restricted = (builtins != getbuiltindict());
	f->f_iblock = 0;
	f->f_nlocals = code->co_nlocals;
	f->f_stacksize = code->co_stacksize;

	while (--extras >= 0)
		f->f_localsplus[extras] = NULL;

	f->f_valuestack = f->f_localsplus + f->f_nlocals;

	return f;
}

/* Block management */

void
setup_block(f, type, handler, level)
	frameobject *f;
	int type;
	int handler;
	int level;
{
	block *b;
	if (f->f_iblock >= CO_MAXBLOCKS)
		fatal("XXX block stack overflow");
	b = &f->f_blockstack[f->f_iblock++];
	b->b_type = type;
	b->b_level = level;
	b->b_handler = handler;
}

block *
pop_block(f)
	frameobject *f;
{
	block *b;
	if (f->f_iblock <= 0)
		fatal("XXX block stack underflow");
	b = &f->f_blockstack[--f->f_iblock];
	return b;
}

/* Convert between "fast" version of locals and dictionary version */

void
fast_2_locals(f)
	frameobject *f;
{
	/* Merge fast locals into f->f_locals */
	object *locals, *map;
	object **fast;
	object *error_type, *error_value, *error_traceback;
	int j;
	if (f == NULL)
		return;
	locals = f->f_locals;
	if (locals == NULL) {
		locals = f->f_locals = newdictobject();
		if (locals == NULL) {
			err_clear(); /* Can't report it :-( */
			return;
		}
	}
	if (f->f_nlocals == 0)
		return;
	map = f->f_code->co_varnames;
	if (!is_dictobject(locals) || !is_tupleobject(map))
		return;
	err_fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = gettuplesize(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	for (; --j >= 0; ) {
		object *key = gettupleitem(map, j);
		object *value = fast[j];
		if (value == NULL) {
			err_clear();
			if (dict2remove(locals, key) != 0)
				err_clear();
		}
		else {
			if (dict2insert(locals, key, value) != 0)
				err_clear();
		}
	}
	err_restore(error_type, error_value, error_traceback);
}

void
locals_2_fast(f, clear)
	frameobject *f;
	int clear;
{
	/* Merge f->f_locals into fast locals */
	object *locals, *map;
	object **fast;
	object *error_type, *error_value, *error_traceback;
	int j;
	if (f == NULL)
		return;
	locals = f->f_locals;
	map = f->f_code->co_varnames;
	if (locals == NULL || fast == NULL || f->f_code->co_nlocals == 0)
		return;
	if (!is_dictobject(locals) || !is_tupleobject(map))
		return;
	err_fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = gettuplesize(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	for (; --j >= 0; ) {
		object *key = gettupleitem(map, j);
		object *value = dict2lookup(locals, key);
		if (value == NULL)
			err_clear();
		else
			INCREF(value);
		if (value != NULL || clear) {
			XDECREF(fast[j]);
			XINCREF(value);
			fast[j] = value;
		}
	}
	err_restore(error_type, error_value, error_traceback);
}
