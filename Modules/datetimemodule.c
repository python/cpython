/*  C implementation for the date/time type documented at
 *  http://www.zope.org/Members/fdrake/DateTimeWiki/FrontPage
 */

#include "Python.h"
#include "modsupport.h"
#include "structmember.h"

#include <time.h>

#include "datetime.h"

/* We require that C int be at least 32 bits, and use int virtually
 * everywhere.  In just a few cases we use a temp long, where a Python
 * API returns a C long.  In such cases, we have to ensure that the
 * final result fits in a C int (this can be an issue on 64-bit boxes).
 */
#if SIZEOF_INT < 4
#	error "datetime.c requires that C int have at least 32 bits"
#endif

#define MINYEAR 1
#define MAXYEAR 9999

/* Nine decimal digits is easy to communicate, and leaves enough room
 * so that two delta days can be added w/o fear of overflowing a signed
 * 32-bit int, and with plenty of room left over to absorb any possible
 * carries from adding seconds.
 */
#define MAX_DELTA_DAYS 999999999

/* Rename the long macros in datetime.h to more reasonable short names. */
#define GET_YEAR		PyDateTime_GET_YEAR
#define GET_MONTH		PyDateTime_GET_MONTH
#define GET_DAY			PyDateTime_GET_DAY
#define DATE_GET_HOUR		PyDateTime_DATE_GET_HOUR
#define DATE_GET_MINUTE		PyDateTime_DATE_GET_MINUTE
#define DATE_GET_SECOND		PyDateTime_DATE_GET_SECOND
#define DATE_GET_MICROSECOND	PyDateTime_DATE_GET_MICROSECOND

/* Date accessors for date and datetime. */
#define SET_YEAR(o, v)		(((o)->data[0] = ((v) & 0xff00) >> 8), \
                                 ((o)->data[1] = ((v) & 0x00ff)))
#define SET_MONTH(o, v)		(PyDateTime_GET_MONTH(o) = (v))
#define SET_DAY(o, v)		(PyDateTime_GET_DAY(o) = (v))

/* Date/Time accessors for datetime. */
#define DATE_SET_HOUR(o, v)	(PyDateTime_DATE_GET_HOUR(o) = (v))
#define DATE_SET_MINUTE(o, v)	(PyDateTime_DATE_GET_MINUTE(o) = (v))
#define DATE_SET_SECOND(o, v)	(PyDateTime_DATE_GET_SECOND(o) = (v))
#define DATE_SET_MICROSECOND(o, v)	\
	(((o)->data[7] = ((v) & 0xff0000) >> 16), \
         ((o)->data[8] = ((v) & 0x00ff00) >> 8), \
         ((o)->data[9] = ((v) & 0x0000ff)))

/* Time accessors for time. */
#define TIME_GET_HOUR		PyDateTime_TIME_GET_HOUR
#define TIME_GET_MINUTE		PyDateTime_TIME_GET_MINUTE
#define TIME_GET_SECOND		PyDateTime_TIME_GET_SECOND
#define TIME_GET_MICROSECOND	PyDateTime_TIME_GET_MICROSECOND
#define TIME_SET_HOUR(o, v)	(PyDateTime_TIME_GET_HOUR(o) = (v))
#define TIME_SET_MINUTE(o, v)	(PyDateTime_TIME_GET_MINUTE(o) = (v))
#define TIME_SET_SECOND(o, v)	(PyDateTime_TIME_GET_SECOND(o) = (v))
#define TIME_SET_MICROSECOND(o, v)	\
	(((o)->data[3] = ((v) & 0xff0000) >> 16), \
         ((o)->data[4] = ((v) & 0x00ff00) >> 8), \
         ((o)->data[5] = ((v) & 0x0000ff)))

/* Delta accessors for timedelta. */
#define GET_TD_DAYS(o)		(((PyDateTime_Delta *)(o))->days)
#define GET_TD_SECONDS(o)	(((PyDateTime_Delta *)(o))->seconds)
#define GET_TD_MICROSECONDS(o)	(((PyDateTime_Delta *)(o))->microseconds)

#define SET_TD_DAYS(o, v)	((o)->days = (v))
#define SET_TD_SECONDS(o, v)	((o)->seconds = (v))
#define SET_TD_MICROSECONDS(o, v) ((o)->microseconds = (v))

/* Forward declarations. */
static PyTypeObject PyDateTime_DateType;
static PyTypeObject PyDateTime_DateTimeType;
static PyTypeObject PyDateTime_DateTimeTZType;
static PyTypeObject PyDateTime_DeltaType;
static PyTypeObject PyDateTime_TimeType;
static PyTypeObject PyDateTime_TZInfoType;
static PyTypeObject PyDateTime_TimeTZType;

/* ---------------------------------------------------------------------------
 * Math utilities.
 */

/* k = i+j overflows iff k differs in sign from both inputs,
 * iff k^i has sign bit set and k^j has sign bit set,
 * iff (k^i)&(k^j) has sign bit set.
 */
#define SIGNED_ADD_OVERFLOWED(RESULT, I, J) \
	((((RESULT) ^ (I)) & ((RESULT) ^ (J))) < 0)

/* Compute Python divmod(x, y), returning the quotient and storing the
 * remainder into *r.  The quotient is the floor of x/y, and that's
 * the real point of this.  C will probably truncate instead (C99
 * requires truncation; C89 left it implementation-defined).
 * Simplification:  we *require* that y > 0 here.  That's appropriate
 * for all the uses made of it.  This simplifies the code and makes
 * the overflow case impossible (divmod(LONG_MIN, -1) is the only
 * overflow case).
 */
static int
divmod(int x, int y, int *r)
{
	int quo;

	assert(y > 0);
	quo = x / y;
	*r = x - quo * y;
	if (*r < 0) {
		--quo;
		*r += y;
	}
	assert(0 <= *r && *r < y);
	return quo;
}

/* Round a double to the nearest long.  |x| must be small enough to fit
 * in a C long; this is not checked.
 */
static long
round_to_long(double x)
{
	if (x >= 0.0)
		x = floor(x + 0.5);
	else
		x = ceil(x - 0.5);
	return (long)x;
}

/* ---------------------------------------------------------------------------
 * General calendrical helper functions
 */

/* For each month ordinal in 1..12, the number of days in that month,
 * and the number of days before that month in the same year.  These
 * are correct for non-leap years only.
 */
