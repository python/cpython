/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* This code implemented by Dag.Gruneau@elsa.preseco.comm.se */

#include "windows.h"
#include "limits.h"

long get_thread_ident(void);

/*
 * Change all headers to pure ANSI as no one will use K&R style on an
 * NT
 */

/*
 * Initialization of the C package, should not be needed.
 */
static void _init_thread(void)
{
}

/*
 * Thread support.
 */
int start_new_thread(void (*func)(void *), void *arg)
{
	HANDLE aThread;
	DWORD aThreadId;

	int success = 0;        /* init not needed when SOLARIS_THREADS and */
                            /* C_THREADS implemented properly */
	dprintf(("%ld: start_new_thread called\n", get_thread_ident()));
	if (!initialized)
		init_thread();

	aThread = CreateThread(
				NULL,                           /* No security attributes               */
				0,                              /* use default stack size               */
				(LPTHREAD_START_ROUTINE) func,  /* thread function                      */
				(LPVOID) arg,                   /* argument to thread function          */
				0,                              /* use the default creation flags       */
				&aThreadId);                    /* returns the thread identifier       */
 
	if (aThread != NULL) {
		CloseHandle(aThread);   /* We do not want to have any zombies hanging around */
		success = 1;
		dprintf(("%ld: start_new_thread succeeded: %ld\n", get_thread_ident(), aThreadId));
	}

	return success;
}

/*
 * Return the thread Id instead of an handle. The Id is said to uniquely identify the
 * thread in the system
 */
long get_thread_ident(void)
{
	if (!initialized)
		init_thread();
        
	return GetCurrentThreadId();
}

static void do_exit_thread(int no_cleanup)
{
	dprintf(("%ld: exit_thread called\n", get_thread_ident()));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	ExitThread(0);
}

void exit_thread(void)
{
	do_exit_thread(0);
}

void _exit_thread(void)
{
	do_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_exit_prog(int status, int no_cleanup)
{
	dprintf(("exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void exit_prog(int status)
{
	do_exit_prog(status, 0);
}

void _exit_prog _P1(int status)
{
	do_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support. It has too be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
type_lock allocate_lock(void)
{
	HANDLE aLock;

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

		aLock = CreateSemaphore(NULL,           /* Security attributes          */
                         1,                     /* Initial value                */
                         1,                     /* Maximum value                */
                         NULL);       
  /* Name of semaphore            */

	dprintf(("%ld: allocate_lock() -> %lx\n", get_thread_ident(), (long)aLock));

	return (type_lock) aLock;
}

void free_lock(type_lock aLock)
{
	dprintf(("%ld: free_lock(%lx) called\n", get_thread_ident(),(long)aLock));

	CloseHandle((HANDLE) aLock);
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
int acquire_lock(type_lock aLock, int waitflag)
{
	int success = 1;
	DWORD waitResult;

	dprintf(("%ld: acquire_lock(%lx, %d) called\n", get_thread_ident(),(long)aLock, waitflag));

	waitResult = WaitForSingleObject((HANDLE) aLock, (waitflag == 1 ? INFINITE : 0));

	if (waitResult != WAIT_OBJECT_0) {
		success = 0;    /* We failed */
	}

	dprintf(("%ld: acquire_lock(%lx, %d) -> %d\n", get_thread_ident(),(long)aLock, waitflag, success));

	return success;
}

void release_lock(type_lock aLock)
{
	dprintf(("%ld: release_lock(%lx) called\n", get_thread_ident(),(long)aLock));

	if (!ReleaseSemaphore(
                        (HANDLE) aLock,                         /* Handle of semaphore                          */
                        1,                                      /* increment count by one                       */
                        NULL))                                  /* not interested in previous count             */
		{
		dprintf(("%ld: Could not release_lock(%lx) error: %l\n", get_thread_ident(), (long)aLock, GetLastError()));
		}
}

/*
 * Semaphore support.
 */
type_sema allocate_sema(int value)
{
	HANDLE aSemaphore;

	dprintf(("%ld: allocate_sema called\n", get_thread_ident()));
	if (!initialized)
		init_thread();

	aSemaphore = CreateSemaphore( NULL,           /* Security attributes          */
                                  value,          /* Initial value                */
                                  INT_MAX,        /* Maximum value                */
                                  NULL);          /* Name of semaphore            */

	dprintf(("%ld: allocate_sema() -> %lx\n", get_thread_ident(), (long)aSemaphore));

	return (type_sema) aSemaphore;
}

void free_sema(type_sema aSemaphore)
{
	dprintf(("%ld: free_sema(%lx) called\n", get_thread_ident(), (long)aSemaphore));

	CloseHandle((HANDLE) aSemaphore);
}

void down_sema(type_sema aSemaphore)
{
	DWORD waitResult;

	dprintf(("%ld: down_sema(%lx) called\n", get_thread_ident(), (long)aSemaphore));

	waitResult = WaitForSingleObject( (HANDLE) aSemaphore, INFINITE);

	dprintf(("%ld: down_sema(%lx) return: %l\n", get_thread_ident(),(long) aSemaphore, waitResult));
}

void up_sema(type_sema aSemaphore)
{
	ReleaseSemaphore(
                (HANDLE) aSemaphore,            /* Handle of semaphore                          */
                1,                              /* increment count by one                       */
                NULL);                          /* not interested in previous count             */
                                                
	dprintf(("%ld: up_sema(%lx)\n", get_thread_ident(), (long)aSemaphore));
}
