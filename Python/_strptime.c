#include "Python.h"
#include "pycore_time.h"


/* Implement common parts of strptime in C to improve performance. Callers
 * fall back to Lib/_strptime.py for a full parse when this doesn't match.
 *
 * Supported inputs are exact str objects containing only ASCII, with:
 * - numeric %Y, %y, %m, %d, %H, %M, %S, and %f fields;
 * - a terminal %z: empty, Z, or +/-HHMM or +/-HH:MM with hours below 24;
 * - literal characters, %%, and ASCII whitespace.
 * Duplicate directives, mixed %Y/%y, and day-of-month without a year use the
 * fallback. So do other directives (including locale-dependent names and
 * week/day-of-year calculations), and offsets with seconds or fractions.
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
strptime_offset(const unsigned char *data, Py_ssize_t length, Py_ssize_t pos,
                _PyTime_StrptimeFields *fields)
{
    Py_ssize_t remaining = length - pos;
    if (remaining == 0) {
        return 1;
    }
    if (remaining == 1 && data[pos] == 'Z') {
        fields->gmtoff = 0;
        return 1;
    }
    /* Offsets containing seconds or fractions fall back to _strptime.py. */
    if ((remaining != 5 && remaining != 6) ||
        (data[pos] != '+' && data[pos] != '-')) {
        return 0;
    }
    int hours, minutes;
    int colon = remaining == 6;
    if ((colon && data[pos + 3] != ':') ||
        strptime_digits(data, length, pos + 1, 2, 2, &hours) != 2 ||
        strptime_digits(data, length, pos + 3 + colon, 2, 2, &minutes) != 2 ||
        hours > 23 || minutes > 59) {
        return 0;
    }
    fields->gmtoff = (hours * 3600 + minutes * 60) *
                    (data[pos] == '-' ? -1 : 1);
    return 1;
}

/* Return 1 for a complete numeric match, or 0 if fallback is required.
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
    };
    /* String subclasses and non-ASCII input retain Python's behavior. */
    if (!PyUnicode_CheckExact(string) || !PyUnicode_CheckExact(format) ||
        !PyUnicode_IS_ASCII(string) || !PyUnicode_IS_ASCII(format)) {
        return 0;
    }
    const unsigned char *data = PyUnicode_1BYTE_DATA(string);
    const unsigned char *fmt = PyUnicode_1BYTE_DATA(format);
    Py_ssize_t length = PyUnicode_GET_LENGTH(string);
    Py_ssize_t fmt_length = PyUnicode_GET_LENGTH(format);
    *fields = (_PyTime_StrptimeFields){.year = 1900, .month = 1, .day = 1, .gmtoff = INT_MIN};
    unsigned int seen = 0;
    Py_ssize_t pos = 0;

    for (Py_ssize_t i = 0; i < fmt_length; i++) {
        unsigned char c = fmt[i];
        if (Py_ISSPACE(c)) {
            if (pos == length || !Py_ISSPACE(data[pos])) {
                return 0;
            }
            while (i + 1 < fmt_length && Py_ISSPACE(fmt[i + 1])) {
                i++;
            }
            do {
                pos++;
            } while (pos < length && Py_ISSPACE(data[pos]));
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
        if (c == '%') {
            if (pos == length || data[pos++] != '%') {
                return 0;
            }
            continue;
        }

        /* Reject duplicate groups, aliases, and locale-dependent directives. */
        unsigned int bit = directive_bits[c];
        if (bit == 0) {
            return 0;
        }
        if (seen & bit) {
            return 0;
        }
        seen |= bit;

        if (c == 'z') {
            if (i != fmt_length - 1 ||
                !strptime_offset(data, length, pos, fields)) {
                return 0;
            }
            pos = length;
            continue;
        }

        int minimum = 1;
        int maximum = 2;
        if (c == 'Y') {
            minimum = maximum = 4;
        }
        else if (c == 'y') {
            minimum = maximum = 2;
        }
        else if (c == 'f') {
            maximum = 6;
        }
        else if ((c == 'd' || c == 'H') && pos < length && data[pos] == ' ') {
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
    /* Mixed %Y/%y and day-without-year diagnostics belong to Python. */
    if (pos != length || (seen & ANY_YEAR) == ANY_YEAR ||
        ((seen & DAY) && !(seen & ANY_YEAR)) ||
        fields->year < 1 ||
        fields->day > month_days) {
        return 0;
    }
    fields->yday = days_before_month[fields->month] + fields->day +
                   (fields->month > 2 && leap);
    return 1;
}