static int _days_in_month[] = {
	0, /* unused; this vector uses 1-based indexing */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int _days_before_month[] = {
	0, /* unused; this vector uses 1-based indexing */
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/* year -> 1 if leap year, else 0. */
static int
is_leap(int year)
{
	/* Cast year to unsigned.  The result is the same either way, but
	 * C can generate faster code for unsigned mod than for signed
	 * mod (especially for % 4 -- a good compiler should just grab
	 * the last 2 bits when the LHS is unsigned).
	 */
	const unsigned int ayear = (unsigned int)year;
	return ayear % 4 == 0 && (ayear % 100 != 0 || ayear % 400 == 0);
}

/* year, month -> number of days in that month in that year */
static int
days_in_month(int year, int month)
{
	assert(month >= 1);
	assert(month <= 12);
	if (month == 2 && is_leap(year))
		return 29;
	else
		return _days_in_month[month];
}

/* year, month -> number of days in year preceeding first day of month */
static int
days_before_month(int year, int month)
{
	int days;

	assert(month >= 1);
	assert(month <= 12);
	days = _days_before_month[month];
	if (month > 2 && is_leap(year))
		++days;
	return days;
}

/* year -> number of days before January 1st of year.  Remember that we
 * start with year 1, so days_before_year(1) == 0.
 */
static int
days_before_year(int year)
{
	int y = year - 1;
	/* This is incorrect if year <= 0; we really want the floor
	 * here.  But so long as MINYEAR is 1, the smallest year this
	 * can see is 0 (this can happen in some normalization endcases),
	 * so we'll just special-case that.
	 */
	assert (year >= 0);
	if (y >= 0)
		return y*365 + y/4 - y/100 + y/400;
	else {
		assert(y == -1);
		return -366;
	}
}

/* Number of days in 4, 100, and 400 year cycles.  That these have
 * the correct values is asserted in the module init function.
 */
#define DI4Y	1461	/* days_before_year(5); days in 4 years */
#define DI100Y	36524	/* days_before_year(101); days in 100 years */
#define DI400Y	146097	/* days_before_year(401); days in 400 years  */

/* ordinal -> year, month, day, considering 01-Jan-0001 as day 1. */
static void
ord_to_ymd(int ordinal, int *year, int *month, int *day)
{
	int n, n1, n4, n100, n400, leapyear, preceding;

	/* ordinal is a 1-based index, starting at 1-Jan-1.  The pattern of
	 * leap years repeats exactly every 400 years.  The basic strategy is
	 * to find the closest 400-year boundary at or before ordinal, then
	 * work with the offset from that boundary to ordinal.  Life is much
	 * clearer if we subtract 1 from ordinal first -- then the values
	 * of ordinal at 400-year boundaries are exactly those divisible
	 * by DI400Y:
	 *
	 *    D  M   Y            n              n-1
	 *    -- --- ----        ----------     ----------------
	 *    31 Dec -400        -DI400Y       -DI400Y -1
	 *     1 Jan -399         -DI400Y +1   -DI400Y      400-year boundary
	 *    ...
	 *    30 Dec  000        -1             -2
	 *    31 Dec  000         0             -1
	 *     1 Jan  001         1              0          400-year boundary
	 *     2 Jan  001         2              1
	 *     3 Jan  001         3              2
	 *    ...
	 *    31 Dec  400         DI400Y        DI400Y -1
	 *     1 Jan  401         DI400Y +1     DI400Y      400-year boundary
	 */
	assert(ordinal >= 1);
	--ordinal;
	n400 = ordinal / DI400Y;
	n = ordinal % DI400Y;
	*year = n400 * 400 + 1;

	/* Now n is the (non-negative) offset, in days, from January 1 of
	 * year, to the desired date.  Now compute how many 100-year cycles
	 * precede n.
	 * Note that it's possible for n100 to equal 4!  In that case 4 full
	 * 100-year cycles precede the desired day, which implies the
	 * desired day is December 31 at the end of a 400-year cycle.
	 */
	n100 = n / DI100Y;
	n = n % DI100Y;

	/* Now compute how many 4-year cycles precede it. */
	n4 = n / DI4Y;
	n = n % DI4Y;

	/* And now how many single years.  Again n1 can be 4, and again
	 * meaning that the desired day is December 31 at the end of the
	 * 4-year cycle.
	 */
	n1 = n / 365;
	n = n % 365;

	*year += n100 * 100 + n4 * 4 + n1;
	if (n1 == 4 || n100 == 4) {
		assert(n == 0);
		*year -= 1;
		*month = 12;
		*day = 31;
		return;
	}

	/* Now the year is correct, and n is the offset from January 1.  We
	 * find the month via an estimate that's either exact or one too
	 * large.
	 */
	leapyear = n1 == 3 && (n4 != 24 || n100 == 3);
	assert(leapyear == is_leap(*year));
	*month = (n + 50) >> 5;
	preceding = (_days_before_month[*month] + (*month > 2 && leapyear));
	if (preceding > n) {
		/* estimate is too large */
		*month -= 1;
		preceding -= days_in_month(*year, *month);
	}
	n -= preceding;
	assert(0 <= n);
	assert(n < days_in_month(*year, *month));

	*day = n + 1;
}

/* year, month, day -> ordinal, considering 01-Jan-0001 as day 1. */
static int
ymd_to_ord(int year, int month, int day)
{
	return days_before_year(year) + days_before_month(year, month) + day;
}

/* Day of week, where Monday==0, ..., Sunday==6.  1/1/1 was a Monday. */
static int
weekday(int year, int month, int day)
{
	return (ymd_to_ord(year, month, day) + 6) % 7;
}

/* Ordinal of the Monday starting week 1 of the ISO year.  Week 1 is the
 * first calendar week containing a Thursday.
 */
static int
iso_week1_monday(int year)
{
	int first_day = ymd_to_ord(year, 1, 1);	/* ord of 1/1 */
	/* 0 if 1/1 is a Monday, 1 if a Tue, etc. */
	int first_weekday = (first_day + 6) % 7;
	/* ordinal of closest Monday at or before 1/1 */
	int week1_monday  = first_day - first_weekday;

	if (first_weekday > 3)	/* if 1/1 was Fri, Sat, Sun */
		week1_monday += 7;
	return week1_monday;
}

/* ---------------------------------------------------------------------------
 * Range checkers.
 */

/* Check that -MAX_DELTA_DAYS <= days <= MAX_DELTA_DAYS.  If so, return 0.
 * If not, raise OverflowError and return -1.
 */
static int
check_delta_day_range(int days)
{
	if (-MAX_DELTA_DAYS <= days && days <= MAX_DELTA_DAYS)
		return 0;
	PyErr_Format(PyExc_OverflowError,
		     "days=%d; must have magnitude <= %d",
		     days, MAX_DELTA_DAYS);
	return -1;
}

/* Check that date arguments are in range.  Return 0 if they are.  If they
 * aren't, raise ValueError and return -1.
 */
static int
check_date_args(int year, int month, int day)
{

	if (year < MINYEAR || year > MAXYEAR) {
		PyErr_SetString(PyExc_ValueError,
				"year is out of range");
		return -1;
	}
	if (month < 1 || month > 12) {
		PyErr_SetString(PyExc_ValueError,
				"month must be in 1..12");
		return -1;
	}
	if (day < 1 || day > days_in_month(year, month)) {
		PyErr_SetString(PyExc_ValueError,
				"day is out of range for month");
		return -1;
	}
	return 0;
}

/* Check that time arguments are in range.  Return 0 if they are.  If they
 * aren't, raise ValueError and return -1.
 */
static int
check_time_args(int h, int m, int s, int us)
{
	if (h < 0 || h > 23) {
		PyErr_SetString(PyExc_ValueError,
				"hour must be in 0..23");
		return -1;
	}
	if (m < 0 || m > 59) {
		PyErr_SetString(PyExc_ValueError,
				"minute must be in 0..59");
		return -1;
	}
	if (s < 0 || s > 59) {
		PyErr_SetString(PyExc_ValueError,
				"second must be in 0..59");
		return -1;
	}
	if (us < 0 || us > 999999) {
		PyErr_SetString(PyExc_ValueError,
				"microsecond must be in 0..999999");
		return -1;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * Normalization utilities.
 */

/* One step of a mixed-radix conversion.  A "hi" unit is equivalent to
 * factor "lo" units.  factor must be > 0.  If *lo is less than 0, or
 * at least factor, enough of *lo is converted into "hi" units so that
 * 0 <= *lo < factor.  The input values must be such that int overflow
 * is impossible.
 */
static void
normalize_pair(int *hi, int *lo, int factor)
{
	assert(factor > 0);
	assert(lo != hi);
	if (*lo < 0 || *lo >= factor) {
		const int num_hi = divmod(*lo, factor, lo);
		const int new_hi = *hi + num_hi;
		assert(! SIGNED_ADD_OVERFLOWED(new_hi, *hi, num_hi));
		*hi = new_hi;
	}
	assert(0 <= *lo && *lo < factor);
}

/* Fiddle days (d), seconds (s), and microseconds (us) so that
 * 	0 <= *s < 24*3600
 * 	0 <= *us < 1000000
 * The input values must be such that the internals don't overflow.
 * The way this routine is used, we don't get close.
 */
static void
normalize_d_s_us(int *d, int *s, int *us)
{
	if (*us < 0 || *us >= 1000000) {
		normalize_pair(s, us, 1000000);
		/* |s| can't be bigger than about
		 * |original s| + |original us|/1000000 now.
		 */

	}
	if (*s < 0 || *s >= 24*3600) {
		normalize_pair(d, s, 24*3600);
		/* |d| can't be bigger than about
		 * |original d| +
		 * (|original s| + |original us|/1000000) / (24*3600) now.
		 */
	}
	assert(0 <= *s && *s < 24*3600);
	assert(0 <= *us && *us < 1000000);
}

/* Fiddle years (y), months (m), and days (d) so that
 * 	1 <= *m <= 12
 * 	1 <= *d <= days_in_month(*y, *m)
 * The input values must be such that the internals don't overflow.
 * The way this routine is used, we don't get close.
 */
static void
normalize_y_m_d(int *y, int *m, int *d)
{
	int dim;	/* # of days in month */

	/* This gets muddy:  the proper range for day can't be determined
	 * without knowing the correct month and year, but if day is, e.g.,
	 * plus or minus a million, the current month and year values make
	 * no sense (and may also be out of bounds themselves).
	 * Saying 12 months == 1 year should be non-controversial.
	 */
	if (*m < 1 || *m > 12) {
		--*m;
		normalize_pair(y, m, 12);
		++*m;
		/* |y| can't be bigger than about
		 * |original y| + |original m|/12 now.
		 */
	}
	assert(1 <= *m && *m <= 12);

	/* Now only day can be out of bounds (year may also be out of bounds
	 * for a datetime object, but we don't care about that here).
	 * If day is out of bounds, what to do is arguable, but at least the
	 * method here is principled and explainable.
	 */
	dim = days_in_month(*y, *m);
	if (*d < 1 || *d > dim) {
		/* Move day-1 days from the first of the month.  First try to
		 * get off cheap if we're only one day out of range
		 * (adjustments for timezone alone can't be worse than that).
		 */
		if (*d == 0) {
			--*m;
			if (*m > 0)
				*d = days_in_month(*y, *m);
			else {
				--*y;
				*m = 12;
				*d = 31;
			}
		}
		else if (*d == dim + 1) {
			/* move forward a day */
			++*m;
			*d = 1;
			if (*m > 12) {
				*m = 1;
				++*y;
			}
		}
		else {
			int ordinal = ymd_to_ord(*y, *m, 1) +
						  *d - 1;
			ord_to_ymd(ordinal, y, m, d);
		}
	}
	assert(*m > 0);
	assert(*d > 0);
}

/* Fiddle out-of-bounds months and days so that the result makes some kind
 * of sense.  The parameters are both inputs and outputs.  Returns < 0 on
 * failure, where failure means the adjusted year is out of bounds.
 */
static int
normalize_date(int *year, int *month, int *day)
{
	int result;

	normalize_y_m_d(year, month, day);
	if (MINYEAR <= *year && *year <= MAXYEAR)
		result = 0;
	else {
		PyErr_SetString(PyExc_OverflowError,
				"date value out of range");
		result = -1;
	}
	return result;
}

/* Force all the datetime fields into range.  The parameters are both
 * inputs and outputs.  Returns < 0 on error.
 */
static int
normalize_datetime(int *year, int *month, int *day,
                   int *hour, int *minute, int *second,
                   int *microsecond)
{
	normalize_pair(second, microsecond, 1000000);
	normalize_pair(minute, second, 60);
	normalize_pair(hour, minute, 60);
	normalize_pair(day, hour, 24);
	return normalize_date(year, month, day);
}

/* ---------------------------------------------------------------------------
 * tzinfo helpers.
 */

/* Ensure that p is None or of a tzinfo subclass.  Return 0 if OK; if not
 * raise TypeError and return -1.
 */
static int
check_tzinfo_subclass(PyObject *p)
{
	if (p == Py_None || PyTZInfo_Check(p))
		return 0;
	PyErr_Format(PyExc_TypeError,
		     "tzinfo argument must be None or of a tzinfo subclass, "
		     "not type '%s'",
		     p->ob_type->tp_name);
	return -1;
}

/* Return tzinfo.methname(tzinfoarg), without any checking of results.
 * If tzinfo is None, returns None.
 */
static PyObject *
call_tzinfo_method(PyObject *tzinfo, char *methname, PyObject *tzinfoarg)
{
	PyObject *result;

	assert(tzinfo && methname && tzinfoarg);
	assert(check_tzinfo_subclass(tzinfo) >= 0);
	if (tzinfo == Py_None) {
		result = Py_None;
		Py_INCREF(result);
	}
	else
		result = PyObject_CallMethod(tzinfo, methname, "O", tzinfoarg);
	return result;
}

/* If self has a tzinfo member, return a BORROWED reference to it.  Else
 * return NULL, which is NOT AN ERROR.  There are no error returns here,
 * and the caller must not decref the result.
 */
static PyObject *
get_tzinfo_member(PyObject *self)
{
	PyObject *tzinfo = NULL;

	if (PyDateTimeTZ_Check(self))
		tzinfo = ((PyDateTime_DateTimeTZ *)self)->tzinfo;
	else if (PyTimeTZ_Check(self))
		tzinfo = ((PyDateTime_TimeTZ *)self)->tzinfo;

	return tzinfo;
}

/* self is a datetimetz.  Replace its tzinfo member. */
void
replace_tzinfo(PyObject *self, PyObject *newtzinfo)
{
	assert(self != NULL);
	assert(PyDateTimeTZ_Check(self));
	assert(check_tzinfo_subclass(newtzinfo) >= 0);
	Py_INCREF(newtzinfo);
	Py_DECREF(((PyDateTime_DateTimeTZ *)self)->tzinfo);
	((PyDateTime_DateTimeTZ *)self)->tzinfo = newtzinfo;
}


/* Call getattr(tzinfo, name)(tzinfoarg), and extract an int from the
 * result.  tzinfo must be an instance of the tzinfo class.  If the method
 * returns None, this returns 0 and sets *none to 1.  If the method doesn't
 * return None or timedelta, TypeError is raised and this returns -1.  If it
 * returnsa timedelta and the value is out of range or isn't a whole number
 * of minutes, ValueError is raised and this returns -1.
 * Else *none is set to 0 and the integer method result is returned.
 */
static int
call_utc_tzinfo_method(PyObject *tzinfo, char *name, PyObject *tzinfoarg,
		       int *none)
{
	PyObject *u;
	int result = -1;

	assert(tzinfo != NULL);
	assert(PyTZInfo_Check(tzinfo));
	assert(tzinfoarg != NULL);

	*none = 0;
	u = call_tzinfo_method(tzinfo, name, tzinfoarg);
	if (u == NULL)
		return -1;

	else if (u == Py_None) {
		result = 0;
		*none = 1;
	}
	else if (PyDelta_Check(u)) {
		const int days = GET_TD_DAYS(u);
		if (days < -1 || days > 0)
			result = 24*60;	/* trigger ValueError below */
		else {
			/* next line can't overflow because we know days
			 * is -1 or 0 now
			 */
			int ss = days * 24 * 3600 + GET_TD_SECONDS(u);
			result = divmod(ss, 60, &ss);
			if (ss || GET_TD_MICROSECONDS(u)) {
				PyErr_Format(PyExc_ValueError,
					     "tzinfo.%s() must return a "
					     "whole number of minutes",
					     name);
				result = -1;
			}
		}
	}
	else {
		PyErr_Format(PyExc_TypeError,
			     "tzinfo.%s() must return None or "
			     "timedelta, not '%s'",
			     name, u->ob_type->tp_name);
	}

	Py_DECREF(u);
	if (result < -1439 || result > 1439) {
		PyErr_Format(PyExc_ValueError,
			     "tzinfo.%s() returned %d; must be in "
			     "-1439 .. 1439",
			     name, result);
		result = -1;
	}
	return result;
}

/* Call tzinfo.utcoffset(tzinfoarg), and extract an integer from the
 * result.  tzinfo must be an instance of the tzinfo class.  If utcoffset()
 * returns None, call_utcoffset returns 0 and sets *none to 1.  If uctoffset()
 * doesn't return None or timedelta, TypeError is raised and this returns -1.
 * If utcoffset() returns an invalid timedelta (out of range, or not a whole
 * # of minutes), ValueError is raised and this returns -1.  Else *none is
 * set to 0 and the offset is returned (as int # of minutes east of UTC).
 */
static int
call_utcoffset(PyObject *tzinfo, PyObject *tzinfoarg, int *none)
{
	return call_utc_tzinfo_method(tzinfo, "utcoffset", tzinfoarg, none);
}

static PyObject *new_delta(int d, int sec, int usec, int normalize);

/* Call tzinfo.name(tzinfoarg), and return the offset as a timedelta or None.
 */
static PyObject *
offset_as_timedelta(PyObject *tzinfo, char *name, PyObject *tzinfoarg) {
	PyObject *result;

	assert(tzinfo && name && tzinfoarg);
	if (tzinfo == Py_None) {
		result = Py_None;
		Py_INCREF(result);
	}
	else {
		int none;
		int offset = call_utc_tzinfo_method(tzinfo, name, tzinfoarg,
						    &none);
		if (offset < 0 && PyErr_Occurred())
			return NULL;
		if (none) {
			result = Py_None;
			Py_INCREF(result);
		}
		else
			result = new_delta(0, offset * 60, 0, 1);
	}
	return result;
}

/* Call tzinfo.dst(tzinfoarg), and extract an integer from the
 * result.  tzinfo must be an instance of the tzinfo class.  If dst()
 * returns None, call_dst returns 0 and sets *none to 1.  If dst()
 & doesn't return None or timedelta, TypeError is raised and this
 * returns -1.  If dst() returns an invalid timedelta for for a UTC offset,
 * ValueError is raised and this returns -1.  Else *none is set to 0 and
 * the offset is returned (as an int # of minutes east of UTC).
 */
static int
call_dst(PyObject *tzinfo, PyObject *tzinfoarg, int *none)
{
	return call_utc_tzinfo_method(tzinfo, "dst", tzinfoarg, none);
}

/* Call tzinfo.tzname(tzinfoarg), and return the result.  tzinfo must be
 * an instance of the tzinfo class or None.  If tzinfo isn't None, and
 * tzname() doesn't return None or a string, TypeError is raised and this
 * returns NULL.
 */
static PyObject *
call_tzname(PyObject *tzinfo, PyObject *tzinfoarg)
{
	PyObject *result;

	assert(tzinfo != NULL);
	assert(check_tzinfo_subclass(tzinfo) >= 0);
	assert(tzinfoarg != NULL);

	if (tzinfo == Py_None) {
		result = Py_None;
		Py_INCREF(result);
	}
	else
		result = PyObject_CallMethod(tzinfo, "tzname", "O", tzinfoarg);

	if (result != NULL && result != Py_None && ! PyString_Check(result)) {
		PyErr_Format(PyExc_TypeError, "tzinfo.tzname() must "
			     "return None or a string, not '%s'",
			     result->ob_type->tp_name);
		Py_DECREF(result);
		result = NULL;
	}
	return result;
}

typedef enum {
	      /* an exception has been set; the caller should pass it on */
	      OFFSET_ERROR,

	      /* type isn't date, datetime, datetimetz subclass, time, or
	       * timetz subclass
	       */
	      OFFSET_UNKNOWN,

	      /* date,
	       * datetime,
	       * datetimetz with None tzinfo,
	       * datetimetz where utcoffset() returns None
	       * time,
	       * timetz with None tzinfo,
	       * timetz where utcoffset() returns None
	       */
	      OFFSET_NAIVE,

	      /* timetz where utcoffset() doesn't return None,
	       * datetimetz where utcoffset() doesn't return None
	       */
	      OFFSET_AWARE,
} naivety;

/* Classify an object as to whether it's naive or offset-aware.  See
 * the "naivety" typedef for details.  If the type is aware, *offset is set
 * to minutes east of UTC (as returned by the tzinfo.utcoffset() method).
 * If the type is offset-naive (or unknown, or error), *offset is set to 0.
 * tzinfoarg is the argument to pass to the tzinfo.utcoffset() method.
 */
static naivety
classify_utcoffset(PyObject *op, PyObject *tzinfoarg, int *offset)
{
	int none;
	PyObject *tzinfo;

	assert(tzinfoarg != NULL);
	*offset = 0;
	tzinfo = get_tzinfo_member(op);	/* NULL means no tzinfo, not error */
	if (tzinfo == Py_None)
		return OFFSET_NAIVE;
	if (tzinfo == NULL) {
		/* note that a datetime passes the PyDate_Check test */
		return (PyTime_Check(op) || PyDate_Check(op)) ?
		       OFFSET_NAIVE : OFFSET_UNKNOWN;
	}
	*offset = call_utcoffset(tzinfo, tzinfoarg, &none);
	if (*offset == -1 && PyErr_Occurred())
		return OFFSET_ERROR;
	return none ? OFFSET_NAIVE : OFFSET_AWARE;
}

/* Classify two objects as to whether they're naive or offset-aware.
 * This isn't quite the same as calling classify_utcoffset() twice:  for
 * binary operations (comparison and subtraction), we generally want to
 * ignore the tzinfo members if they're identical.  This is by design,
 * so that results match "naive" expectations when mixing objects from a
 * single timezone.  So in that case, this sets both offsets to 0 and
 * both naiveties to OFFSET_NAIVE.
 * The function returns 0 if everything's OK, and -1 on error.
 */
static int
classify_two_utcoffsets(PyObject *o1, int *offset1, naivety *n1,
			PyObject *tzinfoarg1,
			PyObject *o2, int *offset2, naivety *n2,
			PyObject *tzinfoarg2)
{
	if (get_tzinfo_member(o1) == get_tzinfo_member(o2)) {
		*offset1 = *offset2 = 0;
		*n1 = *n2 = OFFSET_NAIVE;
	}
	else {
		*n1 = classify_utcoffset(o1, tzinfoarg1, offset1);
		if (*n1 == OFFSET_ERROR)
			return -1;
		*n2 = classify_utcoffset(o2, tzinfoarg2, offset2);
		if (*n2 == OFFSET_ERROR)
			return -1;
	}
	return 0;
}

/* repr is like "someclass(arg1, arg2)".  If tzinfo isn't None,
 * stuff
 *     ", tzinfo=" + repr(tzinfo)
 * before the closing ")".
 */
static PyObject *
append_keyword_tzinfo(PyObject *repr, PyObject *tzinfo)
{
	PyObject *temp;

	assert(PyString_Check(repr));
	assert(tzinfo);
	if (tzinfo == Py_None)
		return repr;
	/* Get rid of the trailing ')'. */
	assert(PyString_AsString(repr)[PyString_Size(repr)-1] == ')');
	temp = PyString_FromStringAndSize(PyString_AsString(repr),
					  PyString_Size(repr) - 1);
	Py_DECREF(repr);
	if (temp == NULL)
		return NULL;
	repr = temp;

	/* Append ", tzinfo=". */
	PyString_ConcatAndDel(&repr, PyString_FromString(", tzinfo="));

	/* Append repr(tzinfo). */
	PyString_ConcatAndDel(&repr, PyObject_Repr(tzinfo));

	/* Add a closing paren. */
	PyString_ConcatAndDel(&repr, PyString_FromString(")"));
	return repr;
}

/* ---------------------------------------------------------------------------
 * String format helpers.
 */

static PyObject *
format_ctime(PyDateTime_Date *date,
             int hours, int minutes, int seconds)
{
	static char *DayNames[] = {
		"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
	};
	static char *MonthNames[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	char buffer[128];
	int wday = weekday(GET_YEAR(date), GET_MONTH(date), GET_DAY(date));

	PyOS_snprintf(buffer, sizeof(buffer), "%s %s %2d %02d:%02d:%02d %04d",
		      DayNames[wday], MonthNames[GET_MONTH(date) - 1],
		      GET_DAY(date), hours, minutes, seconds,
		      GET_YEAR(date));
	return PyString_FromString(buffer);
}

/* Add an hours & minutes UTC offset string to buf.  buf has no more than
 * buflen bytes remaining.  The UTC offset is gotten by calling
 * tzinfo.uctoffset(tzinfoarg).  If that returns None, \0 is stored into
 * *buf, and that's all.  Else the returned value is checked for sanity (an
 * integer in range), and if that's OK it's converted to an hours & minutes
 * string of the form
 *   sign HH sep MM
 * Returns 0 if everything is OK.  If the return value from utcoffset() is
 * bogus, an appropriate exception is set and -1 is returned.
 */
static int
format_utcoffset(char *buf, size_t buflen, const char *sep,
		PyObject *tzinfo, PyObject *tzinfoarg)
{
	int offset;
	int hours;
	int minutes;
	char sign;
	int none;

	offset = call_utcoffset(tzinfo, tzinfoarg, &none);
	if (offset == -1 && PyErr_Occurred())
		return -1;
	if (none) {
		*buf = '\0';
		return 0;
	}
	sign = '+';
	if (offset < 0) {
		sign = '-';
		offset = - offset;
	}
	hours = divmod(offset, 60, &minutes);
	PyOS_snprintf(buf, buflen, "%c%02d%s%02d", sign, hours, sep, minutes);
	return 0;
}

/* I sure don't want to reproduce the strftime code from the time module,
 * so this imports the module and calls it.  All the hair is due to
 * giving special meanings to the %z and %Z format codes via a preprocessing
 * step on the format string.
 * tzinfoarg is the argument to pass to the object's tzinfo method, if
 * needed.
 */
static PyObject *
wrap_strftime(PyObject *object, PyObject *format, PyObject *timetuple,
	      PyObject *tzinfoarg)
{
	PyObject *result = NULL;	/* guilty until proved innocent */

	PyObject *zreplacement = NULL;	/* py string, replacement for %z */
	PyObject *Zreplacement = NULL;	/* py string, replacement for %Z */

	char *pin;	/* pointer to next char in input format */
	char ch;	/* next char in input format */

	PyObject *newfmt = NULL;	/* py string, the output format */
	char *pnew;	/* pointer to available byte in output format */
	char totalnew;	/* number bytes total in output format buffer,
			   exclusive of trailing \0 */
	char usednew;	/* number bytes used so far in output format buffer */

	char *ptoappend; /* pointer to string to append to output buffer */
	int ntoappend;	/* # of bytes to append to output buffer */

	assert(object && format && timetuple);
	assert(PyString_Check(format));

	/* Give up if the year is before 1900.
	 * Python strftime() plays games with the year, and different
	 * games depending on whether envar PYTHON2K is set.  This makes
	 * years before 1900 a nightmare, even if the platform strftime
	 * supports them (and not all do).
	 * We could get a lot farther here by avoiding Python's strftime
	 * wrapper and calling the C strftime() directly, but that isn't
	 * an option in the Python implementation of this module.
	 */
	{
		long year;
		PyObject *pyyear = PySequence_GetItem(timetuple, 0);
		if (pyyear == NULL) return NULL;
		assert(PyInt_Check(pyyear));
		year = PyInt_AsLong(pyyear);
		Py_DECREF(pyyear);
		if (year < 1900) {
			PyErr_Format(PyExc_ValueError, "year=%ld is before "
				     "1900; the datetime strftime() "
	                             "methods require year >= 1900",
	                             year);
	                return NULL;
		}
	}

	/* Scan the input format, looking for %z and %Z escapes, building
	 * a new format.  Since computing the replacements for those codes
	 * is expensive, don't unless they're actually used.
	 */
	totalnew = PyString_Size(format);	/* realistic if no %z/%Z */
	newfmt = PyString_FromStringAndSize(NULL, totalnew);
	if (newfmt == NULL) goto Done;
	pnew = PyString_AsString(newfmt);
	usednew = 0;

	pin = PyString_AsString(format);
	while ((ch = *pin++) != '\0') {
		if (ch != '%') {
			ptoappend = pin - 1;
			ntoappend = 1;
		}
		else if ((ch = *pin++) == '\0') {
			/* There's a lone trailing %; doesn't make sense. */
			PyErr_SetString(PyExc_ValueError, "strftime format "
					"ends with raw %");
			goto Done;
		}
		/* A % has been seen and ch is the character after it. */
		else if (ch == 'z') {
			if (zreplacement == NULL) {
				/* format utcoffset */
				char buf[100];
				PyObject *tzinfo = get_tzinfo_member(object);
				zreplacement = PyString_FromString("");
				if (zreplacement == NULL) goto Done;
				if (tzinfo != Py_None && tzinfo != NULL) {
					assert(tzinfoarg != NULL);
					if (format_utcoffset(buf,
							     sizeof(buf),
							     "",
							     tzinfo,
							     tzinfoarg) < 0)
						goto Done;
					Py_DECREF(zreplacement);
					zreplacement = PyString_FromString(buf);
					if (zreplacement == NULL) goto Done;
				}
			}
			assert(zreplacement != NULL);
			ptoappend = PyString_AsString(zreplacement);
			ntoappend = PyString_Size(zreplacement);
		}
		else if (ch == 'Z') {
			/* format tzname */
			if (Zreplacement == NULL) {
				PyObject *tzinfo = get_tzinfo_member(object);
				Zreplacement = PyString_FromString("");
				if (Zreplacement == NULL) goto Done;
				if (tzinfo != Py_None && tzinfo != NULL) {
					PyObject *temp;
					assert(tzinfoarg != NULL);
					temp = call_tzname(tzinfo, tzinfoarg);
					if (temp == NULL) goto Done;
					if (temp != Py_None) {
						assert(PyString_Check(temp));
						/* Since the tzname is getting
						 * stuffed into the format, we
						 * have to double any % signs
						 * so that strftime doesn't
						 * treat them as format codes.
						 */
						Py_DECREF(Zreplacement);
						Zreplacement = PyObject_CallMethod(
							temp, "replace",
							"ss", "%", "%%");
						Py_DECREF(temp);
						if (Zreplacement == NULL)
							goto Done;
					}
					else
						Py_DECREF(temp);
				}
			}
			assert(Zreplacement != NULL);
			ptoappend = PyString_AsString(Zreplacement);
			ntoappend = PyString_Size(Zreplacement);
		}
		else {
			/* percent followed by neither z nor Z */
			ptoappend = pin - 2;
			ntoappend = 2;
		}

 		/* Append the ntoappend chars starting at ptoappend to
 		 * the new format.
 		 */
 		assert(ntoappend >= 0);
 		if (ntoappend == 0)
 			continue;
 		while (usednew + ntoappend > totalnew) {
 			int bigger = totalnew << 1;
 			if ((bigger >> 1) != totalnew) { /* overflow */
 				PyErr_NoMemory();
 				goto Done;
 			}
 			if (_PyString_Resize(&newfmt, bigger) < 0)
 				goto Done;
 			totalnew = bigger;
 			pnew = PyString_AsString(newfmt) + usednew;
 		}
		memcpy(pnew, ptoappend, ntoappend);
		pnew += ntoappend;
		usednew += ntoappend;
		assert(usednew <= totalnew);
	}  /* end while() */

	if (_PyString_Resize(&newfmt, usednew) < 0)
		goto Done;
	{
		PyObject *time = PyImport_ImportModule("time");
		if (time == NULL)
			goto Done;
		result = PyObject_CallMethod(time, "strftime", "OO",
					     newfmt, timetuple);
		Py_DECREF(time);
    	}
 Done:
	Py_XDECREF(zreplacement);
	Py_XDECREF(Zreplacement);
	Py_XDECREF(newfmt);
    	return result;
}

static char *
isoformat_date(PyDateTime_Date *dt, char buffer[], int bufflen)
{
	int x;
	x = PyOS_snprintf(buffer, bufflen,
			  "%04d-%02d-%02d",
			  GET_YEAR(dt), GET_MONTH(dt), GET_DAY(dt));
	return buffer + x;
}

static void
isoformat_time(PyDateTime_DateTime *dt, char buffer[], int bufflen)
{
	int us = DATE_GET_MICROSECOND(dt);

	PyOS_snprintf(buffer, bufflen,
		      "%02d:%02d:%02d",	/* 8 characters */
		      DATE_GET_HOUR(dt),
		      DATE_GET_MINUTE(dt),
		      DATE_GET_SECOND(dt));
	if (us)
		PyOS_snprintf(buffer + 8, bufflen - 8, ".%06d", us);
}

/* ---------------------------------------------------------------------------
 * Wrap functions from the time module.  These aren't directly available
 * from C.  Perhaps they should be.
 */

/* Call time.time() and return its result (a Python float). */
static PyObject *
time_time(void)
{
	PyObject *result = NULL;
	PyObject *time = PyImport_ImportModule("time");

	if (time != NULL) {
		result = PyObject_CallMethod(time, "time", "()");
		Py_DECREF(time);
	}
	return result;
}

/* Build a time.struct_time.  The weekday and day number are automatically
 * computed from the y,m,d args.
 */
static PyObject *
build_struct_time(int y, int m, int d, int hh, int mm, int ss, int dstflag)
{
	PyObject *time;
	PyObject *result = NULL;

	time = PyImport_ImportModule("time");
	if (time != NULL) {
		result = PyObject_CallMethod(time, "struct_time",
					     "((iiiiiiiii))",
					     y, m, d,
					     hh, mm, ss,
				 	     weekday(y, m, d),
				 	     days_before_month(y, m) + d,
				 	     dstflag);
		Py_DECREF(time);
	}
	return result;
}

/* ---------------------------------------------------------------------------
 * Miscellaneous helpers.
 */

/* For obscure reasons, we need to use tp_richcompare instead of tp_compare.
 * The comparisons here all most naturally compute a cmp()-like result.
 * This little helper turns that into a bool result for rich comparisons.
 */
static PyObject *
diff_to_bool(int diff, int op)
{
	PyObject *result;
	int istrue;

	switch (op) {
		case Py_EQ: istrue = diff == 0; break;
		case Py_NE: istrue = diff != 0; break;
		case Py_LE: istrue = diff <= 0; break;
		case Py_GE: istrue = diff >= 0; break;
		case Py_LT: istrue = diff < 0; break;
		case Py_GT: istrue = diff > 0; break;
		default:
			assert(! "op unknown");
			istrue = 0; /* To shut up compiler */
	}
	result = istrue ? Py_True : Py_False;
	Py_INCREF(result);
	return result;
}

/* ---------------------------------------------------------------------------
 * Helpers for setting object fields.  These work on pointers to the
 * appropriate base class.
 */

/* For date, datetime and datetimetz. */
static void
set_date_fields(PyDateTime_Date *self, int y, int m, int d)
{
	self->hashcode = -1;
	SET_YEAR(self, y);
	SET_MONTH(self, m);
	SET_DAY(self, d);
}

/* For datetime and datetimetz. */
static void
set_datetime_time_fields(PyDateTime_Date *self, int h, int m, int s, int us)
{
	DATE_SET_HOUR(self, h);
	DATE_SET_MINUTE(self, m);
	DATE_SET_SECOND(self, s);
	DATE_SET_MICROSECOND(self, us);
}

/* For time and timetz. */
static void
set_time_fields(PyDateTime_Time *self, int h, int m, int s, int us)
{
	self->hashcode = -1;
	TIME_SET_HOUR(self, h);
	TIME_SET_MINUTE(self, m);
	TIME_SET_SECOND(self, s);
	TIME_SET_MICROSECOND(self, us);
}

/* ---------------------------------------------------------------------------
 * Create various objects, mostly without range checking.
 */

/* Create a date instance with no range checking. */
static PyObject *
new_date(int year, int month, int day)
{
	PyDateTime_Date *self;

	self = PyObject_New(PyDateTime_Date, &PyDateTime_DateType);
	if (self != NULL)
		set_date_fields(self, year, month, day);
	return (PyObject *) self;
}

/* Create a datetime instance with no range checking. */
static PyObject *
new_datetime(int year, int month, int day, int hour, int minute,
             int second, int usecond)
{
	PyDateTime_DateTime *self;

	self = PyObject_New(PyDateTime_DateTime, &PyDateTime_DateTimeType);
	if (self != NULL) {
		set_date_fields((PyDateTime_Date *)self, year, month, day);
		set_datetime_time_fields((PyDateTime_Date *)self,
					 hour, minute, second, usecond);
	}
	return (PyObject *) self;
}

/* Create a datetimetz instance with no range checking. */
static PyObject *
new_datetimetz(int year, int month, int day, int hour, int minute,
	       int second, int usecond, PyObject *tzinfo)
{
	PyDateTime_DateTimeTZ *self;

	self = PyObject_New(PyDateTime_DateTimeTZ, &PyDateTime_DateTimeTZType);
	if (self != NULL) {
		set_date_fields((PyDateTime_Date *)self, year, month, day);
		set_datetime_time_fields((PyDateTime_Date *)self,
					 hour, minute, second, usecond);
		Py_INCREF(tzinfo);
		self->tzinfo = tzinfo;
	}
	return (PyObject *) self;
}

/* Create a time instance with no range checking. */
static PyObject *
new_time(int hour, int minute, int second, int usecond)
{
	PyDateTime_Time *self;

	self = PyObject_New(PyDateTime_Time, &PyDateTime_TimeType);
	if (self != NULL)
		set_time_fields(self, hour, minute, second, usecond);
	return (PyObject *) self;
}

/* Create a timetz instance with no range checking. */
static PyObject *
new_timetz(int hour, int minute, int second, int usecond, PyObject *tzinfo)
{
	PyDateTime_TimeTZ *self;

	self = PyObject_New(PyDateTime_TimeTZ, &PyDateTime_TimeTZType);
	if (self != NULL) {
		set_time_fields((PyDateTime_Time *)self,
				hour, minute, second, usecond);
		Py_INCREF(tzinfo);
		self->tzinfo = tzinfo;
	}
	return (PyObject *) self;
}

/* Create a timedelta instance.  Normalize the members iff normalize is
 * true.  Passing false is a speed optimization, if you know for sure
 * that seconds and microseconds are already in their proper ranges.  In any
 * case, raises OverflowError and returns NULL if the normalized days is out
 * of range).
 */
static PyObject *
new_delta(int days, int seconds, int microseconds, int normalize)
{
	PyDateTime_Delta *self;

	if (normalize)
		normalize_d_s_us(&days, &seconds, &microseconds);
	assert(0 <= seconds && seconds < 24*3600);
	assert(0 <= microseconds && microseconds < 1000000);

 	if (check_delta_day_range(days) < 0)
 		return NULL;

	self = PyObject_New(PyDateTime_Delta, &PyDateTime_DeltaType);
	if (self != NULL) {
		self->hashcode = -1;
		SET_TD_DAYS(self, days);
		SET_TD_SECONDS(self, seconds);
		SET_TD_MICROSECONDS(self, microseconds);
	}
	return (PyObject *) self;
}


/* ---------------------------------------------------------------------------
 * Cached Python objects; these are set by the module init function.
 */

/* Conversion factors. */
static PyObject *us_per_us = NULL;	/* 1 */
static PyObject *us_per_ms = NULL;	/* 1000 */
static PyObject *us_per_second = NULL;	/* 1000000 */
static PyObject *us_per_minute = NULL;	/* 1e6 * 60 as Python int */
static PyObject *us_per_hour = NULL;	/* 1e6 * 3600 as Python long */
static PyObject *us_per_day = NULL;	/* 1e6 * 3600 * 24 as Python long */
static PyObject *us_per_week = NULL;	/* 1e6*3600*24*7 as Python long */
static PyObject *seconds_per_day = NULL; /* 3600*24 as Python int */

/* Callables to support unpickling. */
static PyObject *date_unpickler_object = NULL;
static PyObject *datetime_unpickler_object = NULL;
static PyObject *datetimetz_unpickler_object = NULL;
static PyObject *tzinfo_unpickler_object = NULL;
static PyObject *time_unpickler_object = NULL;
static PyObject *timetz_unpickler_object = NULL;

/* ---------------------------------------------------------------------------
 * Class implementations.
 */

/*
 * PyDateTime_Delta implementation.
 */

/* Convert a timedelta to a number of us,
 * 	(24*3600*self.days + self.seconds)*1000000 + self.microseconds
 * as a Python int or long.
 * Doing mixed-radix arithmetic by hand instead is excruciating in C,
 * due to ubiquitous overflow possibilities.
 */
static PyObject *
delta_to_microseconds(PyDateTime_Delta *self)
{
	PyObject *x1 = NULL;
	PyObject *x2 = NULL;
	PyObject *x3 = NULL;
	PyObject *result = NULL;

	x1 = PyInt_FromLong(GET_TD_DAYS(self));
	if (x1 == NULL)
		goto Done;
	x2 = PyNumber_Multiply(x1, seconds_per_day);	/* days in seconds */
	if (x2 == NULL)
		goto Done;
	Py_DECREF(x1);
	x1 = NULL;

	/* x2 has days in seconds */
	x1 = PyInt_FromLong(GET_TD_SECONDS(self));	/* seconds */
	if (x1 == NULL)
		goto Done;
	x3 = PyNumber_Add(x1, x2);	/* days and seconds in seconds */
	if (x3 == NULL)
		goto Done;
	Py_DECREF(x1);
	Py_DECREF(x2);
	x1 = x2 = NULL;

	/* x3 has days+seconds in seconds */
	x1 = PyNumber_Multiply(x3, us_per_second);	/* us */
	if (x1 == NULL)
		goto Done;
	Py_DECREF(x3);
	x3 = NULL;

	/* x1 has days+seconds in us */
	x2 = PyInt_FromLong(GET_TD_MICROSECONDS(self));
	if (x2 == NULL)
		goto Done;
	result = PyNumber_Add(x1, x2);

Done:
	Py_XDECREF(x1);
	Py_XDECREF(x2);
	Py_XDECREF(x3);
	return result;
}

/* Convert a number of us (as a Python int or long) to a timedelta.
 */
static PyObject *
microseconds_to_delta(PyObject *pyus)
{
	int us;
	int s;
	int d;
	long temp;

	PyObject *tuple = NULL;
	PyObject *num = NULL;
	PyObject *result = NULL;

	tuple = PyNumber_Divmod(pyus, us_per_second);
	if (tuple == NULL)
		goto Done;

	num = PyTuple_GetItem(tuple, 1);	/* us */
	if (num == NULL)
		goto Done;
	temp = PyLong_AsLong(num);
	num = NULL;
	if (temp == -1 && PyErr_Occurred())
		goto Done;
	assert(0 <= temp && temp < 1000000);
	us = (int)temp;
	if (us < 0) {
		/* The divisor was positive, so this must be an error. */
		assert(PyErr_Occurred());
		goto Done;
	}

	num = PyTuple_GetItem(tuple, 0);	/* leftover seconds */
	if (num == NULL)
		goto Done;
	Py_INCREF(num);
	Py_DECREF(tuple);

	tuple = PyNumber_Divmod(num, seconds_per_day);
	if (tuple == NULL)
		goto Done;
	Py_DECREF(num);

	num = PyTuple_GetItem(tuple, 1); 	/* seconds */
	if (num == NULL)
		goto Done;
	temp = PyLong_AsLong(num);
	num = NULL;
	if (temp == -1 && PyErr_Occurred())
		goto Done;
	assert(0 <= temp && temp < 24*3600);
	s = (int)temp;

	if (s < 0) {
		/* The divisor was positive, so this must be an error. */
		assert(PyErr_Occurred());
		goto Done;
	}

	num = PyTuple_GetItem(tuple, 0);	/* leftover days */
	if (num == NULL)
		goto Done;
	Py_INCREF(num);
	temp = PyLong_AsLong(num);
	if (temp == -1 && PyErr_Occurred())
		goto Done;
	d = (int)temp;
	if ((long)d != temp) {
		PyErr_SetString(PyExc_OverflowError, "normalized days too "
				"large to fit in a C int");
		goto Done;
	}
	result = new_delta(d, s, us, 0);

Done:
	Py_XDECREF(tuple);
	Py_XDECREF(num);
	return result;
}

static PyObject *
multiply_int_timedelta(PyObject *intobj, PyDateTime_Delta *delta)
{
	PyObject *pyus_in;
	PyObject *pyus_out;
	PyObject *result;

	pyus_in = delta_to_microseconds(delta);
	if (pyus_in == NULL)
		return NULL;

	pyus_out = PyNumber_Multiply(pyus_in, intobj);
	Py_DECREF(pyus_in);
	if (pyus_out == NULL)
		return NULL;

	result = microseconds_to_delta(pyus_out);
	Py_DECREF(pyus_out);
	return result;
}

static PyObject *
divide_timedelta_int(PyDateTime_Delta *delta, PyObject *intobj)
{
	PyObject *pyus_in;
	PyObject *pyus_out;
	PyObject *result;

	pyus_in = delta_to_microseconds(delta);
	if (pyus_in == NULL)
		return NULL;

	pyus_out = PyNumber_FloorDivide(pyus_in, intobj);
	Py_DECREF(pyus_in);
	if (pyus_out == NULL)
		return NULL;

	result = microseconds_to_delta(pyus_out);
	Py_DECREF(pyus_out);
	return result;
}

static PyObject *
delta_add(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDelta_Check(left) && PyDelta_Check(right)) {
		/* delta + delta */
		/* The C-level additions can't overflow because of the
		 * invariant bounds.
		 */
		int days = GET_TD_DAYS(left) + GET_TD_DAYS(right);
		int seconds = GET_TD_SECONDS(left) + GET_TD_SECONDS(right);
		int microseconds = GET_TD_MICROSECONDS(left) +
				   GET_TD_MICROSECONDS(right);
		result = new_delta(days, seconds, microseconds, 1);
	}

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

static PyObject *
delta_negative(PyDateTime_Delta *self)
{
	return new_delta(-GET_TD_DAYS(self),
			 -GET_TD_SECONDS(self),
			 -GET_TD_MICROSECONDS(self),
			 1);
}

static PyObject *
delta_positive(PyDateTime_Delta *self)
{
	/* Could optimize this (by returning self) if this isn't a
	 * subclass -- but who uses unary + ?  Approximately nobody.
	 */
	return new_delta(GET_TD_DAYS(self),
			 GET_TD_SECONDS(self),
			 GET_TD_MICROSECONDS(self),
			 0);
}

static PyObject *
delta_abs(PyDateTime_Delta *self)
{
	PyObject *result;

	assert(GET_TD_MICROSECONDS(self) >= 0);
	assert(GET_TD_SECONDS(self) >= 0);

	if (GET_TD_DAYS(self) < 0)
		result = delta_negative(self);
	else
		result = delta_positive(self);

	return result;
}

static PyObject *
delta_subtract(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDelta_Check(left) && PyDelta_Check(right)) {
	    	/* delta - delta */
	    	PyObject *minus_right = PyNumber_Negative(right);
	    	if (minus_right) {
	    		result = delta_add(left, minus_right);
	    		Py_DECREF(minus_right);
	    	}
	    	else
	    		result = NULL;
	}

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

/* This is more natural as a tp_compare, but doesn't work then:  for whatever
 * reason, Python's try_3way_compare ignores tp_compare unless
 * PyInstance_Check returns true, but these aren't old-style classes.
 */
static PyObject *
delta_richcompare(PyDateTime_Delta *self, PyObject *other, int op)
{
	int diff;

	if (! PyDelta_CheckExact(other)) {
		PyErr_Format(PyExc_TypeError,
			     "can't compare %s to %s instance",
			     self->ob_type->tp_name, other->ob_type->tp_name);
		return NULL;
	}
	diff = GET_TD_DAYS(self) - GET_TD_DAYS(other);
	if (diff == 0) {
		diff = GET_TD_SECONDS(self) - GET_TD_SECONDS(other);
		if (diff == 0)
			diff = GET_TD_MICROSECONDS(self) -
			       GET_TD_MICROSECONDS(other);
	}
	return diff_to_bool(diff, op);
}

static PyObject *delta_getstate(PyDateTime_Delta *self);

static long
delta_hash(PyDateTime_Delta *self)
{
	if (self->hashcode == -1) {
		PyObject *temp = delta_getstate(self);
		if (temp != NULL) {
			self->hashcode = PyObject_Hash(temp);
			Py_DECREF(temp);
		}
	}
	return self->hashcode;
}

static PyObject *
delta_multiply(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDelta_Check(left)) {
		/* delta * ??? */
		if (PyInt_Check(right) || PyLong_Check(right))
			result = multiply_int_timedelta(right,
					(PyDateTime_Delta *) left);
	}
	else if (PyInt_Check(left) || PyLong_Check(left))
		result = multiply_int_timedelta(left,
						(PyDateTime_Delta *) right);

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

static PyObject *
delta_divide(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDelta_Check(left)) {
		/* delta * ??? */
		if (PyInt_Check(right) || PyLong_Check(right))
			result = divide_timedelta_int(
					(PyDateTime_Delta *)left,
					right);
	}

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

/* Fold in the value of the tag ("seconds", "weeks", etc) component of a
 * timedelta constructor.  sofar is the # of microseconds accounted for
 * so far, and there are factor microseconds per current unit, the number
 * of which is given by num.  num * factor is added to sofar in a
 * numerically careful way, and that's the result.  Any fractional
 * microseconds left over (this can happen if num is a float type) are
 * added into *leftover.
 * Note that there are many ways this can give an error (NULL) return.
 */
static PyObject *
accum(const char* tag, PyObject *sofar, PyObject *num, PyObject *factor,
      double *leftover)
{
	PyObject *prod;
	PyObject *sum;

	assert(num != NULL);

	if (PyInt_Check(num) || PyLong_Check(num)) {
		prod = PyNumber_Multiply(num, factor);
		if (prod == NULL)
			return NULL;
		sum = PyNumber_Add(sofar, prod);
		Py_DECREF(prod);
		return sum;
	}

	if (PyFloat_Check(num)) {
		double dnum;
		double fracpart;
		double intpart;
		PyObject *x;
		PyObject *y;

		/* The Plan:  decompose num into an integer part and a
		 * fractional part, num = intpart + fracpart.
		 * Then num * factor ==
		 *      intpart * factor + fracpart * factor
		 * and the LHS can be computed exactly in long arithmetic.
		 * The RHS is again broken into an int part and frac part.
		 * and the frac part is added into *leftover.
		 */
		dnum = PyFloat_AsDouble(num);
		if (dnum == -1.0 && PyErr_Occurred())
			return NULL;
		fracpart = modf(dnum, &intpart);
		x = PyLong_FromDouble(intpart);
		if (x == NULL)
			return NULL;

		prod = PyNumber_Multiply(x, factor);
		Py_DECREF(x);
		if (prod == NULL)
			return NULL;

		sum = PyNumber_Add(sofar, prod);
		Py_DECREF(prod);
		if (sum == NULL)
			return NULL;

		if (fracpart == 0.0)
			return sum;
		/* So far we've lost no information.  Dealing with the
		 * fractional part requires float arithmetic, and may
		 * lose a little info.
		 */
		assert(PyInt_Check(factor) || PyLong_Check(factor));
		if (PyInt_Check(factor))
			dnum = (double)PyInt_AsLong(factor);
		else
			dnum = PyLong_AsDouble(factor);

		dnum *= fracpart;
		fracpart = modf(dnum, &intpart);
		x = PyLong_FromDouble(intpart);
		if (x == NULL) {
			Py_DECREF(sum);
			return NULL;
		}

		y = PyNumber_Add(sum, x);
		Py_DECREF(sum);
		Py_DECREF(x);
		*leftover += fracpart;
		return y;
	}

	PyErr_Format(PyExc_TypeError,
		     "unsupported type for timedelta %s component: %s",
		     tag, num->ob_type->tp_name);
	return NULL;
}

static PyObject *
delta_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;

	/* Argument objects. */
	PyObject *day = NULL;
	PyObject *second = NULL;
	PyObject *us = NULL;
	PyObject *ms = NULL;
	PyObject *minute = NULL;
	PyObject *hour = NULL;
	PyObject *week = NULL;

	PyObject *x = NULL;	/* running sum of microseconds */
	PyObject *y = NULL;	/* temp sum of microseconds */
	double leftover_us = 0.0;

	static char *keywords[] = {
		"days", "seconds", "microseconds", "milliseconds",
		"minutes", "hours", "weeks", NULL
	};

	if (PyArg_ParseTupleAndKeywords(args, kw, "|OOOOOOO:__new__",
					keywords,
					&day, &second, &us,
					&ms, &minute, &hour, &week) == 0)
		goto Done;

	x = PyInt_FromLong(0);
	if (x == NULL)
		goto Done;

#define CLEANUP 	\
	Py_DECREF(x);	\
	x = y;		\
	if (x == NULL)	\
		goto Done

	if (us) {
		y = accum("microseconds", x, us, us_per_us, &leftover_us);
		CLEANUP;
	}
	if (ms) {
		y = accum("milliseconds", x, ms, us_per_ms, &leftover_us);
		CLEANUP;
	}
	if (second) {
		y = accum("seconds", x, second, us_per_second, &leftover_us);
		CLEANUP;
	}
	if (minute) {
		y = accum("minutes", x, minute, us_per_minute, &leftover_us);
		CLEANUP;
	}
	if (hour) {
		y = accum("hours", x, hour, us_per_hour, &leftover_us);
		CLEANUP;
	}
	if (day) {
		y = accum("days", x, day, us_per_day, &leftover_us);
		CLEANUP;
	}
	if (week) {
		y = accum("weeks", x, week, us_per_week, &leftover_us);
		CLEANUP;
	}
	if (leftover_us) {
		/* Round to nearest whole # of us, and add into x. */
		PyObject *temp = PyLong_FromLong(round_to_long(leftover_us));
		if (temp == NULL) {
			Py_DECREF(x);
			goto Done;
		}
		y = PyNumber_Add(x, temp);
		Py_DECREF(temp);
		CLEANUP;
	}

	self = microseconds_to_delta(x);
	Py_DECREF(x);
Done:
	return self;

#undef CLEANUP
}

static int
delta_nonzero(PyDateTime_Delta *self)
{
	return (GET_TD_DAYS(self) != 0
		|| GET_TD_SECONDS(self) != 0
		|| GET_TD_MICROSECONDS(self) != 0);
}

static PyObject *
delta_repr(PyDateTime_Delta *self)
{
	if (GET_TD_MICROSECONDS(self) != 0)
		return PyString_FromFormat("%s(%d, %d, %d)",
					   self->ob_type->tp_name,
					   GET_TD_DAYS(self),
					   GET_TD_SECONDS(self),
					   GET_TD_MICROSECONDS(self));
	if (GET_TD_SECONDS(self) != 0)
		return PyString_FromFormat("%s(%d, %d)",
					   self->ob_type->tp_name,
					   GET_TD_DAYS(self),
					   GET_TD_SECONDS(self));

	return PyString_FromFormat("%s(%d)",
				   self->ob_type->tp_name,
				   GET_TD_DAYS(self));
}

static PyObject *
delta_str(PyDateTime_Delta *self)
{
	int days = GET_TD_DAYS(self);
	int seconds = GET_TD_SECONDS(self);
	int us = GET_TD_MICROSECONDS(self);
	int hours;
	int minutes;
	char buf[100];
	char *pbuf = buf;
	size_t buflen = sizeof(buf);
	int n;

	minutes = divmod(seconds, 60, &seconds);
	hours = divmod(minutes, 60, &minutes);

	if (days) {
		n = PyOS_snprintf(pbuf, buflen, "%d day%s, ", days,
				  (days == 1 || days == -1) ? "" : "s");
		if (n < 0 || (size_t)n >= buflen)
			goto Fail;
		pbuf += n;
		buflen -= (size_t)n;
	}

	n = PyOS_snprintf(pbuf, buflen, "%d:%02d:%02d",
			  hours, minutes, seconds);
	if (n < 0 || (size_t)n >= buflen)
		goto Fail;
	pbuf += n;
	buflen -= (size_t)n;

	if (us) {
		n = PyOS_snprintf(pbuf, buflen, ".%06d", us);
		if (n < 0 || (size_t)n >= buflen)
			goto Fail;
		pbuf += n;
	}

	return PyString_FromStringAndSize(buf, pbuf - buf);

 Fail:
	PyErr_SetString(PyExc_SystemError, "goofy result from PyOS_snprintf");
	return NULL;
}

/* Pickle support.  Quite a maze!  While __getstate__/__setstate__ sufficed
 * in the Python implementation, the C implementation also requires
 * __reduce__, and a __safe_for_unpickling__ attr in the type object.
 */
static PyObject *
delta_getstate(PyDateTime_Delta *self)
{
	return Py_BuildValue("iii", GET_TD_DAYS(self),
				    GET_TD_SECONDS(self),
				    GET_TD_MICROSECONDS(self));
}

static PyObject *
delta_setstate(PyDateTime_Delta *self, PyObject *state)
{
	int day;
	int second;
	int us;

	if (!PyArg_ParseTuple(state, "iii:__setstate__", &day, &second, &us))
		return NULL;

	self->hashcode = -1;
	SET_TD_DAYS(self, day);
	SET_TD_SECONDS(self, second);
	SET_TD_MICROSECONDS(self, us);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
delta_reduce(PyDateTime_Delta* self)
{
	PyObject* result = NULL;
	PyObject* state  = delta_getstate(self);

	if (state != NULL) {
		/* The funky "()" in the format string creates an empty
		 * tuple as the 2nd component of the result 3-tuple.
		 */
		result = Py_BuildValue("O()O", self->ob_type, state);
		Py_DECREF(state);
	}
	return result;
}

#define OFFSET(field)  offsetof(PyDateTime_Delta, field)

static PyMemberDef delta_members[] = {
	{"days",         T_INT, OFFSET(days),         READONLY,
	 PyDoc_STR("Number of days.")},

	{"seconds",      T_INT, OFFSET(seconds),      READONLY,
	 PyDoc_STR("Number of seconds (>= 0 and less than 1 day).")},

	{"microseconds", T_INT, OFFSET(microseconds), READONLY,
	 PyDoc_STR("Number of microseconds (>= 0 and less than 1 second).")},
	{NULL}
};

static PyMethodDef delta_methods[] = {
	{"__setstate__", (PyCFunction)delta_setstate, METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__reduce__", (PyCFunction)delta_reduce,     METH_NOARGS,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)delta_getstate, METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},
	{NULL,	NULL},
};

static char delta_doc[] =
PyDoc_STR("Difference between two datetime values.");

static PyNumberMethods delta_as_number = {
	delta_add,				/* nb_add */
	delta_subtract,				/* nb_subtract */
	delta_multiply,				/* nb_multiply */
	delta_divide,				/* nb_divide */
	0,					/* nb_remainder */
	0,					/* nb_divmod */
	0,					/* nb_power */
	(unaryfunc)delta_negative,		/* nb_negative */
	(unaryfunc)delta_positive,		/* nb_positive */
	(unaryfunc)delta_abs,			/* nb_absolute */
	(inquiry)delta_nonzero,			/* nb_nonzero */
	0,					/*nb_invert*/
	0,					/*nb_lshift*/
	0,					/*nb_rshift*/
	0,					/*nb_and*/
	0,					/*nb_xor*/
	0,					/*nb_or*/
	0,					/*nb_coerce*/
	0,					/*nb_int*/
	0,					/*nb_long*/
	0,					/*nb_float*/
	0,					/*nb_oct*/
	0, 					/*nb_hex*/
	0,					/*nb_inplace_add*/
	0,					/*nb_inplace_subtract*/
	0,					/*nb_inplace_multiply*/
	0,					/*nb_inplace_divide*/
	0,					/*nb_inplace_remainder*/
	0,					/*nb_inplace_power*/
	0,					/*nb_inplace_lshift*/
	0,					/*nb_inplace_rshift*/
	0,					/*nb_inplace_and*/
	0,					/*nb_inplace_xor*/
	0,					/*nb_inplace_or*/
	delta_divide,				/* nb_floor_divide */
	0,					/* nb_true_divide */
	0,					/* nb_inplace_floor_divide */
	0,					/* nb_inplace_true_divide */
};

static PyTypeObject PyDateTime_DeltaType = {
	PyObject_HEAD_INIT(NULL)
	0,						/* ob_size */
	"datetime.timedelta",				/* tp_name */
	sizeof(PyDateTime_Delta),			/* tp_basicsize */
	0,						/* tp_itemsize */
	0,						/* tp_dealloc */
	0,						/* tp_print */
	0,						/* tp_getattr */
	0,						/* tp_setattr */
	0,						/* tp_compare */
	(reprfunc)delta_repr,				/* tp_repr */
	&delta_as_number,				/* tp_as_number */
	0,						/* tp_as_sequence */
	0,						/* tp_as_mapping */
	(hashfunc)delta_hash,				/* tp_hash */
	0,              				/* tp_call */
	(reprfunc)delta_str,				/* tp_str */
	PyObject_GenericGetAttr,			/* tp_getattro */
	0,						/* tp_setattro */
	0,						/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES,	/* tp_flags */
	delta_doc,					/* tp_doc */
	0,						/* tp_traverse */
	0,						/* tp_clear */
	(richcmpfunc)delta_richcompare,			/* tp_richcompare */
	0,						/* tp_weaklistoffset */
	0,						/* tp_iter */
	0,						/* tp_iternext */
	delta_methods,					/* tp_methods */
	delta_members,					/* tp_members */
	0,						/* tp_getset */
	0,						/* tp_base */
	0,						/* tp_dict */
	0,						/* tp_descr_get */
	0,						/* tp_descr_set */
	0,						/* tp_dictoffset */
	0,						/* tp_init */
	0,						/* tp_alloc */
	delta_new,					/* tp_new */
	_PyObject_Del,					/* tp_free */
};

/*
 * PyDateTime_Date implementation.
 */

/* Accessor properties. */

static PyObject *
date_year(PyDateTime_Date *self, void *unused)
{
	return PyInt_FromLong(GET_YEAR(self));
}

static PyObject *
date_month(PyDateTime_Date *self, void *unused)
{
	return PyInt_FromLong(GET_MONTH(self));
}

static PyObject *
date_day(PyDateTime_Date *self, void *unused)
{
	return PyInt_FromLong(GET_DAY(self));
}

static PyGetSetDef date_getset[] = {
	{"year",        (getter)date_year},
	{"month",       (getter)date_month},
	{"day",         (getter)date_day},
	{NULL}
};

/* Constructors. */

static char *date_kws[] = {"year", "month", "day", NULL};

static PyObject *
date_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	int year;
	int month;
	int day;

	if (PyArg_ParseTupleAndKeywords(args, kw, "iii", date_kws,
					&year, &month, &day)) {
		if (check_date_args(year, month, day) < 0)
			return NULL;
		self = new_date(year, month, day);
	}
	return self;
}

/* Return new date from localtime(t). */
static PyObject *
date_local_from_time_t(PyObject *cls, time_t t)
{
	struct tm *tm;
	PyObject *result = NULL;

	tm = localtime(&t);
	if (tm)
		result = PyObject_CallFunction(cls, "iii",
					       tm->tm_year + 1900,
					       tm->tm_mon + 1,
					       tm->tm_mday);
	else
		PyErr_SetString(PyExc_ValueError,
				"timestamp out of range for "
				"platform localtime() function");
	return result;
}

/* Return new date from current time.
 * We say this is equivalent to fromtimestamp(time.time()), and the
 * only way to be sure of that is to *call* time.time().  That's not
 * generally the same as calling C's time.
 */
static PyObject *
date_today(PyObject *cls, PyObject *dummy)
{
	PyObject *time;
	PyObject *result;

	time = time_time();
	if (time == NULL)
		return NULL;

	/* Note well:  today() is a class method, so this may not call
	 * date.fromtimestamp.  For example, it may call
	 * datetime.fromtimestamp.  That's why we need all the accuracy
	 * time.time() delivers; if someone were gonzo about optimization,
	 * date.today() could get away with plain C time().
	 */
	result = PyObject_CallMethod(cls, "fromtimestamp", "O", time);
	Py_DECREF(time);
	return result;
}

/* Return new date from given timestamp (Python timestamp -- a double). */
static PyObject *
date_fromtimestamp(PyObject *cls, PyObject *args)
{
	double timestamp;
	PyObject *result = NULL;

	if (PyArg_ParseTuple(args, "d:fromtimestamp", &timestamp))
		result = date_local_from_time_t(cls, (time_t)timestamp);
	return result;
}

/* Return new date from proleptic Gregorian ordinal.  Raises ValueError if
 * the ordinal is out of range.
 */
static PyObject *
date_fromordinal(PyObject *cls, PyObject *args)
{
	PyObject *result = NULL;
	int ordinal;

	if (PyArg_ParseTuple(args, "i:fromordinal", &ordinal)) {
		int year;
		int month;
		int day;

		if (ordinal < 1)
			PyErr_SetString(PyExc_ValueError, "ordinal must be "
							  ">= 1");
		else {
			ord_to_ymd(ordinal, &year, &month, &day);
			result = PyObject_CallFunction(cls, "iii",
						       year, month, day);
		}
	}
	return result;
}

/*
 * Date arithmetic.
 */

/* date + timedelta -> date.  If arg negate is true, subtract the timedelta
 * instead.
 */
static PyObject *
add_date_timedelta(PyDateTime_Date *date, PyDateTime_Delta *delta, int negate)
{
	PyObject *result = NULL;
	int year = GET_YEAR(date);
	int month = GET_MONTH(date);
	int deltadays = GET_TD_DAYS(delta);
	/* C-level overflow is impossible because |deltadays| < 1e9. */
	int day = GET_DAY(date) + (negate ? -deltadays : deltadays);

	if (normalize_date(&year, &month, &day) >= 0)
		result = new_date(year, month, day);
	return result;
}

static PyObject *
date_add(PyObject *left, PyObject *right)
{
	if (PyDateTime_Check(left) || PyDateTime_Check(right)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	if (PyDate_CheckExact(left)) {
		/* date + ??? */
		if (PyDelta_Check(right))
			/* date + delta */
			return add_date_timedelta((PyDateTime_Date *) left,
						  (PyDateTime_Delta *) right,
						  0);
	}
	else {
		/* ??? + date
		 * 'right' must be one of us, or we wouldn't have been called
		 */
		if (PyDelta_Check(left))
			/* delta + date */
			return add_date_timedelta((PyDateTime_Date *) right,
						  (PyDateTime_Delta *) left,
						  0);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *
date_subtract(PyObject *left, PyObject *right)
{
	if (PyDateTime_Check(left) || PyDateTime_Check(right)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	if (PyDate_CheckExact(left)) {
		if (PyDate_CheckExact(right)) {
			/* date - date */
			int left_ord = ymd_to_ord(GET_YEAR(left),
						  GET_MONTH(left),
						  GET_DAY(left));
			int right_ord = ymd_to_ord(GET_YEAR(right),
						   GET_MONTH(right),
						   GET_DAY(right));
			return new_delta(left_ord - right_ord, 0, 0, 0);
		}
		if (PyDelta_Check(right)) {
			/* date - delta */
			return add_date_timedelta((PyDateTime_Date *) left,
						  (PyDateTime_Delta *) right,
						  1);
		}
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}


/* Various ways to turn a date into a string. */

static PyObject *
date_repr(PyDateTime_Date *self)
{
	char buffer[1028];
	char *typename;

	typename = self->ob_type->tp_name;
	PyOS_snprintf(buffer, sizeof(buffer), "%s(%d, %d, %d)",
		      typename,
		      GET_YEAR(self), GET_MONTH(self), GET_DAY(self));

	return PyString_FromString(buffer);
}

static PyObject *
date_isoformat(PyDateTime_Date *self)
{
	char buffer[128];

	isoformat_date(self, buffer, sizeof(buffer));
	return PyString_FromString(buffer);
}

/* str() calls the appropriate isofomat() method. */
static PyObject *
date_str(PyDateTime_Date *self)
{
	return PyObject_CallMethod((PyObject *)self, "isoformat", "()");
}


static PyObject *
date_ctime(PyDateTime_Date *self)
{
	return format_ctime(self, 0, 0, 0);
}

static PyObject *
date_strftime(PyDateTime_Date *self, PyObject *args, PyObject *kw)
{
	/* This method can be inherited, and needs to call the
	 * timetuple() method appropriate to self's class.
	 */
	PyObject *result;
	PyObject *format;
	PyObject *tuple;
	static char *keywords[] = {"format", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kw, "O!:strftime", keywords,
					  &PyString_Type, &format))
		return NULL;

	tuple = PyObject_CallMethod((PyObject *)self, "timetuple", "()");
	if (tuple == NULL)
		return NULL;
	result = wrap_strftime((PyObject *)self, format, tuple,
			       (PyObject *)self);
	Py_DECREF(tuple);
	return result;
}

/* ISO methods. */

static PyObject *
date_isoweekday(PyDateTime_Date *self)
{
	int dow = weekday(GET_YEAR(self), GET_MONTH(self), GET_DAY(self));

	return PyInt_FromLong(dow + 1);
}

static PyObject *
date_isocalendar(PyDateTime_Date *self)
{
	int  year         = GET_YEAR(self);
	int  week1_monday = iso_week1_monday(year);
	int today         = ymd_to_ord(year, GET_MONTH(self), GET_DAY(self));
	int  week;
	int  day;

	week = divmod(today - week1_monday, 7, &day);
	if (week < 0) {
		--year;
		week1_monday = iso_week1_monday(year);
		week = divmod(today - week1_monday, 7, &day);
	}
	else if (week >= 52 && today >= iso_week1_monday(year + 1)) {
		++year;
		week = 0;
	}
	return Py_BuildValue("iii", year, week + 1, day + 1);
}

/* Miscellaneous methods. */

/* This is more natural as a tp_compare, but doesn't work then:  for whatever
 * reason, Python's try_3way_compare ignores tp_compare unless
 * PyInstance_Check returns true, but these aren't old-style classes.
 */
static PyObject *
date_richcompare(PyDateTime_Date *self, PyObject *other, int op)
{
	int diff;

	if (! PyDate_Check(other)) {
		PyErr_Format(PyExc_TypeError,
			     "can't compare date to %s instance",
			     other->ob_type->tp_name);
		return NULL;
	}
	diff = memcmp(self->data, ((PyDateTime_Date *)other)->data,
		      _PyDateTime_DATE_DATASIZE);
	return diff_to_bool(diff, op);
}

static PyObject *
date_timetuple(PyDateTime_Date *self)
{
	return build_struct_time(GET_YEAR(self),
				 GET_MONTH(self),
				 GET_DAY(self),
				 0, 0, 0, -1);
}

static PyObject *
date_replace(PyDateTime_Date *self, PyObject *args, PyObject *kw)
{
	PyObject *clone;
	PyObject *tuple;
	int year = GET_YEAR(self);
	int month = GET_MONTH(self);
	int day = GET_DAY(self);

	if (! PyArg_ParseTupleAndKeywords(args, kw, "|iii:replace", date_kws,
					  &year, &month, &day))
		return NULL;
	tuple = Py_BuildValue("iii", year, month, day);
	if (tuple == NULL)
		return NULL;
	clone = date_new(self->ob_type, tuple, NULL);
	Py_DECREF(tuple);
	return clone;
}

static PyObject *date_getstate(PyDateTime_Date *self);

static long
date_hash(PyDateTime_Date *self)
{
	if (self->hashcode == -1) {
		PyObject *temp = date_getstate(self);
		if (temp != NULL) {
			self->hashcode = PyObject_Hash(temp);
			Py_DECREF(temp);
		}
	}
	return self->hashcode;
}

static PyObject *
date_toordinal(PyDateTime_Date *self)
{
	return PyInt_FromLong(ymd_to_ord(GET_YEAR(self), GET_MONTH(self),
					 GET_DAY(self)));
}

static PyObject *
date_weekday(PyDateTime_Date *self)
{
	int dow = weekday(GET_YEAR(self), GET_MONTH(self), GET_DAY(self));

	return PyInt_FromLong(dow);
}

/* Pickle support.  Quite a maze! */

static PyObject *
date_getstate(PyDateTime_Date *self)
{
	return PyString_FromStringAndSize(self->data,
					  _PyDateTime_DATE_DATASIZE);
}

static PyObject *
date_setstate(PyDateTime_Date *self, PyObject *state)
{
	const int len = PyString_Size(state);
	unsigned char *pdata = (unsigned char*)PyString_AsString(state);

	if (! PyString_Check(state) ||
	    len != _PyDateTime_DATE_DATASIZE) {
		PyErr_SetString(PyExc_TypeError,
				"bad argument to date.__setstate__");
		return NULL;
	}
	memcpy(self->data, pdata, _PyDateTime_DATE_DATASIZE);
	self->hashcode = -1;

	Py_INCREF(Py_None);
	return Py_None;
}

/* XXX This seems a ridiculously inefficient way to pickle a short string. */
static PyObject *
date_pickler(PyObject *module, PyDateTime_Date *date)
{
	PyObject *state;
	PyObject *result = NULL;

	if (! PyDate_CheckExact(date)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to date pickler: %s",
			     date->ob_type->tp_name);
		return NULL;
	}
	state = date_getstate(date);
	if (state) {
		result = Py_BuildValue("O(O)", date_unpickler_object, state);
		Py_DECREF(state);
	}
	return result;
}

static PyObject *
date_unpickler(PyObject *module, PyObject *arg)
{
	PyDateTime_Date *self;

	if (! PyString_CheckExact(arg)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to date unpickler: %s",
			     arg->ob_type->tp_name);
		return NULL;
	}
	self = PyObject_New(PyDateTime_Date, &PyDateTime_DateType);
	if (self != NULL) {
		PyObject *res = date_setstate(self, arg);
		if (res == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		Py_DECREF(res);
	}
	return (PyObject *)self;
}

static PyMethodDef date_methods[] = {
	/* Class methods: */
	{"fromtimestamp", (PyCFunction)date_fromtimestamp, METH_VARARGS |
							   METH_CLASS,
	 PyDoc_STR("timestamp -> local date from a POSIX timestamp (like "
	 	   "time.time()).")},

	{"fromordinal", (PyCFunction)date_fromordinal,	METH_VARARGS |
							METH_CLASS,
	 PyDoc_STR("int -> date corresponding to a proleptic Gregorian "
	 	   "ordinal.")},

	{"today",         (PyCFunction)date_today,   METH_NOARGS | METH_CLASS,
	 PyDoc_STR("Current date or datetime:  same as "
	 	   "self.__class__.fromtimestamp(time.time()).")},

	/* Instance methods: */

	{"ctime",       (PyCFunction)date_ctime,        METH_NOARGS,
	 PyDoc_STR("Return ctime() style string.")},

	{"strftime",   	(PyCFunction)date_strftime,	METH_KEYWORDS,
	 PyDoc_STR("format -> strftime() style string.")},

	{"timetuple",   (PyCFunction)date_timetuple,    METH_NOARGS,
         PyDoc_STR("Return time tuple, compatible with time.localtime().")},

	{"isocalendar", (PyCFunction)date_isocalendar,  METH_NOARGS,
	 PyDoc_STR("Return a 3-tuple containing ISO year, week number, and "
	 	   "weekday.")},

	{"isoformat",   (PyCFunction)date_isoformat,	METH_NOARGS,
	 PyDoc_STR("Return string in ISO 8601 format, YYYY-MM-DD.")},

	{"isoweekday",  (PyCFunction)date_isoweekday,   METH_NOARGS,
	 PyDoc_STR("Return the day of the week represented by the date.\n"
	 	   "Monday == 1 ... Sunday == 7")},

	{"toordinal",   (PyCFunction)date_toordinal,    METH_NOARGS,
	 PyDoc_STR("Return proleptic Gregorian ordinal.  January 1 of year "
	 	   "1 is day 1.")},

	{"weekday",     (PyCFunction)date_weekday,      METH_NOARGS,
	 PyDoc_STR("Return the day of the week represented by the date.\n"
		   "Monday == 0 ... Sunday == 6")},

	{"replace",     (PyCFunction)date_replace,      METH_KEYWORDS,
	 PyDoc_STR("Return date with new specified fields.")},

	{"__setstate__", (PyCFunction)date_setstate,	METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)date_getstate,	METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},

	{NULL,	NULL}
};

static char date_doc[] =
PyDoc_STR("Basic date type.");

static PyNumberMethods date_as_number = {
	date_add,					/* nb_add */
	date_subtract,					/* nb_subtract */
	0,						/* nb_multiply */
	0,						/* nb_divide */
	0,						/* nb_remainder */
	0,						/* nb_divmod */
	0,						/* nb_power */
	0,						/* nb_negative */
	0,						/* nb_positive */
	0,						/* nb_absolute */
	0,						/* nb_nonzero */
};

static PyTypeObject PyDateTime_DateType = {
	PyObject_HEAD_INIT(NULL)
	0,						/* ob_size */
	"datetime.date",				/* tp_name */
	sizeof(PyDateTime_Date),			/* tp_basicsize */
	0,						/* tp_itemsize */
	(destructor)PyObject_Del,			/* tp_dealloc */
	0,						/* tp_print */
	0,						/* tp_getattr */
	0,						/* tp_setattr */
	0,						/* tp_compare */
	(reprfunc)date_repr,				/* tp_repr */
	&date_as_number,				/* tp_as_number */
	0,						/* tp_as_sequence */
	0,						/* tp_as_mapping */
	(hashfunc)date_hash,				/* tp_hash */
	0,              				/* tp_call */
	(reprfunc)date_str,				/* tp_str */
	PyObject_GenericGetAttr,			/* tp_getattro */
	0,						/* tp_setattro */
	0,						/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,				/* tp_flags */
	date_doc,					/* tp_doc */
	0,						/* tp_traverse */
	0,						/* tp_clear */
	(richcmpfunc)date_richcompare,			/* tp_richcompare */
	0,						/* tp_weaklistoffset */
	0,						/* tp_iter */
	0,						/* tp_iternext */
	date_methods,					/* tp_methods */
	0,						/* tp_members */
	date_getset,					/* tp_getset */
	0,						/* tp_base */
	0,						/* tp_dict */
	0,						/* tp_descr_get */
	0,						/* tp_descr_set */
	0,						/* tp_dictoffset */
	0,						/* tp_init */
	0,						/* tp_alloc */
	date_new,					/* tp_new */
	_PyObject_Del,					/* tp_free */
};

/*
 * PyDateTime_DateTime implementation.
 */

/* Accessor properties. */

static PyObject *
datetime_hour(PyDateTime_DateTime *self, void *unused)
{
	return PyInt_FromLong(DATE_GET_HOUR(self));
}

static PyObject *
datetime_minute(PyDateTime_DateTime *self, void *unused)
{
	return PyInt_FromLong(DATE_GET_MINUTE(self));
}

static PyObject *
datetime_second(PyDateTime_DateTime *self, void *unused)
{
	return PyInt_FromLong(DATE_GET_SECOND(self));
}

static PyObject *
datetime_microsecond(PyDateTime_DateTime *self, void *unused)
{
	return PyInt_FromLong(DATE_GET_MICROSECOND(self));
}

static PyGetSetDef datetime_getset[] = {
	{"hour",        (getter)datetime_hour},
	{"minute",      (getter)datetime_minute},
	{"second",      (getter)datetime_second},
	{"microsecond", (getter)datetime_microsecond},
	{NULL}
};

/* Constructors. */


static char *datetime_kws[] = {"year", "month", "day",
			       "hour", "minute", "second", "microsecond",
			       NULL};

static PyObject *
datetime_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	int year;
	int month;
	int day;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int usecond = 0;

	if (PyArg_ParseTupleAndKeywords(args, kw, "iii|iiii", datetime_kws,
					&year, &month, &day, &hour, &minute,
					&second, &usecond)) {
		if (check_date_args(year, month, day) < 0)
			return NULL;
		if (check_time_args(hour, minute, second, usecond) < 0)
			return NULL;
		self = new_datetime(year, month, day,
				    hour, minute, second, usecond);
	}
	return self;
}


/* TM_FUNC is the shared type of localtime() and gmtime(). */
typedef struct tm *(*TM_FUNC)(const time_t *timer);

/* Internal helper.
 * Build datetime from a time_t and a distinct count of microseconds.
 * Pass localtime or gmtime for f, to control the interpretation of timet.
 */
static PyObject *
datetime_from_timet_and_us(PyObject *cls, TM_FUNC f, time_t timet, int us)
{
	struct tm *tm;
	PyObject *result = NULL;

	tm = f(&timet);
	if (tm) {
		/* The platform localtime/gmtime may insert leap seconds,
		 * indicated by tm->tm_sec > 59.  We don't care about them,
		 * except to the extent that passing them on to the datetime
		 * constructor would raise ValueError for a reason that
		 * made no sense to the user.
		 */
		if (tm->tm_sec > 59)
			tm->tm_sec = 59;
		result = PyObject_CallFunction(cls, "iiiiiii",
					       tm->tm_year + 1900,
					       tm->tm_mon + 1,
					       tm->tm_mday,
					       tm->tm_hour,
					       tm->tm_min,
					       tm->tm_sec,
					       us);
	}
	else
		PyErr_SetString(PyExc_ValueError,
				"timestamp out of range for "
				"platform localtime()/gmtime() function");
	return result;
}

/* Internal helper.
 * Build datetime from a Python timestamp.  Pass localtime or gmtime for f,
 * to control the interpretation of the timestamp.  Since a double doesn't
 * have enough bits to cover a datetime's full range of precision, it's
 * better to call datetime_from_timet_and_us provided you have a way
 * to get that much precision (e.g., C time() isn't good enough).
 */
static PyObject *
datetime_from_timestamp(PyObject *cls, TM_FUNC f, double timestamp)
{
	time_t timet = (time_t)timestamp;
	double fraction = timestamp - (double)timet;
	int us = (int)round_to_long(fraction * 1e6);

	return datetime_from_timet_and_us(cls, f, timet, us);
}

/* Internal helper.
 * Build most accurate possible datetime for current time.  Pass localtime or
 * gmtime for f as appropriate.
 */
static PyObject *
datetime_best_possible(PyObject *cls, TM_FUNC f)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval t;

#ifdef GETTIMEOFDAY_NO_TZ
	gettimeofday(&t);
#else
	gettimeofday(&t, (struct timezone *)NULL);
#endif
	return datetime_from_timet_and_us(cls, f, t.tv_sec, (int)t.tv_usec);

#else	/* ! HAVE_GETTIMEOFDAY */
	/* No flavor of gettimeofday exists on this platform.  Python's
	 * time.time() does a lot of other platform tricks to get the
	 * best time it can on the platform, and we're not going to do
	 * better than that (if we could, the better code would belong
	 * in time.time()!)  We're limited by the precision of a double,
	 * though.
	 */
	PyObject *time;
	double dtime;

	time = time_time();
    	if (time == NULL)
    		return NULL;
	dtime = PyFloat_AsDouble(time);
	Py_DECREF(time);
	if (dtime == -1.0 && PyErr_Occurred())
		return NULL;
	return datetime_from_timestamp(cls, f, dtime);
#endif	/* ! HAVE_GETTIMEOFDAY */
}

/* Return new local datetime from timestamp (Python timestamp -- a double). */
static PyObject *
datetime_fromtimestamp(PyObject *cls, PyObject *args)
{
	double timestamp;
	PyObject *result = NULL;

	if (PyArg_ParseTuple(args, "d:fromtimestamp", &timestamp))
		result = datetime_from_timestamp(cls, localtime, timestamp);
	return result;
}

/* Return new UTC datetime from timestamp (Python timestamp -- a double). */
static PyObject *
datetime_utcfromtimestamp(PyObject *cls, PyObject *args)
{
	double timestamp;
	PyObject *result = NULL;

	if (PyArg_ParseTuple(args, "d:utcfromtimestamp", &timestamp))
		result = datetime_from_timestamp(cls, gmtime, timestamp);
	return result;
}

/* Return best possible local time -- this isn't constrained by the
 * precision of a timestamp.
 */
static PyObject *
datetime_now(PyObject *cls, PyObject *dummy)
{
	return datetime_best_possible(cls, localtime);
}

/* Return best possible UTC time -- this isn't constrained by the
 * precision of a timestamp.
 */
static PyObject *
datetime_utcnow(PyObject *cls, PyObject *dummy)
{
	return datetime_best_possible(cls, gmtime);
}

/* Return new datetime or datetimetz from date/datetime/datetimetz and
 * time/timetz arguments.
 */
static PyObject *
datetime_combine(PyObject *cls, PyObject *args, PyObject *kw)
{
 	static char *keywords[] = {"date", "time", NULL};
	PyObject *date;
	PyObject *time;
	PyObject *result = NULL;

	if (PyArg_ParseTupleAndKeywords(args, kw, "O!O!:combine", keywords,
					&PyDateTime_DateType, &date,
					&PyDateTime_TimeType, &time))
		result = PyObject_CallFunction(cls, "iiiiiii",
						GET_YEAR(date),
				    		GET_MONTH(date),
						GET_DAY(date),
				    		TIME_GET_HOUR(time),
				    		TIME_GET_MINUTE(time),
				    		TIME_GET_SECOND(time),
				    		TIME_GET_MICROSECOND(time));
	if (result && PyTimeTZ_Check(time) && PyDateTimeTZ_Check(result)) {
		/* Copy the tzinfo field. */
		replace_tzinfo(result, ((PyDateTime_TimeTZ *)time)->tzinfo);
	}
	return result;
}

/* datetime arithmetic. */

static PyObject *
add_datetime_timedelta(PyDateTime_DateTime *date, PyDateTime_Delta *delta)
{
	/* Note that the C-level additions can't overflow, because of
	 * invariant bounds on the member values.
	 */
	int year = GET_YEAR(date);
	int month = GET_MONTH(date);
	int day = GET_DAY(date) + GET_TD_DAYS(delta);
	int hour = DATE_GET_HOUR(date);
	int minute = DATE_GET_MINUTE(date);
	int second = DATE_GET_SECOND(date) + GET_TD_SECONDS(delta);
	int microsecond = DATE_GET_MICROSECOND(date) +
			  GET_TD_MICROSECONDS(delta);

	if (normalize_datetime(&year, &month, &day,
			       &hour, &minute, &second, &microsecond) < 0)
		return NULL;
	else
		return new_datetime(year, month, day,
				    hour, minute, second, microsecond);
}

static PyObject *
sub_datetime_timedelta(PyDateTime_DateTime *date, PyDateTime_Delta *delta)
{
	/* Note that the C-level subtractions can't overflow, because of
	 * invariant bounds on the member values.
	 */
	int year = GET_YEAR(date);
	int month = GET_MONTH(date);
	int day = GET_DAY(date) - GET_TD_DAYS(delta);
	int hour = DATE_GET_HOUR(date);
	int minute = DATE_GET_MINUTE(date);
	int second = DATE_GET_SECOND(date) - GET_TD_SECONDS(delta);
	int microsecond = DATE_GET_MICROSECOND(date) -
			  GET_TD_MICROSECONDS(delta);

	if (normalize_datetime(&year, &month, &day,
			       &hour, &minute, &second, &microsecond) < 0)
		return NULL;
	else
		return new_datetime(year, month, day,
				    hour, minute, second, microsecond);
}

static PyObject *
sub_datetime_datetime(PyDateTime_DateTime *left, PyDateTime_DateTime *right)
{
	int days1 = ymd_to_ord(GET_YEAR(left), GET_MONTH(left), GET_DAY(left));
	int days2 = ymd_to_ord(GET_YEAR(right),
			       GET_MONTH(right),
			       GET_DAY(right));
	/* These can't overflow, since the values are normalized.  At most
	 * this gives the number of seconds in one day.
	 */
	int delta_s = (DATE_GET_HOUR(left) - DATE_GET_HOUR(right)) * 3600 +
	              (DATE_GET_MINUTE(left) - DATE_GET_MINUTE(right)) * 60 +
		      DATE_GET_SECOND(left) - DATE_GET_SECOND(right);
	int delta_us = DATE_GET_MICROSECOND(left) -
		       DATE_GET_MICROSECOND(right);

	return new_delta(days1 - days2, delta_s, delta_us, 1);
}

static PyObject *
datetime_add(PyObject *left, PyObject *right)
{
	if (PyDateTime_Check(left)) {
		/* datetime + ??? */
		if (PyDelta_Check(right))
			/* datetime + delta */
			return add_datetime_timedelta(
					(PyDateTime_DateTime *)left,
					(PyDateTime_Delta *)right);
	}
	else if (PyDelta_Check(left)) {
		/* delta + datetime */
		return add_datetime_timedelta((PyDateTime_DateTime *) right,
					      (PyDateTime_Delta *) left);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *
datetime_subtract(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDateTime_Check(left)) {
		/* datetime - ??? */
		if (PyDateTime_Check(right)) {
			/* datetime - datetime */
			result = sub_datetime_datetime(
					(PyDateTime_DateTime *)left,
					(PyDateTime_DateTime *)right);
		}
		else if (PyDelta_Check(right)) {
			/* datetime - delta */
			result = sub_datetime_timedelta(
					(PyDateTime_DateTime *)left,
					(PyDateTime_Delta *)right);
		}
	}

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

/* Various ways to turn a datetime into a string. */

static PyObject *
datetime_repr(PyDateTime_DateTime *self)
{
	char buffer[1000];
	char *typename = self->ob_type->tp_name;

	if (DATE_GET_MICROSECOND(self)) {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d, %d, %d, %d, %d, %d)",
			      typename,
			      GET_YEAR(self), GET_MONTH(self), GET_DAY(self),
			      DATE_GET_HOUR(self), DATE_GET_MINUTE(self),
			      DATE_GET_SECOND(self),
			      DATE_GET_MICROSECOND(self));
	}
	else if (DATE_GET_SECOND(self)) {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d, %d, %d, %d, %d)",
			      typename,
			      GET_YEAR(self), GET_MONTH(self), GET_DAY(self),
			      DATE_GET_HOUR(self), DATE_GET_MINUTE(self),
			      DATE_GET_SECOND(self));
	}
	else {
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d, %d, %d, %d)",
			      typename,
			      GET_YEAR(self), GET_MONTH(self), GET_DAY(self),
			      DATE_GET_HOUR(self), DATE_GET_MINUTE(self));
	}
	return PyString_FromString(buffer);
}

static PyObject *
datetime_str(PyDateTime_DateTime *self)
{
	return PyObject_CallMethod((PyObject *)self, "isoformat", "(s)", " ");
}

static PyObject *
datetime_isoformat(PyDateTime_DateTime *self,
                   PyObject *args, PyObject *kw)
{
	char sep = 'T';
	static char *keywords[] = {"sep", NULL};
	char buffer[100];
	char *cp;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|c:isoformat", keywords,
					 &sep))
		return NULL;
	cp = isoformat_date((PyDateTime_Date *)self, buffer, sizeof(buffer));
	assert(cp != NULL);
	*cp++ = sep;
	isoformat_time(self, cp, sizeof(buffer) - (cp - buffer));
	return PyString_FromString(buffer);
}

