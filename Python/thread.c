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

/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "config.h"

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
extern char *getenv();
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __DGUX
#define _USING_POSIX4A_DRAFT6
#endif

#ifdef __sgi
#ifndef HAVE_PTHREAD_H /* XXX Need to check in configure.in */
#undef _POSIX_THREADS
#endif
#endif

#include "thread.h"

#ifdef __ksr__
#define _POSIX_THREADS
#endif

#ifndef _POSIX_THREADS

#ifdef __sgi
#define SGI_THREADS
#endif

#ifdef HAVE_THREAD_H
#define SOLARIS_THREADS
#endif

#if defined(sun) && !defined(SOLARIS_THREADS)
#define SUN_LWP
#endif

#endif /* _POSIX_THREADS */

#ifdef __STDC__
#define _P(args)		args
#define _P0()			(void)
#define _P1(v,t)		(t)
#define _P2(v1,t1,v2,t2)	(t1,t2)
#else
#define _P(args)		()
#define _P0()			()
#define _P1(v,t)		(v) t;
#define _P2(v1,t1,v2,t2)	(v1,v2) t1; t2;
#endif /* __STDC__ */

#ifdef Py_DEBUG
static int thread_debug = 0;
#define dprintf(args)	((thread_debug & 1) && printf args)
#define d2printf(args)	((thread_debug & 8) && printf args)
#else
#define dprintf(args)
#define d2printf(args)
#endif

static int initialized;

static void _init_thread(); /* Forward */

void init_thread _P0()
{
#ifdef Py_DEBUG
	char *p = getenv("THREADDEBUG");

	if (p) {
		if (*p)
			thread_debug = atoi(p);
		else
			thread_debug = 1;
	}
#endif /* Py_DEBUG */
	if (initialized)
		return;
	initialized = 1;
	dprintf(("init_thread called\n"));
	_init_thread();
}

#ifdef SGI_THREADS
#include "thread_sgi.h"
#endif

#ifdef SOLARIS_THREADS
#include "thread_solaris.h"
#endif

#ifdef SUN_LWP
#include "thread_lwp.h"
#endif

#ifdef _POSIX_THREADS
#include "thread_pthread.h"
#endif

#ifdef C_THREADS
#include "thread_cthread.h"
#endif

#ifdef NT_THREADS
#include "thread_nt.h"
#endif

/*
#ifdef FOOBAR_THREADS
#include "thread_foobar.h"
#endif
*/
