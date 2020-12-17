/*
 * tclUnixTime.c --
 *
 *	Contains Unix specific versions of Tcl functions that obtain time
 *	values from the operating system.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include <locale.h>
#if defined(TCL_WIDE_CLICKS) && defined(MAC_OSX_TCL)
#include <mach/mach_time.h>
#endif

/*
 * TclpGetDate is coded to return a pointer to a 'struct tm'. For thread
 * safety, this structure must be in thread-specific data. The 'tmKey'
 * variable is the key to this buffer.
 */

static Tcl_ThreadDataKey tmKey;
typedef struct ThreadSpecificData {
    struct tm gmtime_buf;
    struct tm localtime_buf;
} ThreadSpecificData;

/*
 * If we fall back on the thread-unsafe versions of gmtime and localtime, use
 * this mutex to try to protect them.
 */

TCL_DECLARE_MUTEX(tmMutex)

static char *lastTZ = NULL;	/* Holds the last setting of the TZ
				 * environment variable, or an empty string if
				 * the variable was not set. */

/*
 * Static functions declared in this file.
 */

static void		SetTZIfNecessary(void);
static void		CleanupMemory(ClientData clientData);
static void		NativeScaleTime(Tcl_Time *timebuf,
			    ClientData clientData);
static void		NativeGetTime(Tcl_Time *timebuf,
			    ClientData clientData);

/*
 * TIP #233 (Virtualized Time): Data for the time hooks, if any.
 */

Tcl_GetTimeProc *tclGetTimeProcPtr = NativeGetTime;
Tcl_ScaleTimeProc *tclScaleTimeProcPtr = NativeScaleTime;
ClientData tclTimeClientData = NULL;

/*
 *----------------------------------------------------------------------
 *
 * TclpGetSeconds --
 *
 *	This procedure returns the number of seconds from the epoch. On most
 *	Unix systems the epoch is Midnight Jan 1, 1970 GMT.
 *
 * Results:
 *	Number of seconds from the epoch.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TclpGetSeconds(void)
{
    return time(NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetMicroseconds --
 *
 *	This procedure returns the number of microseconds from the epoch.
 *	On most Unix systems the epoch is Midnight Jan 1, 1970 GMT.
 *
 * Results:
 *	Number of microseconds from the epoch.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_WideInt
TclpGetMicroseconds(void)
{
    Tcl_Time time;

    tclGetTimeProcPtr(&time, tclTimeClientData);
    return ((Tcl_WideInt)time.sec)*1000000 + time.usec;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetClicks --
 *
 *	This procedure returns a value that represents the highest resolution
 *	clock available on the system. There are no garantees on what the
 *	resolution will be. In Tcl we will call this value a "click". The
 *	start time is also system dependent.
 *
 * Results:
 *	Number of clicks from some start time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TclpGetClicks(void)
{
    unsigned long now;

#ifdef NO_GETTOD
    if (tclGetTimeProcPtr != NativeGetTime) {
	Tcl_Time time;

	tclGetTimeProcPtr(&time, tclTimeClientData);
	now = time.sec*1000000 + time.usec;
    } else {
	/*
	 * A semi-NativeGetTime, specialized to clicks.
	 */
	struct tms dummy;

	now = (unsigned long) times(&dummy);
    }
#else
    Tcl_Time time;

    tclGetTimeProcPtr(&time, tclTimeClientData);
    now = time.sec*1000000 + time.usec;
#endif

    return now;
}
#ifdef TCL_WIDE_CLICKS

