/* _strptime_impl accelerator C extension module. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include <ctype.h>
#include <string.h>
#include <limits.h>

/*[clinic input]
module _strptime_impl
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f7541041c3424c24]*/

#include "clinic/_strptimemodule.c.h"

/* ========================== helpers ========================== */

/* Parsed fields accumulated while walking the format string. */
typedef struct {
    int year;          /* -1 = not set */
    int month;         /* 1-12, default 1 */
    int day;           /* 1-31, default 1 */
    int hour;          /* 0-23, default 0 */
    int minute;        /* 0-59, default 0 */
    int second;        /* 0-61, default 0 */
    int weekday;       /* 0-6 (Mon=0), -1 = not set */
    int julian;        /* 1-366, -1 = not set */
    int tz;            /* -1 = not set, 0 = no DST, 1 = DST */
    int fraction;      /* microseconds, 0 */
    int gmtoff;        /* seconds east of UTC, INT_MIN = not set */
    int gmtoff_fraction; /* microseconds part of gmtoff, 0 */
    int iso_year;      /* -1 = not set */
    int iso_week;      /* -1 = not set */
    int week_of_year;  /* -1 = not set */
    int week_of_year_start; /* 0 = Mon, 6 = Sun, -1 = not set */
    int century;       /* -1 = not set */
    int has_year;      /* whether %Y was seen */
    int has_short_year;/* whether %y was seen */
    int day_of_month_in_format; /* whether %d was seen */
    int year_in_format;         /* whether %Y/%y/%G was seen */
    int colon_z_in_format;      /* whether %:z was seen */
} ParsedTime;

static void
parsed_time_init(ParsedTime *pt)
{
    pt->year = -1;
    pt->month = 1;
    pt->day = 1;
    pt->hour = 0;
    pt->minute = 0;
    pt->second = 0;
    pt->weekday = -1;
    pt->julian = INT_MIN;
    pt->tz = -1;
    pt->fraction = 0;
    pt->gmtoff = INT_MIN;
    pt->gmtoff_fraction = 0;
    pt->iso_year = -1;
    pt->iso_week = -1;
    pt->week_of_year = -1;
    pt->week_of_year_start = -1;
    pt->century = -1;
    pt->has_year = 0;
    pt->has_short_year = 0;
    pt->day_of_month_in_format = 0;
    pt->year_in_format = 0;
    pt->colon_z_in_format = 0;
}

/* Parse up to max_digits decimal digits from s at position *pos.
   Stores the integer value in *out. Returns number of digits consumed,
   or 0 on failure. Does NOT advance *pos. */
static int
parse_digits(const char *s, Py_ssize_t len, Py_ssize_t pos,
             int min_digits, int max_digits, int *out)
{
    int val = 0;
    int count = 0;
    while (count < max_digits && pos + count < len) {
        char c = s[pos + count];
        if (c < '0' || c > '9') {
            break;
        }
        val = val * 10 + (c - '0');
        count++;
    }
    if (count < min_digits) {
        return 0;
    }
    *out = val;
    return count;
}

/* Check if character at pos is ASCII whitespace */
static int
is_ascii_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '\f' || c == '\v';
}

/* ========================== date math ========================== */

static int
is_leap_year(int year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/* Day of year (1-based) for a given y/m/d */
static int
day_of_year(int year, int month, int day)
{
    static const int cum[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
    int doy = cum[month - 1] + day;
    if (month > 2 && is_leap_year(year)) {
        doy++;
    }
    return doy;
}

/* Compute weekday (0=Mon, 6=Sun) from y/m/d using Tomohiko Sakamoto's algo */
static int
weekday_from_date(int y, int m, int d)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        y--;
    }
    int w = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    /* Sakamoto gives 0=Sun, we want 0=Mon */
    return (w + 6) % 7;
}

/* Compute ordinal (days since 0001-01-01, where 0001-01-01 = ordinal 1).
   This matches datetime.date(year, month, day).toordinal(). */
static long
date_to_ordinal(int year, int month, int day)
{
    /* Days before year: 365*y + leaps */
    long y = (long)year - 1;
    long days_before_year = y * 365 + y / 4 - y / 100 + y / 400;

    /* Days before month in this year */
    static const int days_before_month[13] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    long dbm = days_before_month[month];
    if (month > 2 && is_leap_year(year)) {
        dbm++;
    }

    return days_before_year + dbm + day;
}

