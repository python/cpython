/*
 * tclMacOSXNotify.c --
 *
 *	This file contains the implementation of a merged CFRunLoop/select()
 *	based notifier, which is the lowest-level part of the Tcl event loop.
 *	This file works together with generic/tclNotify.c.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#ifdef HAVE_COREFOUNDATION	/* Traditional unix select-based notifier is
				 * in tclUnixNotfy.c */
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>

/* #define TCL_MAC_DEBUG_NOTIFIER 1 */

/*
 * We use the Darwin-native spinlock API rather than pthread mutexes for
 * notifier locking: this radically simplifies the implementation and lowers
 * overhead. Note that these are not pure spinlocks, they employ various
 * strategies to back off and relinquish the processor, making them immune to
 * most priority-inversion livelocks (c.f. 'man 3 OSSpinLockLock' and Darwin
 * sources: xnu/osfmk/{ppc,i386}/commpage/spinlocks.s).
 */

#if defined(HAVE_LIBKERN_OSATOMIC_H) && defined(HAVE_OSSPINLOCKLOCK)
/*
 * Use OSSpinLock API where available (Tiger or later).
 */

#include <libkern/OSAtomic.h>

#if defined(HAVE_WEAK_IMPORT) && MAC_OS_X_VERSION_MIN_REQUIRED < 1040
/*
 * Support for weakly importing spinlock API.
 */
