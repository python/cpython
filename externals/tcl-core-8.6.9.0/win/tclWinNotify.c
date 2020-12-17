/*
 * tclWinNotify.c --
 *
 *	This file contains Windows-specific procedures for the notifier, which
 *	is the lowest-level part of the Tcl event loop. This file works
 *	together with ../generic/tclNotify.c.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * The follwing static indicates whether this module has been initialized.
 */

#define INTERVAL_TIMER	1	/* Handle of interval timer. */

#define WM_WAKEUP	WM_USER	/* Message that is send by
				 * Tcl_AlertNotifier. */
/*
 * The following static structure contains the state information for the
 * Windows implementation of the Tcl notifier. One of these structures is
 * created for each thread that is using the notifier.
 */

typedef struct ThreadSpecificData {
    CRITICAL_SECTION crit;	/* Monitor for this notifier. */
    DWORD thread;		/* Identifier for thread associated with this
				 * notifier. */
    HANDLE event;		/* Event object used to wake up the notifier
				 * thread. */
    int pending;		/* Alert message pending, this field is locked
				 * by the notifierMutex. */
    HWND hwnd;			/* Messaging window. */
    int timeout;		/* Current timeout value. */
    int timerActive;		/* 1 if interval timer is running. */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following static indicates the number of threads that have initialized
 * notifiers. It controls the lifetime of the TclNotifier window class.
 *
 * You must hold the notifierMutex lock before accessing this variable.
 */

static int notifierCount = 0;
static const TCHAR classname[] = TEXT("TclNotifier");
TCL_DECLARE_MUTEX(notifierMutex)

/*
 * Static routines defined in this file.
 */

static LRESULT CALLBACK		NotifierProc(HWND hwnd, UINT message,
				    WPARAM wParam, LPARAM lParam);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitNotifier --
 *
 *	Initializes the platform specific notifier state.
 *
 * Results:
 *	Returns a handle to the notifier state for this thread..
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_InitNotifier(void)
{
    if (tclNotifierHooks.initNotifierProc) {
	return tclNotifierHooks.initNotifierProc();
    } else {
	ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
	WNDCLASS class;

	/*
	 * Register Notifier window class if this is the first thread to use
	 * this module.
	 */

	Tcl_MutexLock(&notifierMutex);
	if (notifierCount == 0) {
	    class.style = 0;
	    class.cbClsExtra = 0;
	    class.cbWndExtra = 0;
	    class.hInstance = TclWinGetTclInstance();
	    class.hbrBackground = NULL;
	    class.lpszMenuName = NULL;
	    class.lpszClassName = classname;
	    class.lpfnWndProc = NotifierProc;
	    class.hIcon = NULL;
	    class.hCursor = NULL;

	    if (!RegisterClass(&class)) {
		Tcl_Panic("Unable to register TclNotifier window class");
	    }
	}
	notifierCount++;
	Tcl_MutexUnlock(&notifierMutex);

	tsdPtr->pending = 0;
	tsdPtr->timerActive = 0;

	InitializeCriticalSection(&tsdPtr->crit);

	tsdPtr->hwnd = NULL;
	tsdPtr->thread = GetCurrentThreadId();
	tsdPtr->event = CreateEvent(NULL, TRUE /* manual */,
		FALSE /* !signaled */, NULL);

	return tsdPtr;
    }
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
 *	May dispose of the notifier window and class.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FinalizeNotifier(
    ClientData clientData)	/* Pointer to notifier data. */
{
    if (tclNotifierHooks.finalizeNotifierProc) {
	tclNotifierHooks.finalizeNotifierProc(clientData);
	return;
    } else {
	ThreadSpecificData *tsdPtr = (ThreadSpecificData *) clientData;

	/*
	 * Only finalize the notifier if a notifier was installed in the
	 * current thread; there is a route in which this is not guaranteed to
	 * be true (when tclWin32Dll.c:DllMain() is called with the flag
	 * DLL_PROCESS_DETACH by the OS, which could be doing so from a thread
	 * that's never previously been involved with Tcl, e.g. the task
	 * manager) so this check is important.
	 *
	 * Fixes Bug #217982 reported by Hugh Vu and Gene Leache.
	 */

	if (tsdPtr == NULL) {
	    return;
	}

	DeleteCriticalSection(&tsdPtr->crit);
	CloseHandle(tsdPtr->event);

	/*
	 * Clean up the timer and messaging window for this thread.
	 */

	if (tsdPtr->hwnd) {
	    KillTimer(tsdPtr->hwnd, INTERVAL_TIMER);
	    DestroyWindow(tsdPtr->hwnd);
	}

	/*
	 * If this is the last thread to use the notifier, unregister the
	 * notifier window class.
	 */

	Tcl_MutexLock(&notifierMutex);
	notifierCount--;
	if (notifierCount == 0) {
	    UnregisterClass(classname, TclWinGetTclInstance());
	}
	Tcl_MutexUnlock(&notifierMutex);
    }
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
 *	This routine is typically called from a thread other than the
 *	notifier's thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sends a message to the messaging window for the notifier if there
 *	isn't already one pending.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AlertNotifier(
    ClientData clientData)	/* Pointer to thread data. */
{
    if (tclNotifierHooks.alertNotifierProc) {
	tclNotifierHooks.alertNotifierProc(clientData);
	return;
    } else {
	ThreadSpecificData *tsdPtr = (ThreadSpecificData *) clientData;

	/*
	 * Note that we do not need to lock around access to the hwnd because
	 * the race condition has no effect since any race condition implies
	 * that the notifier thread is already awake.
	 */

	if (tsdPtr->hwnd) {
	    /*
	     * We do need to lock around access to the pending flag.
	     */

	    EnterCriticalSection(&tsdPtr->crit);
	    if (!tsdPtr->pending) {
		PostMessage(tsdPtr->hwnd, WM_WAKEUP, 0, 0);
	    }
	    tsdPtr->pending = 1;
	    LeaveCriticalSection(&tsdPtr->crit);
	} else {
	    SetEvent(tsdPtr->event);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetTimer --
 *
 *	This procedure sets the current notifier timer value. The notifier
 *	will ensure that Tcl_ServiceAll() is called after the specified
 *	interval, even if no events have occurred.
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
    const Tcl_Time *timePtr)		/* Maximum block time, or NULL. */
{
    if (tclNotifierHooks.setTimerProc) {
	tclNotifierHooks.setTimerProc(timePtr);
	return;
    } else {
	ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
	UINT timeout;

	/*
	 * We only need to set up an interval timer if we're being called from
	 * an external event loop. If we don't have a window handle then we
	 * just return immediately and let Tcl_WaitForEvent handle timeouts.
	 */

	if (!tsdPtr->hwnd) {
	    return;
	}

	if (!timePtr) {
	    timeout = 0;
	} else {
	    /*
	     * Make sure we pass a non-zero value into the timeout argument.
	     * Windows seems to get confused by zero length timers.
	     */

	    timeout = timePtr->sec * 1000 + timePtr->usec / 1000;
	    if (timeout == 0) {
		timeout = 1;
	    }
	}
	tsdPtr->timeout = timeout;
	if (timeout != 0) {
	    tsdPtr->timerActive = 1;
	    SetTimer(tsdPtr->hwnd, INTERVAL_TIMER,
		    (unsigned long) tsdPtr->timeout, NULL);
	} else {
	    tsdPtr->timerActive = 0;
	    KillTimer(tsdPtr->hwnd, INTERVAL_TIMER);
	}
    }
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
 *	If this is the first time the notifier is set into TCL_SERVICE_ALL,
 *	then the communication window is created.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ServiceModeHook(
    int mode)			/* Either TCL_SERVICE_ALL, or
				 * TCL_SERVICE_NONE. */
{
    if (tclNotifierHooks.serviceModeHookProc) {
	tclNotifierHooks.serviceModeHookProc(mode);
	return;
    } else {
	ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

	/*
	 * If this is the first time that the notifier has been used from a
	 * modal loop, then create a communication window. Note that after this
	 * point, the application needs to service events in a timely fashion
	 * or Windows will hang waiting for the window to respond to
	 * synchronous system messages. At some point, we may want to consider
	 * destroying the window if we leave the modal loop, but for now we'll
	 * leave it around.
	 */

	if (mode == TCL_SERVICE_ALL && !tsdPtr->hwnd) {
	    tsdPtr->hwnd = CreateWindow(classname, classname,
		    WS_TILED, 0, 0, 0, 0, NULL, NULL, TclWinGetTclInstance(),
		    NULL);

	    /*
	     * Send an initial message to the window to ensure that we wake up
	     * the notifier once we get into the modal loop. This will force
	     * the notifier to recompute the timeout value and schedule a timer
	     * if one is needed.
	     */

	    Tcl_AlertNotifier(tsdPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NotifierProc --
 *
 *	This procedure is invoked by Windows to process events on the notifier
 *	window. Messages will be sent to this window in response to external
 *	timer events or calls to TclpAlertTsdPtr->
 *
 * Results:
 *	A standard windows result.
 *
 * Side effects:
 *	Services any pending events.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
NotifierProc(
    HWND hwnd,			/* Passed on... */
    UINT message,		/* What messsage is this? */
    WPARAM wParam,		/* Passed on... */
    LPARAM lParam)		/* Passed on... */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (message == WM_WAKEUP) {
	EnterCriticalSection(&tsdPtr->crit);
	tsdPtr->pending = 0;
	LeaveCriticalSection(&tsdPtr->crit);
    } else if (message != WM_TIMER) {
	return DefWindowProc(hwnd, message, wParam, lParam);
    }

    /*
     * Process all of the runnable events.
     */

    Tcl_ServiceAll();
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WaitForEvent --
 *
 *	This function is called by Tcl_DoOneEvent to wait for new events on
 *	the message queue. If the block time is 0, then Tcl_WaitForEvent just
 *	polls the event queue without blocking.
 *
 * Results:
 *	Returns -1 if a WM_QUIT message is detected, returns 1 if a message
 *	was dispatched, otherwise returns 0.
 *
 * Side effects:
 *	Dispatches a message to a window procedure, which could do anything.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WaitForEvent(
    const Tcl_Time *timePtr)		/* Maximum block time, or NULL. */
{
    if (tclNotifierHooks.waitForEventProc) {
	return tclNotifierHooks.waitForEventProc(timePtr);
    } else {
	ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
	MSG msg;
	DWORD timeout, result;
	int status;

	/*
	 * Compute the timeout in milliseconds.
	 */

	if (timePtr) {
	    /*
	     * TIP #233 (Virtualized Time). Convert virtual domain delay to
	     * real-time.
	     */

	    Tcl_Time myTime;

	    myTime.sec  = timePtr->sec;
	    myTime.usec = timePtr->usec;

	    if (myTime.sec != 0 || myTime.usec != 0) {
		tclScaleTimeProcPtr(&myTime, tclTimeClientData);
	    }

	    timeout = myTime.sec * 1000 + myTime.usec / 1000;
	} else {
	    timeout = INFINITE;
	}

	/*
	 * Check to see if there are any messages in the queue before waiting
	 * because MsgWaitForMultipleObjects will not wake up if there are
	 * events currently sitting in the queue.
	 */

	if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
	    /*
	     * Wait for something to happen (a signal from another thread, a
	     * message, or timeout) or loop servicing asynchronous procedure
	     * calls queued to this thread.
	     */

	again:
	    result = MsgWaitForMultipleObjectsEx(1, &tsdPtr->event, timeout,
		    QS_ALLINPUT, MWMO_ALERTABLE);
	    if (result == WAIT_IO_COMPLETION) {
		goto again;
	    } else if (result == WAIT_FAILED) {
		status = -1;
		goto end;
	    }
	}

	/*
	 * Check to see if there are any messages to process.
	 */

	if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
	    /*
	     * Retrieve and dispatch the first message.
	     */

	    result = GetMessage(&msg, NULL, 0, 0);
	    if (result == 0) {
		/*
		 * We received a request to exit this thread (WM_QUIT), so
		 * propagate the quit message and start unwinding.
		 */

		PostQuitMessage((int) msg.wParam);
		status = -1;
	    } else if (result == (DWORD)-1) {
		/*
		 * We got an error from the system. I have no idea why this
		 * would happen, so we'll just unwind.
		 */

		status = -1;
	    } else {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		status = 1;
	    }
	} else {
	    status = 0;
	}

      end:
	ResetEvent(tsdPtr->event);
	return status;
    }
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
    /*
     * Simply calling 'Sleep' for the requisite number of milliseconds can
     * make the process appear to wake up early because it isn't synchronized
     * with the CPU performance counter that is used in tclWinTime.c. This
     * behavior is probably benign, but messes up some of the corner cases in
     * the test suite. We get around this problem by repeating the 'Sleep'
     * call as many times as necessary to make the clock advance by the
     * requisite amount.
     */

    Tcl_Time now;		/* Current wall clock time. */
    Tcl_Time desired;		/* Desired wakeup time. */
    Tcl_Time vdelay;		/* Time to sleep, for scaling virtual ->
				 * real. */
    DWORD sleepTime;		/* Time to sleep, real-time */

    vdelay.sec  = ms / 1000;
    vdelay.usec = (ms % 1000) * 1000;

    Tcl_GetTime(&now);
    desired.sec  = now.sec  + vdelay.sec;
    desired.usec = now.usec + vdelay.usec;
    if (desired.usec > 1000000) {
	++desired.sec;
	desired.usec -= 1000000;
    }

    /*
     * TIP #233: Scale delay from virtual to real-time.
     */

    tclScaleTimeProcPtr(&vdelay, tclTimeClientData);
    sleepTime = vdelay.sec * 1000 + vdelay.usec / 1000;

    for (;;) {
	SleepEx(sleepTime, TRUE);
	Tcl_GetTime(&now);
	if (now.sec > desired.sec) {
	    break;
	} else if ((now.sec == desired.sec) && (now.usec >= desired.usec)) {
	    break;
	}

	vdelay.sec  = desired.sec  - now.sec;
	vdelay.usec = desired.usec - now.usec;

	tclScaleTimeProcPtr(&vdelay, tclTimeClientData);
	sleepTime = vdelay.sec * 1000 + vdelay.usec / 1000;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
