/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* Frame object implementation */

#include "allobjects.h"

#include "compile.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#define OFF(x) offsetof(frameobject, x)

static struct memberlist frame_memberlist[] = {
	{"f_back",	T_OBJECT,	OFF(f_back)},
	{"f_code",	T_OBJECT,	OFF(f_code)},
	{"f_globals",	T_OBJECT,	OFF(f_globals)},
	{"f_locals",	T_OBJECT,	OFF(f_locals)},
	{"f_owner",	T_OBJECT,	OFF(f_owner)},
/*	{"f_fastlocals",T_OBJECT,	OFF(f_fastlocals)}, /* XXX Unsafe */
	{"f_localmap",	T_OBJECT,	OFF(f_localmap)},
	{"f_lasti",	T_INT,		OFF(f_lasti)},
	{"f_lineno",	T_INT,		OFF(f_lineno)},
	{NULL}	/* Sentinel */
};

static object *
frame_getattr(f, name)
	frameobject *f;
	char *name;
{
	return getmember((char *)f, frame_memberlist, name);
}

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we maintain a
   separate free list of stack frames (just like integers are
   allocated in a special way -- see intobject.c).  When a stack frame
   is on the free list, only the following members have a meaning:
	ob_type		== &Frametype
	f_back		next item on free list, or NULL
	f_nvalues	size of f_valuestack
	f_valuestack	array of (f_nvalues+1) object pointers, or NULL
	f_nblocks	size of f_blockstack
	f_blockstack	array of (f_nblocks+1) blocks, or NULL
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
	XDECREF(f->f_globals);
	XDECREF(f->f_locals);
	XDECREF(f->f_owner);
	XDECREF(f->f_fastlocals);
	XDECREF(f->f_localmap);
	f->f_back = free_list;
	free_list = f;
}

typeobject Frametype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"frame",
	sizeof(frameobject),
	0,
	frame_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	frame_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

frameobject *
newframeobject(back, code, globals, locals, owner, nvalues, nblocks)
	frameobject *back;
	codeobject *code;
	object *globals;
	object *locals;
	object *owner;
	int nvalues;
	int nblocks;
{
	frameobject *f;
	if ((back != NULL && !is_frameobject(back)) ||
		code == NULL || !is_codeobject(code) ||
		globals == NULL || !is_dictobject(globals) ||
		locals == NULL || !is_dictobject(locals) ||
		nvalues < 0 || nblocks < 0) {
		err_badcall();
		return NULL;
	}
	if (free_list == NULL) {
		f = NEWOBJ(frameobject, &Frametype);
		f->f_nvalues = f->f_nblocks = 0;
		f->f_valuestack = NULL;
		f->f_blockstack = NULL;
	}
	else {
		f = free_list;
		free_list = free_list->f_back;
		f->ob_type = &Frametype;
		NEWREF(f);
	}
	if (f != NULL) {
		if (back)
			INCREF(back);
		f->f_back = back;
		INCREF(code);
		f->f_code = code;
		INCREF(globals);
		f->f_globals = globals;
		INCREF(locals);
		f->f_locals = locals;
		XINCREF(owner);
		f->f_owner = owner;
		f->f_fastlocals = NULL;
		f->f_localmap = NULL;
		if (nvalues > f->f_nvalues || f->f_valuestack == NULL) {
			XDEL(f->f_valuestack);
			f->f_valuestack = NEW(object *, nvalues+1);
			f->f_nvalues = nvalues;
		}
		if (nblocks > f->f_nblocks || f->f_blockstack == NULL) {
			XDEL(f->f_blockstack);
			f->f_blockstack = NEW(block, nblocks+1);
			f->f_nblocks = nblocks;
		}
		f->f_iblock = 0;
		f->f_lasti = 0;
		f->f_lineno = -1;
		if (f->f_valuestack == NULL || f->f_blockstack == NULL) {
			err_nomem();
			DECREF(f);
			f = NULL;
		}
	}
	return f;
}

object **
extend_stack(f, level, incr)
	frameobject *f;
	int level;
	int incr;
{
	f->f_nvalues = level + incr + 10;
	f->f_valuestack =
		(object **) realloc((ANY *)f->f_valuestack,
				    sizeof(object *) * (f->f_nvalues + 1));
	if (f->f_valuestack == NULL) {
		err_nomem();
		return NULL;
	}
	return f->f_valuestack + level;
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
	if (f->f_iblock >= f->f_nblocks) {
		fprintf(stderr, "XXX block stack overflow\n");
		abort();
	}
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
	if (f->f_iblock <= 0) {
		fprintf(stderr, "XXX block stack underflow\n");
		abort();
	}
	b = &f->f_blockstack[--f->f_iblock];
	return b;
}
