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

/* System module */

/*
Various bits of information used by the interpreter are collected in
module 'sys'.
Function member:
- exit(sts): raise SystemExit
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
#include "osdefs.h"

object *sys_trace, *sys_profile;
int sys_checkinterval = 10;

static object *sysdict;

#ifdef MS_COREDLL
extern void *PyWin_DLLhModule;
#endif

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
	if (v != NULL && is_fileobject(v))
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
	if (v == NULL) {
		if (dictlookup(sysdict, name) == NULL)
			return 0;
		else
			return dictremove(sysdict, name);
	}
	else
		return dictinsert(sysdict, name, v);
}

static object *
sys_exit(self, args)
	object *self;
	object *args;
{
	/* Raise SystemExit so callers may catch it or clean up. */
	err_setval(SystemExit, args);
	return NULL;
}

static object *
sys_settrace(self, args)
	object *self;
	object *args;
{
	if (args == None)
		args = NULL;
	else
		XINCREF(args);
	XDECREF(sys_trace);
	sys_trace = args;
	INCREF(None);
	return None;
}

static object *
sys_setprofile(self, args)
	object *self;
	object *args;
{
	if (args == None)
		args = NULL;
	else
		XINCREF(args);
	XDECREF(sys_profile);
	sys_profile = args;
	INCREF(None);
	return None;
}

static object *
sys_setcheckinterval(self, args)
	object *self;
	object *args;
{
	if (!newgetargs(args, "i", &sys_checkinterval))
		return NULL;
	INCREF(None);
	return None;
}

#ifdef USE_MALLOPT
/* Link with -lmalloc (or -lmpc) on an SGI */
#include <malloc.h>

static object *
sys_mdebug(self, args)
	object *self;
	object *args;
{
	int flag;
	if (!getargs(args, "i", &flag))
		return NULL;
	mallopt(M_DEBUG, flag);
	INCREF(None);
	return None;
}
#endif /* USE_MALLOPT */

static object *
sys_getrefcount(self, args)
	object *self;
	object *args;
{
	object *arg;
	if (!getargs(args, "O", &arg))
		return NULL;
	return newintobject((long) arg->ob_refcnt);
}

#ifdef COUNT_ALLOCS
static PyObject *
sys_getcounts(self, args)
	PyObject *self, *args;
{
	extern PyObject *get_counts Py_PROTO((void));

	if (!PyArg_Parse(args, ""))
		return NULL;
	return get_counts();
}
#endif

#ifdef Py_TRACE_REFS
/* Defined in objects.c because it uses static globals if that file */
extern PyObject *_Py_GetObjects Py_PROTO((PyObject *, PyObject *));
#endif

#ifdef DYNAMIC_EXECUTION_PROFILE
/* Defined in ceval.c because it uses static globals if that file */
extern PyObject *_Py_GetDXProfile Py_PROTO((PyObject *,  PyObject *));
#endif

static struct methodlist sys_methods[] = {
	/* Might as well keep this in alphabetic order */
	{"exit",	sys_exit, 0},
#ifdef COUNT_ALLOCS
	{"getcounts",	sys_getcounts, 0},
#endif
#ifdef DYNAMIC_EXECUTION_PROFILE
	{"getdxp",	_Py_GetDXProfile, 1},
#endif
#ifdef Py_TRACE_REFS
	{"getobjects",	_Py_GetObjects, 1},
#endif
	{"getrefcount",	sys_getrefcount, 0},
#ifdef USE_MALLOPT
	{"mdebug",	sys_mdebug, 0},
#endif
	{"setcheckinterval",	sys_setcheckinterval, 1},
	{"setprofile",	sys_setprofile, 0},
	{"settrace",	sys_settrace, 0},
	{NULL,		NULL}		/* sentinel */
};

static object *sysin, *sysout, *syserr;

static object *
list_builtin_module_names()
{
	object *list = newlistobject(0);
	int i;
	if (list == NULL)
		return NULL;
	for (i = 0; inittab[i].name != NULL; i++) {
		object *name = newstringobject(inittab[i].name);
		if (name == NULL)
			break;
		addlistitem(list, name);
		DECREF(name);
	}
	if (sortlist(list) != 0) {
		DECREF(list);
		list = NULL;
	}
	if (list) {
		object *v = listtuple(list);
		DECREF(list);
		list = v;
	}
	return list;
}