/*
 *----------------------------------------------------------------------
 *
 * TclpGetWideClicks --
 *
 *	This procedure returns a WideInt value that represents the highest
 *	resolution clock available on the system. There are no garantees on
 *	what the resolution will be. In Tcl we will call this value a "click".
 *	The start time is also system dependent.
 *
 * Results:
 *	Number of WideInt clicks from some start time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_WideInt
TclpGetWideClicks(void)
{
    Tcl_WideInt now;

    if (tclGetTimeProcPtr != NativeGetTime) {
	Tcl_Time time;

	tclGetTimeProcPtr(&time, tclTimeClientData);
	now = ((Tcl_WideInt)time.sec)*1000000 + time.usec;
    } else {
#ifdef MAC_OSX_TCL
	now = (Tcl_WideInt) (mach_absolute_time() & INT64_MAX);
#else
#error Wide high-resolution clicks not implemented on this platform
#endif
    }

    return now;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpWideClicksToNanoseconds --
 *
 *	This procedure converts click values from the TclpGetWideClicks native
 *	resolution to nanosecond resolution.
 *
 * Results:
 *	Number of nanoseconds from some start time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
TclpWideClicksToNanoseconds(
    Tcl_WideInt clicks)
{
    double nsec;

    if (tclGetTimeProcPtr != NativeGetTime) {
	nsec = clicks * 1000;
    } else {
#ifdef MAC_OSX_TCL
	static mach_timebase_info_data_t tb;
	static uint64_t maxClicksForUInt64;

	if (!tb.denom) {
	    mach_timebase_info(&tb);
	    maxClicksForUInt64 = UINT64_MAX / tb.numer;
	}
	if ((uint64_t) clicks < maxClicksForUInt64) {
	    nsec = ((uint64_t) clicks) * tb.numer / tb.denom;
	} else {
	    nsec = ((long double) (uint64_t) clicks) * tb.numer / tb.denom;
	}
#else
#error Wide high-resolution clicks not implemented on this platform
#endif
    }

    return nsec;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpWideClickInMicrosec --
 *
 *	This procedure return scale to convert click values from the
 *	TclpGetWideClicks native resolution to microsecond resolution
 *	and back.
 *
 * Results:
 * 	1 click in microseconds as double.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

double
TclpWideClickInMicrosec(void)
{
    if (tclGetTimeProcPtr != NativeGetTime) {
	return 1.0;
    } else {
#ifdef MAC_OSX_TCL
	static int initialized = 0;
	static double scale = 0.0;

	if (initialized) {
	    return scale;
	} else {
	    mach_timebase_info_data_t tb;

	    mach_timebase_info(&tb);
	    /* value of tb.numer / tb.denom = 1 click in nanoseconds */
	    scale = ((double)tb.numer) / tb.denom / 1000;
	    initialized = 1;
	    return scale;
	}
