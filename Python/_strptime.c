#include "Python.h"
#include "pycore_time.h"

#include <time.h>


/* Implement common parts of strptime in C to improve performance. Callers
 * fall back to Lib/_strptime.py for a full parse when this doesn't match.
 *
 * Supported inputs are exact str objects containing only ASCII, with:
 * - numeric %Y, %y, %m, %d, %H, %M, %S, and %f fields;
 * - ASCII locale month/weekday names and numeric %w/%u weekdays;
 * - %I with ASCII %p indicators, and the %k, %l, %P aliases;
 * - %z and %:z offsets with hours below 24, and terminal UTC/GMT %Z names;
 * - numeric aliases %F, %D, %T, %R, %e and whitespace %n/%t;
 * - ordinal dates (%j), calendar weeks (%U/%W), and ISO weeks (%G/%V);
 * - literal characters, %%, and ASCII whitespace.
 * Duplicate directives, mixed %Y/%y, and day-of-month without a year use the
 * fallback. So do non-ASCII locale names and directives not listed above.
 */

static int
strptime_digits(const unsigned char *data, Py_ssize_t length, Py_ssize_t pos,
                int minimum, int maximum, int *value)
{
    int count = 0;
    *value = 0;
    while (count < maximum && pos + count < length) {
        unsigned char c = data[pos + count];
        if (c < '0' || c > '9') {
            break;
        }
        *value = *value * 10 + c - '0';
        count++;
    }
    return count >= minimum ? count : 0;
}

static int
strptime_name(const unsigned char *data, Py_ssize_t length, Py_ssize_t pos,
              unsigned char directive, int *value)
{
#ifdef HAVE_STRFTIME
    /* Read the current locale without keeping a cache across setlocale().
     * Non-ASCII names retain Python's decoding and Unicode case handling.
     * As in TimeRE, prefer the longest match, then the first equal name.
     */
    int month = directive == 'b' || directive == 'B';
    int count = month ? 12 : directive == 'p' ? 2 : 7;
    char format[] = {'%', directive, '\0'};
    int matched = 0;
    for (int i = 0; i < count; i++) {
        struct tm tm = {.tm_year = 101, .tm_mday = 1,
                        .tm_mon = month ? i : 0,
                        .tm_wday = month ? 0 : (i + 1) % 7,
                        .tm_hour = i == 0 ? 1 : 22};
        char name[256];
        size_t size = strftime(name, sizeof(name), format, &tm);
        if (size == 0) {
            return 0;
        }
        for (size_t j = 0; j < size; j++) {
            if ((unsigned char)name[j] >= 128) {
                return 0;
            }
        }
        char *start = name;
        if (directive == 'p') {
            while (size && Py_UNICODE_ISSPACE(start[size - 1])) {
                size--;
            }
            while (size && Py_UNICODE_ISSPACE(*start)) {
                start++;
                size--;
            }
            /* Empty indicators have special defaulting rules in Python. */
            if (size == 0) {
                return 0;
            }
        }
        if (data == NULL) {
            continue;
        }
        if (size < (size_t)matched ||
            (size == (size_t)matched && directive != 'p') ||
            size > (size_t)(length - pos)) {
            continue;
        }
        size_t j;
        for (j = 0; j < size; j++) {
            if (Py_TOLOWER(data[pos + j]) != Py_TOLOWER(start[j])) {
                break;
            }
        }
        if (j == size) {
            matched = (int)size;
            *value = month ? i + 1 : i;
        }
    }
    return data == NULL ? 1 : matched;
#else
    return 0;
#endif
}

static int
strptime_date_from_ordinal(int ordinal, _PyTime_StrptimeFields *fields)
{
    if (ordinal < 1 || ordinal > _PyTime_DaysBeforeYear(10000)) {
        return 0;
    }
    int lower = 1, upper = 10000;
    while (lower + 1 < upper) {
        int middle = (lower + upper) / 2;
        if (_PyTime_DaysBeforeYear(middle) < ordinal) {
            lower = middle;
        }
        else {
            upper = middle;
        }
    }
    fields->year = lower;
    fields->yday = ordinal - _PyTime_DaysBeforeYear(lower);
    fields->day = fields->yday;
    int leap = lower % 4 == 0 && (lower % 100 != 0 || lower % 400 == 0);
    static const int month_days[] = {31, 28, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};
    for (fields->month = 1; fields->month < 12; fields->month++) {
        int days = month_days[fields->month - 1] + (fields->month == 2 && leap);
        if (fields->day <= days) {
            break;
        }
        fields->day -= days;
    }
    if (fields->weekday < 0) {
        fields->weekday = (ordinal - 1) % 7;
    }
    return 1;
}

static int
strptime_iso_start(int year)
{
    /* ISO week 1 contains January 4. */
    return _PyTime_DaysBeforeYear(year) + 4 - _PyTime_Weekday(year, 4);
}

