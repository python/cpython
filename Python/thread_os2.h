/* This code implemented by cvale@netcom.com */

#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include "os2.h"
#include "limits.h"

#include "process.h"

long PyThread_get_thread_ident(void);


/*
 * Initialization of the C package, should not be needed.
 */
static void
PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */
int
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
  int aThread;
  int success = 1;

  aThread = _beginthread(func,NULL,65536,arg);

  if( aThread == -1 ) {
    success = 0;
    fprintf(stderr,"aThread failed == %d",aThread);
    dprintf(("_beginthread failed. return %ld\n", errno));
  }

  return success;
}

long
PyThread_get_thread_ident(void)
{
  PPIB pib;
  PTIB tib;

  if (!initialized)
    PyThread_init_thread();
        
  DosGetInfoBlocks(&tib,&pib);
  return tib->tib_ptib2->tib2_ultid;
}

static void
do_PyThread_exit_thread(int no_cleanup)
{
  dprintf(("%ld: PyThread_exit_thread called\n", PyThread_get_thread_ident()));
  if (!initialized)
    if (no_cleanup)
      _exit(0);
    else
      exit(0);
  _endthread();
}

void 
PyThread_exit_thread(void)
{
  do_PyThread_exit_thread(0);
}

void 
PyThread__exit_thread(void)
{
  do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void 
do_PyThread_exit_prog(int status, int no_cleanup)
{
  dprintf(("PyThread_exit_prog(%d) called\n", status));
  if (!initialized)
    if (no_cleanup)
      _exit(status);
    else
      exit(status);
}

void 
PyThread_exit_prog(int status)
{
  do_PyThread_exit_prog(status, 0);
}

void 
PyThread__exit_prog(int status)
{
  do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support. It has too be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
PyThread_type_lock 
PyThread_allocate_lock(void)
{
  HMTX   aLock;
  APIRET rc;

  dprintf(("PyThread_allocate_lock called\n"));
  if (!initialized)
    PyThread_init_thread();

  DosCreateMutexSem(NULL,  /* Sem name      */
                    &aLock, /* the semaphore */
                    0,     /* shared ?      */
                    0);    /* initial state */  

  dprintf(("%ld: PyThread_allocate_lock() -> %p\n", PyThread_get_thread_ident(), aLock));

  return (PyThread_type_lock) aLock;
}

void 
PyThread_free_lock(PyThread_type_lock aLock)
{
  dprintf(("%ld: PyThread_free_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

  DosCloseMutexSem((HMTX)aLock);
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
int 
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
  int   success = 1;
  ULONG rc, count;
  PID   pid = 0;
  TID   tid = 0;

  dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n", PyThread_get_thread_ident(),
           aLock, waitflag));

  DosQueryMutexSem((HMTX)aLock,&pid,&tid,&count);
  if( tid == PyThread_get_thread_ident() ) { /* if we own this lock */
    success = 0;
  } else {
    rc = DosRequestMutexSem((HMTX) aLock,
                            (waitflag == 1 ? SEM_INDEFINITE_WAIT : 0));
    
    if( rc != 0) {
      success = 0;                /* We failed */
    }
  }

  dprintf(("%ld: PyThread_acquire_lock(%p, %d) -> %d\n",
           PyThread_get_thread_ident(),aLock, waitflag, success));

  return success;
}

void 
PyThread_release_lock(PyThread_type_lock aLock)
{
  dprintf(("%ld: PyThread_release_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

  if ( DosReleaseMutexSem( (HMTX) aLock ) != 0 ) {
    dprintf(("%ld: Could not PyThread_release_lock(%p) error: %l\n",
             PyThread_get_thread_ident(), aLock, GetLastError()));
  }
}

/*
 * Semaphore support.
 */
PyThread_type_sema 
PyThread_allocate_sema(int value)
{
  return (PyThread_type_sema) 0;
}

void 
PyThread_free_sema(PyThread_type_sema aSemaphore)
{

}

int 
PyThread_down_sema(PyThread_type_sema aSemaphore, int waitflag)
{
  return -1;
}

void 
PyThread_up_sema(PyThread_type_sema aSemaphore)
{
  dprintf(("%ld: PyThread_up_sema(%p)\n", PyThread_get_thread_ident(), aSemaphore));
}
