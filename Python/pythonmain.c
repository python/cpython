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

/* Python interpreter main program */

#include "patchlevel.h"

#include "allobjects.h"

#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "graminit.h"
#include "errcode.h"
#include "sysmodule.h"
#include "compile.h"
#include "ceval.h"
#include "pythonrun.h"
#include "import.h"

extern char *getpythonpath();

extern grammar gram; /* From graminit.c */

#ifdef DEBUG
int debugging; /* Needed by parser.c */
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	char *filename = NULL;
	FILE *fp = stdin;
	
	initargs(&argc, &argv);
	
	if (argc > 1 && strcmp(argv[1], "-") != 0)
		filename = argv[1];
	
	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "python: can't open file '%s'\n",
				filename);
			exit(2);
		}
	}
	
	initall();
	
	setpythonpath(getpythonpath());
	setpythonargv(argc-1, argv+1);
	
	goaway(run(fp, filename == NULL ? "<stdin>" : filename));
	/*NOTREACHED*/
}

/* Initialize all */

void
initall()
{
	static int inited;
	
	if (inited)
		return;
	inited = 1;
	
	initimport();
	
	/* Modules 'builtin' and 'sys' are initialized here,
	   they are needed by random bits of the interpreter.
	   All other modules are optional and should be initialized
	   by the initcalls() of a specific configuration. */
	
	initbuiltin(); /* Also initializes builtin exceptions */
	initsys();
	
	initcalls(); /* Configuration-dependent initializations */
	
	initintr(); /* For intrcheck() */
}

/* Parse input from a file and execute it */

int
run(fp, filename)
	FILE *fp;
	char *filename;
{
	if (filename == NULL)
		filename = "???";
	if (isatty(fileno(fp)))
		return run_tty_loop(fp, filename);
	else
		return run_script(fp, filename);
}

int
run_tty_loop(fp, filename)
	FILE *fp;
	char *filename;
{
	object *v;
	int ret;
	v = sysget("ps1");
	if (v == NULL) {
		sysset("ps1", v = newstringobject(">>> "));
		XDECREF(v);
	}
	v = sysget("ps2");
	if (v == NULL) {
		sysset("ps2", v = newstringobject("... "));
		XDECREF(v);
	}
	for (;;) {
		ret = run_tty_1(fp, filename);
#ifdef REF_DEBUG
		fprintf(stderr, "[%ld refs]\n", ref_total);
#endif
		if (ret == E_EOF)
			return 0;
		/*
		if (ret == E_NOMEM)
			return -1;
		*/
	}
}

int
run_tty_1(fp, filename)
	FILE *fp;
	char *filename;
{
	object *m, *d, *v, *w;
	node *n;
	char *ps1, *ps2;
	int err;
	v = sysget("ps1");
	w = sysget("ps2");
	if (v != NULL && is_stringobject(v)) {
		INCREF(v);
		ps1 = getstringvalue(v);
	}
	else {
		v = NULL;
		ps1 = "";
	}
	if (w != NULL && is_stringobject(w)) {
		INCREF(w);
		ps2 = getstringvalue(w);
	}
	else {
		w = NULL;
		ps2 = "";
	}
	err = parsefile(fp, filename, &gram, single_input, ps1, ps2, &n);
	XDECREF(v);
	XDECREF(w);
	if (err == E_EOF)
		return E_EOF;
	if (err != E_DONE) {
		err_input(err);
		print_error();
		return err;
	}
	m = add_module("__main__");
	if (m == NULL)
		return -1;
	d = getmoduledict(m);
	v = run_node(n, filename, d, d);
	flushline();
	if (v == NULL) {
		print_error();
		return -1;
	}
	DECREF(v);
	return 0;
}

int
run_script(fp, filename)
	FILE *fp;
	char *filename;
{
	object *m, *d, *v;
	m = add_module("__main__");
	if (m == NULL)
		return -1;
	d = getmoduledict(m);
	v = run_file(fp, filename, file_input, d, d);
	flushline();
	if (v == NULL) {
		print_error();
		return -1;
	}
	DECREF(v);
	return 0;
}