/* Convert ordinal back to (year, month, day).
   Inverse of date_to_ordinal(). */
static void
ordinal_to_date(long ordinal, int *year, int *month, int *day)
{
    /* Algorithm from the CPython datetime module (Lib/datetime.py) */
    long n = ordinal - 1;  /* 0-based day count */
    long n400 = n / 146097;
    n = n % 146097;
    long n100 = n / 36524;
    n = n % 36524;
    long n4 = n / 1461;
    n = n % 1461;
    long n1 = n / 365;
    n = n % 365;

    *year = (int)(n400 * 400 + n100 * 100 + n4 * 4 + n1 + 1);

    /* If n1 == 4 or n100 == 4, then the ordinal is the last day of a
       leap year (Dec 31), and the year must be backed up by one. */
    if (n1 == 4 || n100 == 4) {
        *year -= 1;
        n = 365; /* Dec 31 */
    }

    /* n is now the 0-based day within the year */
    static const int days_before_month[13] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int leap = is_leap_year(*year);

    /* Find month */
    int m;
    for (m = 12; m >= 1; m--) {
        int dbm = days_before_month[m];
        if (m > 2 && leap) {
            dbm++;
        }
        if (n >= dbm) {
            *month = m;
            *day = (int)(n - dbm + 1);
            return;
        }
    }
    /* Should never reach here */
    *month = 1;
    *day = (int)(n + 1);
}

/* Return the number of ISO weeks in a given ISO year (52 or 53). */
static int
iso_weeks_in_year(int iso_year)
{
    /* A year has 53 weeks iff Jan 1 is Thursday or Dec 31 is Thursday. */
    int jan1_wd = weekday_from_date(iso_year, 1, 1);  /* 0=Mon */
    int dec31_wd = weekday_from_date(iso_year, 12, 31);
    if (jan1_wd == 3 || dec31_wd == 3) { /* Thursday = 3 */
        return 53;
    }
    return 52;
}

/* Convert ISO calendar (iso_year, iso_week, weekday_1based) to (year, month, day).
   weekday is 1=Monday through 7=Sunday.
   Returns 1 on success, 0 on error (exception set). */
static int
isocalendar_to_date(int iso_year, int iso_week, int iso_weekday,
                    int *year, int *month, int *day)
{
    /* Validate week number */
    if (iso_week > iso_weeks_in_year(iso_year)) {
        PyErr_Format(PyExc_ValueError,
                     "Invalid week: %d", iso_week);
        return 0;
    }

    /* ISO year starts on the Monday of the week containing Jan 4.
       Day 1 of ISO week 1 is the Monday on or before Jan 4. */
    long jan4_ord = date_to_ordinal(iso_year, 1, 4);
    /* Weekday of Jan 4: 0=Mon, 6=Sun */
    int jan4_weekday = (int)((jan4_ord - 1) % 7); /* ordinal 1 = Mon = 0 */
    /* Monday of ISO week 1 */
    long week1_monday = jan4_ord - jan4_weekday;
    /* Target ordinal */
    long target = week1_monday + (iso_week - 1) * 7 + (iso_weekday - 1);
    ordinal_to_date(target, year, month, day);
    return 1;
}

/* Julian day from U or W week number */
static int
calc_julian_from_U_or_W(int year, int week_of_year, int day_of_week,
                         int week_starts_Mon)
{
    int first_weekday = weekday_from_date(year, 1, 1);
    if (!week_starts_Mon) {
        first_weekday = (first_weekday + 1) % 7;
        day_of_week = (day_of_week + 1) % 7;
    }
    int week_0_length = (7 - first_weekday) % 7;
    if (week_of_year == 0) {
        return 1 + day_of_week - first_weekday;
    }
    else {
        int days_to_week = week_0_length + (7 * (week_of_year - 1));
        return 1 + days_to_week + day_of_week;
    }
}

/* ========================== format parsing ========================== */

/* Parse the timezone offset component of %z / %:z.
   s points to the full data string, *ppos is current position.
   data_len is total length of data string.
   On success, advances *ppos and sets pt->gmtoff, pt->gmtoff_fraction.
   colon_z: 1 if %:z directive, 0 if %z directive.
   Returns 1 on success, 0 on parse failure, -1 to signal fallback. */