void
initsys()
{
	extern long getmaxint PROTO((void));
	extern char *getversion PROTO((void));
	extern char *getcopyright PROTO((void));
	extern char *getplatform PROTO((void));
	extern char *Py_GetPrefix PROTO((void));
	extern char *Py_GetExecPrefix PROTO((void));
	extern int fclose PROTO((FILE *));
	object *m = initmodule("sys", sys_methods);
	object *v;
	sysdict = getmoduledict(m);
	INCREF(sysdict);
	/* NB keep an extra ref to the std files to avoid closing them
	   when the user deletes them */
	sysin = newopenfileobject(stdin, "<stdin>", "r", fclose);
	sysout = newopenfileobject(stdout, "<stdout>", "w", fclose);
	syserr = newopenfileobject(stderr, "<stderr>", "w", fclose);
	if (err_occurred())
		fatal("can't initialize sys.std{in,out,err}");
	dictinsert(sysdict, "stdin", sysin);
	dictinsert(sysdict, "stdout", sysout);
	dictinsert(sysdict, "stderr", syserr);
	dictinsert(sysdict, "version", v = newstringobject(getversion()));
	XDECREF(v);
	dictinsert(sysdict, "copyright", v = newstringobject(getcopyright()));
	XDECREF(v);
	dictinsert(sysdict, "platform", v = newstringobject(getplatform()));
	XDECREF(v);
	dictinsert(sysdict, "prefix", v = newstringobject(Py_GetPrefix()));
	XDECREF(v);
	dictinsert(sysdict, "exec_prefix",
		   v = newstringobject(Py_GetExecPrefix()));
	XDECREF(v);
	dictinsert(sysdict, "maxint", v = newintobject(getmaxint()));
	XDECREF(v);
	dictinsert(sysdict, "modules", get_modules());
	dictinsert(sysdict, "builtin_module_names",
		   v = list_builtin_module_names());
	XDECREF(v);
#ifdef MS_COREDLL
	dictinsert(sysdict, "dllhandle", v = newintobject((int)PyWin_DLLhModule));
	XDECREF(v);
	dictinsert(sysdict, "winver", v = newstringobject(MS_DLL_ID));
	XDECREF(v);
#endif
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
	if (argc <= 0 || argv == NULL) {
		/* Ensure at least one (empty) argument is seen */
		static char *empty_argv[1] = {""};
		argv = empty_argv;
		argc = 1;
	}
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
	object *path = sysget("path");
	if (av == NULL)
		fatal("no mem for sys.argv");
	if (sysset("argv", av) != 0)
		fatal("can't assign sys.argv");
	if (path != NULL) {
		char *p = NULL;
		int n = 0;
		object *a;
#if SEP == '\\' /* Special case for MS filename syntax */
		if (argc > 0 && argv[0] != NULL) {
			char *q;
			p = strrchr(argv[0], SEP);
			/* Test for alternate separator */
			q = strrchr(p ? p : argv[0], '/');
			if (q != NULL)
				p = q;
			if (p != NULL) {
				n = p + 1 - argv[0];
				if (n > 1 && p[-1] != ':')
					n--; /* Drop trailing separator */
			}
		}
#else /* All other filename syntaxes */
		if (argc > 0 && argv[0] != NULL)
			p = strrchr(argv[0], SEP);
		if (p != NULL) {
			n = p + 1 - argv[0];
#if SEP == '/' /* Special case for Unix filename syntax */
			if (n > 1)
				n--; /* Drop trailing separator */
#endif /* Unix */
		}
#endif /* All others */
		a = newsizedstringobject(argv[0], n);
		if (a == NULL)
			fatal("no mem for sys.path insertion");
		if (inslistitem(path, 0, a) < 0)
			fatal("sys.path.insert(0) failed");
		DECREF(a);
	}
	DECREF(av);
}
