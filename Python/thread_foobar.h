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

/*
 * Initialization.
 */
static void PyThread__init_thread _P0()
{
}

/*
 * Thread support.
 */
int PyThread_start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();
	return success < 0 ? 0 : 1;
}

long PyThread_get_thread_ident _P0()
{
	if (!initialized)
		PyThread_init_thread();
}

static void do_PyThread_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
}

void PyThread_exit_thread _P0()
{
	do_PyThread_exit_thread(0);
}

void PyThread__exit_thread _P0()
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog _P2(status, int status, no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void PyThread_exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 0);
}

void PyThread__exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock _P0()
{

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	dprintf(("PyThread_allocate_lock() -> %lx\n", (long)lock));
	return (PyThread_type_lock) lock;
}

void PyThread_free_lock _P1(lock, PyThread_type_lock lock)
{
	dprintf(("PyThread_free_lock(%lx) called\n", (long)lock));
}

int PyThread_acquire_lock _P2(lock, PyThread_type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("PyThread_acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
	dprintf(("PyThread_acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void PyThread_release_lock _P1(lock, PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%lx) called\n", (long)lock));
}

/*
 * Semaphore support.
 */
PyThread_type_sema PyThread_allocate_sema _P1(value, int value)
{
	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	dprintf(("PyThread_allocate_sema() -> %lx\n", (long) sema));
	return (PyThread_type_sema) sema;
}

void PyThread_free_sema _P1(sema, PyThread_type_sema sema)
{
	dprintf(("PyThread_free_sema(%lx) called\n", (long) sema));
}

int PyThread_down_sema _P2(sema, PyThread_type_sema sema, waitflag, int waitflag)
{
	dprintf(("PyThread_down_sema(%lx, %d) called\n", (long) sema, waitflag));
	dprintf(("PyThread_down_sema(%lx) return\n", (long) sema));
	return -1;
}

void PyThread_up_sema _P1(sema, PyThread_type_sema sema)
{
	dprintf(("PyThread_up_sema(%lx)\n", (long) sema));
}