#else
#error Wide high-resolution clicks not implemented on this platform
#endif
    }
}
#endif /* TCL_WIDE_CLICKS */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetTime --
 *
 *	Gets the current system time in seconds and microseconds since the
 *	beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 *	This function is hooked, allowing users to specify their own virtual
 *	system time.
 *
 * Results:
 *	Returns the current time in timePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetTime(
    Tcl_Time *timePtr)		/* Location to store time information. */
{
    tclGetTimeProcPtr(timePtr, tclTimeClientData);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDate --
 *
 *	This function converts between seconds and struct tm. If useGMT is
 *	true, then the returned date will be in Greenwich Mean Time (GMT).
 *	Otherwise, it will be in the local time zone.
 *
 * Results:
 *	Returns a static tm structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGetDate(
    const time_t *time,
    int useGMT)
{
    if (useGMT) {
	return TclpGmtime(time);
    } else {
	return TclpLocaltime(time);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGmtime --
 *
 *	Wrapper around the 'gmtime' library function to make it thread safe.
 *
 * Results:
 *	Returns a pointer to a 'struct tm' in thread-specific data.
 *
 * Side effects:
 *	Invokes gmtime or gmtime_r as appropriate.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGmtime(
    const time_t *timePtr)	/* Pointer to the number of seconds since the
				 * local system's epoch */
{
    /*
     * Get a thread-local buffer to hold the returned time.
     */

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&tmKey);

#ifdef HAVE_GMTIME_R
    gmtime_r(timePtr, &tsdPtr->gmtime_buf);
#else
    Tcl_MutexLock(&tmMutex);
    memcpy(&tsdPtr->gmtime_buf, gmtime(timePtr), sizeof(struct tm));
    Tcl_MutexUnlock(&tmMutex);
#endif

    return &tsdPtr->gmtime_buf;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpLocaltime --
 *
 *	Wrapper around the 'localtime' library function to make it thread
 *	safe.
 *
 * Results:
 *	Returns a pointer to a 'struct tm' in thread-specific data.
 *
 * Side effects:
 *	Invokes localtime or localtime_r as appropriate.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpLocaltime(
    const time_t *timePtr)	/* Pointer to the number of seconds since the
				 * local system's epoch */
{
    /*
     * Get a thread-local buffer to hold the returned time.
     */

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&tmKey);

    SetTZIfNecessary();
#ifdef HAVE_LOCALTIME_R
    localtime_r(timePtr, &tsdPtr->localtime_buf);
#else
    Tcl_MutexLock(&tmMutex);
    memcpy(&tsdPtr->localtime_buf, localtime(timePtr), sizeof(struct tm));
    Tcl_MutexUnlock(&tmMutex);
#endif

    return &tsdPtr->localtime_buf;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetTimeProc --
 *
 *	TIP #233 (Virtualized Time): Registers two handlers for the
 *	virtualization of Tcl's access to time information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Remembers the handlers, alters core behaviour.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetTimeProc(
    Tcl_GetTimeProc *getProc,
    Tcl_ScaleTimeProc *scaleProc,
    ClientData clientData)
{
    tclGetTimeProcPtr = getProc;
    tclScaleTimeProcPtr = scaleProc;
    tclTimeClientData = clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_QueryTimeProc --
 *
 *	TIP #233 (Virtualized Time): Query which time handlers are registered.
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
Tcl_QueryTimeProc(
    Tcl_GetTimeProc **getProc,
    Tcl_ScaleTimeProc **scaleProc,
    ClientData *clientData)
{
    if (getProc) {
	*getProc = tclGetTimeProcPtr;
    }
    if (scaleProc) {
	*scaleProc = tclScaleTimeProcPtr;
    }
    if (clientData) {
	*clientData = tclTimeClientData;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NativeScaleTime --
 *
 *	TIP #233: Scale from virtual time to the real-time. For native scaling
 *	the relationship is 1:1 and nothing has to be done.
 *
 * Results:
 *	Scales the time in timePtr.
 *
 * Side effects:
 *	See above.
 *
 *----------------------------------------------------------------------
 */

static void
NativeScaleTime(
    Tcl_Time *timePtr,
    ClientData clientData)
{
    /* Native scale is 1:1. Nothing is done */
}

/*
 *----------------------------------------------------------------------
 *
 * NativeGetTime --
 *
 *	TIP #233: Gets the current system time in seconds and microseconds
 *	since the beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 * Results:
 *	Returns the current time in timePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
NativeGetTime(
    Tcl_Time *timePtr,
    ClientData clientData)
{
    struct timeval tv;

    (void) gettimeofday(&tv, NULL);
    timePtr->sec = tv.tv_sec;
    timePtr->usec = tv.tv_usec;
}
/*
 *----------------------------------------------------------------------
 *
 * SetTZIfNecessary --
 *
 *	Determines whether a call to 'tzset' is needed prior to the next call
 *	to 'localtime' or examination of the 'timezone' variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If 'tzset' has never been called in the current process, or if the
 *	value of the environment variable TZ has changed since the last call
 *	to 'tzset', then 'tzset' is called again.
 *
 *----------------------------------------------------------------------
 */

static void
SetTZIfNecessary(void)
{
    const char *newTZ = getenv("TZ");

    Tcl_MutexLock(&tmMutex);
    if (newTZ == NULL) {
	newTZ = "";
    }
    if (lastTZ == NULL || strcmp(lastTZ, newTZ)) {
	tzset();
	if (lastTZ == NULL) {
	    Tcl_CreateExitHandler(CleanupMemory, NULL);
	} else {
	    ckfree(lastTZ);
	}
	lastTZ = ckalloc(strlen(newTZ) + 1);
	strcpy(lastTZ, newTZ);
    }
    Tcl_MutexUnlock(&tmMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupMemory --
 *
 *	Releases the private copy of the TZ environment variable upon exit
 *	from Tcl.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees allocated memory.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupMemory(
    ClientData ignored)
{
    ckfree(lastTZ);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
