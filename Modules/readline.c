/* The readline module makes GNU readline available to Python.  It
 * has ideas contributed by Lee Busby, LLNL, and William Magro,
 * Cornell Theory Center.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include <setjmp.h>
#include <signal.h>

/* Routines needed from outside (but not declared in a header file). */
extern int (*Py_input_hook)();
extern char *readline();
extern int rl_initialize();
extern int rl_insert();
extern int rl_bind_key();
extern void add_history();
extern char *rl_readline_name;
extern int (*rl_event_hook)();
extern char *(*PyOS_ReadlineFunctionPointer) Py_PROTO((char *));

/* This module's initialization routine */
void initreadline (void);

static void PyOS_ReadlineInit();
static RETSIGTYPE onintr();
static char *PyOS_GnuReadline();
static jmp_buf jbuf;
static PyObject *ReadlineError;
static int already_initialized = 0;
static char readline_module_documentation[] =
"Readline Module, version0.0"
;

static struct PyMethodDef readline_methods[] =
{ 
  { 0, 0 }
};

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

/* Initialize the module.  Actually, that's all you can or need to do. */
void initreadline (void)
{
  PyObject *m, *d;

  if (already_initialized)
    return;
  m = Py_InitModule4 ("readline", readline_methods,
		      readline_module_documentation,
		      (PyObject *) 0, PYTHON_API_VERSION);
  d = PyModule_GetDict (m);
  ReadlineError = PyString_FromString ("Readline.error");
  PyDict_SetItemString (d, "error", ReadlineError);
  if (PyErr_Occurred ()) {
    Py_FatalError ("Cannot initialize module readline");
  }
  if (isatty(fileno(stdin))){
    PyOS_ReadlineFunctionPointer = PyOS_GnuReadline;
    PyOS_ReadlineInit();
  }
  already_initialized = 1;
}

/* ARGSUSED */
static RETSIGTYPE
onintr(sig)
  int sig;
{
  longjmp(jbuf, 1);
}

static void
PyOS_ReadlineInit()
{
    /* Force rebind of TAB to insert-tab */
    rl_readline_name = "python";
    rl_initialize();
    rl_bind_key('\t', rl_insert);
}

static char *
PyOS_GnuReadline(prompt)
	char *prompt;
{
	int n;
	char *p;
	RETSIGTYPE (*old_inthandler)();
	old_inthandler = signal(SIGINT, onintr);
	if (setjmp(jbuf)) {
#ifdef HAVE_SIGRELSE
		/* This seems necessary on SunOS 4.1 (Rasmus Hahn) */
		sigrelse(SIGINT);
#endif
		signal(SIGINT, old_inthandler);
		return NULL;
	}
	rl_event_hook = Py_input_hook;
	p = readline(prompt);
	signal(SIGINT, old_inthandler);
	if (p == NULL) {
		p = malloc(1);
		if (p != NULL)
			*p = '\0';
		return p;
	}
	n = strlen(p);
	if (n > 0)
		add_history(p);
	if ((p = realloc(p, n+2)) != NULL) {
		p[n] = '\n';
		p[n+1] = '\0';
	}
	return p;
}

#ifdef __cplusplus
}
#endif