static PyObject *
datetime_ctime(PyDateTime_DateTime *self)
{
	return format_ctime((PyDateTime_Date *)self,
			    DATE_GET_HOUR(self),
			    DATE_GET_MINUTE(self),
			    DATE_GET_SECOND(self));
}

/* Miscellaneous methods. */

/* This is more natural as a tp_compare, but doesn't work then:  for whatever
 * reason, Python's try_3way_compare ignores tp_compare unless
 * PyInstance_Check returns true, but these aren't old-style classes.
 * Note that this routine handles all comparisons for datetime and datetimetz.
 */
static PyObject *
datetime_richcompare(PyDateTime_DateTime *self, PyObject *other, int op)
{
	int diff;
	naivety n1, n2;
	int offset1, offset2;

	if (! PyDateTime_Check(other)) {
		/* Stop this from falling back to address comparison. */
		PyErr_Format(PyExc_TypeError,
			     "can't compare '%s' to '%s'",
			     self->ob_type->tp_name,
			     other->ob_type->tp_name);
		return NULL;
	}

	if (classify_two_utcoffsets((PyObject *)self, &offset1, &n1,
				    (PyObject *)self,
				     other, &offset2, &n2,
				     other) < 0)
		return NULL;
	assert(n1 != OFFSET_UNKNOWN && n2 != OFFSET_UNKNOWN);
 	/* If they're both naive, or both aware and have the same offsets,
	 * we get off cheap.  Note that if they're both naive, offset1 ==
	 * offset2 == 0 at this point.
	 */
	if (n1 == n2 && offset1 == offset2) {
		diff = memcmp(self->data, ((PyDateTime_DateTime *)other)->data,
			      _PyDateTime_DATETIME_DATASIZE);
		return diff_to_bool(diff, op);
	}

	if (n1 == OFFSET_AWARE && n2 == OFFSET_AWARE) {
		/* We want the sign of
		 *     (self - offset1 minutes) - (other - offset2 minutes) =
		 *     (self - other) + (offset2 - offset1) minutes.
		 */
		PyDateTime_Delta *delta;
		int days, seconds, us;

		assert(offset1 != offset2);	/* else last "if" handled it */
		delta = (PyDateTime_Delta *)sub_datetime_datetime(self,
						(PyDateTime_DateTime *)other);
		if (delta == NULL)
			return NULL;
		days = delta->days;
		seconds = delta->seconds + (offset2 - offset1) * 60;
		us = delta->microseconds;
		Py_DECREF(delta);
		normalize_d_s_us(&days, &seconds, &us);
		diff = days;
		if (diff == 0)
			diff = seconds | us;
		return diff_to_bool(diff, op);
	}

	assert(n1 != n2);
	PyErr_SetString(PyExc_TypeError,
			"can't compare offset-naive and "
			"offset-aware datetimes");
	return NULL;
}

