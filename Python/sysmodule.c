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

/* System module */

/*
Various bits of information used by the interpreter are collected in
module 'sys'.
Function member:
- exit(sts): call (C, POSIX) exit(sts)
Data members:
- stdin, stdout, stderr: standard file objects
- modules: the table of modules (dictionary)
- path: module search path (list of strings)
- argv: script arguments (list of strings)
- ps1, ps2: optional primary and secondary prompts (strings)
*/

#include "allobjects.h"

#include "sysmodule.h"
#include "import.h"
#include "modsupport.h"

/* Define delimiter used in $PYTHONPATH */

#ifdef THINK_C
#define DELIM ' '
#endif

#ifndef DELIM
#define DELIM ':'
#endif

static object *sysdict;

object *
sysget(name)
	char *name;
{
	return dictlookup(sysdict, name);
}

FILE *
sysgetfile(name, def)
	char *name;
	FILE *def;
{
	FILE *fp = NULL;
	object *v = sysget(name);
	if (v != NULL)
		fp = getfilefile(v);
	if (fp == NULL)
		fp = def;
	return fp;
}

int
sysset(name, v)
	char *name;
	object *v;
{
	if (v == NULL)
		return dictremove(sysdict, name);
	else
		return dictinsert(sysdict, name, v);
}

static object *
sys_exit(self, args)
	object *self;
	object *args;
{
	int sts;
	if (!getintarg(args, &sts))
		return NULL;
	goaway(sts);
	exit(sts); /* Just in case */
	/* NOTREACHED */
}

static struct methodlist sys_methods[] = {
	{"exit",	sys_exit},
	{NULL,		NULL}		/* sentinel */
};

static object *sysin, *sysout, *syserr;

void
initsys()
{
	object *m = initmodule("sys", sys_methods);
	sysdict = getmoduledict(m);
	INCREF(sysdict);
	/* NB keep an extra ref to the std files to avoid closing them
	   when the user deletes them */
	/* XXX File objects should have a "don't close" flag instead */
	sysin = newopenfileobject(stdin, "<stdin>", "r");
	sysout = newopenfileobject(stdout, "<stdout>", "w");
	syserr = newopenfileobject(stderr, "<stderr>", "w");
	if (err_occurred())
		fatal("can't create sys.std* file objects");
	dictinsert(sysdict, "stdin", sysin);
	dictinsert(sysdict, "stdout", sysout);
	dictinsert(sysdict, "stderr", syserr);
	dictinsert(sysdict, "modules", get_modules());
	if (err_occurred())
		fatal("can't insert sys.* objects in sys dict");
}

static object *
makepathobject(path, delim)
	char *path;
	int delim;
{
	int i, n;
	char *p;
	object *v, *w;
	
	n = 1;
	p = path;
	while ((p = strchr(p, delim)) != NULL) {
		n++;
		p++;
	}
	v = newlistobject(n);
	if (v == NULL)
		return NULL;
	for (i = 0; ; i++) {
		p = strchr(path, delim);
		if (p == NULL)
			p = strchr(path, '\0'); /* End of string */
		w = newsizedstringobject(path, (int) (p - path));
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		setlistitem(v, i, w);
		if (*p == '\0')
			break;
		path = p+1;
	}
	return v;
}

void
setpythonpath(path)
	char *path;
{
	object *v;
	if ((v = makepathobject(path, DELIM)) == NULL)
		fatal("can't create sys.path");
	if (sysset("path", v) != 0)
		fatal("can't assign sys.path");
	DECREF(v);
}

static object *
makeargvobject(argc, argv)
	int argc;
	char **argv;
{
	object *av;
	if (argc < 0 || argv == NULL)
		argc = 0;
	av = newlistobject(argc);
	if (av != NULL) {
		int i;
		for (i = 0; i < argc; i++) {
			object *v = newstringobject(argv[i]);
			if (v == NULL) {
				DECREF(av);
				av = NULL;
				break;
			}
			setlistitem(av, i, v);
		}
	}
	return av;
}

void
setpythonargv(argc, argv)
	int argc;
	char **argv;
{
	object *av = makeargvobject(argc, argv);
	if (av == NULL)
		fatal("no mem for sys.argv");
	if (sysset("argv", av) != 0)
		fatal("can't assign sys.argv");
	DECREF(av);
}