static int
parse_tz_offset(const char *s, Py_ssize_t len, Py_ssize_t *ppos,
                ParsedTime *pt, int colon_z)
{
    Py_ssize_t pos = *ppos;
    Py_ssize_t tz_start = pos;  /* remember start for error messages */

    /* Check for 'Z' */
    if (pos < len && s[pos] == 'Z') {
        pt->gmtoff = 0;
        pt->gmtoff_fraction = 0;
        *ppos = pos + 1;
        return 1;
    }

    /* %z is optional - no sign means no tz offset */
    if (pos >= len) {
        return 1;
    }

    int sign;
    if (s[pos] == '+') {
        sign = 1;
    } else if (s[pos] == '-') {
        sign = -1;
    } else {
        return 1;
    }
    pos++;

    int hours, minutes = 0, seconds = 0;
    int n;

    /* Hours: exactly 2 digits */
    n = parse_digits(s, len, pos, 2, 2, &hours);
    if (n != 2) {
        PyErr_SetString(PyExc_ValueError,
                        "time data does not match format");
        return 0;
    }
    pos += 2;

    /* Check for colon after hours */
    int has_colon = 0;
    if (pos < len && s[pos] == ':') {
        has_colon = 1;
        pos++;
    }
    else if (colon_z) {
        /* %:z requires colons; without one, just match +HH */
        goto done;
    }

    /* Minutes: exactly 2 digits */
    n = parse_digits(s, len, pos, 2, 2, &minutes);
    if (n != 2) {
        /* No minutes - just hours matched */
        goto done;
    }
    if (minutes > 59) {
        PyErr_SetString(PyExc_ValueError,
                        "time data does not match format");
        return 0;
    }
    pos += 2;

    /* Seconds: check for separator */
    if (pos < len) {
        int sec_sep_colon = (s[pos] == ':');

        if (sec_sep_colon) {
            /* Colon before seconds */
            if (!has_colon) {
                /* No colon after hours, but colon before seconds = inconsistent.
                   E.g. -0130:30 */
                Py_ssize_t tz_end = pos;
                while (tz_end < len && !is_ascii_space(s[tz_end])) {
                    tz_end++;
                }
                PyErr_Format(PyExc_ValueError,
                             "Inconsistent use of : in %.*s",
                             (int)(tz_end - tz_start),
                             s + tz_start);
                return 0;
            }
            pos++; /* consume colon */
        }
        else if (has_colon) {
            /* Colon after hours but no colon before seconds.
               For %:z, just stop — don't consume what follows.
               For %z, if there are digits, it's an inconsistency. */
            if (!colon_z && pos < len && s[pos] >= '0' && s[pos] <= '5') {
                /* Check if this really looks like seconds (2 digits) */
                int tmp;
                int tmp_n = parse_digits(s, len, pos, 2, 2, &tmp);
                if (tmp_n == 2) {
                    /* Has colon after hours, no colon before seconds = inconsistent.
                       E.g. -01:3030 */
                    Py_ssize_t tz_end = pos + tmp_n;
                    /* Include any trailing fraction */
                    while (tz_end < len && !is_ascii_space(s[tz_end])) {
                        tz_end++;
                    }
                    PyErr_Format(PyExc_ValueError,
                                 "Inconsistent use of : in %.*s",
                                 (int)(tz_end - tz_start),
                                 s + tz_start);
                    return 0;
                }
            }
            goto done;
        }
        else {
            /* No colons anywhere - check for seconds without separator */
            /* Only proceed if it looks like 2 digits for seconds */
        }

        /* Try to parse seconds digits */
        int sec_n = parse_digits(s, len, pos, 2, 2, &seconds);
        if (sec_n == 2 && seconds <= 59) {
            pos += 2;

            /* Fractional seconds */
            if (pos < len && s[pos] == '.') {
                int frac;
                int frac_n = parse_digits(s, len, pos + 1, 1, 6, &frac);
                if (frac_n == 0) {
                    /* Decimal point not followed by digits */
                    PyErr_SetString(PyExc_ValueError,
                                    "time data does not match format");
                    return 0;
                }
                /* Check for too many digits */
                if (pos + 1 + frac_n < len &&
                    s[pos + 1 + frac_n] >= '0' &&
                    s[pos + 1 + frac_n] <= '9') {
                    PyErr_SetString(PyExc_ValueError,
                                    "time data does not match format");
                    return 0;
                }
                pos += 1 + frac_n;
                for (int i = frac_n; i < 6; i++) {
                    frac *= 10;
                }
                pt->gmtoff_fraction = sign * frac;
            }
            /* Check for colon used as decimal separator */
            else if (pos < len && s[pos] == ':' &&
                     pos + 1 < len &&
                     s[pos + 1] >= '0' && s[pos + 1] <= '9') {
                PyErr_SetString(PyExc_ValueError,
                                "time data does not match format");
                return 0;
            }
        }
    }

done:
    pt->gmtoff = sign * (hours * 3600 + minutes * 60 + seconds);
    *ppos = pos;
    return 1;
}