static PyObject *datetime_getstate(PyDateTime_DateTime *self);

static long
datetime_hash(PyDateTime_DateTime *self)
{
	if (self->hashcode == -1) {
		naivety n;
		int offset;
		PyObject *temp;

		n = classify_utcoffset((PyObject *)self, (PyObject *)self,
				       &offset);
		assert(n != OFFSET_UNKNOWN);
		if (n == OFFSET_ERROR)
			return -1;

		/* Reduce this to a hash of another object. */
		if (n == OFFSET_NAIVE)
			temp = datetime_getstate(self);
		else {
			int days;
			int seconds;

			assert(n == OFFSET_AWARE);
			assert(PyDateTimeTZ_Check(self));
			days = ymd_to_ord(GET_YEAR(self),
					  GET_MONTH(self),
					  GET_DAY(self));
			seconds = DATE_GET_HOUR(self) * 3600 +
				  (DATE_GET_MINUTE(self) - offset) * 60 +
				  DATE_GET_SECOND(self);
			temp = new_delta(days,
					 seconds,
					 DATE_GET_MICROSECOND(self),
					 1);
		}
		if (temp != NULL) {
			self->hashcode = PyObject_Hash(temp);
			Py_DECREF(temp);
		}
	}
	return self->hashcode;
}

static PyObject *
datetime_replace(PyDateTime_DateTime *self, PyObject *args, PyObject *kw)
{
	PyObject *clone;
	PyObject *tuple;
	int y = GET_YEAR(self);
	int m = GET_MONTH(self);
	int d = GET_DAY(self);
	int hh = DATE_GET_HOUR(self);
	int mm = DATE_GET_MINUTE(self);
	int ss = DATE_GET_SECOND(self);
	int us = DATE_GET_MICROSECOND(self);

	if (! PyArg_ParseTupleAndKeywords(args, kw, "|iiiiiii:replace",
					  datetime_kws,
					  &y, &m, &d, &hh, &mm, &ss, &us))
		return NULL;
	tuple = Py_BuildValue("iiiiiii", y, m, d, hh, mm, ss, us);
	if (tuple == NULL)
		return NULL;
	clone = datetime_new(self->ob_type, tuple, NULL);
	Py_DECREF(tuple);
	return clone;
}

