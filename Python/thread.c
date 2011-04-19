
/* Thread package.
   This is intended to be usable independently from Python.
   The implementation for system foobar is in a file thread_foobar.h
   which is included by this file dependent on config settings.
   Stuff shared by all thread_*.h files is collected here. */

#include "Python.h"


#ifndef _POSIX_THREADS
/* This means pthreads are not implemented in libc headers, hence the macro
   not present in unistd.h. But they still can be implemented as an external
   library (e.g. gnu pth in pthread emulation) */
# ifdef HAVE_PTHREAD_H
#  include <pthread.h> /* _POSIX_THREADS */
# endif
#endif

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#include <stdlib.h>

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

/* Check if we're running on HP-UX and _SC_THREADS is defined. If so, then
   enough of the Posix threads package is implemented to support python
   threads.

   This is valid for HP-UX 11.23 running on an ia64 system. If needed, add
   a check of __ia64 to verify that we're running on a ia64 system instead
   of a pa-risc system.
*/
#ifdef __hpux
#ifdef _SC_THREADS
#define _POSIX_THREADS
#endif
#endif

#endif /* _POSIX_THREADS */


#ifdef Py_DEBUG
static int thread_debug = 0;
#define dprintf(args)   (void)((thread_debug & 1) && printf args)
#define d2printf(args)  ((thread_debug & 8) && printf args)
#else
#define dprintf(args)
#define d2printf(args)
#endif

static int initialized;

static void PyThread__init_thread(void); /* Forward */