static int
strptime_offset(const unsigned char *data, Py_ssize_t length, Py_ssize_t pos,
                int require_colon, _PyTime_StrptimeFields *fields)
{
    Py_ssize_t start = pos;
    if (pos == length) {
        return 0;
    }
    if (data[pos] == 'Z') {
        fields->gmtoff = 0;
        return 1;
    }
    if (data[pos] != '+' && data[pos] != '-') {
        return 0;
    }
    int sign = data[pos++] == '-' ? -1 : 1;
    int hours, minutes, seconds = 0, fraction = 0;
    if (strptime_digits(data, length, pos, 2, 2, &hours) != 2 || hours > 23) {
        return -1;
    }
    pos += 2;
    int colon = pos < length && data[pos] == ':';
    if (require_colon && !colon) {
        return -1;
    }
    pos += colon;
    if (strptime_digits(data, length, pos, 2, 2, &minutes) != 2 || minutes > 59) {
        return -1;
    }
    pos += 2;
    int seconds_colon = pos < length && data[pos] == ':';
    Py_ssize_t seconds_pos = pos + seconds_colon;
    if (strptime_digits(data, length, seconds_pos, 2, 2, &seconds) == 2) {
        if (seconds > 59 || seconds_colon != colon) {
            return -1;
        }
        pos = seconds_pos + 2;
        if (pos < length && data[pos] == '.') {
            int count = strptime_digits(data, length, pos + 1, 1, 6, &fraction);
            if (count == 0) {
                return -1;
            }
            pos += count + 1;
            for (; count < 6; count++) {
                fraction *= 10;
            }
        }
    }
    else {
        seconds = 0;
    }
    fields->gmtoff = sign * (hours * 3600 + minutes * 60 + seconds);
    fields->gmtoff_fraction = sign * fraction;
    return (int)(pos - start);
}

/* Return 1 for a complete match, or 0 if fallback is required.
 *
 * Parsing doesn't allocate or set exceptions. Some mismatches require regex
 * backtracking, but even inputs known to be invalid use the Python parser:
 * it owns the exception messages and error precedence. For example, month 13
 * and empty input can't be repaired by backtracking, but diagnosing them here
 * would duplicate the Python parser's error handling. A return value of 0
 * therefore doesn't distinguish invalid input from an unsupported format.
 */