#define WEAK_IMPORT_SPINLOCKLOCK
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
#define VOLATILE volatile
#else
#define VOLATILE
#endif /* MAC_OS_X_VERSION_MAX_ALLOWED >= 1050 */
#ifndef bool
#define bool int
#endif
extern void		OSSpinLockLock(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
extern void		OSSpinLockUnlock(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
extern bool		OSSpinLockTry(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
extern void		_spin_lock(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
extern void		_spin_unlock(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
extern bool		_spin_lock_try(VOLATILE OSSpinLock *lock)
			    WEAK_IMPORT_ATTRIBUTE;
static void (* lockLock)(VOLATILE OSSpinLock *lock) = NULL;
static void (* lockUnlock)(VOLATILE OSSpinLock *lock) = NULL;
static bool (* lockTry)(VOLATILE OSSpinLock *lock) = NULL;
#undef VOLATILE
static pthread_once_t spinLockLockInitControl = PTHREAD_ONCE_INIT;
static void
SpinLockLockInit(void)
{
    lockLock   = OSSpinLockLock   != NULL ? OSSpinLockLock   : _spin_lock;
    lockUnlock = OSSpinLockUnlock != NULL ? OSSpinLockUnlock : _spin_unlock;
    lockTry    = OSSpinLockTry    != NULL ? OSSpinLockTry    : _spin_lock_try;
    if (lockLock == NULL || lockUnlock == NULL) {
	Tcl_Panic("SpinLockLockInit: no spinlock API available");
    }
}
#define SpinLockLock(p) 	lockLock(p)
#define SpinLockUnlock(p)	lockUnlock(p)
#define SpinLockTry(p)		lockTry(p)
#else
#define SpinLockLock(p) 	OSSpinLockLock(p)
#define SpinLockUnlock(p)	OSSpinLockUnlock(p)
#define SpinLockTry(p)		OSSpinLockTry(p)
#endif /* HAVE_WEAK_IMPORT */
#define SPINLOCK_INIT		OS_SPINLOCK_INIT

#else
/*
 * Otherwise, use commpage spinlock SPI directly.
 */

typedef uint32_t OSSpinLock;
extern void		_spin_lock(OSSpinLock *lock);
extern void		_spin_unlock(OSSpinLock *lock);
extern int		_spin_lock_try(OSSpinLock *lock);
#define SpinLockLock(p) 	_spin_lock(p)
#define SpinLockUnlock(p)	_spin_unlock(p)
#define SpinLockTry(p)		_spin_lock_try(p)
#define SPINLOCK_INIT		0

#endif /* HAVE_LIBKERN_OSATOMIC_H && HAVE_OSSPINLOCKLOCK */

/*
 * These spinlocks lock access to the global notifier state.
 */

static OSSpinLock notifierInitLock = SPINLOCK_INIT;
static OSSpinLock notifierLock     = SPINLOCK_INIT;

/*
 * Macros abstracting notifier locking/unlocking
 */

#define LOCK_NOTIFIER_INIT	SpinLockLock(&notifierInitLock)
#define UNLOCK_NOTIFIER_INIT	SpinLockUnlock(&notifierInitLock)
#define LOCK_NOTIFIER		SpinLockLock(&notifierLock)
#define UNLOCK_NOTIFIER		SpinLockUnlock(&notifierLock)
#define LOCK_NOTIFIER_TSD	SpinLockLock(&tsdPtr->tsdLock)
#define UNLOCK_NOTIFIER_TSD	SpinLockUnlock(&tsdPtr->tsdLock)

#ifdef TCL_MAC_DEBUG_NOTIFIER
#define TclMacOSXNotifierDbgMsg(m, ...) \
    do {								\
	fprintf(notifierLog?notifierLog:stderr, "tclMacOSXNotify.c:%d: " \
		"%s() pid %5d thread %10p: " m "\n", __LINE__, __func__, \
		getpid(), pthread_self(), ##__VA_ARGS__);		\
	fflush(notifierLog?notifierLog:stderr);				\
    } while (0)

/*
 * Debug version of SpinLockLock that logs the time spent waiting for the lock
 */

#define SpinLockLockDbg(p) \
    if (!SpinLockTry(p)) {						\
	Tcl_WideInt s = TclpGetWideClicks(), e;				\
									\
	SpinLockLock(p);						\
	e = TclpGetWideClicks();					\
	TclMacOSXNotifierDbgMsg("waited on %s for %8.0f ns",		\
		#p, TclpWideClicksToNanoseconds(e-s));			\
    }
#undef LOCK_NOTIFIER_INIT
#define LOCK_NOTIFIER_INIT	SpinLockLockDbg(&notifierInitLock)
#undef LOCK_NOTIFIER
#define LOCK_NOTIFIER		SpinLockLockDbg(&notifierLock)
#undef LOCK_NOTIFIER_TSD
#define LOCK_NOTIFIER_TSD	SpinLockLockDbg(&tsdPtr->tsdLock)
#include <asl.h>
static FILE *notifierLog = NULL;
#ifndef NOTIFIER_LOG
#define NOTIFIER_LOG "/tmp/tclMacOSXNotify.log"
#endif
#define OPEN_NOTIFIER_LOG \
    if (!notifierLog) {							\
	notifierLog = fopen(NOTIFIER_LOG, "a");				\
	/*TclMacOSXNotifierDbgMsg("open log");				\
	 *asl_set_filter(NULL,						\
	 *	ASL_FILTER_MASK_UPTO(ASL_LEVEL_DEBUG));			\
	 *asl_add_log_file(NULL, fileno(notifierLog));*/		\
    }
#define CLOSE_NOTIFIER_LOG \
    if (notifierLog) {							\
	/*asl_remove_log_file(NULL, fileno(notifierLog));		\
	 *TclMacOSXNotifierDbgMsg("close log");*/			\
	fclose(notifierLog);						\
	notifierLog = NULL;						\
    }
#define ENABLE_ASL \
    if (notifierLog) {							\
	/*tsdPtr->asl = asl_open(NULL, "com.apple.console",		\
	 *	ASL_OPT_NO_REMOTE);					\
	 *asl_set_filter(tsdPtr->asl,					\
	 *	ASL_FILTER_MASK_UPTO(ASL_LEVEL_DEBUG));			\
	 *asl_add_log_file(tsdPtr->asl, fileno(notifierLog));*/		\
    }
#define DISABLE_ASL \
    /*if (tsdPtr->asl) {						\
     *	if (notifierLog) {						\
     *	    asl_remove_log_file(tsdPtr->asl, fileno(notifierLog));	\
     *	}								\
     *	asl_close(tsdPtr->asl);						\
     *}*/
#define ASLCLIENT_DECL		/*aslclient asl*/
#else
#define TclMacOSXNotifierDbgMsg(m, ...)
#define OPEN_NOTIFIER_LOG
#define CLOSE_NOTIFIER_LOG
#define ENABLE_ASL
#define DISABLE_ASL
#define ASLCLIENT_DECL
#endif /* TCL_MAC_DEBUG_NOTIFIER */

/*
 * This structure is used to keep track of the notifier info for a registered
 * file.
 */

typedef struct FileHandler {
    int fd;
    int mask;			/* Mask of desired events: TCL_READABLE,
				 * etc. */
    int readyMask;		/* Mask of events that have been seen since
				 * the last time file handlers were invoked
				 * for this file. */
    Tcl_FileProc *proc;		/* Function to call, in the style of
				 * Tcl_CreateFileHandler. */
    ClientData clientData;	/* Argument to pass to proc. */
    struct FileHandler *nextPtr;/* Next in list of all files we care about. */
} FileHandler;

/*
 * The following structure is what is added to the Tcl event queue when file
 * handlers are ready to fire.
 */

typedef struct FileHandlerEvent {
    Tcl_Event header;		/* Information that is standard for all
				 * events. */
    int fd;			/* File descriptor that is ready. Used to find
				 * the FileHandler structure for the file
				 * (can't point directly to the FileHandler
				 * structure because it could go away while
				 * the event is queued). */
} FileHandlerEvent;

/*
 * The following structure contains a set of select() masks to track readable,
 * writable, and exceptional conditions.
 */

typedef struct SelectMasks {
    fd_set readable;
    fd_set writable;
    fd_set exceptional;
} SelectMasks;

/*
 * The following static structure contains the state information for the
 * select based implementation of the Tcl notifier. One of these structures is
 * created for each thread that is using the notifier.
 */

typedef struct ThreadSpecificData {
    FileHandler *firstFileHandlerPtr;
				/* Pointer to head of file handler list. */
    int polled;			/* True if the notifier thread has polled for
				 * this thread. */
    int sleeping;		/* True if runloop is inside Tcl_Sleep. */
    int runLoopSourcePerformed;	/* True after the runLoopSource callack was
				 * performed. */
    int runLoopRunning;		/* True if this thread's Tcl runLoop is
				 * running. */
    int runLoopNestingLevel;	/* Level of nested runLoop invocations. */
    int runLoopServicingEvents;	/* True if this thread's runLoop is servicing
				 * Tcl events. */

    /* Must hold the notifierLock before accessing the following fields: */
    /* Start notifierLock section */
    int onList;			/* True if this thread is on the
				 * waitingList */
    struct ThreadSpecificData *nextPtr, *prevPtr;
				/* All threads that are currently waiting on
				 * an event have their ThreadSpecificData
				 * structure on a doubly-linked listed formed
				 * from these pointers. */
    /* End notifierLock section */

    OSSpinLock tsdLock;		/* Must hold this lock before acessing the
				 * following fields from more than one
				 * thread. */
    /* Start tsdLock section */
    SelectMasks checkMasks;	/* This structure is used to build up the
				 * masks to be used in the next call to
				 * select. Bits are set in response to calls
				 * to Tcl_CreateFileHandler. */
    SelectMasks readyMasks;	/* This array reflects the readable/writable
				 * conditions that were found to exist by the
				 * last call to select. */
    int numFdBits;		/* Number of valid bits in checkMasks (one
				 * more than highest fd for which
				 * Tcl_WatchFile has been called). */
    int polling;		/* True if this thread is polling for
				 * events. */
    CFRunLoopRef runLoop;	/* This thread's CFRunLoop, needs to be woken
				 * up whenever the runLoopSource is
				 * signaled. */
    CFRunLoopSourceRef runLoopSource;
				/* Any other thread alerts a notifier that an
				 * event is ready to be processed by signaling
				 * this CFRunLoopSource. */
    CFRunLoopObserverRef runLoopObserver, runLoopObserverTcl;
				/* Adds/removes this thread from waitingList
				 * when the CFRunLoop starts/stops. */
    CFRunLoopTimerRef runLoopTimer;
				/* Wakes up CFRunLoop after given timeout when
				 * running embedded. */
    /* End tsdLock section */

    CFTimeInterval waitTime;	/* runLoopTimer wait time when running
				 * embedded. */
    ASLCLIENT_DECL;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following static indicates the number of threads that have initialized
 * notifiers.
 *
 * You must hold the notifierInitLock before accessing this variable.
 */

static int notifierCount = 0;

/*
 * The following variable points to the head of a doubly-linked list of
 * ThreadSpecificData structures for all threads that are currently waiting on
 * an event.
 *
 * You must hold the notifierLock before accessing this list.
 */

static ThreadSpecificData *waitingListPtr = NULL;

/*
 * The notifier thread spends all its time in select() waiting for a file
 * descriptor associated with one of the threads on the waitingListPtr list to
 * do something interesting. But if the contents of the waitingListPtr list
 * ever changes, we need to wake up and restart the select() system call. You
 * can wake up the notifier thread by writing a single byte to the file
 * descriptor defined below. This file descriptor is the input-end of a pipe
 * and the notifier thread is listening for data on the output-end of the same
 * pipe. Hence writing to this file descriptor will cause the select() system
 * call to return and wake up the notifier thread.
 *
 * You must hold the notifierLock lock before writing to the pipe.
 */

static int triggerPipe = -1;
static int receivePipe = -1; /* Output end of triggerPipe */

/*
 * The following static indicates if the notifier thread is running.
 *
 * You must hold the notifierInitLock before accessing this variable.
 */

static int notifierThreadRunning;

/*
 * This is the thread ID of the notifier thread that does select. Only valid
 * when notifierThreadRunning is non-zero.
 *
 * You must hold the notifierInitLock before accessing this variable.
 */

static pthread_t notifierThread;

/*
 * Custom runloop mode for running with only the runloop source for the
 * notifier thread.
 */

#ifndef TCL_EVENTS_ONLY_RUN_LOOP_MODE
#define TCL_EVENTS_ONLY_RUN_LOOP_MODE	"com.tcltk.tclEventsOnlyRunLoopMode"
#endif
#ifdef __CONSTANT_CFSTRINGS__
#define tclEventsOnlyRunLoopMode	CFSTR(TCL_EVENTS_ONLY_RUN_LOOP_MODE)
#else
static CFStringRef tclEventsOnlyRunLoopMode = NULL;
#endif

/*
 * CFTimeInterval to wait forever.
 */

#define CF_TIMEINTERVAL_FOREVER 5.05e8

/*
 * Static routines defined in this file.
 */

static void		StartNotifierThread(void);
static void		NotifierThreadProc(ClientData clientData)
			    __attribute__ ((__noreturn__));
static int		FileHandlerEventProc(Tcl_Event *evPtr, int flags);
static void		TimerWakeUp(CFRunLoopTimerRef timer, void *info);
static void		QueueFileEvents(void *info);
static void		UpdateWaitingListAndServiceEvents(
			    CFRunLoopObserverRef observer,
			    CFRunLoopActivity activity, void *info);
static int		OnOffWaitingList(ThreadSpecificData *tsdPtr,
			    int onList, int signalNotifier);

#ifdef HAVE_PTHREAD_ATFORK
static int atForkInit = 0;
static void		AtForkPrepare(void);
static void		AtForkParent(void);
static void		AtForkChild(void);
#if defined(HAVE_WEAK_IMPORT) && MAC_OS_X_VERSION_MIN_REQUIRED < 1040
/* Support for weakly importing pthread_atfork. */
#define WEAK_IMPORT_PTHREAD_ATFORK
extern int		pthread_atfork(void (*prepare)(void),
			    void (*parent)(void), void (*child)(void))
			    WEAK_IMPORT_ATTRIBUTE;
#define MayUsePthreadAtfork()	(pthread_atfork != NULL)
#else
#define MayUsePthreadAtfork()	(1)
#endif /* HAVE_WEAK_IMPORT */

/*
 * On Darwin 9 and later, it is not possible to call CoreFoundation after
 * a fork.
 */

#if !defined(MAC_OS_X_VERSION_MIN_REQUIRED) || \
	MAC_OS_X_VERSION_MIN_REQUIRED < 1050
MODULE_SCOPE long tclMacOSXDarwinRelease;
#define noCFafterFork	(tclMacOSXDarwinRelease >= 9)
#else /* MAC_OS_X_VERSION_MIN_REQUIRED */
#define noCFafterFork	1
#endif /* MAC_OS_X_VERSION_MIN_REQUIRED */
#endif /* HAVE_PTHREAD_ATFORK */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitNotifier --
 *
 *	Initializes the platform specific notifier state.
 *
 * Results:
 *	Returns a handle to the notifier state for this thread.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_InitNotifier(void)
{
    ThreadSpecificData *tsdPtr;

    if (tclNotifierHooks.initNotifierProc) {
	return tclNotifierHooks.initNotifierProc();
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);

#ifdef WEAK_IMPORT_SPINLOCKLOCK
    /*
     * Initialize support for weakly imported spinlock API.
     */

    if (pthread_once(&spinLockLockInitControl, SpinLockLockInit)) {
	Tcl_Panic("Tcl_InitNotifier: pthread_once failed");
    }
#endif

#ifndef __CONSTANT_CFSTRINGS__
    if (!tclEventsOnlyRunLoopMode) {
	tclEventsOnlyRunLoopMode = CFSTR(TCL_EVENTS_ONLY_RUN_LOOP_MODE);
    }
#endif

    /*
     * Initialize CFRunLoopSource and add it to CFRunLoop of this thread.
     */

    if (!tsdPtr->runLoop) {
	CFRunLoopRef runLoop = CFRunLoopGetCurrent();
	CFRunLoopSourceRef runLoopSource;
	CFRunLoopSourceContext runLoopSourceContext;
	CFRunLoopObserverContext runLoopObserverContext;
	CFRunLoopObserverRef runLoopObserver, runLoopObserverTcl;

	bzero(&runLoopSourceContext, sizeof(CFRunLoopSourceContext));
	runLoopSourceContext.info = tsdPtr;
	runLoopSourceContext.perform = QueueFileEvents;
	runLoopSource = CFRunLoopSourceCreate(NULL, LONG_MIN,
		&runLoopSourceContext);
	if (!runLoopSource) {
	    Tcl_Panic("Tcl_InitNotifier: could not create CFRunLoopSource");
	}
	CFRunLoopAddSource(runLoop, runLoopSource, kCFRunLoopCommonModes);
	CFRunLoopAddSource(runLoop, runLoopSource, tclEventsOnlyRunLoopMode);

	bzero(&runLoopObserverContext, sizeof(CFRunLoopObserverContext));
	runLoopObserverContext.info = tsdPtr;
	runLoopObserver = CFRunLoopObserverCreate(NULL,
		kCFRunLoopEntry|kCFRunLoopExit|kCFRunLoopBeforeWaiting, TRUE,
		LONG_MIN, UpdateWaitingListAndServiceEvents,
		&runLoopObserverContext);
	if (!runLoopObserver) {
	    Tcl_Panic("Tcl_InitNotifier: could not create "
		    "CFRunLoopObserver");
	}
	CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopCommonModes);

	/*
	 * Create a second CFRunLoopObserver with the same callback as above
	 * for the tclEventsOnlyRunLoopMode to ensure that the callback can be
	 * re-entered via Tcl_ServiceAll() in the kCFRunLoopBeforeWaiting case
	 * (CFRunLoop prevents observer callback re-entry of a given observer
	 * instance).
	 */

	runLoopObserverTcl = CFRunLoopObserverCreate(NULL,
		kCFRunLoopEntry|kCFRunLoopExit|kCFRunLoopBeforeWaiting, TRUE,
		LONG_MIN, UpdateWaitingListAndServiceEvents,
		&runLoopObserverContext);
	if (!runLoopObserverTcl) {
	    Tcl_Panic("Tcl_InitNotifier: could not create "
		    "CFRunLoopObserver");
	}
	CFRunLoopAddObserver(runLoop, runLoopObserverTcl,
		tclEventsOnlyRunLoopMode);

	tsdPtr->runLoop = runLoop;
	tsdPtr->runLoopSource = runLoopSource;
	tsdPtr->runLoopObserver = runLoopObserver;
	tsdPtr->runLoopObserverTcl = runLoopObserverTcl;
	tsdPtr->runLoopTimer = NULL;
	tsdPtr->waitTime = CF_TIMEINTERVAL_FOREVER;
	tsdPtr->tsdLock = SPINLOCK_INIT;
    }

    LOCK_NOTIFIER_INIT;
#ifdef HAVE_PTHREAD_ATFORK
    /*
     * Install pthread_atfork handlers to reinitialize the notifier in the
     * child of a fork.
     */

    if (MayUsePthreadAtfork() && !atForkInit) {
	int result = pthread_atfork(AtForkPrepare, AtForkParent, AtForkChild);

	if (result) {
	    Tcl_Panic("Tcl_InitNotifier: pthread_atfork failed");
	}
	atForkInit = 1;
    }
#endif /* HAVE_PTHREAD_ATFORK */
    if (notifierCount == 0) {
	int fds[2], status;

	/*
	 * Initialize trigger pipe.
	 */

	if (pipe(fds) != 0) {
	    Tcl_Panic("Tcl_InitNotifier: could not create trigger pipe");
	}

	status = fcntl(fds[0], F_GETFL);
	status |= O_NONBLOCK;
	if (fcntl(fds[0], F_SETFL, status) < 0) {
	    Tcl_Panic("Tcl_InitNotifier: could not make receive pipe non "
		    "blocking");
	}
	status = fcntl(fds[1], F_GETFL);
	status |= O_NONBLOCK;
	if (fcntl(fds[1], F_SETFL, status) < 0) {
	    Tcl_Panic("Tcl_InitNotifier: could not make trigger pipe non "
		    "blocking");
	}

	receivePipe = fds[0];
	triggerPipe = fds[1];

	/*
	 * Create notifier thread lazily in Tcl_WaitForEvent() to avoid
	 * interfering with fork() followed immediately by execve() (we cannot
	 * execve() when more than one thread is present).
	 */

	notifierThreadRunning = 0;
	OPEN_NOTIFIER_LOG;
    }
    ENABLE_ASL;
    notifierCount++;
    UNLOCK_NOTIFIER_INIT;

    return tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacOSXNotifierAddRunLoopMode --
 *
 *	Add the tcl notifier RunLoop source, observer and timer (if any)
 *	to the given RunLoop mode.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclMacOSXNotifierAddRunLoopMode(
    const void *runLoopMode)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    CFStringRef mode = (CFStringRef) runLoopMode;

    if (tsdPtr->runLoop) {
	CFRunLoopAddSource(tsdPtr->runLoop, tsdPtr->runLoopSource, mode);
	CFRunLoopAddObserver(tsdPtr->runLoop, tsdPtr->runLoopObserver, mode);
	if (tsdPtr->runLoopTimer) {
	    CFRunLoopAddTimer(tsdPtr->runLoop, tsdPtr->runLoopTimer, mode);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StartNotifierThread --
 *
 *	Start notifier thread if necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
StartNotifierThread(void)
{
    LOCK_NOTIFIER_INIT;
    if (!notifierCount) {
	Tcl_Panic("StartNotifierThread: notifier not initialized");
    }
    if (!notifierThreadRunning) {
	int result;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr, 60 * 1024);
	result = pthread_create(&notifierThread, &attr,
		(void * (*)(void *))NotifierThreadProc, NULL);
	pthread_attr_destroy(&attr);
	if (result) {
	    Tcl_Panic("StartNotifierThread: unable to start notifier thread");
	}
	notifierThreadRunning = 1;
    }
    UNLOCK_NOTIFIER_INIT;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_FinalizeNotifier --
 *
 *	This function is called to cleanup the notifier state before a thread
 *	is terminated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May terminate the background notifier thread if this is the last
 *	notifier instance.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FinalizeNotifier(
    ClientData clientData)		/* Not used. */
{
    ThreadSpecificData *tsdPtr;

    if (tclNotifierHooks.finalizeNotifierProc) {
	tclNotifierHooks.finalizeNotifierProc(clientData);
	return;
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);

    LOCK_NOTIFIER_INIT;
    notifierCount--;
    DISABLE_ASL;

    /*
     * If this is the last thread to use the notifier, close the notifier pipe
     * and wait for the background thread to terminate.
     */

    if (notifierCount == 0) {
	if (triggerPipe != -1) {
	    /*
	     * Send "q" message to the notifier thread so that it will
	     * terminate. The notifier will return from its call to select()
	     * and notice that a "q" message has arrived, it will then close
	     * its side of the pipe and terminate its thread. Note the we can
	     * not just close the pipe and check for EOF in the notifier
	     * thread because if a background child process was created with
	     * exec, select() would not register the EOF on the pipe until the
	     * child processes had terminated. [Bug: 4139] [Bug 1222872]
	     */

	    write(triggerPipe, "q", 1);
	    close(triggerPipe);

	    if (notifierThreadRunning) {
		int result = pthread_join(notifierThread, NULL);

		if (result) {
		    Tcl_Panic("Tcl_FinalizeNotifier: unable to join notifier "
			    "thread");
		}
		notifierThreadRunning = 0;
	    }

	    close(receivePipe);
	    triggerPipe = -1;
	}
	CLOSE_NOTIFIER_LOG;
    }
    UNLOCK_NOTIFIER_INIT;

    LOCK_NOTIFIER_TSD;		/* For concurrency with Tcl_AlertNotifier */
    if (tsdPtr->runLoop) {
	tsdPtr->runLoop = NULL;

	/*
	 * Remove runLoopSource, runLoopObserver and runLoopTimer from all
	 * CFRunLoops.
	 */

	CFRunLoopSourceInvalidate(tsdPtr->runLoopSource);
	CFRelease(tsdPtr->runLoopSource);
	tsdPtr->runLoopSource = NULL;
	CFRunLoopObserverInvalidate(tsdPtr->runLoopObserver);
	CFRelease(tsdPtr->runLoopObserver);
	tsdPtr->runLoopObserver = NULL;
	CFRunLoopObserverInvalidate(tsdPtr->runLoopObserverTcl);
	CFRelease(tsdPtr->runLoopObserverTcl);
	tsdPtr->runLoopObserverTcl = NULL;
	if (tsdPtr->runLoopTimer) {
	    CFRunLoopTimerInvalidate(tsdPtr->runLoopTimer);
	    CFRelease(tsdPtr->runLoopTimer);
	    tsdPtr->runLoopTimer = NULL;
	}
    }
    UNLOCK_NOTIFIER_TSD;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AlertNotifier --
 *
 *	Wake up the specified notifier from any thread. This routine is called
 *	by the platform independent notifier code whenever the Tcl_ThreadAlert
 *	routine is called. This routine is guaranteed not to be called on a
 *	given notifier after Tcl_FinalizeNotifier is called for that notifier.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signals the notifier condition variable for the specified notifier.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AlertNotifier(
    ClientData clientData)
{
    ThreadSpecificData *tsdPtr = clientData;

    if (tclNotifierHooks.alertNotifierProc) {
	tclNotifierHooks.alertNotifierProc(clientData);
	return;
    }

    LOCK_NOTIFIER_TSD;
    if (tsdPtr->runLoop) {
	CFRunLoopSourceSignal(tsdPtr->runLoopSource);
	CFRunLoopWakeUp(tsdPtr->runLoop);
    }
    UNLOCK_NOTIFIER_TSD;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetTimer --
 *
 *	This function sets the current notifier timer value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Replaces any previous timer.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetTimer(
    const Tcl_Time *timePtr)		/* Timeout value, may be NULL. */
{
    ThreadSpecificData *tsdPtr;
    CFRunLoopTimerRef runLoopTimer;
    CFTimeInterval waitTime;

    if (tclNotifierHooks.setTimerProc) {
	tclNotifierHooks.setTimerProc(timePtr);
	return;
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);
    runLoopTimer = tsdPtr->runLoopTimer;
    if (!runLoopTimer) {
	return;
    }
    if (timePtr) {
	Tcl_Time vTime = *timePtr;

	if (vTime.sec != 0 || vTime.usec != 0) {
	    tclScaleTimeProcPtr(&vTime, tclTimeClientData);
	    waitTime = vTime.sec + 1.0e-6 * vTime.usec;
	} else {
	    waitTime = 0;
	}
    } else {
	waitTime = CF_TIMEINTERVAL_FOREVER;
    }
    tsdPtr->waitTime = waitTime;
    CFRunLoopTimerSetNextFireDate(runLoopTimer,
	    CFAbsoluteTimeGetCurrent() + waitTime);
}

/*
 *----------------------------------------------------------------------
 *
 * TimerWakeUp --
 *
 *	CFRunLoopTimer callback.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TimerWakeUp(
    CFRunLoopTimerRef timer,
    void *info)
{
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ServiceModeHook --
 *
 *	This function is invoked whenever the service mode changes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ServiceModeHook(
    int mode)			/* Either TCL_SERVICE_ALL, or
				 * TCL_SERVICE_NONE. */
{
    ThreadSpecificData *tsdPtr;

    if (tclNotifierHooks.serviceModeHookProc) {
	tclNotifierHooks.serviceModeHookProc(mode);
	return;
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);

    if (mode == TCL_SERVICE_ALL && !tsdPtr->runLoopTimer) {
	if (!tsdPtr->runLoop) {
	    Tcl_Panic("Tcl_ServiceModeHook: Notifier not initialized");
	}
	tsdPtr->runLoopTimer = CFRunLoopTimerCreate(NULL,
		CFAbsoluteTimeGetCurrent() + CF_TIMEINTERVAL_FOREVER,
		CF_TIMEINTERVAL_FOREVER, 0, 0, TimerWakeUp, NULL);
	if (tsdPtr->runLoopTimer) {
	    CFRunLoopAddTimer(tsdPtr->runLoop, tsdPtr->runLoopTimer,
		    kCFRunLoopCommonModes);
	    StartNotifierThread();
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateFileHandler --
 *
 *	This function registers a file handler with the select notifier.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a new file handler structure.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateFileHandler(
    int fd,			/* Handle of stream to watch. */
    int mask,			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, and TCL_EXCEPTION: indicates
				 * conditions under which proc should be
				 * called. */
    Tcl_FileProc *proc,		/* Function to call for each selected
				 * event. */
    ClientData clientData)	/* Arbitrary data to pass to proc. */
{
    ThreadSpecificData *tsdPtr;
    FileHandler *filePtr;

    if (tclNotifierHooks.createFileHandlerProc) {
	tclNotifierHooks.createFileHandlerProc(fd, mask, proc, clientData);
	return;
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);

    for (filePtr = tsdPtr->firstFileHandlerPtr; filePtr != NULL;
	    filePtr = filePtr->nextPtr) {
	if (filePtr->fd == fd) {
	    break;
	}
    }
    if (filePtr == NULL) {
	filePtr = ckalloc(sizeof(FileHandler));
	filePtr->fd = fd;
	filePtr->readyMask = 0;
	filePtr->nextPtr = tsdPtr->firstFileHandlerPtr;
	tsdPtr->firstFileHandlerPtr = filePtr;
    }
    filePtr->proc = proc;
    filePtr->clientData = clientData;
    filePtr->mask = mask;

    /*
     * Update the check masks for this file.
     */

    LOCK_NOTIFIER_TSD;
    if (mask & TCL_READABLE) {
	FD_SET(fd, &tsdPtr->checkMasks.readable);
    } else {
	FD_CLR(fd, &tsdPtr->checkMasks.readable);
    }
    if (mask & TCL_WRITABLE) {
	FD_SET(fd, &tsdPtr->checkMasks.writable);
    } else {
	FD_CLR(fd, &tsdPtr->checkMasks.writable);
    }
    if (mask & TCL_EXCEPTION) {
	FD_SET(fd, &tsdPtr->checkMasks.exceptional);
    } else {
	FD_CLR(fd, &tsdPtr->checkMasks.exceptional);
    }
    if (tsdPtr->numFdBits <= fd) {
	tsdPtr->numFdBits = fd+1;
    }
    UNLOCK_NOTIFIER_TSD;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteFileHandler --
 *
 *	Cancel a previously-arranged callback arrangement for a file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a callback was previously registered on file, remove it.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteFileHandler(
    int fd)			/* Stream id for which to remove callback
				 * function. */
{
    FileHandler *filePtr, *prevPtr;
    int i, numFdBits;
    ThreadSpecificData *tsdPtr;

    if (tclNotifierHooks.deleteFileHandlerProc) {
	tclNotifierHooks.deleteFileHandlerProc(fd);
	return;
    }

    tsdPtr = TCL_TSD_INIT(&dataKey);
    numFdBits = -1;

    /*
     * Find the entry for the given file (and return if there isn't one).
     */

    for (prevPtr = NULL, filePtr = tsdPtr->firstFileHandlerPtr; ;
	    prevPtr = filePtr, filePtr = filePtr->nextPtr) {
	if (filePtr == NULL) {
	    return;
	}
	if (filePtr->fd == fd) {
	    break;
	}
    }

    /*
     * Find current max fd.
     */

    if (fd+1 == tsdPtr->numFdBits) {
	numFdBits = 0;
	for (i = fd-1; i >= 0; i--) {
	    if (FD_ISSET(i, &tsdPtr->checkMasks.readable)
		    || FD_ISSET(i, &tsdPtr->checkMasks.writable)
		    || FD_ISSET(i, &tsdPtr->checkMasks.exceptional)) {
		numFdBits = i+1;
		break;
	    }
	}
    }

    LOCK_NOTIFIER_TSD;
    if (numFdBits != -1) {
	tsdPtr->numFdBits = numFdBits;
    }

    /*
     * Update the check masks for this file.
     */

    if (filePtr->mask & TCL_READABLE) {
	FD_CLR(fd, &tsdPtr->checkMasks.readable);
    }
    if (filePtr->mask & TCL_WRITABLE) {
	FD_CLR(fd, &tsdPtr->checkMasks.writable);
    }
    if (filePtr->mask & TCL_EXCEPTION) {
	FD_CLR(fd, &tsdPtr->checkMasks.exceptional);
    }
    UNLOCK_NOTIFIER_TSD;

    /*
     * Clean up information in the callback record.
     */

    if (prevPtr == NULL) {
	tsdPtr->firstFileHandlerPtr = filePtr->nextPtr;
    } else {
	prevPtr->nextPtr = filePtr->nextPtr;
    }
    ckfree(filePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FileHandlerEventProc --
 *
 *	This function is called by Tcl_ServiceEvent when a file event reaches
 *	the front of the event queue. This function is responsible for
 *	actually handling the event by invoking the callback for the file
 *	handler.
 *
 * Results:
 *	Returns 1 if the event was handled, meaning it should be removed from
 *	the queue. Returns 0 if the event was not handled, meaning it should
 *	stay on the queue. The only time the event isn't handled is if the
 *	TCL_FILE_EVENTS flag bit isn't set.
 *
 * Side effects:
 *	Whatever the file handler's callback function does.
 *
 *----------------------------------------------------------------------
 */

static int
FileHandlerEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to handle,
				 * such as TCL_FILE_EVENTS. */
{
    int mask;
    FileHandler *filePtr;
    FileHandlerEvent *fileEvPtr = (FileHandlerEvent *) evPtr;
    ThreadSpecificData *tsdPtr;

    if (!(flags & TCL_FILE_EVENTS)) {
	return 0;
    }

    /*
     * Search through the file handlers to find the one whose handle matches
     * the event. We do this rather than keeping a pointer to the file handler
     * directly in the event, so that the handler can be deleted while the
     * event is queued without leaving a dangling pointer.
     */

    tsdPtr = TCL_TSD_INIT(&dataKey);
    for (filePtr = tsdPtr->firstFileHandlerPtr; filePtr != NULL;
	    filePtr = filePtr->nextPtr) {
	if (filePtr->fd != fileEvPtr->fd) {
	    continue;
	}

	/*
	 * The code is tricky for two reasons:
	 * 1. The file handler's desired events could have changed since the
	 *    time when the event was queued, so AND the ready mask with the
	 *    desired mask.
	 * 2. The file could have been closed and re-opened since the time
	 *    when the event was queued. This is why the ready mask is stored
	 *    in the file handler rather than the queued event: it will be
	 *    zeroed when a new file handler is created for the newly opened
	 *    file.
	 */

	mask = filePtr->readyMask & filePtr->mask;
	filePtr->readyMask = 0;
	if (mask != 0) {
	    LOCK_NOTIFIER_TSD;
	    if (mask & TCL_READABLE) {
		FD_CLR(filePtr->fd, &tsdPtr->readyMasks.readable);
	    }
	    if (mask & TCL_WRITABLE) {
		FD_CLR(filePtr->fd, &tsdPtr->readyMasks.writable);
	    }
	    if (mask & TCL_EXCEPTION) {
		FD_CLR(filePtr->fd, &tsdPtr->readyMasks.exceptional);
	    }
	    UNLOCK_NOTIFIER_TSD;
	    filePtr->proc(filePtr->clientData, mask);
	}
	break;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WaitForEvent --
 *
 *	This function is called by Tcl_DoOneEvent to wait for new events on
 *	the message queue. If the block time is 0, then Tcl_WaitForEvent just
 *	polls without blocking.
 *
 * Results:
 *	Returns 0 if a tcl event or timeout ocurred and 1 if a non-tcl
 *	CFRunLoop source was processed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WaitForEvent(
    const Tcl_Time *timePtr)		/* Maximum block time, or NULL. */
{
    int result, polling, runLoopRunning;
    CFTimeInterval waitTime;
    SInt32 runLoopStatus;
    ThreadSpecificData *tsdPtr;

    if (tclNotifierHooks.waitForEventProc) {
	return tclNotifierHooks.waitForEventProc(timePtr);
    }
    result = -1;
    polling = 0;
    waitTime = CF_TIMEINTERVAL_FOREVER;
    tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!tsdPtr->runLoop) {
	Tcl_Panic("Tcl_WaitForEvent: Notifier not initialized");
    }

    if (timePtr) {
	Tcl_Time vTime = *timePtr;

	/*
	 * TIP #233 (Virtualized Time). Is virtual time in effect? And do we
	 * actually have something to scale? If yes to both then we call the
	 * handler to do this scaling.
	 */

	if (vTime.sec != 0 || vTime.usec != 0) {
	    tclScaleTimeProcPtr(&vTime, tclTimeClientData);
	    waitTime = vTime.sec + 1.0e-6 * vTime.usec;
	} else {
	    /*
	     * Polling: pretend to wait for files and tell the notifier thread
	     * what we are doing. The notifier thread makes sure it goes
	     * through select with its select mask in the same state as ours
	     * currently is. We block until that happens.
	     */

	    polling = 1;
	}
    }

    StartNotifierThread();

    LOCK_NOTIFIER_TSD;
    tsdPtr->polling = polling;
    UNLOCK_NOTIFIER_TSD;
    tsdPtr->runLoopSourcePerformed = 0;

    /*
     * If the Tcl runloop is already running (e.g. if Tcl_WaitForEvent was
     * called recursively) or is servicing events via the runloop observer,
     * re-run it in a custom runloop mode containing only the source for the
     * notifier thread, otherwise wakeups from other sources added to the
     * common runloop modes might get lost or 3rd party event handlers might
     * get called when they do not expect to be.
     */

    runLoopRunning = tsdPtr->runLoopRunning;
    tsdPtr->runLoopRunning = 1;
    runLoopStatus = CFRunLoopRunInMode(tsdPtr->runLoopServicingEvents ||
	    runLoopRunning ? tclEventsOnlyRunLoopMode : kCFRunLoopDefaultMode,
	    waitTime, TRUE);
    tsdPtr->runLoopRunning = runLoopRunning;

    LOCK_NOTIFIER_TSD;
    tsdPtr->polling = 0;
    UNLOCK_NOTIFIER_TSD;
    switch (runLoopStatus) {
    case kCFRunLoopRunFinished:
	Tcl_Panic("Tcl_WaitForEvent: CFRunLoop finished");
	break;
    case kCFRunLoopRunTimedOut:
	QueueFileEvents(tsdPtr);
	result = 0;
	break;
    case kCFRunLoopRunStopped:
    case kCFRunLoopRunHandledSource:
	result = tsdPtr->runLoopSourcePerformed ? 0 : 1;
	break;
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * QueueFileEvents --
 *
 *	CFRunLoopSource callback for queueing file events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Queues file events that are detected by the select.
 *
 *----------------------------------------------------------------------
 */

static void
QueueFileEvents(
    void *info)
{
    SelectMasks readyMasks;
    FileHandler *filePtr;
    ThreadSpecificData *tsdPtr = info;

    /*
     * Queue all detected file events.
     */

    LOCK_NOTIFIER_TSD;
    FD_COPY(&tsdPtr->readyMasks.readable, &readyMasks.readable);
    FD_COPY(&tsdPtr->readyMasks.writable, &readyMasks.writable);
    FD_COPY(&tsdPtr->readyMasks.exceptional, &readyMasks.exceptional);
    FD_ZERO(&tsdPtr->readyMasks.readable);
    FD_ZERO(&tsdPtr->readyMasks.writable);
    FD_ZERO(&tsdPtr->readyMasks.exceptional);
    UNLOCK_NOTIFIER_TSD;
    tsdPtr->runLoopSourcePerformed = 1;

    for (filePtr = tsdPtr->firstFileHandlerPtr; (filePtr != NULL);
	    filePtr = filePtr->nextPtr) {
	int mask = 0;

	if (FD_ISSET(filePtr->fd, &readyMasks.readable)) {
	    mask |= TCL_READABLE;
	}
	if (FD_ISSET(filePtr->fd, &readyMasks.writable)) {
	    mask |= TCL_WRITABLE;
	}
	if (FD_ISSET(filePtr->fd, &readyMasks.exceptional)) {
	    mask |= TCL_EXCEPTION;
	}
	if (!mask) {
	    continue;
	}

	/*
	 * Don't bother to queue an event if the mask was previously non-zero
	 * since an event must still be on the queue.
	 */

	if (filePtr->readyMask == 0) {
	    FileHandlerEvent *fileEvPtr = ckalloc(sizeof(FileHandlerEvent));

	    fileEvPtr->header.proc = FileHandlerEventProc;
	    fileEvPtr->fd = filePtr->fd;
	    Tcl_QueueEvent((Tcl_Event *) fileEvPtr, TCL_QUEUE_TAIL);
	}
	filePtr->readyMask = mask;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateWaitingListAndServiceEvents --
 *
 *	CFRunLoopObserver callback for updating waitingList and
 *	servicing Tcl events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateWaitingListAndServiceEvents(
    CFRunLoopObserverRef observer,
    CFRunLoopActivity activity,
    void *info)
{
    ThreadSpecificData *tsdPtr = info;

    if (tsdPtr->sleeping) {
	return;
    }
    switch (activity) {
    case kCFRunLoopEntry:
	tsdPtr->runLoopNestingLevel++;
	if (tsdPtr->numFdBits > 0 || tsdPtr->polling) {
	    LOCK_NOTIFIER;
	    if (!OnOffWaitingList(tsdPtr, 1, 1) && tsdPtr->polling) {
		write(triggerPipe, "", 1);
	    }
	    UNLOCK_NOTIFIER;
	}
	break;
    case kCFRunLoopExit:
	if (tsdPtr->runLoopNestingLevel == 1) {
	    LOCK_NOTIFIER;
	    OnOffWaitingList(tsdPtr, 0, 1);
	    UNLOCK_NOTIFIER;
	}
	tsdPtr->runLoopNestingLevel--;
	break;
    case kCFRunLoopBeforeWaiting:
	if (tsdPtr->runLoopTimer && !tsdPtr->runLoopServicingEvents &&
		(tsdPtr->runLoopNestingLevel > 1
			|| !tsdPtr->runLoopRunning)) {
	    tsdPtr->runLoopServicingEvents = 1;
            /* This call seems to simply force event processing through and prevents hangups that have long been observed with Tk-Cocoa.  */
	    Tcl_ServiceAll();
	    tsdPtr->runLoopServicingEvents = 0;
	}
	break;
    default:
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * OnOffWaitingList --
 *
 *	Add/remove the specified thread to/from the global waitingList and
 *	optionally signal the notifier.
 *
 *	!!! Requires notifierLock to be held !!!
 *
 * Results:
 *	Boolean indicating whether the waitingList was changed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
OnOffWaitingList(
    ThreadSpecificData *tsdPtr,
    int onList,
    int signalNotifier)
{
    int changeWaitingList;

#ifdef TCL_MAC_DEBUG_NOTIFIER
    if (SpinLockTry(&notifierLock)) {
	Tcl_Panic("OnOffWaitingList: notifierLock unlocked");
    }
#endif
    changeWaitingList = (!onList ^ !tsdPtr->onList);
    if (changeWaitingList) {
	if (onList) {
	    tsdPtr->nextPtr = waitingListPtr;
	    if (waitingListPtr) {
		waitingListPtr->prevPtr = tsdPtr;
	    }
	    tsdPtr->prevPtr = NULL;
	    waitingListPtr = tsdPtr;
	    tsdPtr->onList = 1;
	} else {
	    if (tsdPtr->prevPtr) {
		tsdPtr->prevPtr->nextPtr = tsdPtr->nextPtr;
	    } else {
		waitingListPtr = tsdPtr->nextPtr;
	    }
	    if (tsdPtr->nextPtr) {
		tsdPtr->nextPtr->prevPtr = tsdPtr->prevPtr;
	    }
	    tsdPtr->nextPtr = tsdPtr->prevPtr = NULL;
	    tsdPtr->onList = 0;
	}
	if (signalNotifier) {
	    write(triggerPipe, "", 1);
	}
    }

    return changeWaitingList;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Sleep --
 *
 *	Delay execution for the specified number of milliseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Time passes.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Sleep(
    int ms)			/* Number of milliseconds to sleep. */
{
    Tcl_Time vdelay;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (ms <= 0) {
	return;
    }

    /*
     * TIP #233: Scale from virtual time to real-time.
     */

    vdelay.sec = ms / 1000;
    vdelay.usec = (ms % 1000) * 1000;
    tclScaleTimeProcPtr(&vdelay, tclTimeClientData);


    if (tsdPtr->runLoop) {
	CFTimeInterval waitTime;
	CFRunLoopTimerRef runLoopTimer = tsdPtr->runLoopTimer;
	CFAbsoluteTime nextTimerFire = 0, waitEnd, now;
	SInt32 runLoopStatus;

	waitTime = vdelay.sec + 1.0e-6 * vdelay.usec;
 	now = CFAbsoluteTimeGetCurrent();
	waitEnd = now + waitTime;

	if (runLoopTimer) {
	    nextTimerFire = CFRunLoopTimerGetNextFireDate(runLoopTimer);
	    if (nextTimerFire < waitEnd) {
		CFRunLoopTimerSetNextFireDate(runLoopTimer, now +
			CF_TIMEINTERVAL_FOREVER);
	    } else {
		runLoopTimer = NULL;
	    }
	}
	tsdPtr->sleeping = 1;
	do {
	    runLoopStatus = CFRunLoopRunInMode(kCFRunLoopDefaultMode,
		    waitTime, FALSE);
	    switch (runLoopStatus) {
	    case kCFRunLoopRunFinished:
		Tcl_Panic("Tcl_Sleep: CFRunLoop finished");
		break;
	    case kCFRunLoopRunStopped:
		TclMacOSXNotifierDbgMsg("CFRunLoop stopped");
		waitTime = waitEnd - CFAbsoluteTimeGetCurrent();
		break;
	    case kCFRunLoopRunTimedOut:
		waitTime = 0;
		break;
	    }
	} while (waitTime > 0);
	tsdPtr->sleeping = 0;
 	if (runLoopTimer) {
	    CFRunLoopTimerSetNextFireDate(runLoopTimer, nextTimerFire);
	}
    } else {
	struct timespec waitTime;

	waitTime.tv_sec = vdelay.sec;
	waitTime.tv_nsec = vdelay.usec * 1000;
	while (nanosleep(&waitTime, &waitTime));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclUnixWaitForFile --
 *
 *	This function waits synchronously for a file to become readable or
 *	writable, with an optional timeout.
 *
 * Results:
 *	The return value is an OR'ed combination of TCL_READABLE,
 *	TCL_WRITABLE, and TCL_EXCEPTION, indicating the conditions that are
 *	present on file at the time of the return. This function will not
 *	return until either "timeout" milliseconds have elapsed or at least
 *	one of the conditions given by mask has occurred for file (a return
 *	value of 0 means that a timeout occurred). No normal events will be
 *	serviced during the execution of this function.
 *
 * Side effects:
 *	Time passes.
 *
 *----------------------------------------------------------------------
 */

int
TclUnixWaitForFile(
    int fd,			/* Handle for file on which to wait. */
    int mask,			/* What to wait for: OR'ed combination of
				 * TCL_READABLE, TCL_WRITABLE, and
				 * TCL_EXCEPTION. */
    int timeout)		/* Maximum amount of time to wait for one of
				 * the conditions in mask to occur, in
				 * milliseconds. A value of 0 means don't wait
				 * at all, and a value of -1 means wait
				 * forever. */
{
    Tcl_Time abortTime = {0, 0}, now; /* silence gcc 4 warning */
    struct timeval blockTime, *timeoutPtr;
    int numFound, result = 0;
    fd_set readableMask;
    fd_set writableMask;
    fd_set exceptionalMask;

#define SET_BITS(var, bits)	((var) |= (bits))
#define CLEAR_BITS(var, bits)	((var) &= ~(bits))

#ifndef _DARWIN_C_SOURCE
    /*
     * Sanity check fd.
     */

    if (fd >= FD_SETSIZE) {
	Tcl_Panic("TclUnixWaitForFile can't handle file id %d", fd);
	/* must never get here, or select masks overrun will occur below */
    }
#endif

    /*
     * If there is a non-zero finite timeout, compute the time when we give
     * up.
     */

    if (timeout > 0) {
	Tcl_GetTime(&now);
	abortTime.sec = now.sec + timeout/1000;
	abortTime.usec = now.usec + (timeout%1000)*1000;
	if (abortTime.usec >= 1000000) {
	    abortTime.usec -= 1000000;
	    abortTime.sec += 1;
	}
	timeoutPtr = &blockTime;
    } else if (timeout == 0) {
	timeoutPtr = &blockTime;
	blockTime.tv_sec = 0;
	blockTime.tv_usec = 0;
    } else {
	timeoutPtr = NULL;
    }

    /*
     * Initialize the select masks.
     */

    FD_ZERO(&readableMask);
    FD_ZERO(&writableMask);
    FD_ZERO(&exceptionalMask);

    /*
     * Loop in a mini-event loop of our own, waiting for either the file to
     * become ready or a timeout to occur.
     */

    while (1) {
	if (timeout > 0) {
	    blockTime.tv_sec = abortTime.sec - now.sec;
	    blockTime.tv_usec = abortTime.usec - now.usec;
	    if (blockTime.tv_usec < 0) {
		blockTime.tv_sec -= 1;
		blockTime.tv_usec += 1000000;
	    }
	    if (blockTime.tv_sec < 0) {
		blockTime.tv_sec = 0;
		blockTime.tv_usec = 0;
	    }
	}

	/*
	 * Setup the select masks for the fd.
	 */

	if (mask & TCL_READABLE) {
	    FD_SET(fd, &readableMask);
	}
	if (mask & TCL_WRITABLE) {
	    FD_SET(fd, &writableMask);
	}
	if (mask & TCL_EXCEPTION) {
	    FD_SET(fd, &exceptionalMask);
	}

	/*
	 * Wait for the event or a timeout.
	 */

	numFound = select(fd + 1, &readableMask, &writableMask,
		&exceptionalMask, timeoutPtr);
	if (numFound == 1) {
	    if (FD_ISSET(fd, &readableMask)) {
		SET_BITS(result, TCL_READABLE);
	    }
	    if (FD_ISSET(fd, &writableMask)) {
		SET_BITS(result, TCL_WRITABLE);
	    }
	    if (FD_ISSET(fd, &exceptionalMask)) {
		SET_BITS(result, TCL_EXCEPTION);
	    }
	    result &= mask;
	    if (result) {
		break;
	    }
	}
	if (timeout == 0) {
	    break;
	}
	if (timeout < 0) {
	    continue;
	}

	/*
	 * The select returned early, so we need to recompute the timeout.
	 */

	Tcl_GetTime(&now);
	if ((abortTime.sec < now.sec)
		|| (abortTime.sec==now.sec && abortTime.usec<=now.usec)) {
	    break;
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NotifierThreadProc --
 *
 *	This routine is the initial (and only) function executed by the
 *	special notifier thread. Its job is to wait for file descriptors to
 *	become readable or writable or to have an exception condition and then
 *	to notify other threads who are interested in this information by
 *	signalling a condition variable. Other threads can signal this
 *	notifier thread of a change in their interests by writing a single
 *	byte to a special pipe that the notifier thread is monitoring.
 *
 * Result:
 *	None. Once started, this routine never exits. It dies with the overall
 *	process.
 *
 * Side effects:
 *	The trigger pipe used to signal the notifier thread is created when
 *	the notifier thread first starts.
 *
 *----------------------------------------------------------------------
 */

static void
NotifierThreadProc(
    ClientData clientData)	/* Not used. */
{
    ThreadSpecificData *tsdPtr;
    fd_set readableMask, writableMask, exceptionalMask;
    int i, numFdBits = 0, polling;
    struct timeval poll = {0., 0.}, *timePtr;
    char buf[2];

    /*
     * Look for file events and report them to interested threads.
     */

    while (1) {
	FD_ZERO(&readableMask);
	FD_ZERO(&writableMask);
	FD_ZERO(&exceptionalMask);

	/*
	 * Compute the logical OR of the select masks from all the waiting
	 * notifiers.
	 */

	timePtr = NULL;
	LOCK_NOTIFIER;
	for (tsdPtr = waitingListPtr; tsdPtr; tsdPtr = tsdPtr->nextPtr) {
	    LOCK_NOTIFIER_TSD;
	    for (i = tsdPtr->numFdBits-1; i >= 0; --i) {
		if (FD_ISSET(i, &tsdPtr->checkMasks.readable)) {
		    FD_SET(i, &readableMask);
		}
		if (FD_ISSET(i, &tsdPtr->checkMasks.writable)) {
		    FD_SET(i, &writableMask);
		}
		if (FD_ISSET(i, &tsdPtr->checkMasks.exceptional)) {
		    FD_SET(i, &exceptionalMask);
		}
	    }
	    if (tsdPtr->numFdBits > numFdBits) {
		numFdBits = tsdPtr->numFdBits;
	    }
	    polling = tsdPtr->polling;
	    UNLOCK_NOTIFIER_TSD;
	    if ((tsdPtr->polled = polling)) {
		timePtr = &poll;
	    }
	}
	UNLOCK_NOTIFIER;

	/*
	 * Set up the select mask to include the receive pipe.
	 */

	if (receivePipe >= numFdBits) {
	    numFdBits = receivePipe + 1;
	}
	FD_SET(receivePipe, &readableMask);

	if (select(numFdBits, &readableMask, &writableMask, &exceptionalMask,
		timePtr) == -1) {
	    /*
	     * Try again immediately on an error.
	     */

	    continue;
	}

	/*
	 * Alert any threads that are waiting on a ready file descriptor.
	 */

	LOCK_NOTIFIER;
	for (tsdPtr = waitingListPtr; tsdPtr; tsdPtr = tsdPtr->nextPtr) {
	    int found = 0;
	    SelectMasks readyMasks, checkMasks;

	    LOCK_NOTIFIER_TSD;
	    FD_COPY(&tsdPtr->checkMasks.readable, &checkMasks.readable);
	    FD_COPY(&tsdPtr->checkMasks.writable, &checkMasks.writable);
	    FD_COPY(&tsdPtr->checkMasks.exceptional, &checkMasks.exceptional);
	    UNLOCK_NOTIFIER_TSD;
	    found = tsdPtr->polled;
	    FD_ZERO(&readyMasks.readable);
	    FD_ZERO(&readyMasks.writable);
	    FD_ZERO(&readyMasks.exceptional);

	    for (i = tsdPtr->numFdBits-1; i >= 0; --i) {
		if (FD_ISSET(i, &checkMasks.readable)
			&& FD_ISSET(i, &readableMask)) {
		    FD_SET(i, &readyMasks.readable);
		    found = 1;
		}
		if (FD_ISSET(i, &checkMasks.writable)
			&& FD_ISSET(i, &writableMask)) {
		    FD_SET(i, &readyMasks.writable);
		    found = 1;
		}
		if (FD_ISSET(i, &checkMasks.exceptional)
			&& FD_ISSET(i, &exceptionalMask)) {
		    FD_SET(i, &readyMasks.exceptional);
		    found = 1;
		}
	    }

	    if (found) {
		/*
		 * Remove the ThreadSpecificData structure of this thread from
		 * the waiting list. This prevents us from spinning
		 * continuously on select until the other threads runs and
		 * services the file event.
		 */

		OnOffWaitingList(tsdPtr, 0, 0);

		LOCK_NOTIFIER_TSD;
		FD_COPY(&readyMasks.readable, &tsdPtr->readyMasks.readable);
		FD_COPY(&readyMasks.writable, &tsdPtr->readyMasks.writable);
		FD_COPY(&readyMasks.exceptional,
			&tsdPtr->readyMasks.exceptional);
		UNLOCK_NOTIFIER_TSD;
		tsdPtr->polled = 0;
		if (tsdPtr->runLoop) {
		    CFRunLoopSourceSignal(tsdPtr->runLoopSource);
		    CFRunLoopWakeUp(tsdPtr->runLoop);
		}
	    }
	}
	UNLOCK_NOTIFIER;

	/*
	 * Consume the next byte from the notifier pipe if the pipe was
	 * readable. Note that there may be multiple bytes pending, but to
	 * avoid a race condition we only read one at a time.
	 */

	if (FD_ISSET(receivePipe, &readableMask)) {
	    i = read(receivePipe, buf, 1);

	    if ((i == 0) || ((i == 1) && (buf[0] == 'q'))) {
		/*
		 * Someone closed the write end of the pipe or sent us a Quit
		 * message [Bug: 4139] and then closed the write end of the
		 * pipe so we need to shut down the notifier thread.
		 */

		break;
	    }
	}
    }
    pthread_exit(0);
}

#ifdef HAVE_PTHREAD_ATFORK
/*
 *----------------------------------------------------------------------
 *
 * AtForkPrepare --
 *
 *	Lock the notifier in preparation for a fork.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AtForkPrepare(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    LOCK_NOTIFIER_INIT;
    LOCK_NOTIFIER;
    LOCK_NOTIFIER_TSD;
}

/*
 *----------------------------------------------------------------------
 *
 * AtForkParent --
 *
 *	Unlock the notifier in the parent after a fork.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AtForkParent(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    UNLOCK_NOTIFIER_TSD;
    UNLOCK_NOTIFIER;
    UNLOCK_NOTIFIER_INIT;
}

/*
 *----------------------------------------------------------------------
 *
 * AtForkChild --
 *
 *	Unlock and reinstall the notifier in the child after a fork.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AtForkChild(void)
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    UNLOCK_NOTIFIER_TSD;
    UNLOCK_NOTIFIER;
    UNLOCK_NOTIFIER_INIT;
    if (tsdPtr->runLoop) {
	tsdPtr->runLoop = NULL;
	if (!noCFafterFork) {
	    CFRunLoopSourceInvalidate(tsdPtr->runLoopSource);
	    CFRelease(tsdPtr->runLoopSource);
	    if (tsdPtr->runLoopTimer) {
		CFRunLoopTimerInvalidate(tsdPtr->runLoopTimer);
		CFRelease(tsdPtr->runLoopTimer);
	    }
	}
	tsdPtr->runLoopSource = NULL;
	tsdPtr->runLoopTimer = NULL;
    }
    if (notifierCount > 0) {
	notifierCount = 1;
	notifierThreadRunning = 0;

	/*
	 * Assume that the return value of Tcl_InitNotifier in the child will
	 * be identical to the one stored as clientData in tclNotify.c's
	 * ThreadSpecificData by the parent's TclInitNotifier, so discard the
	 * return value here. This assumption may require the fork() to be
	 * executed in the main thread of the parent, otherwise
	 * Tcl_AlertNotifier may break in the child.
	 */

	if (!noCFafterFork) {
	    Tcl_InitNotifier();
	}
    }
}
#endif /* HAVE_PTHREAD_ATFORK */

#else /* HAVE_COREFOUNDATION */

void
TclMacOSXNotifierAddRunLoopMode(
    const void *runLoopMode)
{
    Tcl_Panic("TclMacOSXNotifierAddRunLoopMode: "
	    "Tcl not built with CoreFoundation support");
}

#endif /* HAVE_COREFOUNDATION */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