/* Parse a single directive.
   Returns 1 on success (consumed chars stored in *consumed_out),
   0 on parse error, -1 to signal "fallback to Python". */
static int
parse_directive(const char *data, Py_ssize_t data_len, Py_ssize_t data_pos,
                const char *fmt, Py_ssize_t fmt_len, Py_ssize_t *fmt_pos,
                ParsedTime *pt, Py_ssize_t *consumed_out)
{
    Py_ssize_t fpos = *fmt_pos;
    /* fpos points to char after '%' */
    if (fpos >= fmt_len) {
        /* stray % at end of format - fall back to Python for proper error */
        return -1;
    }

    /* Skip modifier flags: -, _, 0, ^, # and width digits */
    while (fpos < fmt_len &&
           (fmt[fpos] == '-' || fmt[fpos] == '_' || fmt[fpos] == '0' ||
            fmt[fpos] == '^' || fmt[fpos] == '#')) {
        fpos++;
    }
    /* Skip width digits */
    while (fpos < fmt_len && fmt[fpos] >= '0' && fmt[fpos] <= '9') {
        fpos++;
    }

    if (fpos >= fmt_len) {
        /* stray % with only flags - fall back to Python for proper error */
        return -1;
    }

    char directive = fmt[fpos];
    fpos++;

    int val;
    int n;
    Py_ssize_t consumed = 0;

    switch (directive) {
    case 'Y': /* 4-digit year */
        n = parse_digits(data, data_len, data_pos, 4, 4, &val);
        if (n != 4) {
            goto match_fail;
        }
        pt->year = val;
        pt->has_year = 1;
        pt->year_in_format = 1;
        consumed = 4;
        break;

    case 'y': /* 2-digit year */
        n = parse_digits(data, data_len, data_pos, 2, 2, &val);
        if (n != 2) {
            goto match_fail;
        }
        pt->year = val;
        pt->has_short_year = 1;
        pt->year_in_format = 1;
        consumed = 2;
        break;

    case 'C': /* century, 2 digits */
        n = parse_digits(data, data_len, data_pos, 2, 2, &val);
        if (n != 2) {
            goto match_fail;
        }
        pt->century = val;
        consumed = 2;
        break;

    case 'm': /* month 01-12 or 1-12 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val < 1 || val > 12) {
            goto match_fail;
        }
        pt->month = val;
        consumed = n;
        break;

    case 'd': /* day 01-31 or 1-31 or space-padded */
    case 'e':
        pt->day_of_month_in_format = 1;
        /* Handle space-padded day */
        if (data_pos < data_len && data[data_pos] == ' ') {
            data_pos++;
            consumed = 1;
        }
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val < 1 || val > 31) {
            goto match_fail;
        }
        pt->day = val;
        consumed += n;
        break;

    case 'H': /* hour 00-23, 0-23, or space-padded */
    case 'k':
        if (data_pos < data_len && data[data_pos] == ' ') {
            data_pos++;
            consumed = 1;
        }
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val > 23) {
            goto match_fail;
        }
        pt->hour = val;
        consumed += n;
        break;

    case 'I': /* 12-hour: 01-12 or 1-12 or space-padded */
    case 'l':
        /* Fall back to Python - needs %p for AM/PM resolution */
        return -1;

    case 'M': /* minute 00-59 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val > 59) {
            goto match_fail;
        }
        pt->minute = val;
        consumed = n;
        break;

    case 'S': /* second 00-61 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val > 61) {
            goto match_fail;
        }
        pt->second = val;
        consumed = n;
        break;

    case 'f': /* microseconds, 1-6 digits */
    {
        int frac;
        n = parse_digits(data, data_len, data_pos, 1, 6, &frac);
        if (n == 0) {
            goto match_fail;
        }
        /* Pad to 6 digits */
        for (int i = n; i < 6; i++) {
            frac *= 10;
        }
        pt->fraction = frac;
        consumed = n;
        break;
    }

    case 'j': /* day of year 001-366 */
        n = parse_digits(data, data_len, data_pos, 1, 3, &val);
        if (n == 0 || val < 1 || val > 366) {
            goto match_fail;
        }
        pt->julian = val;
        consumed = n;
        break;

    case 'w': /* weekday 0=Sun, 6=Sat */
        n = parse_digits(data, data_len, data_pos, 1, 1, &val);
        if (n == 0 || val > 6) {
            goto match_fail;
        }
        /* Convert: Python uses 0=Mon, %w has 0=Sun */
        if (val == 0) {
            pt->weekday = 6;
        } else {
            pt->weekday = val - 1;
        }
        consumed = 1;
        break;

    case 'u': /* weekday 1=Mon, 7=Sun (ISO) */
        n = parse_digits(data, data_len, data_pos, 1, 1, &val);
        if (n == 0 || val < 1 || val > 7) {
            goto match_fail;
        }
        pt->weekday = val - 1; /* 0=Mon */
        consumed = 1;
        break;

    case 'G': /* ISO year, 4 digits */
        n = parse_digits(data, data_len, data_pos, 4, 4, &val);
        if (n != 4) {
            goto match_fail;
        }
        pt->iso_year = val;
        pt->year_in_format = 1;
        consumed = 4;
        break;

    case 'V': /* ISO week 01-53 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val < 1 || val > 53) {
            goto match_fail;
        }
        pt->iso_week = val;
        consumed = n;
        break;

    case 'U': /* week number (Sunday start) 00-53 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val > 53) {
            goto match_fail;
        }
        pt->week_of_year = val;
        pt->week_of_year_start = 6; /* Sunday */
        consumed = n;
        break;

    case 'W': /* week number (Monday start) 00-53 */
        n = parse_digits(data, data_len, data_pos, 1, 2, &val);
        if (n == 0 || val > 53) {
            goto match_fail;
        }
        pt->week_of_year = val;
        pt->week_of_year_start = 0; /* Monday */
        consumed = n;
        break;

    case 'z': /* timezone offset */
    {
        Py_ssize_t zpos = data_pos;
        int rc = parse_tz_offset(data, data_len, &zpos, pt, 0);
        if (rc == 0) return 0;
        if (rc == -1) return -1;
        consumed = zpos - data_pos;
        break;
    }

    case ':': /* %:z */
        if (fpos < fmt_len && fmt[fpos] == 'z') {
            fpos++;
            pt->colon_z_in_format = 1;
            Py_ssize_t zpos = data_pos;
            int rc = parse_tz_offset(data, data_len, &zpos, pt, 1);
            if (rc == 0) return 0;
            if (rc == -1) return -1;
            consumed = zpos - data_pos;
        } else {
            /* Unknown directive %:X - fall back */
            return -1;
        }
        break;

    case '%': /* literal % */
        if (data_pos >= data_len || data[data_pos] != '%') {
            goto match_fail;
        }
        consumed = 1;
        break;

    case 'n': /* newline */
        if (data_pos >= data_len || data[data_pos] != '\n') {
            goto match_fail;
        }
        consumed = 1;
        break;

    case 't': /* tab */
        if (data_pos >= data_len || data[data_pos] != '\t') {
            goto match_fail;
        }
        consumed = 1;
        break;

    /* Compound directives that expand to other directives */
    case 'F': /* %Y-%m-%d */
    case 'T': /* %H:%M:%S */
    case 'R': /* %H:%M */
    {
        /* We handle these by setting up a sub-format and parsing it inline */
        const char *sub_fmt;
        Py_ssize_t sub_fmt_len;
        if (directive == 'F') {
            sub_fmt = "%Y-%m-%d";
            sub_fmt_len = 8;
        } else if (directive == 'T') {
            sub_fmt = "%H:%M:%S";
            sub_fmt_len = 8;
        } else {
            sub_fmt = "%H:%M";
            sub_fmt_len = 5;
        }

        Py_ssize_t sub_fpos = 0;
        Py_ssize_t sub_dpos = data_pos;
        while (sub_fpos < sub_fmt_len) {
            if (sub_fmt[sub_fpos] == '%') {
                sub_fpos++; /* skip % */
                Py_ssize_t sub_consumed;
                int ret = parse_directive(data, data_len, sub_dpos,
                                          sub_fmt, sub_fmt_len,
                                          &sub_fpos, pt, &sub_consumed);
                if (ret <= 0) {
                    if (ret == 0 && !PyErr_Occurred()) goto match_fail;
                    return ret;
                }
                sub_dpos += sub_consumed;
            } else {
                /* literal */
                if (sub_dpos >= data_len || data[sub_dpos] != sub_fmt[sub_fpos]) {
                    goto match_fail;
                }
                sub_dpos++;
                sub_fpos++;
            }
        }
        consumed = sub_dpos - data_pos;
        break;
    }

    /* Locale-dependent directives: fall back to Python */
    case 'b': case 'B': case 'a': case 'A':
    case 'p': case 'P':
    case 'c': case 'x': case 'X': case 'r':
    case 'Z':
        return -1;

    case 'O': case 'E':
        /* %O* and %E* modifiers: fall back to Python */
        return -1;

    default:
        /* Unknown directive */
        return -1;
    }

    *fmt_pos = fpos;
    *consumed_out = consumed;
    return 1;

