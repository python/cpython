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

int debugging; /* Needed by parser.c */

/* Interface to getopt(): */
extern int optind;
extern char *optarg;
extern int getopt PROTO((int, char **, char *));

main(argc, argv)
	int argc;
	char **argv;
{
	int c;
	int sts;
	char *command = NULL;
	char *filename = NULL;
	FILE *fp = stdin;
	
	initargs(&argc, &argv); /* Defined in config*.c */

	while ((c = getopt(argc, argv, "c:d")) != EOF) {
		if (c == 'c') {
			/* -c is the last option; following arguments
			   that look like options are left for the
			   the command to interpret. */
			command = malloc(strlen(optarg) + 2);
			/* Ignore malloc errors this early... */
			strcpy(command, optarg);
			strcat(command, "\n");
			break;
		}
		
		switch (c) {

		case 'd':
			debugging++;
			break;

		/* This space reserved for other options */

		default:
			fprintf(stderr,
				"usage: %s [-c cmd | file | -] [arg] ...\n",
				argv[0]);
			exit(2);
			/*NOTREACHED*/

		}
	}
	
	if (command == NULL && optind < argc && strcmp(argv[optind], "-") != 0)
		filename = argv[optind];
	
	if (filename != NULL) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "%s: can't open file '%s'\n",
				argv[0], filename);
			exit(2);
		}
	}
	
	initall(); /* Defined in config*.c */
	
	setpythonpath(getpythonpath());
	if (command != NULL) {
		/* Backup optind and force sys.argv[0] = '-c' */
		optind--;
		argv[optind] = "-c";
	}
	setpythonargv(argc-optind, argv+optind);

	if (command) {
		sts = run_command(command) != 0;
	}
	else {
		sts = run(fp, filename == NULL ? "<stdin>" : filename) != 0;
	}

	goaway(sts);
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

int
run_command(command)
	char *command;
{
	object *m, *d, *v;
	m = add_module("__main__");
	if (m == NULL)
		return -1;
	d = getmoduledict(m);
	v = run_string(command, file_input, d, d);
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
	if (exception == SystemExit) {
		if (v == NULL || v == None)
			goaway(0);
		if (is_intobject(v))
			goaway((int)getintvalue(v));
		else {
			printobject(v, stderr, PRINT_RAW);
			fprintf(stderr, "\n");
			goaway(1);
		}
	}
	sysset("last_type", exception);
	sysset("last_value", v);
	if (printobject(exception, stderr, PRINT_RAW) != 0)
		err_clear();
	if (v != NULL && v != None) {
		fprintf(stderr, ": ");
		if (printobject(v, stderr, PRINT_RAW) != 0)
			err_clear();
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

#ifdef TRACE_REFS
	if (askyesno("Print left references?")) {
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

#ifdef applec /* MPW (also usable for Think C 3.0) */

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
}

#endif