static PyObject *
datetime_astimezone(PyDateTime_DateTime *self, PyObject *args, PyObject *kw)
{
	PyObject *tzinfo;
	static char *keywords[] = {"tz", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kw, "O:astimezone", keywords,
					  &tzinfo))
		return NULL;
	if (check_tzinfo_subclass(tzinfo) < 0)
		return NULL;
	return new_datetimetz(GET_YEAR(self), GET_MONTH(self), GET_DAY(self),
			      DATE_GET_HOUR(self), DATE_GET_MINUTE(self),
			      DATE_GET_SECOND(self),
			      DATE_GET_MICROSECOND(self),
			      tzinfo);
}

static PyObject *
datetime_timetuple(PyDateTime_DateTime *self)
{
	return build_struct_time(GET_YEAR(self),
				 GET_MONTH(self),
				 GET_DAY(self),
				 DATE_GET_HOUR(self),
				 DATE_GET_MINUTE(self),
				 DATE_GET_SECOND(self),
				 -1);
}

static PyObject *
datetime_getdate(PyDateTime_DateTime *self)
{
	return new_date(GET_YEAR(self),
			GET_MONTH(self),
			GET_DAY(self));
}

static PyObject *
datetime_gettime(PyDateTime_DateTime *self)
{
	return new_time(DATE_GET_HOUR(self),
			DATE_GET_MINUTE(self),
			DATE_GET_SECOND(self),
			DATE_GET_MICROSECOND(self));
}

/* Pickle support.  Quite a maze! */

static PyObject *
datetime_getstate(PyDateTime_DateTime *self)
{
	return PyString_FromStringAndSize(self->data,
					  _PyDateTime_DATETIME_DATASIZE);
}

static PyObject *
datetime_setstate(PyDateTime_DateTime *self, PyObject *state)
{
	const int len = PyString_Size(state);
	unsigned char *pdata = (unsigned char*)PyString_AsString(state);

	if (! PyString_Check(state) ||
	    len != _PyDateTime_DATETIME_DATASIZE) {
		PyErr_SetString(PyExc_TypeError,
				"bad argument to datetime.__setstate__");
		return NULL;
	}
	memcpy(self->data, pdata, _PyDateTime_DATETIME_DATASIZE);
	self->hashcode = -1;

	Py_INCREF(Py_None);
	return Py_None;
}

/* XXX This seems a ridiculously inefficient way to pickle a short string. */
static PyObject *
datetime_pickler(PyObject *module, PyDateTime_DateTime *datetime)
{
	PyObject *state;
	PyObject *result = NULL;

	if (! PyDateTime_CheckExact(datetime)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to datetime pickler: %s",
			     datetime->ob_type->tp_name);
		return NULL;
	}
	state = datetime_getstate(datetime);
	if (state) {
		result = Py_BuildValue("O(O)",
				       datetime_unpickler_object,
				       state);
		Py_DECREF(state);
	}
	return result;
}

static PyObject *
datetime_unpickler(PyObject *module, PyObject *arg)
{
	PyDateTime_DateTime *self;

	if (! PyString_CheckExact(arg)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to datetime unpickler: %s",
			     arg->ob_type->tp_name);
		return NULL;
	}
	self = PyObject_New(PyDateTime_DateTime, &PyDateTime_DateTimeType);
	if (self != NULL) {
		PyObject *res = datetime_setstate(self, arg);
		if (res == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		Py_DECREF(res);
	}
	return (PyObject *)self;
}

static PyMethodDef datetime_methods[] = {
	/* Class methods: */
	{"now",         (PyCFunction)datetime_now,
	 METH_NOARGS | METH_CLASS,
	 PyDoc_STR("Return a new datetime representing local day and time.")},

	{"utcnow",         (PyCFunction)datetime_utcnow,
	 METH_NOARGS | METH_CLASS,
	 PyDoc_STR("Return a new datetime representing UTC day and time.")},

	{"fromtimestamp", (PyCFunction)datetime_fromtimestamp,
	 METH_VARARGS | METH_CLASS,
	 PyDoc_STR("timestamp -> local datetime from a POSIX timestamp "
	 	   "(like time.time()).")},

	{"utcfromtimestamp", (PyCFunction)datetime_utcfromtimestamp,
	 METH_VARARGS | METH_CLASS,
	 PyDoc_STR("timestamp -> UTC datetime from a POSIX timestamp "
	 	   "(like time.time()).")},

	{"combine", (PyCFunction)datetime_combine,
	 METH_VARARGS | METH_KEYWORDS | METH_CLASS,
	 PyDoc_STR("date, time -> datetime with same date and time fields")},

	/* Instance methods: */
	{"timetuple",   (PyCFunction)datetime_timetuple, METH_NOARGS,
         PyDoc_STR("Return time tuple, compatible with time.localtime().")},

	{"date",   (PyCFunction)datetime_getdate, METH_NOARGS,
         PyDoc_STR("Return date object with same year, month and day.")},

	{"time",   (PyCFunction)datetime_gettime, METH_NOARGS,
         PyDoc_STR("Return time object with same hour, minute, second and "
         	   "microsecond.")},

	{"ctime",       (PyCFunction)datetime_ctime,	METH_NOARGS,
	 PyDoc_STR("Return ctime() style string.")},

	{"isoformat",   (PyCFunction)datetime_isoformat, METH_KEYWORDS,
	 PyDoc_STR("[sep] -> string in ISO 8601 format, "
	 	   "YYYY-MM-DDTHH:MM:SS[.mmmmmm].\n\n"
	 	   "sep is used to separate the year from the time, and "
	 	   "defaults\n"
	 	   "to 'T'.")},

	{"replace",     (PyCFunction)datetime_replace,	METH_KEYWORDS,
	 PyDoc_STR("Return datetime with new specified fields.")},

	{"astimezone",  (PyCFunction)datetime_astimezone, METH_KEYWORDS,
	 PyDoc_STR("tz -> datetimetz with same date & time, and tzinfo=tz\n")},

	{"__setstate__", (PyCFunction)datetime_setstate, METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)datetime_getstate, METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},
	{NULL,	NULL}
};