match_fail:
    /* Don't set error here - parse_format will set the proper message
       with data_string and format included */
    return 0;
}


/* Main parsing function: walks the format string, dispatching directives.
   data_obj and fmt_obj are the original Python string objects, used
   for error messages. */
static int
parse_format(const char *data, Py_ssize_t data_len,
             const char *fmt, Py_ssize_t fmt_len,
             ParsedTime *pt,
             PyObject *data_obj, PyObject *fmt_obj)
{
    Py_ssize_t dpos = 0;
    Py_ssize_t fpos = 0;

/* Macro to set the standard mismatch error with data and format repr */
#define SET_MISMATCH_ERROR() do {                                       \
    PyObject *data_repr = PyObject_Repr(data_obj);                      \
    PyObject *fmt_repr = PyObject_Repr(fmt_obj);                        \
    if (data_repr && fmt_repr) {                                        \
        PyErr_Format(PyExc_ValueError,                                  \
                     "time data %U does not match format %U",           \
                     data_repr, fmt_repr);                              \
    }                                                                   \
    Py_XDECREF(data_repr);                                              \
    Py_XDECREF(fmt_repr);                                               \
} while (0)

    while (fpos < fmt_len) {
        char fc = fmt[fpos];

        if (fc == '%') {
            fpos++; /* skip '%' */
            Py_ssize_t directive_consumed;
            int ret = parse_directive(data, data_len, dpos,
                                      fmt, fmt_len, &fpos, pt,
                                      &directive_consumed);
            if (ret < 0) {
                /* Signal fallback: return -1, no error set */
                PyErr_Clear();
                return -1;
            }
            if (ret == 0) {
                /* Error may or may not be set by parse_directive.
                   If not set (match_fail), set the standard mismatch error. */
                if (!PyErr_Occurred()) {
                    SET_MISMATCH_ERROR();
                }
                return 0;
            }
            dpos += directive_consumed;
        }
        else if (is_ascii_space(fc)) {
            /* Whitespace in format matches 1+ whitespace in data */
            if (dpos >= data_len || !is_ascii_space(data[dpos])) {
                SET_MISMATCH_ERROR();
                return 0;
            }
            /* Skip all whitespace in format */
            while (fpos < fmt_len && is_ascii_space(fmt[fpos])) {
                fpos++;
            }
            /* Skip all whitespace in data */
            while (dpos < data_len && is_ascii_space(data[dpos])) {
                dpos++;
            }
        }
        else if (fc == '\'') {
            /* Apostrophe matches ' or \u02bc - but \u02bc is multi-byte UTF-8.
               For ASCII-only fast path, just match '. */
            if (dpos < data_len && data[dpos] == '\'') {
                dpos++;
                fpos++;
            }
            /* Check for \u02bc (UTF-8: 0xCA 0xBC) */
            else if (dpos + 1 < data_len &&
                     (unsigned char)data[dpos] == 0xCA &&
                     (unsigned char)data[dpos + 1] == 0xBC) {
                dpos += 2;
                fpos++;
            }
            else {
                SET_MISMATCH_ERROR();
                return 0;
            }
        }
        else {
            /* Literal character match */
            if (dpos >= data_len || data[dpos] != fc) {
                SET_MISMATCH_ERROR();
                return 0;
            }
            dpos++;
            fpos++;
        }
    }