void
print_error()
{
	object *exception, *v;
	err_get(&exception, &v);
	fprintf(stderr, "Unhandled exception: ");
	printobject(exception, stderr, PRINT_RAW);
	if (v != NULL && v != None) {
		fprintf(stderr, ": ");
		printobject(v, stderr, PRINT_RAW);
	}
	fprintf(stderr, "\n");
	XDECREF(exception);
	XDECREF(v);
	printtraceback(stderr);
}

object *
run_string(str, start, globals, locals)
	char *str;
	int start;
	/*dict*/object *globals, *locals;
{
	node *n;
	int err;
	err = parse_string(str, start, &n);
	return run_err_node(err, n, "<string>", globals, locals);
}

object *
run_file(fp, filename, start, globals, locals)
	FILE *fp;
	char *filename;
	int start;
	/*dict*/object *globals, *locals;
{
	node *n;
	int err;
	err = parse_file(fp, filename, start, &n);
	return run_err_node(err, n, filename, globals, locals);
}

object *
run_err_node(err, n, filename, globals, locals)
	int err;
	node *n;
	char *filename;
	/*dict*/object *globals, *locals;
{
	if (err != E_DONE) {
		err_input(err);
		return NULL;
	}
	return run_node(n, filename, globals, locals);
}

object *
run_node(n, filename, globals, locals)
	node *n;
	char *filename;
	/*dict*/object *globals, *locals;
{
	if (globals == NULL) {
		globals = getglobals();
		if (locals == NULL)
			locals = getlocals();
	}
	else {
		if (locals == NULL)
			locals = globals;
	}
	return eval_node(n, filename, globals, locals);
}

object *
eval_node(n, filename, globals, locals)
	node *n;
	char *filename;
	object *globals;
	object *locals;
{
	codeobject *co;
	object *v;
	co = compile(n, filename);
	freetree(n);
	if (co == NULL)
		return NULL;
	v = eval_code(co, globals, locals, (object *)NULL);
	DECREF(co);
	return v;
}

/* Simplified interface to parsefile */

int
parse_file(fp, filename, start, n_ret)
	FILE *fp;
	char *filename;
	int start;
	node **n_ret;
{
	return parsefile(fp, filename, &gram, start,
				(char *)0, (char *)0, n_ret);
}

/* Simplified interface to parsestring */

int
parse_string(str, start, n_ret)
	char *str;
	int start;
	node **n_ret;
{
	int err = parsestring(str, &gram, start, n_ret);
	/* Don't confuse early end of string with early end of input */
	if (err == E_EOF)
		err = E_SYNTAX;
	return err;
}

/* Print fatal error message and abort */

void
fatal(msg)
	char *msg;
{
	fprintf(stderr, "Fatal error: %s\n", msg);
	abort();
}

/* Clean up and exit */

void
goaway(sts)
	int sts;
{
	flushline();
	
	/* XXX Call doneimport() before donecalls(), since donecalls()
	   calls wdone(), and doneimport() may close windows */
	doneimport();
	donecalls();
	
	err_clear();

#ifdef REF_DEBUG
	fprintf(stderr, "[%ld refs]\n", ref_total);
#endif

#ifdef THINK_C_3_0
	if (sts == 0)
		Click_On(0);
#endif

#ifdef TRACE_REFS
	if (askyesno("Print left references?")) {
#ifdef THINK_C_3_0
		Click_On(1);
#endif
		printrefs(stderr);
	}
#endif /* TRACE_REFS */

	exit(sts);
	/*NOTREACHED*/
}

static
finaloutput()
{
#ifdef TRACE_REFS
	if (!askyesno("Print left references?"))
		return;
#ifdef THINK_C_3_0
	Click_On(1);
#endif
	printrefs(stderr);
#endif /* TRACE_REFS */
}

/* Ask a yes/no question */

static int
askyesno(prompt)
	char *prompt;
{
	char buf[256];
	
	printf("%s [ny] ", prompt);
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return 0;
	return buf[0] == 'y' || buf[0] == 'Y';
}

#ifdef THINK_C_3_0

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
}

#endif

/*	XXX WISH LIST

	- possible new types:
		- iterator (for range, keys, ...)
	- improve interpreter error handling, e.g., true tracebacks
	- save precompiled modules on file?
	- fork threads, locking
	- allow syntax extensions
*/

/* "Floccinaucinihilipilification" */
