/* A multi-threaded telnet-like server that gives a Python prompt.

Usage: pysvr [port]

For security reasons, it only accepts requests from the current host.
This can still be insecure, but restricts violations from people who
can log in on your machine.  Use with caution!

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

/* XXX Umpfh.
   Python.h defines a typedef destructor, which conflicts with pthread.h.
   So Python.h must be included after pthread.h. */

#include <Python.h>

#ifndef PORT
#define PORT 4000
#endif

extern int optind;
extern char *optarg;
extern int getopt();

struct workorder {
	int conn;
	struct sockaddr_in addr;
};

/* Forward */
static void init_python(void);
static void usage(void);
static void oprogname(void);
static void main_thread(int);
static void create_thread(int, struct sockaddr_in *);
static void *service_thread(struct workorder *);
static void run_interpreter(FILE *, FILE *);
static int run_command(char *, PyObject *);

static char *progname = "pysvr";

main(int argc, char **argv)
{
	int port = PORT;
	int c;

	if (argc > 0 && argv[0] != NULL && argv[0][0] != '\0')
		progname = argv[0];

	while ((c = getopt(argc, argv, "")) != EOF) {
		switch (c) {
		default:
			usage();
		}
	}

	if (optind < argc) {
		if (optind+1 < argc) {
			oprogname();
			fprintf(stderr, "too many arguments\n");
			usage();
		}
		port = atoi(argv[optind]);
		if (port <= 0) {
			fprintf(stderr, "bad port (%s)\n", argv[optind]);
			usage();
		}
	}

	main_thread(port);

	fprintf(stderr, "Bye.\n");

	exit(0);
}

static char usage_line[] = "usage: %s [port]\n";

static void
usage()
{
	fprintf(stderr, usage_line, progname);
	exit(2);
}

static void
main_thread(int port)
{
	int sock, conn, size;
	struct sockaddr_in addr, clientaddr;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		oprogname();
		perror("can't create socket");
		exit(1);
	}

	memset(&addr, '\0', sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = 0L;
	if (bind(sock, (struct sockaddr *)&addr, sizeof addr) < 0) {
		oprogname();
		perror("can't bind socket to address");
		exit(1);
	}

	if (listen(sock, 5) < 0) {
		oprogname();
		perror("can't listen on socket");
		exit(1);
	}

	fprintf(stderr, "Listening on port %d...\n", port);

	for (;;) {
		size = sizeof clientaddr;
		conn = accept(sock, (struct sockaddr *) &clientaddr, &size);
		if (conn < 0) {
			oprogname();
			perror("can't accept connection from socket");
			exit(1);
		}

		size = sizeof addr;
		if (getsockname(conn, (struct sockaddr *)&addr, &size) < 0) {
			oprogname();
			perror("can't get socket name of connection");
			exit(1);
		}
		if (clientaddr.sin_addr.s_addr != addr.sin_addr.s_addr) {
			oprogname();
			perror("connection from non-local host refused");
			fprintf(stderr, "(addr=%lx, clientaddr=%lx)\n",
				ntohl(addr.sin_addr.s_addr),
				ntohl(clientaddr.sin_addr.s_addr));
			close(conn);
			continue;
		}
		create_thread(conn, &clientaddr);
	}
}

static void
create_thread(int conn, struct sockaddr_in *addr)
{
	struct workorder *work;
	pthread_t tdata;

	work = malloc(sizeof(struct workorder));
	if (work == NULL) {
		oprogname();
		fprintf(stderr, "out of memory for thread.\n");
		close(conn);
		return;
	}
	work->conn = conn;
	work->addr = *addr;

	init_python();

	if (pthread_create(&tdata, NULL, (void *)service_thread, work) < 0) {
		oprogname();
		perror("can't create new thread");
		close(conn);
		return;
	}

	if (pthread_detach(tdata) < 0) {
		oprogname();
		perror("can't detach from thread");
	}
}

static PyThreadState *the_tstate;
static PyInterpreterState *the_interp;
static PyObject *the_builtins;