#undef SET_MISMATCH_ERROR

    /* Check for unconverted data */
    if (dpos != data_len) {
        const char *rest = data + dpos;
        /* Specific check for %:z directive */
        if (pt->colon_z_in_format && pt->gmtoff != INT_MIN &&
            rest[0] != ':') {
            PyErr_Format(PyExc_ValueError,
                         "Missing colon in %%:z before '%s', got '%s'",
                         rest, data);
            return 0;
        }
        PyErr_Format(PyExc_ValueError,
                     "unconverted data remains: %s",
                     rest);
        return 0;
    }

    return 1; /* success */
}


/* ========================== post-processing ========================== */

/* Resolve parsed fields into final values, replicating _strptime.py logic.
   Returns 1 on success, 0 on error (exception set), -1 for fallback. */
static int
resolve_date_fields(ParsedTime *pt)
{
    /* Handle %y with %C (century) */
    if (pt->has_short_year) {
        if (pt->century >= 0) {
            pt->year += pt->century * 100;
        } else {
            if (pt->year <= 68) {
                pt->year += 2000;
            } else {
                pt->year += 1900;
            }
        }
    }

    /* ISO year validation */
    if (pt->iso_year >= 0) {
        if (pt->julian != INT_MIN) {
            PyErr_SetString(PyExc_ValueError,
                            "Day of the year directive '%j' is not "
                            "compatible with ISO year directive '%G'. "
                            "Use '%Y' instead.");
            return 0;
        }
        if (pt->iso_week < 0 || pt->weekday < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "ISO year directive '%G' must be used with "
                            "the ISO week directive '%V' and a weekday "
                            "directive ('%A', '%a', '%w', or '%u').");
            return 0;
        }
    }
    else if (pt->iso_week >= 0) {
        if (pt->year < 0 || pt->weekday < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "ISO week directive '%V' must be used with "
                            "the ISO year directive '%G' and a weekday "
                            "directive ('%A', '%a', '%w', or '%u').");
            return 0;
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "ISO week directive '%V' is incompatible with "
                            "the year directive '%Y'. Use the ISO year '%G' "
                            "instead.");
            return 0;
        }
    }

    /* Default year handling */
    int leap_year_fix = 0;
    if (pt->year < 0) {
        if (pt->month == 2 && pt->day == 29) {
            pt->year = 1904;
            leap_year_fix = 1;
        } else {
            pt->year = 1900;
        }
    }

    /* Julian / weekday calculation */
    if (pt->julian == INT_MIN && pt->weekday >= 0) {
        if (pt->week_of_year >= 0) {
            int week_starts_Mon = (pt->week_of_year_start == 0) ? 1 : 0;
            pt->julian = calc_julian_from_U_or_W(pt->year, pt->week_of_year,
                                                  pt->weekday, week_starts_Mon);
        }
        else if (pt->iso_year >= 0 && pt->iso_week >= 0) {
            if (!isocalendar_to_date(pt->iso_year, pt->iso_week,
                                     pt->weekday + 1,
                                     &pt->year, &pt->month, &pt->day)) {
                return 0;
            }
        }

        if (pt->julian != INT_MIN && pt->julian <= 0) {
            pt->year -= 1;
            int yday = is_leap_year(pt->year) ? 366 : 365;
            pt->julian += yday;
        }
    }

    if (pt->julian == INT_MIN) {
        /* Compute julian from year/month/day */
        pt->julian = day_of_year(pt->year, pt->month, pt->day);
    } else {
        /* Compute year/month/day from julian */
        long jan1_ord = date_to_ordinal(pt->year, 1, 1);
        long target_ord = jan1_ord + pt->julian - 1;
        ordinal_to_date(target_ord, &pt->year, &pt->month, &pt->day);
    }

    if (pt->weekday < 0) {
        pt->weekday = weekday_from_date(pt->year, pt->month, pt->day);
    }

    if (leap_year_fix) {
        pt->year = 1900;
    }

    return 1;
}


