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

/* Traceback implementation */

#include "allobjects.h"

#include "sysmodule.h"
#include "compile.h"
#include "frameobject.h"
#include "traceback.h"
#include "structmember.h"
#include "osdefs.h"

typedef struct _tracebackobject {
	OB_HEAD
	struct _tracebackobject *tb_next;
	frameobject *tb_frame;
	int tb_lasti;
	int tb_lineno;
} tracebackobject;

#define OFF(x) offsetof(tracebackobject, x)

static struct memberlist tb_memberlist[] = {
	{"tb_next",	T_OBJECT,	OFF(tb_next)},
	{"tb_frame",	T_OBJECT,	OFF(tb_frame)},
	{"tb_lasti",	T_INT,		OFF(tb_lasti)},
	{"tb_lineno",	T_INT,		OFF(tb_lineno)},
	{NULL}	/* Sentinel */
};

static object *
tb_getattr(tb, name)
	tracebackobject *tb;
	char *name;
{
	return getmember((char *)tb, tb_memberlist, name);
}

static void
tb_dealloc(tb)
	tracebackobject *tb;
{
	XDECREF(tb->tb_next);
	XDECREF(tb->tb_frame);
	DEL(tb);
}

#define Tracebacktype PyTraceback_Type
#define is_tracebackobject PyTraceback_Check

typeobject Tracebacktype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"traceback",
	sizeof(tracebackobject),
	0,
	(destructor)tb_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)tb_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

static tracebackobject *
newtracebackobject(next, frame, lasti, lineno)
	tracebackobject *next;
	frameobject *frame;
	int lasti, lineno;
{
	tracebackobject *tb;
	if ((next != NULL && !is_tracebackobject(next)) ||
			frame == NULL || !is_frameobject(frame)) {
		err_badcall();
		return NULL;
	}
	tb = NEWOBJ(tracebackobject, &Tracebacktype);
	if (tb != NULL) {
		XINCREF(next);
		tb->tb_next = next;
		XINCREF(frame);
		tb->tb_frame = frame;
		tb->tb_lasti = lasti;
		tb->tb_lineno = lineno;
	}
	return tb;
}

static tracebackobject *tb_current = NULL;

int
tb_here(frame)
	frameobject *frame;
{
	tracebackobject *tb;
	tb = newtracebackobject(tb_current, frame, frame->f_lasti, frame->f_lineno);
	if (tb == NULL)
		return -1;
	XDECREF(tb_current);
	tb_current = tb;
	return 0;
}

object *
tb_fetch()
{
	object *v;
	v = (object *)tb_current;
	tb_current = NULL;
	return v;
}

int
tb_store(v)
	object *v;
{
	if (v != NULL && !is_tracebackobject(v)) {
		err_badcall();
		return -1;
	}
	XDECREF(tb_current);
	XINCREF(v);
	tb_current = (tracebackobject *)v;
	return 0;
}

static void
tb_displayline(f, filename, lineno, name)
	object *f;
	char *filename;
	int lineno;
	char *name;
{
	FILE *xfp;
	char linebuf[1000];
	int i;
#ifdef MPW
	/* This is needed by MPW's File and Line commands */
#define FMT "  File \"%.900s\"; line %d # in %s\n"
#else
	/* This is needed by Emacs' compile command */
#define FMT "  File \"%.900s\", line %d, in %s\n"
#endif
	xfp = fopen(filename, "r");
	if (xfp == NULL) {
		/* Search tail of filename in sys.path before giving up */
		object *path;
		char *tail = strrchr(filename, SEP);
		if (tail == NULL)
			tail = filename;
		else
			tail++;
		path = sysget("path");
		if (path != NULL && is_listobject(path)) {
			int npath = getlistsize(path);
			int taillen = strlen(tail);
			char namebuf[MAXPATHLEN+1];
			for (i = 0; i < npath; i++) {
				object *v = getlistitem(path, i);
				if (is_stringobject(v)) {
					int len;
					len = getstringsize(v);
					if (len + 1 + taillen >= MAXPATHLEN)
						continue; /* Too long */
					strcpy(namebuf, getstringvalue(v));
					if (strlen(namebuf) != len)
						continue; /* v contains '\0' */
					if (len > 0 && namebuf[len-1] != SEP)
						namebuf[len++] = SEP;
					strcpy(namebuf+len, tail);
					xfp = fopen(namebuf, "r");
					if (xfp != NULL) {
						filename = namebuf;
						break;
					}
				}
			}
		}
	}
	sprintf(linebuf, FMT, filename, lineno, name);
	writestring(linebuf, f);
	if (xfp == NULL)
		return;
	for (i = 0; i < lineno; i++) {
		if (fgets(linebuf, sizeof linebuf, xfp) == NULL)
			break;
	}
	if (i == lineno) {
		char *p = linebuf;
		while (*p == ' ' || *p == '\t' || *p == '\014')
			p++;
		writestring("    ", f);
		writestring(p, f);
		if (strchr(p, '\n') == NULL)
			writestring("\n", f);
	}
	fclose(xfp);
}

static void
tb_printinternal(tb, f, limit)
	tracebackobject *tb;
	object *f;
	int limit;
{
	int depth = 0;
	tracebackobject *tb1 = tb;
	while (tb1 != NULL) {
		depth++;
		tb1 = tb1->tb_next;
	}
	while (tb != NULL && !intrcheck()) {
		if (depth <= limit)
			tb_displayline(f,
			    getstringvalue(tb->tb_frame->f_code->co_filename),
			    tb->tb_lineno,
			    getstringvalue(tb->tb_frame->f_code->co_name));
		depth--;
		tb = tb->tb_next;
	}
}

int
tb_print(v, f)
	object *v;
	object *f;
{
	object *limitv;
	int limit = 1000;
	if (v == NULL)
		return 0;
	if (!is_tracebackobject(v)) {
		err_badcall();
		return -1;
	}
	limitv = sysget("tracebacklimit");
	if (limitv && is_intobject(limitv)) {
		limit = getintvalue(limitv);
		if (limit <= 0)
			return 0;
	}
	writestring("Traceback (innermost last):\n", f);
	tb_printinternal((tracebackobject *)v, f, limit);
	return 0;
}
