/*
 * tclClock.c --
 *
 *	Contains the time and date related commands. This code is derived from
 *	the time and date facilities of TclX, by Mark Diekhans and Karl
 *	Lehenbauer.
 *
 * Copyright 1991-1995 Karl Lehenbauer and Mark Diekhans.
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * Copyright (c) 2004 by Kevin B. Kenny. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Windows has mktime. The configurators do not check.
 */

#ifdef _WIN32
#define HAVE_MKTIME 1
#endif

/*
 * Constants
 */

#define JULIAN_DAY_POSIX_EPOCH		2440588
#define SECONDS_PER_DAY			86400
#define JULIAN_SEC_POSIX_EPOCH	      (((Tcl_WideInt) JULIAN_DAY_POSIX_EPOCH) \
					* SECONDS_PER_DAY)
#define FOUR_CENTURIES			146097	/* days */
#define JDAY_1_JAN_1_CE_JULIAN		1721424
#define JDAY_1_JAN_1_CE_GREGORIAN	1721426
#define ONE_CENTURY_GREGORIAN		36524	/* days */
#define FOUR_YEARS			1461	/* days */
#define ONE_YEAR			365	/* days */

/*
 * Table of the days in each month, leap and common years
 */

static const int hath[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
static const int daysInPriorMonths[2][13] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

/*
 * Enumeration of the string literals used in [clock]
 */

typedef enum ClockLiteral {
    LIT__NIL,
    LIT__DEFAULT_FORMAT,
    LIT_BCE,		LIT_C,
    LIT_CANNOT_USE_GMT_AND_TIMEZONE,
    LIT_CE,
    LIT_DAYOFMONTH,	LIT_DAYOFWEEK,		LIT_DAYOFYEAR,
    LIT_ERA,		LIT_GMT,		LIT_GREGORIAN,
    LIT_INTEGER_VALUE_TOO_LARGE,
    LIT_ISO8601WEEK,	LIT_ISO8601YEAR,
    LIT_JULIANDAY,	LIT_LOCALSECONDS,
    LIT_MONTH,
    LIT_SECONDS,	LIT_TZNAME,		LIT_TZOFFSET,
    LIT_YEAR,
    LIT__END
} ClockLiteral;
static const char *const literals[] = {
    "",
    "%a %b %d %H:%M:%S %Z %Y",
    "BCE",		"C",
    "cannot use -gmt and -timezone in same call",
    "CE",
    "dayOfMonth",	"dayOfWeek",		"dayOfYear",
    "era",		":GMT",			"gregorian",
    "integer value too large to represent",
    "iso8601Week",	"iso8601Year",
    "julianDay",	"localSeconds",
    "month",
    "seconds",		"tzName",		"tzOffset",
    "year"
};

/*
 * Structure containing the client data for [clock]
 */

typedef struct {
    size_t refCount;		/* Number of live references. */
    Tcl_Obj **literals;		/* Pool of object literals. */
} ClockClientData;

/*
 * Structure containing the fields used in [clock format] and [clock scan]
 */

typedef struct TclDateFields {
    Tcl_WideInt seconds;	/* Time expressed in seconds from the Posix
				 * epoch */
    Tcl_WideInt localSeconds;	/* Local time expressed in nominal seconds
				 * from the Posix epoch */
    int tzOffset;		/* Time zone offset in seconds east of
				 * Greenwich */
    Tcl_Obj *tzName;		/* Time zone name */
    int julianDay;		/* Julian Day Number in local time zone */
    enum {BCE=1, CE=0} era;	/* Era */
    int gregorian;		/* Flag == 1 if the date is Gregorian */
    int year;			/* Year of the era */
    int dayOfYear;		/* Day of the year (1 January == 1) */
    int month;			/* Month number */
    int dayOfMonth;		/* Day of the month */
    int iso8601Year;		/* ISO8601 week-based year */
    int iso8601Week;		/* ISO8601 week number */
    int dayOfWeek;		/* Day of the week */
} TclDateFields;
static const char *const eras[] = { "CE", "BCE", NULL };

/*
 * Thread specific data block holding a 'struct tm' for the 'gmtime' and
 * 'localtime' library calls.
 */

static Tcl_ThreadDataKey tmKey;

/*
 * Mutex protecting 'gmtime', 'localtime' and 'mktime' calls and the statics
 * in the date parsing code.
 */

TCL_DECLARE_MUTEX(clockMutex)

/*
 * Function prototypes for local procedures in this file:
 */

static int		ConvertUTCToLocal(Tcl_Interp *,
			    TclDateFields *, Tcl_Obj *, int);
static int		ConvertUTCToLocalUsingTable(Tcl_Interp *,
			    TclDateFields *, int, Tcl_Obj *const[]);
static int		ConvertUTCToLocalUsingC(Tcl_Interp *,
			    TclDateFields *, int);
static int		ConvertLocalToUTC(Tcl_Interp *,
			    TclDateFields *, Tcl_Obj *, int);
static int		ConvertLocalToUTCUsingTable(Tcl_Interp *,
			    TclDateFields *, int, Tcl_Obj *const[]);
static int		ConvertLocalToUTCUsingC(Tcl_Interp *,
			    TclDateFields *, int);
static Tcl_Obj *	LookupLastTransition(Tcl_Interp *, Tcl_WideInt,
			    int, Tcl_Obj *const *);
static void		GetYearWeekDay(TclDateFields *, int);
static void		GetGregorianEraYearDay(TclDateFields *, int);
static void		GetMonthDay(TclDateFields *);
static void		GetJulianDayFromEraYearWeekDay(TclDateFields *, int);
static void		GetJulianDayFromEraYearMonthDay(TclDateFields *, int);
static int		IsGregorianLeapYear(TclDateFields *);
static int		WeekdayOnOrBefore(int, int);
static int		ClockClicksObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockConvertlocaltoutcObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockGetdatefieldsObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockGetjuliandayfromerayearmonthdayObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockGetjuliandayfromerayearweekdayObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockGetenvObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockMicrosecondsObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockMillisecondsObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockParseformatargsObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static int		ClockSecondsObjCmd(
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static struct tm *	ThreadSafeLocalTime(const time_t *);
static void		TzsetIfNecessary(void);
static void		ClockDeleteCmdProc(ClientData);

/*
 * Structure containing description of "native" clock commands to create.
 */

struct ClockCommand {
    const char *name;		/* The tail of the command name. The full name
				 * is "::tcl::clock::<name>". When NULL marks
				 * the end of the table. */
    Tcl_ObjCmdProc *objCmdProc;	/* Function that implements the command. This
				 * will always have the ClockClientData sent
				 * to it, but may well ignore this data. */
};

static const struct ClockCommand clockCommands[] = {
    { "getenv",			ClockGetenvObjCmd },
    { "Oldscan",		TclClockOldscanObjCmd },
    { "ConvertLocalToUTC",	ClockConvertlocaltoutcObjCmd },
    { "GetDateFields",		ClockGetdatefieldsObjCmd },
    { "GetJulianDayFromEraYearMonthDay",
		ClockGetjuliandayfromerayearmonthdayObjCmd },
    { "GetJulianDayFromEraYearWeekDay",
		ClockGetjuliandayfromerayearweekdayObjCmd },
    { "ParseFormatArgs",	ClockParseformatargsObjCmd },
    { NULL, NULL }
};

/*
 *----------------------------------------------------------------------
 *
 * TclClockInit --
 *
 *	Registers the 'clock' subcommands with the Tcl interpreter and
 *	initializes its client data (which consists mostly of constant
 *	Tcl_Obj's that it is too much trouble to keep recreating).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Installs the commands and creates the client data
 *
 *----------------------------------------------------------------------
 */

void
TclClockInit(
    Tcl_Interp *interp)		/* Tcl interpreter */
{
    const struct ClockCommand *clockCmdPtr;
    char cmdName[50];		/* Buffer large enough to hold the string
				 *::tcl::clock::GetJulianDayFromEraYearMonthDay
				 * plus a terminating NUL. */
    ClockClientData *data;
    int i;

    /* Structure of the 'clock' ensemble */

    static const EnsembleImplMap clockImplMap[] = {
	{"add",          NULL,                    TclCompileBasicMin1ArgCmd, NULL, NULL,       0},
	{"clicks",       ClockClicksObjCmd,       TclCompileClockClicksCmd,  NULL, NULL,       0},
	{"format",       NULL,                    TclCompileBasicMin1ArgCmd, NULL, NULL,       0},
	{"microseconds", ClockMicrosecondsObjCmd, TclCompileClockReadingCmd, NULL, INT2PTR(1), 0},
	{"milliseconds", ClockMillisecondsObjCmd, TclCompileClockReadingCmd, NULL, INT2PTR(2), 0},
	{"scan",         NULL,                    TclCompileBasicMin1ArgCmd, NULL, NULL      , 0},
	{"seconds",      ClockSecondsObjCmd,      TclCompileClockReadingCmd, NULL, INT2PTR(3), 0},
	{NULL,           NULL,                    NULL,                      NULL, NULL,       0}
    };

    /*
     * Safe interps get [::clock] as alias to a master, so do not need their
     * own copies of the support routines.
     */

    if (Tcl_IsSafe(interp)) {
	return;
    }

    /*
     * Create the client data, which is a refcounted literal pool.
     */

    data = ckalloc(sizeof(ClockClientData));
    data->refCount = 0;
    data->literals = ckalloc(LIT__END * sizeof(Tcl_Obj*));
    for (i = 0; i < LIT__END; ++i) {
	data->literals[i] = Tcl_NewStringObj(literals[i], -1);
	Tcl_IncrRefCount(data->literals[i]);
    }

    /*
     * Install the commands.
     * TODO - Let Tcl_MakeEnsemble do this?
     */

#define TCL_CLOCK_PREFIX_LEN 14 /* == strlen("::tcl::clock::") */
    memcpy(cmdName, "::tcl::clock::", TCL_CLOCK_PREFIX_LEN);
    for (clockCmdPtr=clockCommands ; clockCmdPtr->name!=NULL ; clockCmdPtr++) {
	strcpy(cmdName + TCL_CLOCK_PREFIX_LEN, clockCmdPtr->name);
	data->refCount++;
	Tcl_CreateObjCommand(interp, cmdName, clockCmdPtr->objCmdProc, data,
		ClockDeleteCmdProc);
    }

    /* Make the clock ensemble */

    TclMakeEnsemble(interp, "clock", clockImplMap);
}

/*
 *----------------------------------------------------------------------
 *
 * ClockConvertlocaltoutcObjCmd --
 *
 *	Tcl command that converts a UTC time to a local time by whatever means
 *	is available.
 *
 * Usage:
 *	::tcl::clock::ConvertUTCToLocal dictionary tzdata changeover
 *
 * Parameters:
 *	dict - Dictionary containing a 'localSeconds' entry.
 *	tzdata - Time zone data
 *	changeover - Julian Day of the adoption of the Gregorian calendar.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	On success, sets the interpreter result to the given dictionary
 *	augmented with a 'seconds' field giving the UTC time. On failure,
 *	leaves an error message in the interpreter result.
 *
 *----------------------------------------------------------------------
 */

static int
ClockConvertlocaltoutcObjCmd(
    ClientData clientData,	/* Client data */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter vector */
{
    ClockClientData *data = clientData;
    Tcl_Obj *const *literals = data->literals;
    Tcl_Obj *secondsObj;
    Tcl_Obj *dict;
    int changeover;
    TclDateFields fields;
    int created = 0;
    int status;

    /*
     * Check params and convert time.
     */

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "dict tzdata changeover");
	return TCL_ERROR;
    }
    dict = objv[1];
    if (Tcl_DictObjGet(interp, dict, literals[LIT_LOCALSECONDS],
	    &secondsObj)!= TCL_OK) {
	return TCL_ERROR;
    }
    if (secondsObj == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("key \"localseconds\" not "
		"found in dictionary", -1));
	return TCL_ERROR;
    }
    if ((TclGetWideIntFromObj(interp, secondsObj,
	    &fields.localSeconds) != TCL_OK)
	|| (TclGetIntFromObj(interp, objv[3], &changeover) != TCL_OK)
	|| ConvertLocalToUTC(interp, &fields, objv[2], changeover)) {
	return TCL_ERROR;
    }

    /*
     * Copy-on-write; set the 'seconds' field in the dictionary and place the
     * modified dictionary in the interpreter result.
     */

    if (Tcl_IsShared(dict)) {
	dict = Tcl_DuplicateObj(dict);
	created = 1;
	Tcl_IncrRefCount(dict);
    }
    status = Tcl_DictObjPut(interp, dict, literals[LIT_SECONDS],
	    Tcl_NewWideIntObj(fields.seconds));
    if (status == TCL_OK) {
	Tcl_SetObjResult(interp, dict);
    }
    if (created) {
	Tcl_DecrRefCount(dict);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ClockGetdatefieldsObjCmd --
 *
 *	Tcl command that determines the values that [clock format] will use in
 *	formatting a date, and populates a dictionary with them.
 *
 * Usage:
 *	::tcl::clock::GetDateFields seconds tzdata changeover
 *
 * Parameters:
 *	seconds - Time expressed in seconds from the Posix epoch.
 *	tzdata - Time zone data of the time zone in which time is to be
 *		 expressed.
 *	changeover - Julian Day Number at which the current locale adopted
 *		     the Gregorian calendar
 *
 * Results:
 *	Returns a dictonary populated with the fields:
 *		seconds - Seconds from the Posix epoch
 *		localSeconds - Nominal seconds from the Posix epoch in the
 *			       local time zone.
 *		tzOffset - Time zone offset in seconds east of Greenwich
 *		tzName - Time zone name
 *		julianDay - Julian Day Number in the local time zone
 *
 *----------------------------------------------------------------------
 */

int
ClockGetdatefieldsObjCmd(
    ClientData clientData,	/* Opaque pointer to literal pool, etc. */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter vector */
{
    TclDateFields fields;
    Tcl_Obj *dict;
    ClockClientData *data = clientData;
    Tcl_Obj *const *literals = data->literals;
    int changeover;

    /*
     * Check params.
     */

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "seconds tzdata changeover");
	return TCL_ERROR;
    }
    if (TclGetWideIntFromObj(interp, objv[1], &fields.seconds) != TCL_OK
	    || TclGetIntFromObj(interp, objv[3], &changeover) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * fields.seconds could be an unsigned number that overflowed. Make sure
     * that it isn't.
     */

    if (objv[1]->typePtr == &tclBignumType) {
	Tcl_SetObjResult(interp, literals[LIT_INTEGER_VALUE_TOO_LARGE]);
	return TCL_ERROR;
    }

    /*
     * Convert UTC time to local.
     */

    if (ConvertUTCToLocal(interp, &fields, objv[2], changeover) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Extract Julian day.
     */

    fields.julianDay = (int) ((fields.localSeconds + JULIAN_SEC_POSIX_EPOCH)
	    / SECONDS_PER_DAY);

    /*
     * Convert to Julian or Gregorian calendar.
     */

    GetGregorianEraYearDay(&fields, changeover);
    GetMonthDay(&fields);
    GetYearWeekDay(&fields, changeover);

    dict = Tcl_NewDictObj();
    Tcl_DictObjPut(NULL, dict, literals[LIT_LOCALSECONDS],
	    Tcl_NewWideIntObj(fields.localSeconds));
    Tcl_DictObjPut(NULL, dict, literals[LIT_SECONDS],
	    Tcl_NewWideIntObj(fields.seconds));
    Tcl_DictObjPut(NULL, dict, literals[LIT_TZNAME], fields.tzName);
    Tcl_DecrRefCount(fields.tzName);
    Tcl_DictObjPut(NULL, dict, literals[LIT_TZOFFSET],
	    Tcl_NewIntObj(fields.tzOffset));
    Tcl_DictObjPut(NULL, dict, literals[LIT_JULIANDAY],
	    Tcl_NewIntObj(fields.julianDay));
    Tcl_DictObjPut(NULL, dict, literals[LIT_GREGORIAN],
	    Tcl_NewIntObj(fields.gregorian));
    Tcl_DictObjPut(NULL, dict, literals[LIT_ERA],
	    literals[fields.era ? LIT_BCE : LIT_CE]);
    Tcl_DictObjPut(NULL, dict, literals[LIT_YEAR],
	    Tcl_NewIntObj(fields.year));
    Tcl_DictObjPut(NULL, dict, literals[LIT_DAYOFYEAR],
	    Tcl_NewIntObj(fields.dayOfYear));
    Tcl_DictObjPut(NULL, dict, literals[LIT_MONTH],
	    Tcl_NewIntObj(fields.month));
    Tcl_DictObjPut(NULL, dict, literals[LIT_DAYOFMONTH],
	    Tcl_NewIntObj(fields.dayOfMonth));
    Tcl_DictObjPut(NULL, dict, literals[LIT_ISO8601YEAR],
	    Tcl_NewIntObj(fields.iso8601Year));
    Tcl_DictObjPut(NULL, dict, literals[LIT_ISO8601WEEK],
	    Tcl_NewIntObj(fields.iso8601Week));
    Tcl_DictObjPut(NULL, dict, literals[LIT_DAYOFWEEK],
	    Tcl_NewIntObj(fields.dayOfWeek));
    Tcl_SetObjResult(interp, dict);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ClockGetjuliandayfromerayearmonthdayObjCmd --
 *
 *	Tcl command that converts a time from era-year-month-day to a Julian
 *	Day Number.
 *
 * Parameters:
 *	dict - Dictionary that contains 'era', 'year', 'month' and
 *	       'dayOfMonth' keys.
 *	changeover - Julian Day of changeover to the Gregorian calendar
 *
 * Results:
 *	Result is either TCL_OK, with the interpreter result being the
 *	dictionary augmented with a 'julianDay' key, or TCL_ERROR,
 *	with the result being an error message.
 *
 *----------------------------------------------------------------------
 */

static int
FetchEraField(
    Tcl_Interp *interp,
    Tcl_Obj *dict,
    Tcl_Obj *key,
    int *storePtr)
{
    Tcl_Obj *value = NULL;

    if (Tcl_DictObjGet(interp, dict, key, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"expected key(s) not found in dictionary", -1));
	return TCL_ERROR;
    }
    return Tcl_GetIndexFromObj(interp, value, eras, "era", TCL_EXACT, storePtr);
}

static int
FetchIntField(
    Tcl_Interp *interp,
    Tcl_Obj *dict,
    Tcl_Obj *key,
    int *storePtr)
{
    Tcl_Obj *value = NULL;

    if (Tcl_DictObjGet(interp, dict, key, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"expected key(s) not found in dictionary", -1));
	return TCL_ERROR;
    }
    return TclGetIntFromObj(interp, value, storePtr);
}

static int
ClockGetjuliandayfromerayearmonthdayObjCmd(
    ClientData clientData,	/* Opaque pointer to literal pool, etc. */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter vector */
{
    TclDateFields fields;
    Tcl_Obj *dict;
    ClockClientData *data = clientData;
    Tcl_Obj *const *literals = data->literals;
    int changeover;
    int copied = 0;
    int status;
    int era = 0;

    /*
     * Check params.
     */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "dict changeover");
	return TCL_ERROR;
    }
    dict = objv[1];
    if (FetchEraField(interp, dict, literals[LIT_ERA], &era) != TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_YEAR], &fields.year)
		!= TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_MONTH], &fields.month)
		!= TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_DAYOFMONTH],
		&fields.dayOfMonth) != TCL_OK
	    || TclGetIntFromObj(interp, objv[2], &changeover) != TCL_OK) {
	return TCL_ERROR;
    }
    fields.era = era;

    /*
     * Get Julian day.
     */

    GetJulianDayFromEraYearMonthDay(&fields, changeover);

    /*
     * Store Julian day in the dictionary - copy on write.
     */

    if (Tcl_IsShared(dict)) {
	dict = Tcl_DuplicateObj(dict);
	Tcl_IncrRefCount(dict);
	copied = 1;
    }
    status = Tcl_DictObjPut(interp, dict, literals[LIT_JULIANDAY],
	    Tcl_NewIntObj(fields.julianDay));
    if (status == TCL_OK) {
	Tcl_SetObjResult(interp, dict);
    }
    if (copied) {
	Tcl_DecrRefCount(dict);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ClockGetjuliandayfromerayearweekdayObjCmd --
 *
 *	Tcl command that converts a time from the ISO calendar to a Julian Day
 *	Number.
 *
 * Parameters:
 *	dict - Dictionary that contains 'era', 'iso8601Year', 'iso8601Week'
 *	       and 'dayOfWeek' keys.
 *	changeover - Julian Day of changeover to the Gregorian calendar
 *
 * Results:
 *	Result is either TCL_OK, with the interpreter result being the
 *	dictionary augmented with a 'julianDay' key, or TCL_ERROR, with the
 *	result being an error message.
 *
 *----------------------------------------------------------------------
 */

static int
ClockGetjuliandayfromerayearweekdayObjCmd(
    ClientData clientData,	/* Opaque pointer to literal pool, etc. */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter vector */
{
    TclDateFields fields;
    Tcl_Obj *dict;
    ClockClientData *data = clientData;
    Tcl_Obj *const *literals = data->literals;
    int changeover;
    int copied = 0;
    int status;
    int era = 0;

    /*
     * Check params.
     */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "dict changeover");
	return TCL_ERROR;
    }
    dict = objv[1];
    if (FetchEraField(interp, dict, literals[LIT_ERA], &era) != TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_ISO8601YEAR],
		&fields.iso8601Year) != TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_ISO8601WEEK],
		&fields.iso8601Week) != TCL_OK
	    || FetchIntField(interp, dict, literals[LIT_DAYOFWEEK],
		&fields.dayOfWeek) != TCL_OK
	    || TclGetIntFromObj(interp, objv[2], &changeover) != TCL_OK) {
	return TCL_ERROR;
    }
    fields.era = era;

    /*
     * Get Julian day.
     */

    GetJulianDayFromEraYearWeekDay(&fields, changeover);

    /*
     * Store Julian day in the dictionary - copy on write.
     */

    if (Tcl_IsShared(dict)) {
	dict = Tcl_DuplicateObj(dict);
	Tcl_IncrRefCount(dict);
	copied = 1;
    }
    status = Tcl_DictObjPut(interp, dict, literals[LIT_JULIANDAY],
	    Tcl_NewIntObj(fields.julianDay));
    if (status == TCL_OK) {
	Tcl_SetObjResult(interp, dict);
    }
    if (copied) {
	Tcl_DecrRefCount(dict);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertLocalToUTC --
 *
 *	Converts a time (in a TclDateFields structure) from the local wall
 *	clock to UTC.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Populates the 'seconds' field if successful; stores an error message
 *	in the interpreter result on failure.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertLocalToUTC(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Fields of the time */
    Tcl_Obj *tzdata,		/* Time zone data */
    int changeover)		/* Julian Day of the Gregorian transition */
{
    int rowc;			/* Number of rows in tzdata */
    Tcl_Obj **rowv;		/* Pointers to the rows */

    /*
     * Unpack the tz data.
     */

    if (TclListObjGetElements(interp, tzdata, &rowc, &rowv) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Special case: If the time zone is :localtime, the tzdata will be empty.
     * Use 'mktime' to convert the time to local
     */

    if (rowc == 0) {
	return ConvertLocalToUTCUsingC(interp, fields, changeover);
    } else {
	return ConvertLocalToUTCUsingTable(interp, fields, rowc, rowv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertLocalToUTCUsingTable --
 *
 *	Converts a time (in a TclDateFields structure) from local time in a
 *	given time zone to UTC.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Stores an error message in the interpreter if an error occurs; if
 *	successful, stores the 'seconds' field in 'fields.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertLocalToUTCUsingTable(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Time to convert, with 'seconds' filled in */
    int rowc,			/* Number of points at which time changes */
    Tcl_Obj *const rowv[])	/* Points at which time changes */
{
    Tcl_Obj *row;
    int cellc;
    Tcl_Obj **cellv;
    int have[8];
    int nHave = 0;
    int i;
    int found;

    /*
     * Perform an initial lookup assuming that local == UTC, and locate the
     * last time conversion prior to that time. Get the offset from that row,
     * and look up again. Continue until we find an offset that we found
     * before. This definition, rather than "the same offset" ensures that we
     * don't enter an endless loop, as would otherwise happen when trying to
     * convert a non-existent time such as 02:30 during the US Spring Daylight
     * Saving Time transition.
     */

    found = 0;
    fields->tzOffset = 0;
    fields->seconds = fields->localSeconds;
    while (!found) {
	row = LookupLastTransition(interp, fields->seconds, rowc, rowv);
	if ((row == NULL)
		|| TclListObjGetElements(interp, row, &cellc,
		    &cellv) != TCL_OK
		|| TclGetIntFromObj(interp, cellv[1],
		    &fields->tzOffset) != TCL_OK) {
	    return TCL_ERROR;
	}
	found = 0;
	for (i = 0; !found && i < nHave; ++i) {
	    if (have[i] == fields->tzOffset) {
		found = 1;
		break;
	    }
	}
	if (!found) {
	    if (nHave == 8) {
		Tcl_Panic("loop in ConvertLocalToUTCUsingTable");
	    }
	    have[nHave++] = fields->tzOffset;
	}
	fields->seconds = fields->localSeconds - fields->tzOffset;
    }
    fields->tzOffset = have[i];
    fields->seconds = fields->localSeconds - fields->tzOffset;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertLocalToUTCUsingC --
 *
 *	Converts a time from local wall clock to UTC when the local time zone
 *	cannot be determined. Uses 'mktime' to do the job.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Stores an error message in the interpreter if an error occurs; if
 *	successful, stores the 'seconds' field in 'fields.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertLocalToUTCUsingC(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Time to convert, with 'seconds' filled in */
    int changeover)		/* Julian Day of the Gregorian transition */
{
    struct tm timeVal;
    int localErrno;
    int secondOfDay;
    Tcl_WideInt jsec;

    /*
     * Convert the given time to a date.
     */

    jsec = fields->localSeconds + JULIAN_SEC_POSIX_EPOCH;
    fields->julianDay = (int) (jsec / SECONDS_PER_DAY);
    secondOfDay = (int)(jsec % SECONDS_PER_DAY);
    if (secondOfDay < 0) {
	secondOfDay += SECONDS_PER_DAY;
	fields->julianDay--;
    }
    GetGregorianEraYearDay(fields, changeover);
    GetMonthDay(fields);

    /*
     * Convert the date/time to a 'struct tm'.
     */

    timeVal.tm_year = fields->year - 1900;
    timeVal.tm_mon = fields->month - 1;
    timeVal.tm_mday = fields->dayOfMonth;
    timeVal.tm_hour = (secondOfDay / 3600) % 24;
    timeVal.tm_min = (secondOfDay / 60) % 60;
    timeVal.tm_sec = secondOfDay % 60;
    timeVal.tm_isdst = -1;
    timeVal.tm_wday = -1;
    timeVal.tm_yday = -1;

    /*
     * Get local time. It is rumored that mktime is not thread safe on some
     * platforms, so seize a mutex before attempting this.
     */

    TzsetIfNecessary();
    Tcl_MutexLock(&clockMutex);
    errno = 0;
    fields->seconds = (Tcl_WideInt) mktime(&timeVal);
    localErrno = errno;
    Tcl_MutexUnlock(&clockMutex);

    /*
     * If conversion fails, report an error.
     */

    if (localErrno != 0
	    || (fields->seconds == -1 && timeVal.tm_yday == -1)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"time value too large/small to represent", -1));
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertUTCToLocal --
 *
 *	Converts a time (in a TclDateFields structure) from UTC to local time.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Populates the 'tzName' and 'tzOffset' fields.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertUTCToLocal(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Fields of the time */
    Tcl_Obj *tzdata,		/* Time zone data */
    int changeover)		/* Julian Day of the Gregorian transition */
{
    int rowc;			/* Number of rows in tzdata */
    Tcl_Obj **rowv;		/* Pointers to the rows */

    /*
     * Unpack the tz data.
     */

    if (TclListObjGetElements(interp, tzdata, &rowc, &rowv) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Special case: If the time zone is :localtime, the tzdata will be empty.
     * Use 'localtime' to convert the time to local
     */

    if (rowc == 0) {
	return ConvertUTCToLocalUsingC(interp, fields, changeover);
    } else {
	return ConvertUTCToLocalUsingTable(interp, fields, rowc, rowv);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertUTCToLocalUsingTable --
 *
 *	Converts UTC to local time, given a table of transition points
 *
 * Results:
 *	Returns a standard Tcl result
 *
 * Side effects:
 *	On success, fills fields->tzName, fields->tzOffset and
 *	fields->localSeconds. On failure, places an error message in the
 *	interpreter result.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertUTCToLocalUsingTable(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Fields of the date */
    int rowc,			/* Number of rows in the conversion table
				 * (>= 1) */
    Tcl_Obj *const rowv[])	/* Rows of the conversion table */
{
    Tcl_Obj *row;		/* Row containing the current information */
    int cellc;			/* Count of cells in the row (must be 4) */
    Tcl_Obj **cellv;		/* Pointers to the cells */

    /*
     * Look up the nearest transition time.
     */

    row = LookupLastTransition(interp, fields->seconds, rowc, rowv);
    if (row == NULL ||
	    TclListObjGetElements(interp, row, &cellc, &cellv) != TCL_OK ||
	    TclGetIntFromObj(interp, cellv[1], &fields->tzOffset) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Convert the time.
     */

    fields->tzName = cellv[3];
    Tcl_IncrRefCount(fields->tzName);
    fields->localSeconds = fields->seconds + fields->tzOffset;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertUTCToLocalUsingC --
 *
 *	Converts UTC to localtime in cases where the local time zone is not
 *	determinable, using the C 'localtime' function to do it.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	On success, fills fields->tzName, fields->tzOffset and
 *	fields->localSeconds. On failure, places an error message in the
 *	interpreter result.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertUTCToLocalUsingC(
    Tcl_Interp *interp,		/* Tcl interpreter */
    TclDateFields *fields,	/* Time to convert, with 'seconds' filled in */
    int changeover)		/* Julian Day of the Gregorian transition */
{
    time_t tock;
    struct tm *timeVal;		/* Time after conversion */
    int diff;			/* Time zone diff local-Greenwich */
    char buffer[8];		/* Buffer for time zone name */

    /*
     * Use 'localtime' to determine local year, month, day, time of day.
     */

    tock = (time_t) fields->seconds;
    if ((Tcl_WideInt) tock != fields->seconds) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"number too large to represent as a Posix time", -1));
	Tcl_SetErrorCode(interp, "CLOCK", "argTooLarge", NULL);
	return TCL_ERROR;
    }
    TzsetIfNecessary();
    timeVal = ThreadSafeLocalTime(&tock);
    if (timeVal == NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"localtime failed (clock value may be too "
		"large/small to represent)", -1));
	Tcl_SetErrorCode(interp, "CLOCK", "localtimeFailed", NULL);
	return TCL_ERROR;
    }

    /*
     * Fill in the date in 'fields' and use it to derive Julian Day.
     */

    fields->era = CE;
    fields->year = timeVal->tm_year + 1900;
    fields->month = timeVal->tm_mon + 1;
    fields->dayOfMonth = timeVal->tm_mday;
    GetJulianDayFromEraYearMonthDay(fields, changeover);

    /*
     * Convert that value to seconds.
     */

    fields->localSeconds = (((fields->julianDay * (Tcl_WideInt) 24
	    + timeVal->tm_hour) * 60 + timeVal->tm_min) * 60
	    + timeVal->tm_sec) - JULIAN_SEC_POSIX_EPOCH;

    /*
     * Determine a time zone offset and name; just use +hhmm for the name.
     */

    diff = (int) (fields->localSeconds - fields->seconds);
    fields->tzOffset = diff;
    if (diff < 0) {
	*buffer = '-';
	diff = -diff;
    } else {
	*buffer = '+';
    }
    sprintf(buffer+1, "%02d", diff / 3600);
    diff %= 3600;
    sprintf(buffer+3, "%02d", diff / 60);
    diff %= 60;
    if (diff > 0) {
	sprintf(buffer+5, "%02d", diff);
    }
    fields->tzName = Tcl_NewStringObj(buffer, -1);
    Tcl_IncrRefCount(fields->tzName);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * LookupLastTransition --
 *
 *	Given a UTC time and a tzdata array, looks up the last transition on
 *	or before the given time.
 *
 * Results:
 *	Returns a pointer to the row, or NULL if an error occurs.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
LookupLastTransition(
    Tcl_Interp *interp,		/* Interpreter for error messages */
    Tcl_WideInt tick,		/* Time from the epoch */
    int rowc,			/* Number of rows of tzdata */
    Tcl_Obj *const *rowv)	/* Rows in tzdata */
{
    int l;
    int u;
    Tcl_Obj *compObj;
    Tcl_WideInt compVal;

    /*
     * Examine the first row to make sure we're in bounds.
     */

    if (Tcl_ListObjIndex(interp, rowv[0], 0, &compObj) != TCL_OK
	    || TclGetWideIntFromObj(interp, compObj, &compVal) != TCL_OK) {
	return NULL;
    }

    /*
     * Bizarre case - first row doesn't begin at MIN_WIDE_INT. Return it
     * anyway.
     */

    if (tick < compVal) {
	return rowv[0];
    }

    /*
     * Binary-search to find the transition.
     */

    l = 0;
    u = rowc-1;
    while (l < u) {
	int m = (l + u + 1) / 2;

	if (Tcl_ListObjIndex(interp, rowv[m], 0, &compObj) != TCL_OK ||
		TclGetWideIntFromObj(interp, compObj, &compVal) != TCL_OK) {
	    return NULL;
	}
	if (tick >= compVal) {
	    l = m;
	} else {
	    u = m-1;
	}
    }
    return rowv[l];
}

/*
 *----------------------------------------------------------------------
 *
 * GetYearWeekDay --
 *
 *	Given a date with Julian Calendar Day, compute the year, week, and day
 *	in the ISO8601 calendar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores 'iso8601Year', 'iso8601Week' and 'dayOfWeek' in the date
 *	fields.
 *
 *----------------------------------------------------------------------
 */

static void
GetYearWeekDay(
    TclDateFields *fields,	/* Date to convert, must have 'julianDay' */
    int changeover)		/* Julian Day Number of the Gregorian
				 * transition */
{
    TclDateFields temp;
    int dayOfFiscalYear;

    /*
     * Find the given date, minus three days, plus one year. That date's
     * iso8601 year is an upper bound on the ISO8601 year of the given date.
     */

    temp.julianDay = fields->julianDay - 3;
    GetGregorianEraYearDay(&temp, changeover);
    if (temp.era == BCE) {
	temp.iso8601Year = temp.year - 1;
    } else {
	temp.iso8601Year = temp.year + 1;
    }
    temp.iso8601Week = 1;
    temp.dayOfWeek = 1;
    GetJulianDayFromEraYearWeekDay(&temp, changeover);

    /*
     * temp.julianDay is now the start of an ISO8601 year, either the one
     * corresponding to the given date, or the one after. If we guessed high,
     * move one year earlier
     */

    if (fields->julianDay < temp.julianDay) {
	if (temp.era == BCE) {
	    temp.iso8601Year += 1;
	} else {
	    temp.iso8601Year -= 1;
	}
	GetJulianDayFromEraYearWeekDay(&temp, changeover);
    }

    fields->iso8601Year = temp.iso8601Year;
    dayOfFiscalYear = fields->julianDay - temp.julianDay;
    fields->iso8601Week = (dayOfFiscalYear / 7) + 1;
    fields->dayOfWeek = (dayOfFiscalYear + 1) % 7;
    if (fields->dayOfWeek < 1) {
	fields->dayOfWeek += 7;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetGregorianEraYearDay --
 *
 *	Given a Julian Day Number, extracts the year and day of the year and
 *	puts them into TclDateFields, along with the era (BCE or CE) and a
 *	flag indicating whether the date is Gregorian or Julian.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores 'era', 'gregorian', 'year', and 'dayOfYear'.
 *
 *----------------------------------------------------------------------
 */

static void
GetGregorianEraYearDay(
    TclDateFields *fields,	/* Date fields containing 'julianDay' */
    int changeover)		/* Gregorian transition date */
{
    int jday = fields->julianDay;
    int day;
    int year;
    int n;

    if (jday >= changeover) {
	/*
	 * Gregorian calendar.
	 */

	fields->gregorian = 1;
	year = 1;

	/*
	 * n = Number of 400-year cycles since 1 January, 1 CE in the
	 * proleptic Gregorian calendar. day = remaining days.
	 */

	day = jday - JDAY_1_JAN_1_CE_GREGORIAN;
	n = day / FOUR_CENTURIES;
	day %= FOUR_CENTURIES;
	if (day < 0) {
	    day += FOUR_CENTURIES;
	    n--;
	}
	year += 400 * n;

	/*
	 * n = number of centuries since the start of (year);
	 * day = remaining days
	 */

	n = day / ONE_CENTURY_GREGORIAN;
	day %= ONE_CENTURY_GREGORIAN;
	if (n > 3) {
	    /*
	     * 31 December in the last year of a 400-year cycle.
	     */

	    n = 3;
	    day += ONE_CENTURY_GREGORIAN;
	}
	year += 100 * n;
    } else {
	/*
	 * Julian calendar.
	 */

	fields->gregorian = 0;
	year = 1;
	day = jday - JDAY_1_JAN_1_CE_JULIAN;
    }

    /*
     * n = number of 4-year cycles; days = remaining days.
     */

    n = day / FOUR_YEARS;
    day %= FOUR_YEARS;
    if (day < 0) {
	day += FOUR_YEARS;
	n--;
    }
    year += 4 * n;

    /*
     * n = number of years; days = remaining days.
     */

    n = day / ONE_YEAR;
    day %= ONE_YEAR;
    if (n > 3) {
	/*
	 * 31 December of a leap year.
	 */

	n = 3;
	day += 365;
    }
    year += n;

    /*
     * store era/year/day back into fields.
     */

    if (year <= 0) {
	fields->era = BCE;
	fields->year = 1 - year;
    } else {
	fields->era = CE;
	fields->year = year;
    }
    fields->dayOfYear = day + 1;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMonthDay --
 *
 *	Given a date as year and day-of-year, find month and day.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores 'month' and 'dayOfMonth' in the 'fields' structure.
 *
 *----------------------------------------------------------------------
 */

static void
GetMonthDay(
    TclDateFields *fields)	/* Date to convert */
{
    int day = fields->dayOfYear;
    int month;
    const int *h = hath[IsGregorianLeapYear(fields)];

    for (month = 0; month < 12 && day > h[month]; ++month) {
	day -= h[month];
    }
    fields->month = month+1;
    fields->dayOfMonth = day;
}

/*
 *----------------------------------------------------------------------
 *
 * GetJulianDayFromEraYearWeekDay --
 *
 *	Given a TclDateFields structure containing era, ISO8601 year, ISO8601
 *	week, and day of week, computes the Julian Day Number.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores 'julianDay' in the fields.
 *
 *----------------------------------------------------------------------
 */

static void
GetJulianDayFromEraYearWeekDay(
    TclDateFields *fields,	/* Date to convert */
    int changeover)		/* Julian Day Number of the Gregorian
				 * transition */
{
    int firstMonday;		/* Julian day number of week 1, day 1 in the
				 * given year */
    TclDateFields firstWeek;

    /*
     * Find January 4 in the ISO8601 year, which will always be in week 1.
     */

    firstWeek.era = fields->era;
    firstWeek.year = fields->iso8601Year;
    firstWeek.month = 1;
    firstWeek.dayOfMonth = 4;
    GetJulianDayFromEraYearMonthDay(&firstWeek, changeover);

    /*
     * Find Monday of week 1.
     */

    firstMonday = WeekdayOnOrBefore(1, firstWeek.julianDay);

    /*
     * Advance to the given week and day.
     */

    fields->julianDay = firstMonday + 7 * (fields->iso8601Week - 1)
	    + fields->dayOfWeek - 1;
}

/*
 *----------------------------------------------------------------------
 *
 * GetJulianDayFromEraYearMonthDay --
 *
 *	Given era, year, month, and dayOfMonth (in TclDateFields), and the
 *	Gregorian transition date, computes the Julian Day Number.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores day number in 'julianDay'
 *
 *----------------------------------------------------------------------
 */

static void
GetJulianDayFromEraYearMonthDay(
    TclDateFields *fields,	/* Date to convert */
    int changeover)		/* Gregorian transition date as a Julian Day */
{
    int year, ym1, month, mm1, q, r, ym1o4, ym1o100, ym1o400;

    if (fields->era == BCE) {
	year = 1 - fields->year;
    } else {
	year = fields->year;
    }

    /*
     * Reduce month modulo 12.
     */

    month = fields->month;
    mm1 = month - 1;
    q = mm1 / 12;
    r = (mm1 % 12);
    if (r < 0) {
	r += 12;
	q -= 1;
    }
    year += q;
    month = r + 1;
    ym1 = year - 1;

    /*
     * Adjust the year after reducing the month.
     */

    fields->gregorian = 1;
    if (year < 1) {
	fields->era = BCE;
	fields->year = 1-year;
    } else {
	fields->era = CE;
	fields->year = year;
    }

    /*
     * Try an initial conversion in the Gregorian calendar.
     */

#if 0 /* BUG https://core.tcl-lang.org/tcl/tktview?name=da340d4f32 */
    ym1o4 = ym1 / 4;
#else
    /*
     * Have to make sure quotient is truncated towards 0 when negative.
     * See above bug for details. The casts are necessary.
     */
    if (ym1 >= 0)
	ym1o4 = ym1 / 4;
    else {
	ym1o4 = - (int) (((unsigned int) -ym1) / 4);
    }
#endif
    if (ym1 % 4 < 0) {
	ym1o4--;
    }
    ym1o100 = ym1 / 100;
    if (ym1 % 100 < 0) {
	ym1o100--;
    }
    ym1o400 = ym1 / 400;
    if (ym1 % 400 < 0) {
	ym1o400--;
    }
    fields->julianDay = JDAY_1_JAN_1_CE_GREGORIAN - 1
	    + fields->dayOfMonth
	    + daysInPriorMonths[IsGregorianLeapYear(fields)][month - 1]
	    + (ONE_YEAR * ym1)
	    + ym1o4
	    - ym1o100
	    + ym1o400;

    /*
     * If the resulting date is before the Gregorian changeover, convert in
     * the Julian calendar instead.
     */

    if (fields->julianDay < changeover) {
	fields->gregorian = 0;
	fields->julianDay = JDAY_1_JAN_1_CE_JULIAN - 1
		+ fields->dayOfMonth
		+ daysInPriorMonths[year%4 == 0][month - 1]
		+ (365 * ym1)
		+ ym1o4;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * IsGregorianLeapYear --
 *
 *	Tests whether a given year is a leap year, in either Julian or
 *	Gregorian calendar.
 *
 * Results:
 *	Returns 1 for a leap year, 0 otherwise.
 *
 *----------------------------------------------------------------------
 */

static int
IsGregorianLeapYear(
    TclDateFields *fields)	/* Date to test */
{
    int year = fields->year;

    if (fields->era == BCE) {
	year = 1 - year;
    }
    if (year%4 != 0) {
	return 0;
    } else if (!(fields->gregorian)) {
	return 1;
    } else if (year%400 == 0) {
	return 1;
    } else if (year%100 == 0) {
	return 0;
    } else {
	return 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WeekdayOnOrBefore --
 *
 *	Finds the Julian Day Number of a given day of the week that falls on
 *	or before a given date, expressed as Julian Day Number.
 *
 * Results:
 *	Returns the Julian Day Number
 *
 *----------------------------------------------------------------------
 */

static int
WeekdayOnOrBefore(
    int dayOfWeek,		/* Day of week; Sunday == 0 or 7 */
    int julianDay)		/* Reference date */
{
    int k = (dayOfWeek + 6) % 7;
    if (k < 0) {
	k += 7;
    }
    return julianDay - ((julianDay - k) % 7);
}

/*
 *----------------------------------------------------------------------
 *
 * ClockGetenvObjCmd --
 *
 *	Tcl command that reads an environment variable from the system
 *
 * Usage:
 *	::tcl::clock::getEnv NAME
 *
 * Parameters:
 *	NAME - Name of the environment variable desired
 *
 * Results:
 *	Returns a standard Tcl result. Returns an error if the variable does
 *	not exist, with a message left in the interpreter. Returns TCL_OK and
 *	the value of the variable if the variable does exist,
 *
 *----------------------------------------------------------------------
 */

int
ClockGetenvObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    const char *varName;
    const char *varValue;
    (void)clientData;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "name");
	return TCL_ERROR;
    }
    varName = TclGetString(objv[1]);
    varValue = getenv(varName);
    if (varValue == NULL) {
	varValue = "";
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(varValue, -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSafeLocalTime --
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

static struct tm *
ThreadSafeLocalTime(
    const time_t *timePtr)	/* Pointer to the number of seconds since the
				 * local system's epoch */
{
    /*
     * Get a thread-local buffer to hold the returned time.
     */

    struct tm *tmPtr = Tcl_GetThreadData(&tmKey, sizeof(struct tm));
#ifdef HAVE_LOCALTIME_R
    localtime_r(timePtr, tmPtr);
#else
    struct tm *sysTmPtr;

    Tcl_MutexLock(&clockMutex);
    sysTmPtr = localtime(timePtr);
    if (sysTmPtr == NULL) {
	Tcl_MutexUnlock(&clockMutex);
	return NULL;
    }
    memcpy(tmPtr, localtime(timePtr), sizeof(struct tm));
    Tcl_MutexUnlock(&clockMutex);
#endif
    return tmPtr;
}

/*----------------------------------------------------------------------
 *
 * ClockClicksObjCmd --
 *
 *	Returns a high-resolution counter.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 * This function implements the 'clock clicks' Tcl command. Refer to the user
 * documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

int
ClockClicksObjCmd(
    ClientData clientData,	/* Client data is unused */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter values */
{
    static const char *const clicksSwitches[] = {
	"-milliseconds", "-microseconds", NULL
    };
    enum ClicksSwitch {
	CLICKS_MILLIS, CLICKS_MICROS, CLICKS_NATIVE
    };
    int index = CLICKS_NATIVE;
    Tcl_Time now;
    Tcl_WideInt clicks = 0;
    (void)clientData;

    switch (objc) {
    case 1:
	break;
    case 2:
	if (Tcl_GetIndexFromObj(interp, objv[1], clicksSwitches, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    default:
	Tcl_WrongNumArgs(interp, 1, objv, "?-switch?");
	return TCL_ERROR;
    }

    switch (index) {
    case CLICKS_MILLIS:
	Tcl_GetTime(&now);
	clicks = (Tcl_WideInt) now.sec * 1000 + now.usec / 1000;
	break;
    case CLICKS_NATIVE:
#ifdef TCL_WIDE_CLICKS
	clicks = TclpGetWideClicks();
#else
	clicks = (Tcl_WideInt) TclpGetClicks();
#endif
	break;
    case CLICKS_MICROS:
	clicks = TclpGetMicroseconds();
	break;
    }

    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(clicks));
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * ClockMillisecondsObjCmd -
 *
 *	Returns a count of milliseconds since the epoch.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 * This function implements the 'clock milliseconds' Tcl command. Refer to the
 * user documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

int
ClockMillisecondsObjCmd(
    ClientData clientData,	/* Client data is unused */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter values */
{
    Tcl_Time now;
    (void)clientData;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }
    Tcl_GetTime(&now);
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)
	    now.sec * 1000 + now.usec / 1000));
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * ClockMicrosecondsObjCmd -
 *
 *	Returns a count of microseconds since the epoch.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 * This function implements the 'clock microseconds' Tcl command. Refer to the
 * user documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

int
ClockMicrosecondsObjCmd(
    ClientData clientData,	/* Client data is unused */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter values */
{
    (void)clientData;
    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(TclpGetMicroseconds()));
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * ClockParseformatargsObjCmd --
 *
 *	Parses the arguments for [clock format].
 *
 * Results:
 *	Returns a standard Tcl result, whose value is a four-element list
 *	comprising the time format, the locale, and the timezone.
 *
 * This function exists because the loop that parses the [clock format]
 * options is a known performance "hot spot", and is implemented in an effort
 * to speed that particular code up.
 *
 *-----------------------------------------------------------------------------
 */

static int
ClockParseformatargsObjCmd(
    ClientData clientData,	/* Client data containing literal pool */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[])	/* Parameter vector */
{
    ClockClientData *dataPtr = clientData;
    Tcl_Obj **litPtr = dataPtr->literals;
    Tcl_Obj *results[3];	/* Format, locale and timezone */
#define formatObj results[0]
#define localeObj results[1]
#define timezoneObj results[2]
    int gmtFlag = 0;
    static const char *const options[] = { /* Command line options expected */
	"-format",	"-gmt",		"-locale",
	"-timezone",	NULL };
    enum optionInd {
	CLOCK_FORMAT_FORMAT,	CLOCK_FORMAT_GMT,	CLOCK_FORMAT_LOCALE,
	CLOCK_FORMAT_TIMEZONE
    };
    int optionIndex;		/* Index of an option. */
    int saw = 0;		/* Flag == 1 if option was seen already. */
    Tcl_WideInt clockVal;	/* Clock value - just used to parse. */
    int i;

    /*
     * Args consist of a time followed by keyword-value pairs.
     */

    if (objc < 2 || (objc % 2) != 0) {
	Tcl_WrongNumArgs(interp, 0, objv,
		"clock format clockval ?-format string? "
		"?-gmt boolean? ?-locale LOCALE? ?-timezone ZONE?");
	Tcl_SetErrorCode(interp, "CLOCK", "wrongNumArgs", NULL);
	return TCL_ERROR;
    }

    /*
     * Extract values for the keywords.
     */

    formatObj = litPtr[LIT__DEFAULT_FORMAT];
    localeObj = litPtr[LIT_C];
    timezoneObj = litPtr[LIT__NIL];
    for (i = 2; i < objc; i+=2) {
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		&optionIndex) != TCL_OK) {
	    Tcl_SetErrorCode(interp, "CLOCK", "badOption",
		    Tcl_GetString(objv[i]), NULL);
	    return TCL_ERROR;
	}
	switch (optionIndex) {
	case CLOCK_FORMAT_FORMAT:
	    formatObj = objv[i+1];
	    break;
	case CLOCK_FORMAT_GMT:
	    if (Tcl_GetBooleanFromObj(interp, objv[i+1], &gmtFlag) != TCL_OK){
		return TCL_ERROR;
	    }
	    break;
	case CLOCK_FORMAT_LOCALE:
	    localeObj = objv[i+1];
	    break;
	case CLOCK_FORMAT_TIMEZONE:
	    timezoneObj = objv[i+1];
	    break;
	}
	saw |= 1 << optionIndex;
    }

    /*
     * Check options.
     */

    if (TclGetWideIntFromObj(interp, objv[1], &clockVal) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((saw & (1 << CLOCK_FORMAT_GMT))
	    && (saw & (1 << CLOCK_FORMAT_TIMEZONE))) {
	Tcl_SetObjResult(interp, litPtr[LIT_CANNOT_USE_GMT_AND_TIMEZONE]);
	Tcl_SetErrorCode(interp, "CLOCK", "gmtWithTimezone", NULL);
	return TCL_ERROR;
    }
    if (gmtFlag) {
	timezoneObj = litPtr[LIT_GMT];
    }

    /*
     * Return options as a list.
     */

    Tcl_SetObjResult(interp, Tcl_NewListObj(3, results));
    return TCL_OK;

#undef timezoneObj
#undef localeObj
#undef formatObj
}

/*----------------------------------------------------------------------
 *
 * ClockSecondsObjCmd -
 *
 *	Returns a count of microseconds since the epoch.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 * This function implements the 'clock seconds' Tcl command. Refer to the user
 * documentation for details on what it does.
 *
 *----------------------------------------------------------------------
 */

int
ClockSecondsObjCmd(
    ClientData clientData,	/* Client data is unused */
    Tcl_Interp *interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const *objv)	/* Parameter values */
{
    Tcl_Time now;
    (void)clientData;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }
    Tcl_GetTime(&now);
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt) now.sec));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TzsetIfNecessary --
 *
 *	Calls the tzset() library function if the contents of the TZ
 *	environment variable has changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls tzset.
 *
 *----------------------------------------------------------------------
 */

static void
TzsetIfNecessary(void)
{
    static char* tzWas = INT2PTR(-1);	/* Previous value of TZ, protected by
				 * clockMutex. */
    const char *tzIsNow;	/* Current value of TZ */

    Tcl_MutexLock(&clockMutex);
    tzIsNow = getenv("TZ");
    if (tzIsNow != NULL && (tzWas == NULL || tzWas == INT2PTR(-1)
	    || strcmp(tzIsNow, tzWas) != 0)) {
	tzset();
	if (tzWas != NULL && tzWas != INT2PTR(-1)) {
	    ckfree(tzWas);
	}
	tzWas = ckalloc(strlen(tzIsNow) + 1);
	strcpy(tzWas, tzIsNow);
    } else if (tzIsNow == NULL && tzWas != NULL) {
	tzset();
	if (tzWas != INT2PTR(-1)) ckfree(tzWas);
	tzWas = NULL;
    }
    Tcl_MutexUnlock(&clockMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ClockDeleteCmdProc --
 *
 *	Remove a reference to the clock client data, and clean up memory
 *	when it's all gone.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
ClockDeleteCmdProc(
    ClientData clientData)	/* Opaque pointer to the client data */
{
    ClockClientData *data = clientData;
    int i;

    if (data->refCount-- <= 1) {
	for (i = 0; i < LIT__END; ++i) {
	    Tcl_DecrRefCount(data->literals[i]);
	}
	ckfree(data->literals);
	ckfree(data);
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