/* Build the return 3-tuple matching Python's _strptime() return value:
   ((y, m, d, H, M, S, wd, jd, tz, tzname, gmtoff), fraction, gmtoff_fraction)
*/
static PyObject *
build_result(ParsedTime *pt)
{
    /* tzname: None if gmtoff not set, else string representation or None */
    PyObject *tzname_obj;
    PyObject *gmtoff_obj;

    if (pt->gmtoff == INT_MIN) {
        tzname_obj = Py_NewRef(Py_None);
        gmtoff_obj = Py_NewRef(Py_None);
    } else {
        /* For numeric timezone offsets parsed from %z, we don't have a name */
        tzname_obj = Py_NewRef(Py_None);
        gmtoff_obj = PyLong_FromLong(pt->gmtoff);
        if (!gmtoff_obj) {
            Py_DECREF(tzname_obj);
            return NULL;
        }
    }

    PyObject *inner = Py_BuildValue(
        "(iiiiiiiiiOO)",
        pt->year, pt->month, pt->day,
        pt->hour, pt->minute, pt->second,
        pt->weekday, pt->julian, pt->tz,
        tzname_obj, gmtoff_obj
    );

    Py_DECREF(tzname_obj);
    Py_DECREF(gmtoff_obj);

    if (!inner) {
        return NULL;
    }

    PyObject *result = Py_BuildValue("(Oii)", inner, pt->fraction,
                                     pt->gmtoff_fraction);
    Py_DECREF(inner);
    return result;
}


