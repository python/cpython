/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Python interpreter top-level routines, including init/exit */

#include "allobjects.h"

#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "graminit.h"
#include "errcode.h"
#include "sysmodule.h"
#include "compile.h"
#include "eval.h"
#include "ceval.h"
#include "pythonrun.h"
#include "import.h"
#include "marshal.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

extern char *getpythonpath();

extern grammar gram; /* From graminit.c */

/* Forward */
static object *run_err_node PROTO((node *n, char *filename,
				   object *globals, object *locals));
static object *run_node PROTO((node *n, char *filename,
			       object *globals, object *locals));
static void err_input PROTO((perrdetail *));
static void initsigs PROTO((void));

int debugging; /* Needed by parser.c */
int verbose; /* Needed by import.c */
int suppress_print; /* Needed by ceval.c */

/* Initialize all */

void
initall()
{
	static int inited;
	
	if (inited)
		return;
	inited = 1;
	
	initimport();
	
	/* Modules '__builtin__' and 'sys' are initialized here,
	   they are needed by random bits of the interpreter.
	   All other modules are optional and are initialized
	   when they are first imported. */
	
	initbuiltin(); /* Also initializes builtin exceptions */
	initsys();

	setpythonpath(getpythonpath());

	initsigs(); /* Signal handling stuff, including initintr() */
}

/* Parse input from a file and execute it */