static char datetime_doc[] =
PyDoc_STR("Basic date/time type.");

static PyNumberMethods datetime_as_number = {
	datetime_add,				/* nb_add */
	datetime_subtract,			/* nb_subtract */
	0,					/* nb_multiply */
	0,					/* nb_divide */
	0,					/* nb_remainder */
	0,					/* nb_divmod */
	0,					/* nb_power */
	0,					/* nb_negative */
	0,					/* nb_positive */
	0,					/* nb_absolute */
	0,					/* nb_nonzero */
};

statichere PyTypeObject PyDateTime_DateTimeType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"datetime.datetime",			/* tp_name */
	sizeof(PyDateTime_DateTime),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)PyObject_Del,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)datetime_repr,		/* tp_repr */
	&datetime_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)datetime_hash,		/* tp_hash */
	0,              			/* tp_call */
	(reprfunc)datetime_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,			/* tp_flags */
	datetime_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	(richcmpfunc)datetime_richcompare,	/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	datetime_methods,			/* tp_methods */
	0,					/* tp_members */
	datetime_getset,			/* tp_getset */
	&PyDateTime_DateType,			/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	datetime_new,				/* tp_new */
	_PyObject_Del,				/* tp_free */
};

/*
 * PyDateTime_Time implementation.
 */

/* Accessor properties. */

static PyObject *
time_hour(PyDateTime_Time *self, void *unused)
{
	return PyInt_FromLong(TIME_GET_HOUR(self));
}

static PyObject *
time_minute(PyDateTime_Time *self, void *unused)
{
	return PyInt_FromLong(TIME_GET_MINUTE(self));
}

static PyObject *
py_time_second(PyDateTime_Time *self, void *unused)
{
	return PyInt_FromLong(TIME_GET_SECOND(self));
}

static PyObject *
time_microsecond(PyDateTime_Time *self, void *unused)
{
	return PyInt_FromLong(TIME_GET_MICROSECOND(self));
}

static PyGetSetDef time_getset[] = {
	{"hour",        (getter)time_hour},
	{"minute",      (getter)time_minute},
	{"second",      (getter)py_time_second},
	{"microsecond", (getter)time_microsecond},
	{NULL}
};

/* Constructors. */

static char *time_kws[] = {"hour", "minute", "second", "microsecond", NULL};

static PyObject *
time_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int usecond = 0;


	if (PyArg_ParseTupleAndKeywords(args, kw, "|iiii", time_kws,
					&hour, &minute, &second, &usecond)) {
		if (check_time_args(hour, minute, second, usecond) < 0)
			return NULL;
		self = new_time(hour, minute, second, usecond);
	}
	return self;
}

/* Various ways to turn a time into a string. */

static PyObject *
time_repr(PyDateTime_Time *self)
{
	char buffer[100];
	char *typename = self->ob_type->tp_name;
	int h = TIME_GET_HOUR(self);
	int m = TIME_GET_MINUTE(self);
	int s = TIME_GET_SECOND(self);
	int us = TIME_GET_MICROSECOND(self);

	if (us)
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d, %d, %d)", typename, h, m, s, us);
	else if (s)
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d, %d)", typename, h, m, s);
	else
		PyOS_snprintf(buffer, sizeof(buffer),
			      "%s(%d, %d)", typename, h, m);
	return PyString_FromString(buffer);
}

static PyObject *
time_str(PyDateTime_Time *self)
{
	return PyObject_CallMethod((PyObject *)self, "isoformat", "()");
}

static PyObject *
time_isoformat(PyDateTime_Time *self)
{
	char buffer[100];
	/* Reuse the time format code from the datetime type. */
	PyDateTime_DateTime datetime;
	PyDateTime_DateTime *pdatetime = &datetime;

	/* Copy over just the time bytes. */
	memcpy(pdatetime->data + _PyDateTime_DATE_DATASIZE,
	       self->data,
	       _PyDateTime_TIME_DATASIZE);

	isoformat_time(pdatetime, buffer, sizeof(buffer));
	return PyString_FromString(buffer);
}

static PyObject *
time_strftime(PyDateTime_Time *self, PyObject *args, PyObject *kw)
{
	PyObject *result;
	PyObject *format;
	PyObject *tuple;
	static char *keywords[] = {"format", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kw, "O!:strftime", keywords,
					  &PyString_Type, &format))
		return NULL;

	/* Python's strftime does insane things with the year part of the
	 * timetuple.  The year is forced to (the otherwise nonsensical)
	 * 1900 to worm around that.
	 */
	tuple = Py_BuildValue("iiiiiiiii",
		              1900, 0, 0, /* year, month, day */
			      TIME_GET_HOUR(self),
			      TIME_GET_MINUTE(self),
			      TIME_GET_SECOND(self),
			      0, 0, -1); /* weekday, daynum, dst */
	if (tuple == NULL)
		return NULL;
	assert(PyTuple_Size(tuple) == 9);
	result = wrap_strftime((PyObject *)self, format, tuple, Py_None);
	Py_DECREF(tuple);
	return result;
}

/* Miscellaneous methods. */

/* This is more natural as a tp_compare, but doesn't work then:  for whatever
 * reason, Python's try_3way_compare ignores tp_compare unless
 * PyInstance_Check returns true, but these aren't old-style classes.
 * Note that this routine handles all comparisons for time and timetz.
 */
static PyObject *
time_richcompare(PyDateTime_Time *self, PyObject *other, int op)
{
	int diff;
	naivety n1, n2;
	int offset1, offset2;

	if (! PyTime_Check(other)) {
		/* Stop this from falling back to address comparison. */
		PyErr_Format(PyExc_TypeError,
			     "can't compare '%s' to '%s'",
			     self->ob_type->tp_name,
			     other->ob_type->tp_name);
		return NULL;
	}
	if (classify_two_utcoffsets((PyObject *)self, &offset1, &n1, Py_None,
				     other, &offset2, &n2, Py_None) < 0)
		return NULL;
	assert(n1 != OFFSET_UNKNOWN && n2 != OFFSET_UNKNOWN);
	/* If they're both naive, or both aware and have the same offsets,
	 * we get off cheap.  Note that if they're both naive, offset1 ==
	 * offset2 == 0 at this point.
	 */
	if (n1 == n2 && offset1 == offset2) {
		diff = memcmp(self->data, ((PyDateTime_Time *)other)->data,
			      _PyDateTime_TIME_DATASIZE);
		return diff_to_bool(diff, op);
	}

	if (n1 == OFFSET_AWARE && n2 == OFFSET_AWARE) {
		assert(offset1 != offset2);	/* else last "if" handled it */
		/* Convert everything except microseconds to seconds.  These
		 * can't overflow (no more than the # of seconds in 2 days).
		 */
		offset1 = TIME_GET_HOUR(self) * 3600 +
			  (TIME_GET_MINUTE(self) - offset1) * 60 +
			  TIME_GET_SECOND(self);
		offset2 = TIME_GET_HOUR(other) * 3600 +
			  (TIME_GET_MINUTE(other) - offset2) * 60 +
			  TIME_GET_SECOND(other);
		diff = offset1 - offset2;
		if (diff == 0)
			diff = TIME_GET_MICROSECOND(self) -
			       TIME_GET_MICROSECOND(other);
		return diff_to_bool(diff, op);
	}

	assert(n1 != n2);
	PyErr_SetString(PyExc_TypeError,
			"can't compare offset-naive and "
			"offset-aware times");
	return NULL;
}

static PyObject *time_getstate(PyDateTime_Time *self);

static long
time_hash(PyDateTime_Time *self)
{
	if (self->hashcode == -1) {
		naivety n;
		int offset;
		PyObject *temp;

		n = classify_utcoffset((PyObject *)self, Py_None, &offset);
		assert(n != OFFSET_UNKNOWN);
		if (n == OFFSET_ERROR)
			return -1;

		/* Reduce this to a hash of another object. */
		if (offset == 0)
			temp = time_getstate(self);
		else {
			int hour;
			int minute;

			assert(n == OFFSET_AWARE);
			assert(PyTimeTZ_Check(self));
			hour = divmod(TIME_GET_HOUR(self) * 60 +
					TIME_GET_MINUTE(self) - offset,
				      60,
				      &minute);
			if (0 <= hour && hour < 24)
				temp = new_time(hour, minute,
						TIME_GET_SECOND(self),
						TIME_GET_MICROSECOND(self));
			else
				temp = Py_BuildValue("iiii",
					   hour, minute,
					   TIME_GET_SECOND(self),
					   TIME_GET_MICROSECOND(self));
		}
		if (temp != NULL) {
			self->hashcode = PyObject_Hash(temp);
			Py_DECREF(temp);
		}
	}
	return self->hashcode;
}

static PyObject *
time_replace(PyDateTime_Time *self, PyObject *args, PyObject *kw)
{
	PyObject *clone;
	PyObject *tuple;
	int hh = TIME_GET_HOUR(self);
	int mm = TIME_GET_MINUTE(self);
	int ss = TIME_GET_SECOND(self);
	int us = TIME_GET_MICROSECOND(self);

	if (! PyArg_ParseTupleAndKeywords(args, kw, "|iiii:replace",
					  time_kws,
					  &hh, &mm, &ss, &us))
		return NULL;
	tuple = Py_BuildValue("iiii", hh, mm, ss, us);
	if (tuple == NULL)
		return NULL;
	clone = time_new(self->ob_type, tuple, NULL);
	Py_DECREF(tuple);
	return clone;
}

static int
time_nonzero(PyDateTime_Time *self)
{
	return TIME_GET_HOUR(self) ||
	       TIME_GET_MINUTE(self) ||
	       TIME_GET_SECOND(self) ||
	       TIME_GET_MICROSECOND(self);
}

/* Pickle support.  Quite a maze! */

static PyObject *
time_getstate(PyDateTime_Time *self)
{
	return PyString_FromStringAndSize(self->data,
					  _PyDateTime_TIME_DATASIZE);
}

static PyObject *
time_setstate(PyDateTime_Time *self, PyObject *state)
{
	const int len = PyString_Size(state);
	unsigned char *pdata = (unsigned char*)PyString_AsString(state);

	if (! PyString_Check(state) ||
	    len != _PyDateTime_TIME_DATASIZE) {
		PyErr_SetString(PyExc_TypeError,
				"bad argument to time.__setstate__");
		return NULL;
	}
	memcpy(self->data, pdata, _PyDateTime_TIME_DATASIZE);
	self->hashcode = -1;

	Py_INCREF(Py_None);
	return Py_None;
}

/* XXX This seems a ridiculously inefficient way to pickle a short string. */
static PyObject *
time_pickler(PyObject *module, PyDateTime_Time *time)
{
	PyObject *state;
	PyObject *result = NULL;

	if (! PyTime_CheckExact(time)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to time pickler: %s",
			     time->ob_type->tp_name);
		return NULL;
	}
	state = time_getstate(time);
	if (state) {
		result = Py_BuildValue("O(O)",
				       time_unpickler_object,
				       state);
		Py_DECREF(state);
	}
	return result;
}

static PyObject *
time_unpickler(PyObject *module, PyObject *arg)
{
	PyDateTime_Time *self;

	if (! PyString_CheckExact(arg)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to time unpickler: %s",
			     arg->ob_type->tp_name);
		return NULL;
	}
	self = PyObject_New(PyDateTime_Time, &PyDateTime_TimeType);
	if (self != NULL) {
		PyObject *res = time_setstate(self, arg);
		if (res == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		Py_DECREF(res);
	}
	return (PyObject *)self;
}

static PyMethodDef time_methods[] = {
	{"isoformat",   (PyCFunction)time_isoformat,	METH_KEYWORDS,
	 PyDoc_STR("Return string in ISO 8601 format, HH:MM:SS[.mmmmmm].")},

	{"strftime",   	(PyCFunction)time_strftime,	METH_KEYWORDS,
	 PyDoc_STR("format -> strftime() style string.")},

	{"replace",     (PyCFunction)time_replace,	METH_KEYWORDS,
	 PyDoc_STR("Return datetime with new specified fields.")},

	{"__setstate__", (PyCFunction)time_setstate,	METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)time_getstate,	METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},
	{NULL,	NULL}
};

static char time_doc[] =
PyDoc_STR("Basic time type.");

static PyNumberMethods time_as_number = {
	0,					/* nb_add */
	0,					/* nb_subtract */
	0,					/* nb_multiply */
	0,					/* nb_divide */
	0,					/* nb_remainder */
	0,					/* nb_divmod */
	0,					/* nb_power */
	0,					/* nb_negative */
	0,					/* nb_positive */
	0,					/* nb_absolute */
	(inquiry)time_nonzero,			/* nb_nonzero */
};

statichere PyTypeObject PyDateTime_TimeType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"datetime.time",			/* tp_name */
	sizeof(PyDateTime_Time),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)PyObject_Del,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)time_repr,			/* tp_repr */
	&time_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	(hashfunc)time_hash,			/* tp_hash */
	0,              			/* tp_call */
	(reprfunc)time_str,			/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,			/* tp_flags */
	time_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	(richcmpfunc)time_richcompare,		/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	time_methods,				/* tp_methods */
	0,					/* tp_members */
	time_getset,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	time_new,				/* tp_new */
	_PyObject_Del,				/* tp_free */
};

/*
 * PyDateTime_TZInfo implementation.
 */

/* This is a pure abstract base class, so doesn't do anything beyond
 * raising NotImplemented exceptions.  Real tzinfo classes need
 * to derive from this.  This is mostly for clarity, and for efficiency in
 * datetimetz and timetz constructors (their tzinfo arguments need to
 * be subclasses of this tzinfo class, which is easy and quick to check).
 *
 * Note:  For reasons having to do with pickling of subclasses, we have
 * to allow tzinfo objects to be instantiated.  This wasn't an issue
 * in the Python implementation (__init__() could raise NotImplementedError
 * there without ill effect), but doing so in the C implementation hit a
 * brick wall.
 */

static PyObject *
tzinfo_nogo(const char* methodname)
{
	PyErr_Format(PyExc_NotImplementedError,
		     "a tzinfo subclass must implement %s()",
		     methodname);
	return NULL;
}

/* Methods.  A subclass must implement these. */

static PyObject*
tzinfo_tzname(PyDateTime_TZInfo *self, PyObject *dt)
{
	return tzinfo_nogo("tzname");
}

static PyObject*
tzinfo_utcoffset(PyDateTime_TZInfo *self, PyObject *dt)
{
	return tzinfo_nogo("utcoffset");
}

static PyObject*
tzinfo_dst(PyDateTime_TZInfo *self, PyObject *dt)
{
	return tzinfo_nogo("dst");
}

/*
 * Pickle support.  This is solely so that tzinfo subclasses can use
 * pickling -- tzinfo itself is supposed to be uninstantiable.  The
 * pickler and unpickler functions are given module-level private
 * names, and registered with copy_reg, by the module init function.
 */

static PyObject*
tzinfo_pickler(PyDateTime_TZInfo *self) {
	return Py_BuildValue("O()", tzinfo_unpickler_object);
}

static PyObject*
tzinfo_unpickler(PyObject * unused) {
 	return PyType_GenericNew(&PyDateTime_TZInfoType, NULL, NULL);
}


static PyMethodDef tzinfo_methods[] = {
	{"tzname",	(PyCFunction)tzinfo_tzname,		METH_O,
	 PyDoc_STR("datetime -> string name of time zone.")},

	{"utcoffset",	(PyCFunction)tzinfo_utcoffset,		METH_O,
	 PyDoc_STR("datetime -> minutes east of UTC (negative for "
	 	   "west of UTC).")},

	{"dst",		(PyCFunction)tzinfo_dst,		METH_O,
	 PyDoc_STR("datetime -> DST offset in minutes east of UTC.")},

	{NULL, NULL}
};

static char tzinfo_doc[] =
PyDoc_STR("Abstract base class for time zone info objects.");

 statichere PyTypeObject PyDateTime_TZInfoType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"datetime.tzinfo",			/* tp_name */
	sizeof(PyDateTime_TZInfo),		/* tp_basicsize */
	0,					/* tp_itemsize */
	0,					/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,              			/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,			/* tp_flags */
	tzinfo_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	tzinfo_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	PyType_GenericNew,			/* tp_new */
	0,					/* tp_free */
};

/*
 * PyDateTime_TimeTZ implementation.
 */

/* Accessor properties.  Properties for hour, minute, second and microsecond
 * are inherited from time.
 */

static PyObject *
timetz_tzinfo(PyDateTime_TimeTZ *self, void *unused)
{
	Py_INCREF(self->tzinfo);
	return self->tzinfo;
}

static PyGetSetDef timetz_getset[] = {
	{"tzinfo", (getter)timetz_tzinfo},
	{NULL}
};

/*
 * Constructors.
 */

static char *timetz_kws[] = {"hour", "minute", "second", "microsecond",
			     "tzinfo", NULL};

static PyObject *
timetz_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int usecond = 0;
	PyObject *tzinfo = Py_None;

	if (PyArg_ParseTupleAndKeywords(args, kw, "|iiiiO", timetz_kws,
					&hour, &minute, &second, &usecond,
					&tzinfo)) {
		if (check_time_args(hour, minute, second, usecond) < 0)
			return NULL;
		if (check_tzinfo_subclass(tzinfo) < 0)
			return NULL;
		self = new_timetz(hour, minute, second, usecond, tzinfo);
	}
	return self;
}

/*
 * Destructor.
 */

static void
timetz_dealloc(PyDateTime_TimeTZ *self)
{
	Py_XDECREF(self->tzinfo);
	self->ob_type->tp_free((PyObject *)self);
}

/*
 * Indirect access to tzinfo methods.
 */

/* These are all METH_NOARGS, so don't need to check the arglist. */
static PyObject *
timetz_utcoffset(PyDateTime_TimeTZ *self, PyObject *unused) {
	return offset_as_timedelta(self->tzinfo, "utcoffset", Py_None);
}

static PyObject *
timetz_dst(PyDateTime_TimeTZ *self, PyObject *unused) {
	return offset_as_timedelta(self->tzinfo, "dst", Py_None);
}

static PyObject *
timetz_tzname(PyDateTime_TimeTZ *self, PyObject *unused) {
	return call_tzname(self->tzinfo, Py_None);
}

/*
 * Various ways to turn a timetz into a string.
 */

static PyObject *
timetz_repr(PyDateTime_TimeTZ *self)
{
	PyObject *baserepr = time_repr((PyDateTime_Time *)self);

	if (baserepr == NULL)
		return NULL;
	return append_keyword_tzinfo(baserepr, self->tzinfo);
}

/* Note:  tp_str is inherited from time. */

static PyObject *
timetz_isoformat(PyDateTime_TimeTZ *self)
{
	char buf[100];
	PyObject *result = time_isoformat((PyDateTime_Time *)self);

	if (result == NULL || self->tzinfo == Py_None)
		return result;

	/* We need to append the UTC offset. */
	if (format_utcoffset(buf, sizeof(buf), ":", self->tzinfo,
			     Py_None) < 0) {
		Py_DECREF(result);
		return NULL;
	}
	PyString_ConcatAndDel(&result, PyString_FromString(buf));
	return result;
}

/* Note:  strftime() is inherited from time. */

/*
 * Miscellaneous methods.
 */

/* Note:  tp_richcompare and tp_hash are inherited from time. */

static PyObject *
timetz_replace(PyDateTime_TimeTZ *self, PyObject *args, PyObject *kw)
{
	PyObject *clone;
	PyObject *tuple;
	int hh = TIME_GET_HOUR(self);
	int mm = TIME_GET_MINUTE(self);
	int ss = TIME_GET_SECOND(self);
	int us = TIME_GET_MICROSECOND(self);
	PyObject *tzinfo = self->tzinfo;

	if (! PyArg_ParseTupleAndKeywords(args, kw, "|iiiiO:replace",
					  timetz_kws,
					  &hh, &mm, &ss, &us, &tzinfo))
		return NULL;
	tuple = Py_BuildValue("iiiiO", hh, mm, ss, us, tzinfo);
	if (tuple == NULL)
		return NULL;
	clone = timetz_new(self->ob_type, tuple, NULL);
	Py_DECREF(tuple);
	return clone;
}

static int
timetz_nonzero(PyDateTime_TimeTZ *self)
{
	int offset;
	int none;

	if (TIME_GET_SECOND(self) || TIME_GET_MICROSECOND(self)) {
		/* Since utcoffset is in whole minutes, nothing can
		 * alter the conclusion that this is nonzero.
		 */
		return 1;
	}
	offset = 0;
	if (self->tzinfo != Py_None) {
		offset = call_utcoffset(self->tzinfo, Py_None, &none);
		if (offset == -1 && PyErr_Occurred())
			return -1;
	}
	return (TIME_GET_MINUTE(self) - offset + TIME_GET_HOUR(self)*60) != 0;
}

/*
 * Pickle support.  Quite a maze!
 */

/* Let basestate be the state string returned by time_getstate.
 * If tzinfo is None, this returns (basestate,), else (basestate, tzinfo).
 * So it's a tuple in any (non-error) case.
 */