/* ========================== module function ========================== */

/*[clinic input]
_strptime_impl._strptime_parse

    data_string: unicode
    format: unicode
    /

Parse a time string according to a format.

Returns a 3-tuple on success, or None if the format contains
directives that require the Python fallback path.

[clinic start generated code]*/

static PyObject *
_strptime_impl__strptime_parse_impl(PyObject *module, PyObject *data_string,
                                    PyObject *format)
/*[clinic end generated code: output=c3c1f836ef1972ae input=9a670dbb56f2ad26]*/
{
    const char *data_str = PyUnicode_AsUTF8(data_string);
    if (data_str == NULL) {
        /* Strings with surrogates can't be encoded to UTF-8.
           Fall back to the Python path which handles them. */
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    const char *fmt_str = PyUnicode_AsUTF8(format);
    if (fmt_str == NULL) {
        PyErr_Clear();
        Py_RETURN_NONE;
    }

    Py_ssize_t data_len = (Py_ssize_t)strlen(data_str);
    Py_ssize_t fmt_len = (Py_ssize_t)strlen(fmt_str);

    ParsedTime pt;
    parsed_time_init(&pt);

    int rc = parse_format(data_str, data_len, fmt_str, fmt_len, &pt,
                          data_string, format);
    if (rc == -1) {
        /* Fallback signal: return None */
        Py_RETURN_NONE;
    }
    if (rc == 0) {
        /* Error already set */
        return NULL;
    }

    /* DeprecationWarning for %d without year */
    if (pt.day_of_month_in_format && !pt.year_in_format) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
            "Parsing dates involving a day of month without a year specified "
            "is ambiguous and fails to parse leap day. The default behavior "
            "will change in Python 3.15 to either always raise an exception "
            "or to use a different default year (TBD). To avoid trouble, add "
            "a specific year to the input & format. "
            "See https://github.com/python/cpython/issues/70647.", 2) < 0) {
            return NULL;
        }
    }

    /* Post-processing */
    rc = resolve_date_fields(&pt);
    if (rc == -1) {
        Py_RETURN_NONE;
    }
    if (rc == 0) {
        return NULL;
    }

    return build_result(&pt);
}


/* ========================== module definition ========================== */

static PyMethodDef strptime_methods[] = {
    _STRPTIME_IMPL__STRPTIME_PARSE_METHODDEF
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(strptime_doc,
"C accelerator for _strptime time parsing.\n");

static struct PyModuleDef_Slot _strptimemodule_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _strptimemodule = {
    PyModuleDef_HEAD_INIT,
    "_strptime_impl",
    strptime_doc,
    0,
    strptime_methods,
    _strptimemodule_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__strptime_impl(void)
{
    return PyModuleDef_Init(&_strptimemodule);
}
