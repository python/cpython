
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "config.h"

/* config.h may or may not define DL_IMPORT */
#ifndef DL_IMPORT	/* declarations for DLL import/export */
#define DL_IMPORT(RTYPE) RTYPE
#endif

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#ifdef Py_DEBUG
extern char *getenv(const char *);
#endif
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

#include "pythread.h"

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

#if defined(__MWERKS__) && !defined(__BEOS__)
#define _POSIX_THREADS
#endif

#endif /* _POSIX_THREADS */


#ifdef Py_DEBUG
static int thread_debug = 0;
#define dprintf(args)	((thread_debug & 1) && printf args)
#define d2printf(args)	((thread_debug & 8) && printf args)
#else
#define dprintf(args)
#define d2printf(args)
#endif

static int initialized;

static void PyThread__init_thread(void); /* Forward */

void PyThread_init_thread(void)
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
	dprintf(("PyThread_init_thread called\n"));
	PyThread__init_thread();
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

#ifdef HAVE_PTH
#include "thread_pth.h"
#undef _POSIX_THREADS
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

#ifdef OS2_THREADS
#include "thread_os2.h"
#endif

#ifdef BEOS_THREADS
#include "thread_beos.h"
#endif

#ifdef WINCE_THREADS
#include "thread_wince.h"
#endif

/*
#ifdef FOOBAR_THREADS
#include "thread_foobar.h"
#endif
*/