int
_PyTime_Strptime(PyObject *string, PyObject *format, _PyTime_StrptimeFields *fields)
{
    enum {
        YEAR = 1U << 0,
        SHORT_YEAR = 1U << 1,
        MONTH = 1U << 2,
        DAY = 1U << 3,
        HOUR = 1U << 4,
        MINUTE = 1U << 5,
        SECOND = 1U << 6,
        FRACTION = 1U << 7,
        OFFSET = 1U << 8,
        WEEKDAY = 1U << 9,
        AMPM = 1U << 10,
        ZONE = 1U << 11,
        JULIAN = 1U << 12,
        WEEK = 1U << 13,
        ISO_YEAR = 1U << 14,
        ISO_WEEK = 1U << 15,
        ANY_YEAR = YEAR | SHORT_YEAR,
    };
    static const unsigned short directive_bits[128] = {
        ['Y'] = YEAR,
        ['y'] = SHORT_YEAR,
        ['m'] = MONTH,
        ['d'] = DAY,
        ['H'] = HOUR,
        ['M'] = MINUTE,
        ['S'] = SECOND,
        ['f'] = FRACTION,
        ['z'] = OFFSET,
        ['w'] = WEEKDAY,
        ['p'] = AMPM,
        ['Z'] = ZONE,
        ['j'] = JULIAN,
        ['U'] = WEEK,
        ['G'] = ISO_YEAR,
        ['V'] = ISO_WEEK,
    };
    /* String subclasses and non-ASCII input retain Python's behavior. */
    if (!PyUnicode_CheckExact(string) || !PyUnicode_CheckExact(format) ||
        !PyUnicode_IS_ASCII(string) || !PyUnicode_IS_ASCII(format)) {
        return 0;
    }
    const unsigned char *data = PyUnicode_1BYTE_DATA(string);
    const unsigned char *fmt = PyUnicode_1BYTE_DATA(format);
    const unsigned char *outer_fmt = NULL;
    Py_ssize_t outer_length = 0, outer_pos = 0;
    Py_ssize_t length = PyUnicode_GET_LENGTH(string);
    Py_ssize_t fmt_length = PyUnicode_GET_LENGTH(format);
    *fields = (_PyTime_StrptimeFields){.year = 1900, .month = 1, .day = 1,
                                     .gmtoff = INT_MIN, .weekday = -1};
    unsigned int seen = 0;
    int hour12 = 0;
    int ampm = -1;
    int julian = -1, week = -1, monday_first = 0;
    int have_julian = 0;
    int iso_year = -1, iso_week = -1;
    Py_ssize_t pos = 0;

    for (Py_ssize_t i = 0; ; i++) {
        if (i == fmt_length) {
            if (outer_fmt == NULL) {
                break;
            }
            fmt = outer_fmt;
            fmt_length = outer_length;
            i = outer_pos;
            outer_fmt = NULL;
            if (i == fmt_length) {
                break;
            }
        }
        unsigned char c = fmt[i];
        /* Match regex \s, including ASCII separators U+001C through U+001F. */
        if (Py_UNICODE_ISSPACE(c)) {
            if (pos == length || !Py_UNICODE_ISSPACE(data[pos])) {
                return 0;
            }
            while (i + 1 < fmt_length && Py_UNICODE_ISSPACE(fmt[i + 1])) {
                i++;
            }
            do {
                pos++;
            } while (pos < length && Py_UNICODE_ISSPACE(data[pos]));
            continue;
        }
        if (c != '%') {
            if (pos == length || data[pos++] != c) {
                return 0;
            }
            continue;
        }
        if (++i == fmt_length) {
            return 0;
        }
        c = fmt[i];
        const char *alias = NULL;
        switch (c) {
            case 'F': alias = "%Y-%m-%d"; break;
            case 'D': alias = "%m/%d/%y"; break;
            case 'T': alias = "%H:%M:%S"; break;
            case 'R': alias = "%H:%M"; break;
            case 'e': c = 'd'; break;
        }
        if (alias != NULL) {
            /* These fixed expansions contain no further aliases. Parsing
             * them in place keeps duplicate-field checks and avoids copying
             * an arbitrarily long user format into a temporary buffer.
             */
            assert(outer_fmt == NULL);
            outer_fmt = fmt;
            outer_length = fmt_length;
            outer_pos = i + 1;
            fmt = (const unsigned char *)alias;
            fmt_length = strlen(alias);
            i = -1;
            continue;
        }
        if (c == 'n' || c == 't') {
            while (pos < length && Py_UNICODE_ISSPACE(data[pos])) {
                pos++;
            }
            continue;
        }
        int require_colon = 0;
        if (c == ':' && i + 1 < fmt_length && fmt[i + 1] == 'z') {
            c = fmt[++i];
            require_colon = 1;
        }
        if (c == 'k') {
            c = 'H';
        }
        else if (c == 'l') {
            c = 'I';
        }
        else if (c == 'P') {
            c = 'p';
        }
        if (c == '%') {
            if (pos == length || data[pos++] != '%') {
                return 0;
            }
            continue;
        }

        /* Mixed spellings of the same field retain Python's precedence. */
        unsigned char field = c;
        if (c == 'b' || c == 'B') {
            field = 'm';
        }
        else if (c == 'a' || c == 'A' || c == 'u') {
            field = 'w';
        }
        else if (c == 'I') {
            field = 'H';
        }
        else if (c == 'W') {
            field = 'U';
        }
        unsigned int bit = directive_bits[field];
        if (bit == 0) {
            return 0;
        }
        if (seen & bit) {
            return 0;
        }
        seen |= bit;

        if (c == 'b' || c == 'B' || c == 'a' || c == 'A' || c == 'p') {
            int value;
            int count = strptime_name(data, length, pos, c, &value);
            if (count == 0) {
                return 0;
            }
            pos += count;
            if (field == 'm') {
                fields->month = value;
            }
            else if (c == 'p') {
                ampm = value;
            }
            else {
                fields->weekday = value;
            }
            continue;
        }

        if (c == 'Z') {
            /* Local timezone names and nonterminal names retain Python's
             * tzset state, DST rules and regex backtracking.
             */
            if (i != fmt_length - 1 || length - pos != 3 ||
                !((Py_TOLOWER(data[pos]) == 'u' &&
                   Py_TOLOWER(data[pos + 1]) == 't' &&
                   Py_TOLOWER(data[pos + 2]) == 'c') ||
                  (Py_TOLOWER(data[pos]) == 'g' &&
                   Py_TOLOWER(data[pos + 1]) == 'm' &&
                   Py_TOLOWER(data[pos + 2]) == 't'))) {
                return 0;
            }
            fields->zone_start = pos;
            fields->zone_length = 3;
            pos = length;
            continue;
        }
        if (c == 'z') {
            int count = strptime_offset(data, length, pos, require_colon, fields);
            if (count < 0) {
                return 0;
            }
            pos += count;
            continue;
        }

        int minimum = 1;
        int maximum = 2;
        if (c == 'Y' || c == 'G') {
            minimum = maximum = 4;
        }
        else if (c == 'y') {
            minimum = maximum = 2;
        }
        else if (c == 'f') {
            maximum = 6;
        }
        else if (c == 'j') {
            maximum = 3;
        }
        else if (field == 'w') {
            maximum = 1;
        }
        else if ((c == 'd' || c == 'H' || c == 'I') &&
                 pos < length && data[pos] == ' ') {
            pos++;
            maximum = 1;
        }
        int value;
        int count = strptime_digits(data, length, pos, minimum, maximum, &value);
        if (count == 0) {
            return 0;
        }
        pos += count;
        switch (c) {
            case 'j':
                if (value < 1 || value > 366) {
                    return 0;
                }
                julian = value;
                have_julian = 1;
                break;
            case 'U':
            case 'W':
                if (value > 53) {
                    return 0;
                }
                week = value;
                monday_first = c == 'W';
                break;
            case 'G':
                iso_year = value;
                break;
            case 'V':
                if (value < 1 || value > 53) {
                    return 0;
                }
                iso_week = value;
                break;
            case 'I':
                if (value < 1 || value > 12) {
                    return 0;
                }
                hour12 = 1;
                fields->hour = value;
                break;
            case 'w':
                if (value > 6) {
                    return 0;
                }
                fields->weekday = (value + 6) % 7;
                break;
            case 'u':
                if (value < 1 || value > 7) {
                    return 0;
                }
                fields->weekday = value - 1;
                break;
            case 'Y':
                fields->year = value;
                break;
            case 'y':
                fields->year = value + (value <= 68 ? 2000 : 1900);
                break;
            case 'm':
                if (value < 1 || value > 12) {
                    return 0;
                }
                fields->month = value;
                break;
            case 'd':
                if (value < 1 || value > 31) {
                    return 0;
                }
                fields->day = value;
                break;
            case 'H':
                if (value > 23) {
                    return 0;
                }
                fields->hour = value;
                break;
            case 'M':
                if (value > 59) {
                    return 0;
                }
                fields->minute = value;
                break;
            case 'S':
                if (value > 61) {
                    return 0;
                }
                fields->second = value;
                break;
            case 'f':
                for (; count < 6; count++) {
                    value *= 10;
                }
                fields->fraction = value;
                break;
        }
    }
    if (hour12) {
        if (ampm < 0 && !strptime_name(NULL, 0, 0, 'p', &ampm)) {
            return 0;
        }
        fields->hour = fields->hour % 12 + (ampm == 1 ? 12 : 0);
    }
    /* Mixed fields and incomplete ISO dates retain Python's diagnostics. */
    if (pos != length || (seen & ANY_YEAR) == ANY_YEAR ||
        ((seen & DAY) && !(seen & ANY_YEAR) && iso_year < 0)) {
        return 0;
    }
    if (iso_year >= 0 || iso_week >= 0) {
        if (iso_year < 1 || iso_week < 1 || fields->weekday < 0 ||
            julian >= 0 || week >= 0) {
            return 0;
        }
        int monday = strptime_iso_start(iso_year) + 7 * (iso_week - 1);
        if (monday >= strptime_iso_start(iso_year + 1) ||
            !strptime_date_from_ordinal(monday + fields->weekday, fields)) {
            return 0;
        }
    }
    if (fields->year < 1) {
        return 0;
    }
    if (julian < 0 && week >= 0 && fields->weekday >= 0) {
        int first = _PyTime_Weekday(fields->year, 1);
        int weekday = fields->weekday;
        if (!monday_first) {
            first = (first + 1) % 7;
            weekday = (weekday + 1) % 7;
        }
        julian = week == 0 ? 1 + weekday - first
                          : 1 + (7 - first) % 7 + 7 * (week - 1) + weekday;
        have_julian = 1;
    }
    if (have_julian) {
        /* Week zero can fall in the previous year. A day spilling into the
         * next year retains its original yday in time.strptime's result.
         */
        int year = fields->year;
        if (julian <= 0) {
            if (year == 1) {
                return 0;
            }
            year--;
            julian += _PyTime_DaysBeforeYear(year + 1) - _PyTime_DaysBeforeYear(year);
        }
        if (!strptime_date_from_ordinal(_PyTime_DaysBeforeYear(year) + julian, fields)) {
            return 0;
        }
        fields->yday = julian;
    }
    static const int days_before_month[] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };
    int leap = fields->year % 4 == 0 &&
               (fields->year % 100 != 0 || fields->year % 400 == 0);
    int month_days = days_before_month[fields->month + 1] -
                     days_before_month[fields->month];
    if (fields->month == 2) {
        month_days += leap;
    }
    if (fields->day > month_days) {
        return 0;
    }
    if (!have_julian) {
        fields->yday = days_before_month[fields->month] + fields->day +
                       (fields->month > 2 && leap);
    }
    if (fields->weekday < 0) {
        fields->weekday = _PyTime_Weekday(fields->year, fields->yday);
    }
    return 1;
}