static void
init_python()
{
	if (the_interp)
		return;
	Py_Initialize(); /* Initialize the interpreter */
	the_builtins = PyEval_GetBuiltins(); /* Get __builtins__ */
	PyEval_InitThreads(); /* Create and acquire the interpreter lock */
	the_tstate = PyEval_SaveThread(); /* Release lock & get thread state */
	the_interp = the_tstate->interpreter_state; /* Get interp state */
}

static void *
service_thread(struct workorder *work)
{
	FILE *input, *output;

	fprintf(stderr, "Start thread for connection %d.\n", work->conn);

	input = fdopen(work->conn, "r");
	if (input == NULL) {
		oprogname();
		perror("can't create input stream");
		goto done;
	}

	output = fdopen(work->conn, "w");
	if (output == NULL) {
		oprogname();
		perror("can't create output stream");
		fclose(input);
		goto done;
	}

	setvbuf(input, NULL, _IONBF, 0);
	setvbuf(output, NULL, _IONBF, 0);

	run_interpreter(input, output);

	fclose(input);
	fclose(output);

  done:
	fprintf(stderr, "End thread for connection %d.\n", work->conn);
	close(work->conn);
	free(work);
}

static void
oprogname()
{
	int save = errno;
	fprintf(stderr, "%s: ", progname);
	errno = save;
}

static void
run_interpreter(FILE *input, FILE *output)
{
	PyThreadState *tstate;
	PyObject *new_stdin, *new_stdout;
	PyObject *old_stdin, *old_stdout, *old_stderr;
	PyObject *globals;
	char buffer[1000];
	char *p, *q;
	int n, end;

	tstate = PyThreadState_New(the_interp);
	PyEval_AcquireThread(tstate);

	globals = PyDict_New();
	PyDict_SetItemString(globals, "__builtins__", the_builtins);

	new_stdin = PyFile_FromFile(input, "<socket-in>", "r", NULL);
	new_stdout = PyFile_FromFile(output, "<socket-out>", "w", NULL);

	old_stdin = PySys_GetObject("stdin");
	old_stdout = PySys_GetObject("stdout");
	old_stderr = PySys_GetObject("stderr");

	for (n = 1; !PyErr_Occurred(); n++) {
		Py_BEGIN_ALLOW_THREADS
		fprintf(output, "%d> ", n);
		p = fgets(buffer, sizeof buffer, input);
		Py_END_ALLOW_THREADS

		if (p == NULL)
			break;
		if (p[0] == '\377' && p[1] == '\354')
			break;

		q = strrchr(p, '\r');
		if (q && q[1] == '\n' && q[2] == '\0') {
			*q++ = '\n';
			*q++ = '\0';
		}

		while (*p && isspace(*p))
			p++;
		if (p[0] == '#' || p[0] == '\0')
			continue;

		PySys_SetObject("stdin", new_stdin);
		PySys_SetObject("stdout", new_stdout);
		PySys_SetObject("stderr", new_stdout);

		end = run_command(buffer, globals);
		if (end < 0)
			PyErr_Print();

		PySys_SetObject("stdin", old_stdin);
		PySys_SetObject("stdout", old_stdout);
		PySys_SetObject("stderr", old_stderr);

		if (end)
			break;
	}

	Py_XDECREF(globals);
	Py_XDECREF(new_stdin);
	Py_XDECREF(new_stdout);

	PyEval_ReleaseThread(tstate);
	PyThreadState_Delete(tstate);

	fprintf(output, "Goodbye!\n");
}

static int
run_command(char *buffer, PyObject *globals)
{
	PyObject *m, *d, *v;
	v = PyRun_String(buffer, Py_single_input, globals, globals);
	if (v == NULL) {
		if (PyErr_Occurred() == PyExc_SystemExit) {
			PyErr_Clear();
			return 1;
		}
		PyErr_Print();
		return 0;
	}
	Py_DECREF(v);
	return 0;
}