static PyObject *
timetz_getstate(PyDateTime_TimeTZ *self)
{
	PyObject *basestate;
	PyObject *result = NULL;

	basestate = time_getstate((PyDateTime_Time *)self);
	if (basestate != NULL) {
		if (self->tzinfo == Py_None)
			result = Py_BuildValue("(O)", basestate);
		else
			result = Py_BuildValue("OO", basestate, self->tzinfo);
		Py_DECREF(basestate);
	}
	return result;
}

static PyObject *
timetz_setstate(PyDateTime_TimeTZ *self, PyObject *state)
{
	PyObject *temp;
	PyObject *basestate;
	PyObject *tzinfo = Py_None;

	if (! PyArg_ParseTuple(state, "O!|O:__setstate__",
			       &PyString_Type, &basestate,
			       &tzinfo))
		return NULL;
	temp = time_setstate((PyDateTime_Time *)self, basestate);
	if (temp == NULL)
		return NULL;
	Py_DECREF(temp);

	Py_INCREF(tzinfo);
	Py_XDECREF(self->tzinfo);
	self->tzinfo = tzinfo;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
timetz_pickler(PyObject *module, PyDateTime_TimeTZ *timetz)
{
	PyObject *state;
	PyObject *result = NULL;

	if (! PyTimeTZ_CheckExact(timetz)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to timetz pickler: %s",
			     timetz->ob_type->tp_name);
		return NULL;
	}
	state = timetz_getstate(timetz);
	if (state) {
		result = Py_BuildValue("O(O)",
				       timetz_unpickler_object,
				       state);
		Py_DECREF(state);
	}
	return result;
}

static PyObject *
timetz_unpickler(PyObject *module, PyObject *arg)
{
	PyDateTime_TimeTZ *self;

	self = PyObject_New(PyDateTime_TimeTZ, &PyDateTime_TimeTZType);
	if (self != NULL) {
		PyObject *res;

		self->tzinfo = NULL;
		res = timetz_setstate(self, arg);
		if (res == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		Py_DECREF(res);
	}
	return (PyObject *)self;
}

static PyMethodDef timetz_methods[] = {
	{"isoformat",   (PyCFunction)timetz_isoformat,	METH_KEYWORDS,
	 PyDoc_STR("Return string in ISO 8601 format, HH:MM:SS[.mmmmmm]"
	 	   "[+HH:MM].")},

	{"utcoffset",	(PyCFunction)timetz_utcoffset,	METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.utcoffset(self).")},

	{"tzname",	(PyCFunction)timetz_tzname,	METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.tzname(self).")},

	{"dst",		(PyCFunction)timetz_dst,	METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.dst(self).")},

	{"replace",     (PyCFunction)timetz_replace,	METH_KEYWORDS,
	 PyDoc_STR("Return timetz with new specified fields.")},

	{"__setstate__", (PyCFunction)timetz_setstate,	METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)timetz_getstate,	METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},
	{NULL,	NULL}

};

static char timetz_doc[] =
PyDoc_STR("Time type.");

static PyNumberMethods timetz_as_number = {
	0,					/* nb_add */
	0,					/* nb_subtract */
	0,					/* nb_multiply */
	0,					/* nb_divide */
	0,					/* nb_remainder */
	0,					/* nb_divmod */
	0,					/* nb_power */
	0,					/* nb_negative */
	0,					/* nb_positive */
	0,					/* nb_absolute */
	(inquiry)timetz_nonzero,		/* nb_nonzero */
};

statichere PyTypeObject PyDateTime_TimeTZType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"datetime.timetz",			/* tp_name */
	sizeof(PyDateTime_TimeTZ),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)timetz_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)timetz_repr,			/* tp_repr */
	&timetz_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,              			/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,			/* tp_flags */
	timetz_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	timetz_methods,				/* tp_methods */
	0,					/* tp_members */
	timetz_getset,				/* tp_getset */
	&PyDateTime_TimeType,			/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	timetz_new,				/* tp_new */
	_PyObject_Del,				/* tp_free */
};

/*
 * PyDateTime_DateTimeTZ implementation.
 */

/* Accessor properties.  Properties for day, month, year, hour, minute,
 * second and microsecond are inherited from datetime.
 */

static PyObject *
datetimetz_tzinfo(PyDateTime_DateTimeTZ *self, void *unused)
{
	Py_INCREF(self->tzinfo);
	return self->tzinfo;
}

static PyGetSetDef datetimetz_getset[] = {
	{"tzinfo", (getter)datetimetz_tzinfo},
	{NULL}
};

/*
 * Constructors.
 * These are like the datetime methods of the same names, but allow an
 * optional tzinfo argument.
 */

static char *datetimetz_kws[] = {
	"year", "month", "day", "hour", "minute", "second",
	"microsecond", "tzinfo", NULL
};

static PyObject *
datetimetz_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	int year;
	int month;
	int day;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int usecond = 0;
	PyObject *tzinfo = Py_None;

	if (PyArg_ParseTupleAndKeywords(args, kw, "iii|iiiiO", datetimetz_kws,
					&year, &month, &day, &hour, &minute,
					&second, &usecond, &tzinfo)) {
		if (check_date_args(year, month, day) < 0)
			return NULL;
		if (check_time_args(hour, minute, second, usecond) < 0)
			return NULL;
		if (check_tzinfo_subclass(tzinfo) < 0)
			return NULL;
		self = new_datetimetz(year, month, day,
				      hour, minute, second, usecond,
				      tzinfo);
	}
	return self;
}

/* Return best possible local time -- this isn't constrained by the
 * precision of a timestamp.
 */
static PyObject *
datetimetz_now(PyObject *cls, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	PyObject *tzinfo = Py_None;
	static char *keywords[] = {"tzinfo", NULL};

	if (PyArg_ParseTupleAndKeywords(args, kw, "|O:now", keywords,
					&tzinfo)) {
		if (check_tzinfo_subclass(tzinfo) < 0)
			return NULL;
		self = datetime_best_possible(cls, localtime);
		if (self != NULL)
			replace_tzinfo(self, tzinfo);
	}
	return self;
}

/* Return new local datetime from timestamp (Python timestamp -- a double). */
static PyObject *
datetimetz_fromtimestamp(PyObject *cls, PyObject *args, PyObject *kw)
{
	PyObject *self = NULL;
	double timestamp;
	PyObject *tzinfo = Py_None;
	static char *keywords[] = {"timestamp", "tzinfo", NULL};

	if (PyArg_ParseTupleAndKeywords(args, kw, "d|O:fromtimestamp",
					keywords, &timestamp, &tzinfo)) {
		if (check_tzinfo_subclass(tzinfo) < 0)
			return NULL;
		self = datetime_from_timestamp(cls, localtime, timestamp);
		if (self != NULL)
			replace_tzinfo(self, tzinfo);
	}
	return self;
}

/* Note:  utcnow() is inherited, and doesn't accept tzinfo.
 * Ditto utcfromtimestamp().  Ditto combine().
 */


/*
 * Destructor.
 */

static void
datetimetz_dealloc(PyDateTime_DateTimeTZ *self)
{
	Py_XDECREF(self->tzinfo);
	self->ob_type->tp_free((PyObject *)self);
}

/*
 * Indirect access to tzinfo methods.
 */

/* These are all METH_NOARGS, so don't need to check the arglist. */
static PyObject *
datetimetz_utcoffset(PyDateTime_DateTimeTZ *self, PyObject *unused) {
	return offset_as_timedelta(self->tzinfo, "utcoffset",
				   (PyObject *)self);
}

static PyObject *
datetimetz_dst(PyDateTime_DateTimeTZ *self, PyObject *unused) {
	return offset_as_timedelta(self->tzinfo, "dst", (PyObject *)self);
}

static PyObject *
datetimetz_tzname(PyDateTime_DateTimeTZ *self, PyObject *unused) {
	return call_tzname(self->tzinfo, (PyObject *)self);
}

/*
 * datetimetz arithmetic.
 */

/* If base is Py_NotImplemented or NULL, just return it.
 * Else base is a datetime, exactly one of {left, right} is a datetimetz,
 * and we want to create a datetimetz with the same date and time fields
 * as base, and with the tzinfo field from left or right.  Do that,
 * return it, and decref base.  This is used to transform the result of
 * a binary datetime operation (base) into a datetimetz result.
 */
static PyObject *
attach_tzinfo(PyObject *base, PyObject *left, PyObject *right)
{
	PyDateTime_DateTimeTZ *self;
	PyDateTime_DateTimeTZ *result;

	if (base == NULL || base == Py_NotImplemented)
		return base;

	assert(PyDateTime_CheckExact(base));

	if (PyDateTimeTZ_Check(left)) {
		assert(! PyDateTimeTZ_Check(right));
		self = (PyDateTime_DateTimeTZ *)left;
	}
	else {
		assert(PyDateTimeTZ_Check(right));
		self = (PyDateTime_DateTimeTZ *)right;
	}
	result = PyObject_New(PyDateTime_DateTimeTZ,
			      &PyDateTime_DateTimeTZType);
	if (result != NULL) {
		memcpy(result->data, ((PyDateTime_DateTime *)base)->data,
		       _PyDateTime_DATETIME_DATASIZE);
		Py_INCREF(self->tzinfo);
		result->tzinfo = self->tzinfo;
	}
	Py_DECREF(base);
	return (PyObject *)result;
}

static PyObject *
datetimetz_add(PyObject *left, PyObject *right)
{
	return attach_tzinfo(datetime_add(left, right), left, right);
}

static PyObject *
datetimetz_subtract(PyObject *left, PyObject *right)
{
	PyObject *result = Py_NotImplemented;

	if (PyDateTime_Check(left)) {
		/* datetime - ??? */
		if (PyDateTime_Check(right)) {
			/* datetime - datetime */
			naivety n1, n2;
			int offset1, offset2;
			PyDateTime_Delta *delta;

			if (classify_two_utcoffsets(left, &offset1, &n1, left,
						    right, &offset2, &n2,
						    right) < 0)
				return NULL;
			assert(n1 != OFFSET_UNKNOWN && n2 != OFFSET_UNKNOWN);
			if (n1 != n2) {
				PyErr_SetString(PyExc_TypeError,
					"can't subtract offset-naive and "
					"offset-aware datetimes");
				return NULL;
			}
			delta = (PyDateTime_Delta *)sub_datetime_datetime(
						(PyDateTime_DateTime *)left,
						(PyDateTime_DateTime *)right);
			if (delta == NULL || offset1 == offset2)
				return (PyObject *)delta;
			/* (left - offset1) - (right - offset2) =
			 * (left - right) + (offset2 - offset1)
			 */
			result = new_delta(delta->days,
					   delta->seconds +
					   	(offset2 - offset1) * 60,
					   delta->microseconds,
					   1);
			Py_DECREF(delta);
		}
		else if (PyDelta_Check(right)) {
			/* datetimetz - delta */
			result = sub_datetime_timedelta(
					(PyDateTime_DateTime *)left,
					(PyDateTime_Delta *)right);
			result = attach_tzinfo(result, left, right);
		}
	}

	if (result == Py_NotImplemented)
		Py_INCREF(result);
	return result;
}

/* Various ways to turn a datetime into a string. */

static PyObject *
datetimetz_repr(PyDateTime_DateTimeTZ *self)
{
	PyObject *baserepr = datetime_repr((PyDateTime_DateTime *)self);

	if (baserepr == NULL)
		return NULL;
	return append_keyword_tzinfo(baserepr, self->tzinfo);
}

/* Note:  tp_str is inherited from datetime. */

static PyObject *
datetimetz_isoformat(PyDateTime_DateTimeTZ *self,
		     PyObject *args, PyObject *kw)
{
	char buf[100];
	PyObject *result = datetime_isoformat((PyDateTime_DateTime *)self,
					      args, kw);

	if (result == NULL || self->tzinfo == Py_None)
		return result;

	/* We need to append the UTC offset. */
	if (format_utcoffset(buf, sizeof(buf), ":", self->tzinfo,
			     (PyObject *)self) < 0) {
		Py_DECREF(result);
		return NULL;
	}
	PyString_ConcatAndDel(&result, PyString_FromString(buf));
	return result;
}

/* Miscellaneous methods. */

/* Note:  tp_richcompare and tp_hash are inherited from datetime. */

static PyObject *
datetimetz_replace(PyDateTime_DateTimeTZ *self, PyObject *args, PyObject *kw)
{
	PyObject *clone;
	PyObject *tuple;
	int y = GET_YEAR(self);
	int m = GET_MONTH(self);
	int d = GET_DAY(self);
	int hh = DATE_GET_HOUR(self);
	int mm = DATE_GET_MINUTE(self);
	int ss = DATE_GET_SECOND(self);
	int us = DATE_GET_MICROSECOND(self);
	PyObject *tzinfo = self->tzinfo;

	if (! PyArg_ParseTupleAndKeywords(args, kw, "|iiiiiiiO:replace",
					  datetimetz_kws,
					  &y, &m, &d, &hh, &mm, &ss, &us,
					  &tzinfo))
		return NULL;
	tuple = Py_BuildValue("iiiiiiiO", y, m, d, hh, mm, ss, us, tzinfo);
	if (tuple == NULL)
		return NULL;
	clone = datetimetz_new(self->ob_type, tuple, NULL);
	Py_DECREF(tuple);
	return clone;
}

static PyObject *
datetimetz_astimezone(PyDateTime_DateTimeTZ *self, PyObject *args,
		      PyObject *kw)
{
	int y = GET_YEAR(self);
	int m = GET_MONTH(self);
	int d = GET_DAY(self);
	int hh = DATE_GET_HOUR(self);
	int mm = DATE_GET_MINUTE(self);
	int ss = DATE_GET_SECOND(self);
	int us = DATE_GET_MICROSECOND(self);

	PyObject *result;
	PyObject *temp;
	int selfoff, resoff, dst1, dst2;
	int none;
	int delta;

	PyObject *tzinfo;
	static char *keywords[] = {"tz", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kw, "O:astimezone", keywords,
					  &tzinfo))
		return NULL;
	if (check_tzinfo_subclass(tzinfo) < 0)
		return NULL;

        /* Don't call utcoffset unless necessary. */
	result = new_datetimetz(y, m, d, hh, mm, ss, us, tzinfo);
	if (result == NULL ||
	    tzinfo == Py_None ||
	    self->tzinfo == Py_None ||
	    self->tzinfo == tzinfo)
		return result;

        /* Get the offsets.  If either object turns out to be naive, again
         * there's no conversion of date or time fields.
         */
	selfoff = call_utcoffset(self->tzinfo, (PyObject *)self, &none);
	if (selfoff == -1 && PyErr_Occurred())
		goto Fail;
	if (none)
		return result;

	resoff = call_utcoffset(tzinfo, result, &none);
	if (resoff == -1 && PyErr_Occurred())
		goto Fail;
	if (none)
		return result;

	/* See the long comment block at the end of this file for an
	 * explanation of this algorithm.  That it always works requires a
	 * pretty intricate proof.  There are many equivalent ways to code
	 * up the proof as an algorithm.  This way favors calling dst() over
	 * calling utcoffset(), because "the usual" utcoffset() calls dst()
	 * itself, and calling the latter instead saves a Python-level
	 * function call.  This way of coding it also follows the proof
	 * closely, w/ x=self, y=result, z=result, and z'=temp.
	 */
	dst1 = call_dst(tzinfo, result, &none);
	if (dst1 == -1 && PyErr_Occurred())
		goto Fail;
	if (none) {
		PyErr_SetString(PyExc_ValueError, "astimezone(): utcoffset() "
		"returned a duration but dst() returned None");
		goto Fail;
	}
	delta = resoff - dst1 - selfoff;
	if (delta) {
		mm += delta;
		if ((mm < 0 || mm >= 60) &&
		    normalize_datetime(&y, &m, &d, &hh, &mm, &ss, &us) < 0)
			goto Fail;
		temp = new_datetimetz(y, m, d, hh, mm, ss, us, tzinfo);
		if (temp == NULL)
			goto Fail;
		Py_DECREF(result);
		result = temp;

		dst1 = call_dst(tzinfo, result, &none);
		if (dst1 == -1 && PyErr_Occurred())
			goto Fail;
		if (none)
			goto Inconsistent;
	}
	if (dst1 == 0)
		return result;

	mm += dst1;
	if ((mm < 0 || mm >= 60) &&
	    normalize_datetime(&y, &m, &d, &hh, &mm, &ss, &us) < 0)
		goto Fail;
	temp = new_datetimetz(y, m, d, hh, mm, ss, us, tzinfo);
	if (temp == NULL)
		goto Fail;

	dst2 = call_dst(tzinfo, temp, &none);
	if (dst2 == -1 && PyErr_Occurred()) {
		Py_DECREF(temp);
		goto Fail;
	}
	if (none) {
		Py_DECREF(temp);
		goto Inconsistent;
	}

	if (dst1 == dst2) {
		/* The normal case:  we want temp, not result. */
		Py_DECREF(result);
		result = temp;
	}
	else {
		/* The "unspellable hour" at the end of DST. */
		Py_DECREF(temp);
	}
	return result;

Inconsistent:
	PyErr_SetString(PyExc_ValueError, "astimezone(): tz.dst() gave"
			"inconsistent results; cannot convert");

	/* fall thru to failure */
Fail:
	Py_DECREF(result);
	return NULL;
}

static PyObject *
datetimetz_timetuple(PyDateTime_DateTimeTZ *self)
{
	int dstflag = -1;

	if (self->tzinfo != Py_None) {
		int none;

		dstflag = call_dst(self->tzinfo, (PyObject *)self, &none);
		if (dstflag == -1 && PyErr_Occurred())
			return NULL;

		if (none)
			dstflag = -1;
		else if (dstflag != 0)
			dstflag = 1;

	}
	return build_struct_time(GET_YEAR(self),
				 GET_MONTH(self),
				 GET_DAY(self),
				 DATE_GET_HOUR(self),
				 DATE_GET_MINUTE(self),
				 DATE_GET_SECOND(self),
				 dstflag);
}

static PyObject *
datetimetz_utctimetuple(PyDateTime_DateTimeTZ *self)
{
	int y = GET_YEAR(self);
	int m = GET_MONTH(self);
	int d = GET_DAY(self);
	int hh = DATE_GET_HOUR(self);
	int mm = DATE_GET_MINUTE(self);
	int ss = DATE_GET_SECOND(self);
	int us = 0;	/* microseconds are ignored in a timetuple */
	int offset = 0;

	if (self->tzinfo != Py_None) {
		int none;

		offset = call_utcoffset(self->tzinfo, (PyObject *)self, &none);
		if (offset == -1 && PyErr_Occurred())
			return NULL;
	}
	/* Even if offset is 0, don't call timetuple() -- tm_isdst should be
	 * 0 in a UTC timetuple regardless of what dst() says.
	 */
	if (offset) {
		/* Subtract offset minutes & normalize. */
		int stat;

		mm -= offset;
		stat = normalize_datetime(&y, &m, &d, &hh, &mm, &ss, &us);
		if (stat < 0) {
			/* At the edges, it's possible we overflowed
			 * beyond MINYEAR or MAXYEAR.
			 */
			if (PyErr_ExceptionMatches(PyExc_OverflowError))
				PyErr_Clear();
			else
				return NULL;
		}
	}
	return build_struct_time(y, m, d, hh, mm, ss, 0);
}

static PyObject *
datetimetz_gettimetz(PyDateTime_DateTimeTZ *self)
{
	return new_timetz(DATE_GET_HOUR(self),
			  DATE_GET_MINUTE(self),
			  DATE_GET_SECOND(self),
			  DATE_GET_MICROSECOND(self),
			  self->tzinfo);
}

/*
 * Pickle support.  Quite a maze!
 */

/* Let basestate be the state string returned by datetime_getstate.
 * If tzinfo is None, this returns (basestate,), else (basestate, tzinfo).
 * So it's a tuple in any (non-error) case.
 */
static PyObject *
datetimetz_getstate(PyDateTime_DateTimeTZ *self)
{
	PyObject *basestate;
	PyObject *result = NULL;

	basestate = datetime_getstate((PyDateTime_DateTime *)self);
	if (basestate != NULL) {
		if (self->tzinfo == Py_None)
			result = Py_BuildValue("(O)", basestate);
		else
			result = Py_BuildValue("OO", basestate, self->tzinfo);
		Py_DECREF(basestate);
	}
	return result;
}

