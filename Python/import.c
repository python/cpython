/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* Module definition and import implementation */

#include "allobjects.h"

#include "node.h"
#include "token.h"
#include "graminit.h"
#include "import.h"
#include "errcode.h"
#include "sysmodule.h"
#include "pythonrun.h"

/* Define pathname separator used in file names */

#ifdef THINK_C
#define SEP ':'
#endif

#ifdef MSDOS
#define SEP '\\'
#endif

#ifndef SEP
#define SEP '/'
#endif

static object *modules;

/* Forward */
static int init_builtin PROTO((char *));

/* Initialization */

void
initimport()
{
	if ((modules = newdictobject()) == NULL)
		fatal("no mem for dictionary of modules");
}

object *
get_modules()
{
	return modules;
}

object *
add_module(name)
	char *name;
{
	object *m;
	if ((m = dictlookup(modules, name)) != NULL && is_moduleobject(m))
		return m;
	m = newmoduleobject(name);
	if (m == NULL)
		return NULL;
	if (dictinsert(modules, name, m) != 0) {
		DECREF(m);
		return NULL;
	}
	DECREF(m); /* Yes, it still exists, in modules! */
	return m;
}

static FILE *
open_module(name, suffix, namebuf)
	char *name;
	char *suffix;
	char *namebuf; /* XXX No buffer overflow checks! */
{
	object *path;
	FILE *fp;
	
	path = sysget("path");
	if (path == NULL || !is_listobject(path)) {
		strcpy(namebuf, name);
		strcat(namebuf, suffix);
		fp = fopen(namebuf, "r");
	}
	else {
		int npath = getlistsize(path);
		int i;
		fp = NULL;
		for (i = 0; i < npath; i++) {
			object *v = getlistitem(path, i);
			int len;
			if (!is_stringobject(v))
				continue;
			strcpy(namebuf, getstringvalue(v));
			len = getstringsize(v);
			if (len > 0 && namebuf[len-1] != SEP)
				namebuf[len++] = SEP;
			strcpy(namebuf+len, name);
			strcat(namebuf, suffix);
			fp = fopen(namebuf, "r");
			if (fp != NULL)
				break;
		}
	}
	return fp;
}

static object *
get_module(m, name, m_ret)
	/*module*/object *m;
	char *name;
	object **m_ret;
{
	object *d;
	FILE *fp;
	node *n;
	int err;
	char namebuf[256];
	
	fp = open_module(name, ".py", namebuf);
	if (fp == NULL) {
		if (m == NULL)
			err_setstr(NameError, name);
		else
			err_setstr(RuntimeError, "no module source file");
		return NULL;
	}
	err = parse_file(fp, namebuf, file_input, &n);
	fclose(fp);
	if (err != E_DONE) {
		err_input(err);
		return NULL;
	}
	if (m == NULL) {
		m = add_module(name);
		if (m == NULL) {
			freetree(n);
			return NULL;
		}
		*m_ret = m;
	}
	d = getmoduledict(m);
	return run_node(n, namebuf, d, d);
}

static object *
load_module(name)
	char *name;
{
	object *m, *v;
	v = get_module((object *)NULL, name, &m);
	if (v == NULL)
		return NULL;
	DECREF(v);
	return m;
}

object *
import_module(name)
	char *name;
{
	object *m;
	if ((m = dictlookup(modules, name)) == NULL) {
		if (init_builtin(name)) {
			if ((m = dictlookup(modules, name)) == NULL)
				err_setstr(SystemError, "builtin module missing");
		}
		else {
			m = load_module(name);
		}
	}
	return m;
}

object *
reload_module(m)
	object *m;
{
	if (m == NULL || !is_moduleobject(m)) {
		err_setstr(TypeError, "reload() argument must be module");
		return NULL;
	}
	/* XXX Ought to check for builtin modules -- can't reload these... */
	return get_module(m, getmodulename(m), (object **)NULL);
}

static void
cleardict(d)
	object *d;
{
	int i;
	for (i = getdictsize(d); --i >= 0; ) {
		char *k;
		k = getdictkey(d, i);
		if (k != NULL)
			(void) dictremove(d, k);
	}
}

void
doneimport()
{
	if (modules != NULL) {
		int i;
		/* Explicitly erase all modules; this is the safest way
		   to get rid of at least *some* circular dependencies */
		for (i = getdictsize(modules); --i >= 0; ) {
			char *k;
			k = getdictkey(modules, i);
			if (k != NULL) {
				object *m;
				m = dictlookup(modules, k);
				if (m != NULL && is_moduleobject(m)) {
					object *d;
					d = getmoduledict(m);
					if (d != NULL && is_dictobject(d)) {
						cleardict(d);
					}
				}
			}
		}
		cleardict(modules);
	}
	DECREF(modules);
}


/* Initialize built-in modules when first imported */

extern struct {
	char *name;
	void (*initfunc)();
} inittab[];

static int
init_builtin(name)
	char *name;
{
	int i;
	for (i = 0; inittab[i].name != NULL; i++) {
		if (strcmp(name, inittab[i].name) == 0) {
			(*inittab[i].initfunc)();
			return 1;
		}
	}
	return 0;
}
