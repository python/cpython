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
#include "marshal.h"
#include "compile.h"
#include "ceval.h"

/* Magic word to reject pre-0.9.4 .pyc files */

#define MAGIC 0x949494L

/* Define pathname separator used in file names */

#ifdef macintosh
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
	codeobject *co = NULL;
	object *v, *d;
	FILE *fp, *fpc;
	node *n;
	int err;
	char namebuf[258];
	int namelen;
	long mtime;
	extern long getmtime();
	
	fp = open_module(name, ".py", namebuf);
	if (fp == NULL) {
		if (m == NULL) {
			sprintf(namebuf, "no module named %.200s", name);
			err_setstr(ImportError, namebuf);
		}
		else {
			sprintf(namebuf, "no source for module %.200s", name);
			err_setstr(ImportError, namebuf);
		}
		return NULL;
	}
	/* Get mtime -- always useful */
	mtime = getmtime(namebuf);
	/* Check ".pyc" file first */
	namelen = strlen(namebuf);
	namebuf[namelen] = 'c';
	namebuf[namelen+1] = '\0';
	fpc = fopen(namebuf, "rb");
	if (fpc != NULL) {
		long pyc_mtime;
		long magic;
		magic = rd_long(fpc);
		pyc_mtime = rd_long(fpc);
		if (magic == MAGIC && pyc_mtime == mtime && mtime != 0 && mtime != -1) {
			v = rd_object(fpc);
			if (v == NULL || err_occurred() || !is_codeobject(v)) {
				err_clear();
				XDECREF(v);
			}
			else
				co = (codeobject *)v;
		}
		fclose(fpc);
	}
	namebuf[namelen] = '\0';
	if (co == NULL)
		err = parse_file(fp, namebuf, file_input, &n);
	else
		err = E_DONE;
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
	if (co == NULL) {
		co = compile(n, namebuf);
		freetree(n);
		if (co == NULL)
			return NULL;
		/* Now write the code object to the ".pyc" file */
		namebuf[namelen] = 'c';
		namebuf[namelen+1] = '\0';
		fpc = fopen(namebuf, "wb");
		if (fpc != NULL) {
			wr_long(MAGIC, fpc);
			/* First write a 0 for mtime */
			wr_long(0L, fpc);
			wr_object((object *)co, fpc);
			if (ferror(fpc)) {
				/* Don't keep partial file */
				fclose(fpc);
				(void) unlink(namebuf);
			}
			else {
				/* Now write the true mtime */
				fseek(fpc, 4L, 0);
				wr_long(mtime, fpc);
				fflush(fpc);
				fclose(fpc);
			}
		}
	}
	v = eval_code(co, d, d, (object *)NULL);
	DECREF(co);
	return v;
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
			object *k;
			k = getdict2key(modules, i);
			if (k != NULL) {
				object *m;
				m = dict2lookup(modules, k);
				if (m == NULL)
					err_clear();
				else if (is_moduleobject(m)) {
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