static PyObject *
datetimetz_setstate(PyDateTime_DateTimeTZ *self, PyObject *state)
{
	PyObject *temp;
	PyObject *basestate;
	PyObject *tzinfo = Py_None;

	if (! PyArg_ParseTuple(state, "O!|O:__setstate__",
			       &PyString_Type, &basestate,
			       &tzinfo))
		return NULL;
	temp = datetime_setstate((PyDateTime_DateTime *)self, basestate);
	if (temp == NULL)
		return NULL;
	Py_DECREF(temp);

	Py_INCREF(tzinfo);
	Py_XDECREF(self->tzinfo);
	self->tzinfo = tzinfo;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
datetimetz_pickler(PyObject *module, PyDateTime_DateTimeTZ *datetimetz)
{
	PyObject *state;
	PyObject *result = NULL;

	if (! PyDateTimeTZ_CheckExact(datetimetz)) {
		PyErr_Format(PyExc_TypeError,
			     "bad type passed to datetimetz pickler: %s",
			     datetimetz->ob_type->tp_name);
		return NULL;
	}
	state = datetimetz_getstate(datetimetz);
	if (state) {
		result = Py_BuildValue("O(O)",
				       datetimetz_unpickler_object,
				       state);
		Py_DECREF(state);
	}
	return result;
}

static PyObject *
datetimetz_unpickler(PyObject *module, PyObject *arg)
{
	PyDateTime_DateTimeTZ *self;

	self = PyObject_New(PyDateTime_DateTimeTZ, &PyDateTime_DateTimeTZType);
	if (self != NULL) {
		PyObject *res;

		self->tzinfo = NULL;
		res = datetimetz_setstate(self, arg);
		if (res == NULL) {
			Py_DECREF(self);
			return NULL;
		}
		Py_DECREF(res);
	}
	return (PyObject *)self;
}


static PyMethodDef datetimetz_methods[] = {
	/* Class methods: */
	/* Inherited: combine(), utcnow(), utcfromtimestamp() */

	{"now",         (PyCFunction)datetimetz_now,
	 METH_KEYWORDS | METH_CLASS,
	 PyDoc_STR("[tzinfo] -> new datetimetz with local day and time.")},

	{"fromtimestamp", (PyCFunction)datetimetz_fromtimestamp,
	 METH_KEYWORDS | METH_CLASS,
	 PyDoc_STR("timestamp[, tzinfo] -> local time from POSIX timestamp.")},

	/* Instance methods: */
	/* Inherited:  date(), time(), ctime(). */
	{"timetuple",   (PyCFunction)datetimetz_timetuple, METH_NOARGS,
         PyDoc_STR("Return time tuple, compatible with time.localtime().")},

	{"utctimetuple",   (PyCFunction)datetimetz_utctimetuple, METH_NOARGS,
         PyDoc_STR("Return UTC time tuple, compatible with time.localtime().")},

	{"timetz",   (PyCFunction)datetimetz_gettimetz, METH_NOARGS,
         PyDoc_STR("Return timetz object with same hour, minute, second, "
         	   "microsecond, and tzinfo.")},

	{"isoformat",   (PyCFunction)datetimetz_isoformat, METH_KEYWORDS,
	 PyDoc_STR("[sep] -> string in ISO 8601 format, "
	 	   "YYYY-MM-DDTHH:MM:SS[.mmmmmm][+HH:MM].\n\n"
	 	   "sep is used to separate the year from the time, and "
	 	   "defaults to 'T'.")},

	{"utcoffset",	(PyCFunction)datetimetz_utcoffset, METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.utcoffset(self).")},

	{"tzname",	(PyCFunction)datetimetz_tzname,	METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.tzname(self).")},

	{"dst",		(PyCFunction)datetimetz_dst, METH_NOARGS,
	 PyDoc_STR("Return self.tzinfo.dst(self).")},

	{"replace",     (PyCFunction)datetimetz_replace,	METH_KEYWORDS,
	 PyDoc_STR("Return datetimetz with new specified fields.")},

	{"astimezone",  (PyCFunction)datetimetz_astimezone, METH_KEYWORDS,
	 PyDoc_STR("tz -> convert to local time in new timezone tz\n")},

	{"__setstate__", (PyCFunction)datetimetz_setstate, METH_O,
	 PyDoc_STR("__setstate__(state)")},

	{"__getstate__", (PyCFunction)datetimetz_getstate, METH_NOARGS,
	 PyDoc_STR("__getstate__() -> state")},
	{NULL,	NULL}
};

static char datetimetz_doc[] =
PyDoc_STR("date/time type.");

static PyNumberMethods datetimetz_as_number = {
	datetimetz_add,				/* nb_add */
	datetimetz_subtract,			/* nb_subtract */
	0,					/* nb_multiply */
	0,					/* nb_divide */
	0,					/* nb_remainder */
	0,					/* nb_divmod */
	0,					/* nb_power */
	0,					/* nb_negative */
	0,					/* nb_positive */
	0,					/* nb_absolute */
	0,					/* nb_nonzero */
};

statichere PyTypeObject PyDateTime_DateTimeTZType = {
	PyObject_HEAD_INIT(NULL)
	0,					/* ob_size */
	"datetime.datetimetz",			/* tp_name */
	sizeof(PyDateTime_DateTimeTZ),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)datetimetz_dealloc,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	(reprfunc)datetimetz_repr,		/* tp_repr */
	&datetimetz_as_number,			/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,              			/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
        Py_TPFLAGS_BASETYPE,			/* tp_flags */
	datetimetz_doc,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	datetimetz_methods,			/* tp_methods */
	0,					/* tp_members */
	datetimetz_getset,			/* tp_getset */
	&PyDateTime_DateTimeType,		/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	0,					/* tp_alloc */
	datetimetz_new,				/* tp_new */
	_PyObject_Del,				/* tp_free */
};

/* ---------------------------------------------------------------------------
 * Module methods and initialization.
 */

static PyMethodDef module_methods[] = {
	/* Private functions for pickling support, registered with the
	 * copy_reg module by the module init function.
	 */
	{"_date_pickler",	(PyCFunction)date_pickler,	METH_O, NULL},
	{"_date_unpickler",	(PyCFunction)date_unpickler, 	METH_O, NULL},
	{"_datetime_pickler",	(PyCFunction)datetime_pickler,	METH_O, NULL},
	{"_datetime_unpickler",	(PyCFunction)datetime_unpickler,METH_O, NULL},
	{"_datetimetz_pickler",	(PyCFunction)datetimetz_pickler,METH_O, NULL},
	{"_datetimetz_unpickler",(PyCFunction)datetimetz_unpickler,METH_O, NULL},
	{"_time_pickler",	(PyCFunction)time_pickler,	METH_O, NULL},
	{"_time_unpickler",	(PyCFunction)time_unpickler,	METH_O, NULL},
	{"_timetz_pickler",	(PyCFunction)timetz_pickler,	METH_O, NULL},
	{"_timetz_unpickler",	(PyCFunction)timetz_unpickler,	METH_O, NULL},
	{"_tzinfo_pickler",	(PyCFunction)tzinfo_pickler,	METH_O, NULL},
	{"_tzinfo_unpickler",	(PyCFunction)tzinfo_unpickler,	METH_NOARGS,
	 NULL},
	{NULL, NULL}
};

PyMODINIT_FUNC
initdatetime(void)
{
	PyObject *m;	/* a module object */
	PyObject *d;	/* its dict */
	PyObject *x;

	/* Types that use __reduce__ for pickling need to set the following
	 * magical attr in the type dict, with a true value.
	 */
	PyObject *safepickle = PyString_FromString("__safe_for_unpickling__");
	if (safepickle == NULL)
		return;

	m = Py_InitModule3("datetime", module_methods,
			   "Fast implementation of the datetime type.");

	if (PyType_Ready(&PyDateTime_DateType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_DateTimeType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_DeltaType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_TimeType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_TZInfoType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_TimeTZType) < 0)
		return;
	if (PyType_Ready(&PyDateTime_DateTimeTZType) < 0)
		return;

	/* Pickling support, via registering functions with copy_reg. */
	{
		PyObject *pickler;
		PyObject *copyreg = PyImport_ImportModule("copy_reg");

		if (copyreg == NULL) return;

		pickler = PyObject_GetAttrString(m, "_date_pickler");
		if (pickler == NULL) return;
		date_unpickler_object = PyObject_GetAttrString(m,
						"_date_unpickler");
		if (date_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_DateType,
	    				pickler,
		                    	date_unpickler_object);
		if (x == NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		pickler = PyObject_GetAttrString(m, "_datetime_pickler");
		if (pickler == NULL) return;
		datetime_unpickler_object = PyObject_GetAttrString(m,
						"_datetime_unpickler");
		if (datetime_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_DateTimeType,
	    				pickler,
		                    	datetime_unpickler_object);
		if (x == NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		pickler = PyObject_GetAttrString(m, "_time_pickler");
		if (pickler == NULL) return;
		time_unpickler_object = PyObject_GetAttrString(m,
						"_time_unpickler");
		if (time_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_TimeType,
	    				pickler,
		                	time_unpickler_object);
		if (x == NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		pickler = PyObject_GetAttrString(m, "_timetz_pickler");
		if (pickler == NULL) return;
		timetz_unpickler_object = PyObject_GetAttrString(m,
						"_timetz_unpickler");
		if (timetz_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_TimeTZType,
	    				pickler,
		                	timetz_unpickler_object);
		if (x == NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		pickler = PyObject_GetAttrString(m, "_tzinfo_pickler");
		if (pickler == NULL) return;
		tzinfo_unpickler_object = PyObject_GetAttrString(m,
							"_tzinfo_unpickler");
		if (tzinfo_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_TZInfoType,
	    				pickler,
		        		tzinfo_unpickler_object);
		if (x== NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		pickler = PyObject_GetAttrString(m, "_datetimetz_pickler");
		if (pickler == NULL) return;
		datetimetz_unpickler_object = PyObject_GetAttrString(m,
						 "_datetimetz_unpickler");
		if (datetimetz_unpickler_object == NULL) return;
	    	x = PyObject_CallMethod(copyreg, "pickle", "OOO",
	    				&PyDateTime_DateTimeTZType,
	    				pickler,
		                	datetimetz_unpickler_object);
		if (x== NULL) return;
		Py_DECREF(x);
		Py_DECREF(pickler);

		Py_DECREF(copyreg);
	}

	/* timedelta values */
	d = PyDateTime_DeltaType.tp_dict;

	if (PyDict_SetItem(d, safepickle, Py_True) < 0)
		return;

	x = new_delta(0, 0, 1, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(-MAX_DELTA_DAYS, 0, 0, 0);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(MAX_DELTA_DAYS, 24*3600-1, 1000000-1, 0);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	/* date values */
	d = PyDateTime_DateType.tp_dict;

	x = new_date(1, 1, 1);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_date(MAXYEAR, 12, 31);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(1, 0, 0, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	/* datetime values */
	d = PyDateTime_DateTimeType.tp_dict;

	x = new_datetime(1, 1, 1, 0, 0, 0, 0);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_datetime(MAXYEAR, 12, 31, 23, 59, 59, 999999);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(0, 0, 1, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	/* time values */
	d = PyDateTime_TimeType.tp_dict;

	x = new_time(0, 0, 0, 0);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_time(23, 59, 59, 999999);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(0, 0, 1, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	/* timetz values */
	d = PyDateTime_TimeTZType.tp_dict;

	x = new_timetz(0, 0, 0, 0, Py_None);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_timetz(23, 59, 59, 999999, Py_None);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(0, 0, 1, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	/* datetimetz values */
	d = PyDateTime_DateTimeTZType.tp_dict;

	x = new_datetimetz(1, 1, 1, 0, 0, 0, 0, Py_None);
	if (x == NULL || PyDict_SetItemString(d, "min", x) < 0)
		return;
	Py_DECREF(x);

	x = new_datetimetz(MAXYEAR, 12, 31, 23, 59, 59, 999999, Py_None);
	if (x == NULL || PyDict_SetItemString(d, "max", x) < 0)
		return;
	Py_DECREF(x);

	x = new_delta(0, 0, 1, 0);
	if (x == NULL || PyDict_SetItemString(d, "resolution", x) < 0)
		return;
	Py_DECREF(x);

	Py_DECREF(safepickle);

	/* module initialization */
	PyModule_AddIntConstant(m, "MINYEAR", MINYEAR);
	PyModule_AddIntConstant(m, "MAXYEAR", MAXYEAR);

	Py_INCREF(&PyDateTime_DateType);
	PyModule_AddObject(m, "date", (PyObject *) &PyDateTime_DateType);

	Py_INCREF(&PyDateTime_DateTimeType);
	PyModule_AddObject(m, "datetime",
			   (PyObject *) &PyDateTime_DateTimeType);

	Py_INCREF(&PyDateTime_DeltaType);
	PyModule_AddObject(m, "timedelta", (PyObject *) &PyDateTime_DeltaType);

	Py_INCREF(&PyDateTime_TimeType);
	PyModule_AddObject(m, "time", (PyObject *) &PyDateTime_TimeType);

	Py_INCREF(&PyDateTime_TZInfoType);
	PyModule_AddObject(m, "tzinfo", (PyObject *) &PyDateTime_TZInfoType);

	Py_INCREF(&PyDateTime_TimeTZType);
	PyModule_AddObject(m, "timetz", (PyObject *) &PyDateTime_TimeTZType);

	Py_INCREF(&PyDateTime_DateTimeTZType);
	PyModule_AddObject(m, "datetimetz",
			   (PyObject *)&PyDateTime_DateTimeTZType);

	/* A 4-year cycle has an extra leap day over what we'd get from
	 * pasting together 4 single years.
	 */
	assert(DI4Y == 4 * 365 + 1);
	assert(DI4Y == days_before_year(4+1));

	/* Similarly, a 400-year cycle has an extra leap day over what we'd
	 * get from pasting together 4 100-year cycles.
	 */
	assert(DI400Y == 4 * DI100Y + 1);
	assert(DI400Y == days_before_year(400+1));

	/* OTOH, a 100-year cycle has one fewer leap day than we'd get from
	 * pasting together 25 4-year cycles.
	 */
	assert(DI100Y == 25 * DI4Y - 1);
	assert(DI100Y == days_before_year(100+1));

	us_per_us = PyInt_FromLong(1);
	us_per_ms = PyInt_FromLong(1000);
	us_per_second = PyInt_FromLong(1000000);
	us_per_minute = PyInt_FromLong(60000000);
	seconds_per_day = PyInt_FromLong(24 * 3600);
	if (us_per_us == NULL || us_per_ms == NULL || us_per_second == NULL ||
	    us_per_minute == NULL || seconds_per_day == NULL)
		return;

	/* The rest are too big for 32-bit ints, but even
	 * us_per_week fits in 40 bits, so doubles should be exact.
	 */
	us_per_hour = PyLong_FromDouble(3600000000.0);
	us_per_day = PyLong_FromDouble(86400000000.0);
	us_per_week = PyLong_FromDouble(604800000000.0);
	if (us_per_hour == NULL || us_per_day == NULL || us_per_week == NULL)
		return;
}

/* ---------------------------------------------------------------------------
Some time zone algebra.  For a datetimetz x, let
    x.n = x stripped of its timezone -- its naive time.
    x.o = x.utcoffset(), and assuming that doesn't raise an exception or
          return None
    x.d = x.dst(), and assuming that doesn't raise an exception or
          return None
    x.s = x's standard offset, x.o - x.d

Now some derived rules, where k is a duration (timedelta).

1. x.o = x.s + x.d
   This follows from the definition of x.s.

2. If x and y have the same tzinfo member, x.s = y.s.
   This is actually a requirement, an assumption we need to make about
   sane tzinfo classes.

3. The naive UTC time corresponding to x is x.n - x.o.
   This is again a requirement for a sane tzinfo class.

4. (x+k).s = x.s
   This follows from #2, and that datimetimetz+timedelta preserves tzinfo.

5. (x+k).n = x.n + k
   Again follows from how arithmetic is defined.

Now we can explain x.astimezone(tz).  Let's assume it's an interesting case
(meaning that the various tzinfo methods exist, and don't blow up or return
None when called).

The function wants to return a datetimetz y with timezone tz, equivalent to x.

By #3, we want

    y.n - y.o = x.n - x.o                       [1]

The algorithm starts by attaching tz to x.n, and calling that y.  So
x.n = y.n at the start.  Then it wants to add a duration k to y, so that [1]
becomes true; in effect, we want to solve [2] for k:

   (y+k).n - (y+k).o = x.n - x.o                [2]

By #1, this is the same as

   (y+k).n - ((y+k).s + (y+k).d) = x.n - x.o    [3]

By #5, (y+k).n = y.n + k, which equals x.n + k because x.n=y.n at the start.
Substituting that into [3],

   x.n + k - (y+k).s - (y+k).d = x.n - x.o; the x.n terms cancel, leaving
   k - (y+k).s - (y+k).d = - x.o; rearranging,
   k = (y+k).s - x.o - (y+k).d; by #4, (y+k).s == y.s, so
   k = y.s - x.o - (y+k).d; then by #1, y.s = y.o - y.d, so
   k = y.o - y.d - x.o - (y+k).d

On the RHS, (y+k).d can't be computed directly, but all the rest can be, and
we approximate k by ignoring the (y+k).d term at first.  Note that k can't
be very large, since all offset-returning methods return a duration of
magnitude less than 24 hours.  For that reason, if y is firmly in std time,
(y+k).d must be 0, so ignoring it has no consequence then.

In any case, the new value is

    z = y + y.o - y.d - x.o                     [4]

It's helpful to step back at look at [4] from a higher level:  rewrite it as

    z = (y - x.o) + (y.o - y.d)

(y - x.o).n = [by #5] y.n - x.o = [since y.n=x.n] x.n - x.o = [by #3] x's
UTC equivalent time.  So the y-x.o part essentially converts x to UTC.  Then
the y.o-y.d part essentially converts x's UTC equivalent into tz's standard
time (y.o-y.d=y.s by #1).

At this point, if

    z.n - z.o = x.n - x.o                       [5]

we have an equivalent time, and are almost done.  The insecurity here is
at the start of daylight time.  Picture US Eastern for concreteness.  The wall
time jumps from 1:59 to 3:00, and wall hours of the form 2:MM don't make good
sense then.  A sensible Eastern tzinfo class will consider such a time to be
EDT (because it's "after 2"), which is a redundant spelling of 1:MM EST on the
day DST starts.  We want to return the 1:MM EST spelling because that's
the only spelling that makes sense on the local wall clock.

In fact, if [5] holds at this point, we do have the standard-time spelling,
but that takes a bit of proof.  We first prove a stronger result.  What's the
difference between the LHS and RHS of [5]?  Let

    diff = (x.n - x.o) - (z.n - z.o)            [6]

Now
    z.n =                       by [4]
    (y + y.o - y.d - x.o).n =   by #5
    y.n + y.o - y.d - x.o =     since y.n = x.n
    x.n + y.o - y.d - x.o =     since y.o = y.s + y.d by #1
    x.n + (y.s + y.d) - y.d - x.o =     cancelling the y.d terms
    x.n + y.s - x.o =           since z and y are have the same tzinfo member,
                                y.s = z.s by #2
    x.n + z.s - x.o

Plugging that back into [6] gives

    diff =
    (x.n - x.o) - ((x.n + z.s - x.o) - z.o)     = expanding
    x.n - x.o - x.n - z.s + x.o + z.o           = cancelling
    - z.s + z.o                                 = by #2
    z.d

So diff = z.d.

If [5] is true now, diff = 0, so z.d = 0 too, and we have the standard-time
spelling we wanted in the endcase described above.  We're done.

If [5] is not true now, diff = z.d != 0, and z.d is the offset we need to
add to z (in effect, z is in tz's standard time, and we need to shift the
offset into tz's daylight time).

Let

    z' = z + z.d = z + diff                     [7]

and we can again ask whether

    z'.n - z'.o = x.n - x.o                     [8]

If so, we're done.  If not, the tzinfo class is insane, or we're trying to
convert to the hour that can't be spelled in tz.  This also requires a
bit of proof.  As before, let's compute the difference between the LHS and
RHS of [8] (and skipping some of the justifications for the kinds of
substitutions we've done several times already):

    diff' = (x.n - x.o) - (z'.n - z'.o) =       replacing z'.n via [7]
            (x.n - x.o) - (z.n + diff - z'.o) = replacing diff via [6]
            (x.n - x.o) - (z.n + (x.n - x.o) - (z.n - z.o) - z'.o) =
            x.n - x.o - z.n - x.n + x.o + z.n - z.o + z'.o = cancel x.n
            - x.o - z.n + x.o + z.n - z.o + z'.o = cancel x.o
            - z.n + z.n - z.o + z'.o =          cancel z.n
            - z.o + z'.o =                      #1 twice
            -z.s - z.d + z'.s + z'.d =          z and z' have same tzinfo
            z'.d - z.d

So z' is UTC-equivalent to x iff z'.d = z.d at this point.  If they are equal,
we've found the UTC-equivalent so are done.

How could they differ?  z' = z + z.d [7], so merely moving z' by a dst()
offset, and starting *from* a time already in DST (we know z.d != 0), would
have to change the result dst() returns:  we start in DST, and moving a
little further into it takes us out of DST.

There's (only) one sane case where this can happen:  at the end of DST,
there's an hour in UTC with no spelling in a hybrid tzinfo class.  In US
Eastern, that's 6:MM UTC = 1:MM EST = 2:MM EDT.  During that hour, on an
Eastern clock 1:MM is taken as being in daylight time (5:MM UTC), but 2:MM is
taken as being in standard time (7:MM UTC).  There is no local time mapping to
6:MM UTC.  The local clock jumps from 1:59 back to 1:00 again, and repeats the
1:MM hour in standard time.  Since that's what the local clock *does*, we want
to map both UTC hours 5:MM and 6:MM to 1:MM Eastern.  The result is ambiguous
in local time, but so it goes -- it's the way the local clock works.

When x = 6:MM UTC is the input to this algorithm, x.o=0, y.o=-5 and y.d=0,
so z=1:MM.  z.d=60 (minutes) then, so [5] doesn't hold and we keep going.
z' = z + z.d = 2:MM then, and z'.d=0, and z'.d - z.d = -60 != 0 so [8]
(correctly) concludes that z' is not UTC-equivalent to x.

Because we know z.d said z was in daylight time (else [5] would have held and
we would have stopped then), and we know z.d != z'.d (else [8] would have held
and we we have stopped then), and there are only 2 possible values dst() can
return in Eastern, it follows that z'.d must be 0 (which it is in the example,
but the reasoning doesn't depend on the example -- it depends on there being
two possible dst() outcomes, one zero and the other non-zero).  Therefore
z' must be in standard time, and is not the spelling we want in this case.
z is in daylight time, and is the spelling we want.  Note again that z is
not UTC-equivalent as far as the hybrid tzinfo class is concerned (because
it takes z as being in standard time rather than the daylight time we intend
here), but returning it gives the real-life "local clock repeats an hour"
behavior when mapping the "unspellable" UTC hour into tz.
--------------------------------------------------------------------------- */