void
PyThread_init_thread(void)
{
#ifdef Py_DEBUG
    char *p = Py_GETENV("PYTHONTHREADDEBUG");

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

/* Support for runtime thread stack size tuning.
   A value of 0 means using the platform's default stack size
   or the size specified by the THREAD_STACK_SIZE macro. */
static size_t _pythread_stacksize = 0;

#ifdef SGI_THREADS
#error SGI Irix threads are now unsupported, and code will be removed in 3.3.
#include "thread_sgi.h"
#endif

#ifdef SOLARIS_THREADS
#define PYTHREAD_NAME "solaris"
#include "thread_solaris.h"
#endif

#ifdef SUN_LWP
#error SunOS lightweight processes are now unsupported, and code will be removed in 3.3.
#include "thread_lwp.h"
#endif

#ifdef HAVE_PTH
#error GNU pth threads are now unsupported, and code will be removed in 3.3.
#include "thread_pth.h"
#undef _POSIX_THREADS
#endif

#ifdef _POSIX_THREADS
#define PYTHREAD_NAME "pthread"
#include "thread_pthread.h"
#endif

#ifdef C_THREADS
#error Mach C Threads are now unsupported, and code will be removed in 3.3.
#include "thread_cthread.h"
#endif

#ifdef NT_THREADS
#define PYTHREAD_NAME "nt"
#include "thread_nt.h"
#endif

#ifdef OS2_THREADS
#define PYTHREAD_NAME "os2"
#include "thread_os2.h"
#endif

#ifdef PLAN9_THREADS
#define PYTHREAD_NAME "plan9"
#include "thread_plan9.h"
#endif

/*
#ifdef FOOBAR_THREADS
#include "thread_foobar.h"
#endif
*/

/* return the current thread stack size */
size_t
PyThread_get_stacksize(void)
{
    return _pythread_stacksize;
}

/* Only platforms defining a THREAD_SET_STACKSIZE() macro
   in thread_<platform>.h support changing the stack size.
   Return 0 if stack size is valid,
      -1 if stack size value is invalid,
      -2 if setting stack size is not supported. */
int
PyThread_set_stacksize(size_t size)
{
#if defined(THREAD_SET_STACKSIZE)
    return THREAD_SET_STACKSIZE(size);
#else
    return -2;
#endif
}

#ifndef Py_HAVE_NATIVE_TLS
/* If the platform has not supplied a platform specific
   TLS implementation, provide our own.

   This code stolen from "thread_sgi.h", where it was the only
   implementation of an existing Python TLS API.
*/
/* ------------------------------------------------------------------------
Per-thread data ("key") support.

Use PyThread_create_key() to create a new key.  This is typically shared
across threads.

Use PyThread_set_key_value(thekey, value) to associate void* value with
thekey in the current thread.  Each thread has a distinct mapping of thekey
to a void* value.  Caution:  if the current thread already has a mapping
for thekey, value is ignored.

Use PyThread_get_key_value(thekey) to retrieve the void* value associated
with thekey in the current thread.  This returns NULL if no value is
associated with thekey in the current thread.

Use PyThread_delete_key_value(thekey) to forget the current thread's associated
value for thekey.  PyThread_delete_key(thekey) forgets the values associated
with thekey across *all* threads.

While some of these functions have error-return values, none set any
Python exception.

None of the functions does memory management on behalf of the void* values.
You need to allocate and deallocate them yourself.  If the void* values
happen to be PyObject*, these functions don't do refcount operations on
them either.

The GIL does not need to be held when calling these functions; they supply
their own locking.  This isn't true of PyThread_create_key(), though (see
next paragraph).

There's a hidden assumption that PyThread_create_key() will be called before
any of the other functions are called.  There's also a hidden assumption
that calls to PyThread_create_key() are serialized externally.
------------------------------------------------------------------------ */

/* A singly-linked list of struct key objects remembers all the key->value
 * associations.  File static keyhead heads the list.  keymutex is used
 * to enforce exclusion internally.
 */
struct key {
    /* Next record in the list, or NULL if this is the last record. */
    struct key *next;

    /* The thread id, according to PyThread_get_thread_ident(). */
    long id;

    /* The key and its associated value. */
    int key;
    void *value;
};

static struct key *keyhead = NULL;
static PyThread_type_lock keymutex = NULL;
static int nkeys = 0;  /* PyThread_create_key() hands out nkeys+1 next */

/* Internal helper.
 * If the current thread has a mapping for key, the appropriate struct key*
 * is returned.  NB:  value is ignored in this case!
 * If there is no mapping for key in the current thread, then:
 *     If value is NULL, NULL is returned.
 *     Else a mapping of key to value is created for the current thread,
 *     and a pointer to a new struct key* is returned; except that if
 *     malloc() can't find room for a new struct key*, NULL is returned.
 * So when value==NULL, this acts like a pure lookup routine, and when
 * value!=NULL, this acts like dict.setdefault(), returning an existing
 * mapping if one exists, else creating a new mapping.
 *
 * Caution:  this used to be too clever, trying to hold keymutex only
 * around the "p->next = keyhead; keyhead = p" pair.  That allowed
 * another thread to mutate the list, via key deletion, concurrent with
 * find_key() crawling over the list.  Hilarity ensued.  For example, when
 * the for-loop here does "p = p->next", p could end up pointing at a
 * record that PyThread_delete_key_value() was concurrently free()'ing.
 * That could lead to anything, from failing to find a key that exists, to
 * segfaults.  Now we lock the whole routine.
 */
static struct key *
find_key(int key, void *value)
{
    struct key *p, *prev_p;
    long id = PyThread_get_thread_ident();

    if (!keymutex)
        return NULL;
    PyThread_acquire_lock(keymutex, 1);
    prev_p = NULL;
    for (p = keyhead; p != NULL; p = p->next) {
        if (p->id == id && p->key == key)
            goto Done;
        /* Sanity check.  These states should never happen but if
         * they do we must abort.  Otherwise we'll end up spinning in
         * in a tight loop with the lock held.  A similar check is done
         * in pystate.c tstate_delete_common().  */
        if (p == prev_p)
            Py_FatalError("tls find_key: small circular list(!)");
        prev_p = p;
        if (p->next == keyhead)
            Py_FatalError("tls find_key: circular list(!)");
    }
    if (value == NULL) {
        assert(p == NULL);
        goto Done;
    }
    p = (struct key *)malloc(sizeof(struct key));
    if (p != NULL) {
        p->id = id;
        p->key = key;
        p->value = value;
        p->next = keyhead;
        keyhead = p;
    }
 Done:
    PyThread_release_lock(keymutex);
    return p;
}

/* Return a new key.  This must be called before any other functions in
 * this family, and callers must arrange to serialize calls to this
 * function.  No violations are detected.
 */
int
PyThread_create_key(void)
{
    /* All parts of this function are wrong if it's called by multiple
     * threads simultaneously.
     */
    if (keymutex == NULL)
        keymutex = PyThread_allocate_lock();
    return ++nkeys;
}

/* Forget the associations for key across *all* threads. */
void
PyThread_delete_key(int key)
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

/* Confusing:  If the current thread has an association for key,
 * value is ignored, and 0 is returned.  Else an attempt is made to create
 * an association of key to value for the current thread.  0 is returned
 * if that succeeds, but -1 is returned if there's not enough memory
 * to create the association.  value must not be NULL.
 */
int
PyThread_set_key_value(int key, void *value)
{
    struct key *p;

    assert(value != NULL);
    p = find_key(key, value);
    if (p == NULL)
        return -1;
    else
        return 0;
}

/* Retrieve the value associated with key in the current thread, or NULL
 * if the current thread doesn't have an association for key.
 */
void *
PyThread_get_key_value(int key)
{
    struct key *p = find_key(key, NULL);

    if (p == NULL)
        return NULL;
    else
        return p->value;
}

/* Forget the current thread's association for key, if any. */
void
PyThread_delete_key_value(int key)
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

/* Forget everything not associated with the current thread id.
 * This function is called from PyOS_AfterFork().  It is necessary
 * because other thread ids which were in use at the time of the fork
 * may be reused for new threads created in the forked process.
 */
void
PyThread_ReInitTLS(void)
{
    long id = PyThread_get_thread_ident();
    struct key *p, **q;

    if (!keymutex)
        return;

    /* As with interpreter_lock in PyEval_ReInitThreads()
       we just create a new lock without freeing the old one */
    keymutex = PyThread_allocate_lock();

    /* Delete all keys which do not match the current thread id */
    q = &keyhead;
    while ((p = *q) != NULL) {
        if (p->id != id) {
            *q = p->next;
            free((void *)p);
            /* NB This does *not* free p->value! */
        }
        else
            q = &p->next;
    }
}

#endif /* Py_HAVE_NATIVE_TLS */

PyObject*
_PyThread_Info(void)
{
    PyObject *info, *value;
    int ret;
    char buffer[255];
    int len;

    info = PyDict_New();
    if (info == NULL)
        return NULL;

    value = PyUnicode_FromString(PYTHREAD_NAME);
    ret = PyDict_SetItemString(info, "name", value);
    Py_DECREF(value);
    if (ret)
        goto error;

#ifdef _POSIX_THREADS
#ifdef USE_SEMAPHORES
    value = PyUnicode_FromString("semaphore");
#else
    value = PyUnicode_FromString("mutex+cond");
#endif
    if (value == NULL)
        return NULL;
    ret = PyDict_SetItemString(info, "lock_implementation", value);
    Py_DECREF(value);
    if (ret)
        goto error;

#if defined(HAVE_CONFSTR) && defined(_CS_GNU_LIBPTHREAD_VERSION)
    len = confstr(_CS_GNU_LIBPTHREAD_VERSION, buffer, sizeof(buffer));
    if (0 < len && len < sizeof(buffer)) {
        value = PyUnicode_DecodeFSDefaultAndSize(buffer, len-1);
        if (value == NULL)
            goto error;
        ret = PyDict_SetItemString(info, "pthread_version", value);
        Py_DECREF(value);
        if (ret)
            goto error;
    }
#endif
#endif

    return info;

error:
    Py_DECREF(info);
    return NULL;
}