int
run(fp, filename)
	FILE *fp;
	char *filename;
{
	if (filename == NULL)
		filename = "???";
	if (isatty((int)fileno(fp)))
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
	perrdetail err;
	char *ps1, *ps2;
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
	BGN_SAVE
	n = parsefile(fp, filename, &gram, single_input, ps1, ps2, &err);
	END_SAVE
	XDECREF(v);
	XDECREF(w);
	if (n == NULL) {
		if (err.error == E_EOF) {
			if (err.text)
				free(err.text);
			return E_EOF;
		}
		err_input(&err);
		print_error();
		return err.error;
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
	char *ext;

	m = add_module("__main__");
	if (m == NULL)
		return -1;
	d = getmoduledict(m);
	ext = filename + strlen(filename) - 4;
	if ( strcmp(ext, ".pyc") == 0 ) {
		/* Try to run a pyc file. First, re-open in binary */
		/* Don't close, done in main: fclose(fp); */
		if( (fp = fopen(filename, "rb")) == NULL ) {
			fprintf(stderr, "python: Can't reopen .pyc file\n");
			return -1;
		}
		v = run_pyc_file(fp, filename, d, d);
	} else {
		v = run_file(fp, filename, file_input, d, d);
	}
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
	object *exception, *v, *f;
	err_get(&exception, &v);
	if (exception == NULL) {
		fprintf(stderr, "print_error called but no exception\n");
		abort();
	}
	if (exception == SystemExit) {
		if (v == NULL || v == None)
			goaway(0);
		if (is_intobject(v))
			goaway((int)getintvalue(v));
		else {
			/* OK to use real stderr here */
			printobject(v, stderr, PRINT_RAW);
			fprintf(stderr, "\n");
			goaway(1);
		}
	}
	sysset("last_type", exception);
	sysset("last_value", v);
	f = sysget("stderr");
	if (f == NULL)
		fprintf(stderr, "lost sys.stderr\n");
	else {
		printtraceback(f);
		if (exception == SyntaxError) {
			object *message;
			char *filename, *text;
			int lineno, offset;
			if (!getargs(v, "(O(ziiz))", &message,
				     &filename, &lineno, &offset, &text))
				err_clear();
			else {
				char buf[10];
				writestring("  File \"", f);
				if (filename == NULL)
					writestring("<string>", f);
				else
					writestring(filename, f);
				writestring("\", line ", f);
				sprintf(buf, "%d", lineno);
				writestring(buf, f);
				writestring("\n", f);
				if (text != NULL) {
					char *nl;
					if (offset > 0 &&
					    offset == strlen(text))
						offset--;
					for (;;) {
						nl = strchr(text, '\n');
						if (nl == NULL ||
						    nl-text >= offset)
							break;
						offset -= (nl+1-text);
						text = nl+1;
					}
					while (*text == ' ' || *text == '\t') {
						text++;
						offset--;
					}
					writestring("    ", f);
					writestring(text, f);
					if (*text == '\0' ||
					    text[strlen(text)-1] != '\n')
						writestring("\n", f);
					writestring("    ", f);
					offset--;
					while (offset > 0) {
						writestring(" ", f);
						offset--;
					}
					writestring("^\n", f);
				}
				INCREF(message);
				DECREF(v);
				v = message;
			}
		}
		if (writeobject(exception, f, PRINT_RAW) != 0)
			err_clear();
		if (v != NULL && v != None) {
			writestring(": ", f);
			if (writeobject(v, f, PRINT_RAW) != 0)
				err_clear();
		}
		writestring("\n", f);
	}
	XDECREF(exception);
	XDECREF(v);
}

object *
run_string(str, start, globals, locals)
	char *str;
	int start;
	object *globals, *locals;
{
	return run_err_node(parse_string(str, start),
			    "<string>", globals, locals);
}

object *
run_file(fp, filename, start, globals, locals)
	FILE *fp;
	char *filename;
	int start;
	object *globals, *locals;
{
	return run_err_node(parse_file(fp, filename, start),
			    filename, globals, locals);
}

static object *
run_err_node(n, filename, globals, locals)
	node *n;
	char *filename;
	object *globals, *locals;
{
	if (n == NULL)
		return  NULL;
	return run_node(n, filename, globals, locals);
}

static object *
run_node(n, filename, globals, locals)
	node *n;
	char *filename;
	object *globals, *locals;
{
	codeobject *co;
	object *v;
	co = compile(n, filename);
	freetree(n);
	if (co == NULL)
		return NULL;
	v = eval_code(co, globals, locals, (object *)NULL, (object *)NULL);
	DECREF(co);
	return v;
}

object *
run_pyc_file(fp, filename, globals, locals)
	FILE *fp;
	char *filename;
	object *globals, *locals;
{
	codeobject *co;
	object *v;
	long magic;
	long get_pyc_magic();

	magic = rd_long(fp);
	if (magic != get_pyc_magic()) {
		err_setstr(RuntimeError,
			   "Bad magic number in .pyc file");
		return NULL;
	}
	(void) rd_long(fp);
	v = rd_object(fp);
	fclose(fp);
	if (v == NULL || !is_codeobject(v)) {
		XDECREF(v);
		err_setstr(RuntimeError,
			   "Bad code object in .pyc file");
		return NULL;
	}
	co = (codeobject *)v;
	v = eval_code(co, globals, locals, (object *)NULL, (object *)NULL);
	DECREF(co);
	return v;
}

object *
compile_string(str, filename, start)
	char *str;
	char *filename;
	int start;
{
	node *n;
	int err;
	codeobject *co;
	n = parse_string(str, start);
	if (n == NULL)
		return NULL;
	co = compile(n, filename);
	freetree(n);
	return (object *)co;
}

/* Simplified interface to parsefile -- return node or set exception */

node *
parse_file(fp, filename, start)
	FILE *fp;
	char *filename;
	int start;
{
	node *n;
	perrdetail err;
	BGN_SAVE
	n = parsefile(fp, filename, &gram, start,
				(char *)0, (char *)0, &err);
	END_SAVE
	if (n == NULL)
		err_input(&err);
	return n;
}

/* Simplified interface to parsestring -- return node or set exception */

node *
parse_string(str, start)
	char *str;
	int start;
{
	node *n;
	perrdetail err;
	n = parsestring(str, &gram, start, &err);
	if (n == NULL)
		err_input(&err);
	return n;
}

/* Set the error appropriate to the given input error code (see errcode.h) */

static void
err_input(err)
	perrdetail *err;
{
	object *v, *w;
	char *msg = NULL;
	v = mkvalue("(ziiz)", err->filename,
			    err->lineno, err->offset, err->text);
	if (err->text != NULL) {
		free(err->text);
		err->text = NULL;
	}
	switch (err->error) {
	case E_SYNTAX:
		msg = "invalid syntax";
		break;
	case E_TOKEN:
		msg = "invalid token";

		break;
	case E_INTR:
		err_set(KeyboardInterrupt);
		return;
	case E_NOMEM:
		err_nomem();
		return;
	case E_EOF:
		msg = "unexpected EOF while parsing";
		break;
	default:
		fprintf(stderr, "error=%d\n", err->error);
		msg = "unknown parsing error";
		break;
	}
	w = mkvalue("(sO)", msg, v);
	XDECREF(v);
	err_setval(SyntaxError, w);
	XDECREF(w);
}

/* Print fatal error message and abort */

void
fatal(msg)
	char *msg;
{
	fprintf(stderr, "Fatal Python error: %s\n", msg);
	abort();
}

/* Clean up and exit */

#ifdef WITH_THREAD
#include "thread.h"
int threads_started = 0; /* Set by threadmodule.c and maybe others */
#endif

#define NEXITFUNCS 32
static void (*exitfuncs[NEXITFUNCS])();
static int nexitfuncs = 0;

int Py_AtExit(func)
	void (*func) PROTO((void));
{
	if (nexitfuncs >= NEXITFUNCS)
		return -1;
	exitfuncs[nexitfuncs++] = func;
	return 0;
}

void
cleanup()
{
	object *exitfunc = sysget("exitfunc");

	if (exitfunc) {
		object *arg;
		object *res;
		sysset("exitfunc", (object *)NULL);
		arg = newtupleobject(0);
		if (arg == NULL)
			res = NULL;
		else {
			res = call_object(exitfunc, arg);
			DECREF(arg);
		}
		if (res == NULL) {
			fprintf(stderr, "Error in sys.exitfunc:\n");
			print_error();
		}
	}

	flushline();

	while (nexitfuncs > 0)
		(*exitfuncs[--nexitfuncs])();
}

#ifdef COUNT_ALLOCS
extern void dump_counts PROTO((void));
#endif

void
goaway(sts)
	int sts;
{
	cleanup();

#ifdef COUNT_ALLOCS
	dump_counts();
#endif

#ifdef WITH_THREAD

	/* Other threads may still be active, so skip most of the
	   cleanup actions usually done (these are mostly for
	   debugging anyway). */
	
	(void) save_thread();
#ifndef NO_EXIT_PROG
	if (threads_started)
		_exit_prog(sts);
	else
		exit_prog(sts);
#else /* !NO_EXIT_PROG */
	if (threads_started)
		_exit(sts);
	else
		exit(sts);
#endif /* !NO_EXIT_PROG */
	
#else /* WITH_THREAD */
	
	doneimport();
	
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
#endif /* WITH_THREAD */
	/*NOTREACHED*/
}

#ifdef HAVE_SIGNAL_H
static RETSIGTYPE
sighandler(sig)
	int sig;
{
	signal(sig, SIG_DFL); /* Don't catch recursive signals */
	cleanup(); /* Do essential clean-up */
#ifdef HAVE_GETPID
	kill(getpid(), sig); /* Pretend the signal killed us */
#else
	exit(1);
#endif
	/*NOTREACHED*/
}
#endif

static void
initsigs()
{
	RETSIGTYPE (*t)();
#ifdef HAVE_SIGNAL_H
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGHUP
	t = signal(SIGHUP, SIG_IGN);
	if (t == SIG_DFL)
		signal(SIGHUP, sighandler);
	else
		signal(SIGHUP, t);
#endif              
#ifdef SIGTERM
	t = signal(SIGTERM, SIG_IGN);
	if (t == SIG_DFL)
		signal(SIGTERM, sighandler);
	else
		signal(SIGTERM, t);
#endif
#endif /* HAVE_SIGNAL_H */
	initintr(); /* May imply initsignal() */
}

#ifdef TRACE_REFS
/* Ask a yes/no question */

int
askyesno(prompt)
	char *prompt;
{
	char buf[256];
	
	printf("%s [ny] ", prompt);
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return 0;
	return buf[0] == 'y' || buf[0] == 'Y';
}
#endif

#ifdef MPW

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
}

#endif
