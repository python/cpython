/* System module */

/*
Various bits of information used by the interpreter are collected in
module 'sys'.
Data members:
- stdin, stdout, stderr: standard file objects
- ps1, ps2: primary and secondary prompts (strings)
- path: module search path (list of strings)
- modules: the table of modules (dictionary)
Function members:
- exit(sts): call exit()
*/

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "fileobject.h"
#include "moduleobject.h"
#include "sysmodule.h"
#include "node.h" /* For context.h */
#include "context.h" /* For import.h */
#include "import.h"
#include "methodobject.h"
#include "modsupport.h"
#include "errors.h"

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
makeargv(argc, argv)
	int argc;
	char **argv;
{
	int i;
	object *av, *v;
	if (argc < 0 || argv == NULL)
		argc = 0;
	av = newlistobject(argc);
	if (av != NULL) {
		for (i = 0; i < argc; i++) {
			v = newstringobject(argv[i]);
			if (v == NULL) {
				DECREF(av);
				av = NULL;
				break;
			}
			setlistitem(av, i, v);
		}
	}
	if (av == NULL)
		fatal("no mem for sys.argv");
	return av;
}

/* sys.exit method */

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

static object *sysin, *sysout, *syserr;

void
initsys(argc, argv)
	int argc;
	char **argv;
{
	object *v;
	object *exit;
	if ((sysdict = newdictobject()) == NULL)
		fatal("can't create sys dict");
	/* NB keep an extra ref to the std files to avoid closing them
	   when the user deletes them */
	sysin = newopenfileobject(stdin, "<stdin>", "r");
	sysout = newopenfileobject(stdout, "<stdout>", "w");
	syserr = newopenfileobject(stderr, "<stderr>", "w");
	v = makeargv(argc, argv);
	exit = newmethodobject("exit", sys_exit, (object *)NULL);
	if (err_occurred())
		fatal("can't create sys.* objects");
	dictinsert(sysdict, "stdin", sysin);
	dictinsert(sysdict, "stdout", sysout);
	dictinsert(sysdict, "stderr", syserr);
	dictinsert(sysdict, "argv", v);
	dictinsert(sysdict, "exit", exit);
	if (err_occurred())
		fatal("can't insert sys.* objects in sys dict");
	DECREF(exit);
	DECREF(v);
	/* The other symbols are added elsewhere */
	
	/* Only now can we initialize the import stuff, after which
	   we can turn ourselves into a module */
	initimport();
	if ((v = new_module("sys")) == NULL)
		fatal("can't create sys module");
	if (setmoduledict(v, sysdict) != 0)
		fatal("can't assign sys dict to sys module");
	DECREF(v);
}

static void
cleardict(d)
	object *d;
{
	int i;
	for (i = getdictsize(d); --i >= 0; ) {
		char *k;
		k = getdictkey(d, i);
		if (k != NULL) {
			(void) dictremove(d, k);
		}
	}
}

void
closesys()
{
	object *modules;
	modules = sysget("modules");
	if (modules != NULL && is_dictobject(modules)) {
		int i;
		/* Explicitly erase all modules; this is the safest way
		   to get rid of at least *some* circular dependencies */
		INCREF(modules);
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
		DECREF(modules);
	}
	DECREF(sysdict);
}
