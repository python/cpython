
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "Python.h"

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

#ifdef __DGUX
#define _USING_POSIX4A_DRAFT6
#endif

#ifdef __sgi
#ifndef HAVE_PTHREAD_H /* XXX Need to check in configure.in */
#undef _POSIX_THREADS
#endif
#endif

#include "pythread.h"

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
#define dprintf(args)	(void)((thread_debug & 1) && printf args)
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

#ifdef PLAN9_THREADS
#include "thread_plan9.h"
#endif

#ifdef ATHEOS_THREADS
#include "thread_atheos.h"
#endif

/*
#ifdef FOOBAR_THREADS
#include "thread_foobar.h"
#endif
*/

#ifndef Py_HAVE_NATIVE_TLS
/* If the platform has not supplied a platform specific
   TLS implementation, provide our own.

   This code stolen from "thread_sgi.h", where it was the only
   implementation of an existing Python TLS API.
*/
/*
 * Per-thread data ("key") support.
 */

struct key {
	struct key *next;
	long id;
	int key;
	void *value;
};

static struct key *keyhead = NULL;
static int nkeys = 0;
static PyThread_type_lock keymutex = NULL;

static struct key *find_key(int key, void *value)
{
	struct key *p;
	long id = PyThread_get_thread_ident();
	for (p = keyhead; p != NULL; p = p->next) {
		if (p->id == id && p->key == key)
			return p;
	}
	if (value == NULL)
		return NULL;
	p = (struct key *)malloc(sizeof(struct key));
	if (p != NULL) {
		p->id = id;
		p->key = key;
		p->value = value;
		PyThread_acquire_lock(keymutex, 1);
		p->next = keyhead;
		keyhead = p;
		PyThread_release_lock(keymutex);
	}
	return p;
}

int PyThread_create_key(void)
{
	if (keymutex == NULL)
		keymutex = PyThread_allocate_lock();
	return ++nkeys;
}

void PyThread_delete_key(int key)
{
	struct key *p, **q;
	PyThread_acquire_lock(keymutex, 1);
	q = &keyhead;
	while ((p = *q) != NULL) {
		if (p->key == key) {
			*q = p->next;
			free((void *)p);
			/* NB This does *not* free p->value! */
		}
		else
			q = &p->next;
	}
	PyThread_release_lock(keymutex);
}

int PyThread_set_key_value(int key, void *value)
{
	struct key *p = find_key(key, value);
	if (p == NULL)
		return -1;
	else
		return 0;
}

void *PyThread_get_key_value(int key)
{
	struct key *p = find_key(key, NULL);
	if (p == NULL)
		return NULL;
	else
		return p->value;
}

void PyThread_delete_key_value(int key)
{
	long id = PyThread_get_thread_ident();
	struct key *p, **q;
	PyThread_acquire_lock(keymutex, 1);
	q = &keyhead;
	while ((p = *q) != NULL) {
		if (p->key == key && p->id == id) {
			*q = p->next;
			free((void *)p);
			/* NB This does *not* free p->value! */
			break;
		}
		else
			q = &p->next;
	}
	PyThread_release_lock(keymutex);
}

#endif /* Py_HAVE_NATIVE_TLS */
