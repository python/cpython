"""Strptime-related classes and functions.

CLASSES:
    LocaleTime -- Discovers and/or stores locale-specific time information
    TimeRE -- Creates regexes for pattern matching a string of text containing
                time information as is returned by time.strftime()

FUNCTIONS:
    _getlang -- Figure out what language is being used for the locale
    strptime -- Calculates the time struct represented by the passed-in string

Requires Python 2.2.1 or higher (mainly because of the use of property()).
Can be used in Python 2.2 if the following line is added:
    True = 1; False = 0
"""
import time
import locale
import calendar
from re import compile as re_compile
from re import IGNORECASE
from datetime import date as datetime_date

__author__ = "Brett Cannon"
__email__ = "brett@python.org"

__all__ = ['strptime']

def _getlang():
    # Figure out what the current language is set to.
    return locale.getlocale(locale.LC_TIME)

class LocaleTime(object):
    """Stores and handles locale-specific information related to time.

    This is not thread-safe!  Attributes are lazily calculated and no
    precaution is taken to check to see if the locale information has changed
    since the creation of the instance in use.

    ATTRIBUTES (all read-only after instance creation! Instance variables that
                store the values have mangled names):
        f_weekday -- full weekday names (7-item list)
        a_weekday -- abbreviated weekday names (7-item list)
        f_month -- full weekday names (14-item list; dummy value in [0], which
                    is added by code)
        a_month -- abbreviated weekday names (13-item list, dummy value in
                    [0], which is added by code)
        am_pm -- AM/PM representation (2-item list)
        LC_date_time -- format string for date/time representation (string)
        LC_date -- format string for date representation (string)
        LC_time -- format string for time representation (string)
        timezone -- daylight- and non-daylight-savings timezone representation
                    (3-item list; code tacks on blank item at end for
                    possible lack of timezone such as UTC)
        lang -- Language used by instance (string)
    """

    def __init__(self, f_weekday=None, a_weekday=None, f_month=None,
                 a_month=None, am_pm=None, LC_date_time=None, LC_time=None,
                 LC_date=None, timezone=None, lang=None):
        """Optionally set attributes with passed-in values."""
        if f_weekday is None:
            self.__f_weekday = None
        elif len(f_weekday) == 7:
            self.__f_weekday = list(f_weekday)
        else:
            raise TypeError("full weekday names must be a 7-item sequence")
        if a_weekday is None:
            self.__a_weekday = None
        elif len(a_weekday) == 7:
            self.__a_weekday = list(a_weekday)
        else:
            raise TypeError(
                "abbreviated weekday names must be a 7-item  sequence")
        if f_month is None:
            self.__f_month = None
        elif len(f_month) == 12:
            self.__f_month = self.__pad(f_month, True)
        else:
            raise TypeError("full month names must be a 12-item sequence")
        if a_month is None:
            self.__a_month = None
        elif len(a_month) == 12:
            self.__a_month = self.__pad(a_month, True)
        else:
            raise TypeError(
                "abbreviated month names must be a 12-item sequence")
        if am_pm is None:
            self.__am_pm = None
        elif len(am_pm) == 2:
            self.__am_pm = am_pm
        else:
            raise TypeError("AM/PM representation must be a 2-item sequence")
        self.__LC_date_time = LC_date_time
        self.__LC_time = LC_time
        self.__LC_date = LC_date
        self.__timezone = timezone
        if timezone:
            if len(timezone) != 2:
                raise TypeError("timezone names must contain 2 items")
            else:
                self.__timezone = self.__pad(timezone, False)
        if lang:
            self.__lang = lang
        else:
            self.__lang = _getlang()

    def __pad(self, seq, front):
        # Add '' to seq to either front (is True), else the back.
        seq = list(seq)
        if front:
            seq.insert(0, '')
        else:
            seq.append('')
        return seq

    def __set_nothing(self, stuff):
        # Raise TypeError when trying to set an attribute.
        raise TypeError("attribute does not support assignment")

    def __get_f_weekday(self):
        # Fetch self.f_weekday.
        if not self.__f_weekday:
            self.__calc_weekday()
        return self.__f_weekday

    def __get_a_weekday(self):
        # Fetch self.a_weekday.
        if not self.__a_weekday:
            self.__calc_weekday()
        return self.__a_weekday

    f_weekday = property(__get_f_weekday, __set_nothing,
                         doc="Full weekday names")
    a_weekday = property(__get_a_weekday, __set_nothing,
                         doc="Abbreviated weekday names")

    def __get_f_month(self):
        # Fetch self.f_month.
        if not self.__f_month:
            self.__calc_month()
        return self.__f_month

    def __get_a_month(self):
        # Fetch self.a_month.
        if not self.__a_month:
            self.__calc_month()
        return self.__a_month

    f_month = property(__get_f_month, __set_nothing,
                       doc="Full month names (dummy value at index 0)")
    a_month = property(__get_a_month, __set_nothing,
                       doc="Abbreviated month names (dummy value at index 0)")

    def __get_am_pm(self):
        # Fetch self.am_pm.
        if not self.__am_pm:
            self.__calc_am_pm()
        return self.__am_pm

    am_pm = property(__get_am_pm, __set_nothing, doc="AM/PM representation")

    def __get_timezone(self):
        # Fetch self.timezone.
        if not self.__timezone:
            self.__calc_timezone()
        return self.__timezone

    timezone = property(__get_timezone, __set_nothing,
                        doc="Timezone representation (dummy value at index 2)")

    def __get_LC_date_time(self):
        # Fetch self.LC_date_time.
        if not self.__LC_date_time:
            self.__calc_date_time()
        return self.__LC_date_time

    def __get_LC_date(self):
        # Fetch self.LC_date.
        if not self.__LC_date:
            self.__calc_date_time()
        return self.__LC_date

    def __get_LC_time(self):
        # Fetch self.LC_time.
        if not self.__LC_time:
            self.__calc_date_time()
        return self.__LC_time

    LC_date_time = property(
        __get_LC_date_time, __set_nothing,
        doc=
        "Format string for locale's date/time representation ('%c' format)")
    LC_date = property(__get_LC_date, __set_nothing,
        doc="Format string for locale's date representation ('%x' format)")
    LC_time = property(__get_LC_time, __set_nothing,
        doc="Format string for locale's time representation ('%X' format)")

    lang = property(lambda self: self.__lang, __set_nothing,
                    doc="Language used for instance")

    def __calc_weekday(self):
        # Set self.__a_weekday and self.__f_weekday using the calendar
        # module.
        a_weekday = [calendar.day_abbr[i] for i in range(7)]
        f_weekday = [calendar.day_name[i] for i in range(7)]
        if not self.__a_weekday:
            self.__a_weekday = a_weekday
        if not self.__f_weekday:
            self.__f_weekday = f_weekday

    def __calc_month(self):
        # Set self.__f_month and self.__a_month using the calendar module.
        a_month = [calendar.month_abbr[i] for i in range(13)]
        f_month = [calendar.month_name[i] for i in range(13)]
        if not self.__a_month:
            self.__a_month = a_month
        if not self.__f_month:
            self.__f_month = f_month

    def __calc_am_pm(self):
        # Set self.__am_pm by using time.strftime().

        # The magic date (1999,3,17,hour,44,55,2,76,0) is not really that
        # magical; just happened to have used it everywhere else where a
        # static date was needed.
        am_pm = []
        for hour in (01,22):
            time_tuple = time.struct_time((1999,3,17,hour,44,55,2,76,0))
            am_pm.append(time.strftime("%p", time_tuple))
        self.__am_pm = am_pm

    def __calc_date_time(self):
        # Set self.__date_time, self.__date, & self.__time by using
        # time.strftime().

        # Use (1999,3,17,22,44,55,2,76,0) for magic date because the amount of
        # overloaded numbers is minimized.  The order in which searches for
        # values within the format string is very important; it eliminates
        # possible ambiguity for what something represents.
        time_tuple = time.struct_time((1999,3,17,22,44,55,2,76,0))
        date_time = [None, None, None]
        date_time[0] = time.strftime("%c", time_tuple)
        date_time[1] = time.strftime("%x", time_tuple)
        date_time[2] = time.strftime("%X", time_tuple)
        for offset,directive in ((0,'%c'), (1,'%x'), (2,'%X')):
            current_format = date_time[offset]
            for old, new in (
                    ('%', '%%'), (self.f_weekday[2], '%A'),
                    (self.f_month[3], '%B'), (self.a_weekday[2], '%a'),
                    (self.a_month[3], '%b'), (self.am_pm[1], '%p'),
                    (self.timezone[0], '%Z'), (self.timezone[1], '%Z'),
                    ('1999', '%Y'), ('99', '%y'), ('22', '%H'),
                    ('44', '%M'), ('55', '%S'), ('76', '%j'),
                    ('17', '%d'), ('03', '%m'), ('3', '%m'),
                    # '3' needed for when no leading zero.
                    ('2', '%w'), ('10', '%I')):
                # Must deal with possible lack of locale info
                # manifesting itself as the empty string (e.g., Swedish's
                # lack of AM/PM info) or a platform returning a tuple of empty
                # strings (e.g., MacOS 9 having timezone as ('','')).
                if old:
                    current_format = current_format.replace(old, new)
            time_tuple = time.struct_time((1999,1,3,1,1,1,6,3,0))
            if time.strftime(directive, time_tuple).find('00'):
                U_W = '%U'
            else:
                U_W = '%W'
            date_time[offset] = current_format.replace('11', U_W)
        if not self.__LC_date_time:
            self.__LC_date_time = date_time[0]
        if not self.__LC_date:
            self.__LC_date = date_time[1]
        if not self.__LC_time:
            self.__LC_time = date_time[2]

    def __calc_timezone(self):
        # Set self.__timezone by using time.tzname.
        #
        # Empty string used for matching when timezone is not used/needed.
        try:
            time.tzset()
        except AttributeError:
            pass
        time_zones = ["UTC", "GMT"]
        if time.daylight:
            time_zones.extend(time.tzname)
        else:
            time_zones.append(time.tzname[0])
        self.__timezone = self.__pad(time_zones, 0)


class TimeRE(dict):
    """Handle conversion from format directives to regexes."""

    def __init__(self, locale_time=None):
        """Init inst with non-locale regexes and store LocaleTime object."""
        #XXX: Does 'Y' need to worry about having less or more than 4 digits?
        base = super(TimeRE, self)
        base.__init__({
            # The " \d" option is to make %c from ANSI C work
            'd': r"(?P<d>3[0-1]|[1-2]\d|0[1-9]|[1-9]| [1-9])",
            'H': r"(?P<H>2[0-3]|[0-1]\d|\d)",
            'I': r"(?P<I>1[0-2]|0[1-9]|[1-9])",
            'j': r"(?P<j>36[0-6]|3[0-5]\d|[1-2]\d\d|0[1-9]\d|00[1-9]|[1-9]\d|0[1-9]|[1-9])",
            'm': r"(?P<m>1[0-2]|0[1-9]|[1-9])",
            'M': r"(?P<M>[0-5]\d|\d)",
            'S': r"(?P<S>6[0-1]|[0-5]\d|\d)",
            'U': r"(?P<U>5[0-3]|[0-4]\d|\d)",
            'w': r"(?P<w>[0-6])",
            # W is set below by using 'U'
            'y': r"(?P<y>\d\d)",
            'Y': r"(?P<Y>\d\d\d\d)"})
        base.__setitem__('W', base.__getitem__('U'))
        if locale_time:
            self.locale_time = locale_time
        else:
            self.locale_time = LocaleTime()

    def __getitem__(self, fetch):
        """Try to fetch regex; if it does not exist, construct it."""
        try:
            return super(TimeRE, self).__getitem__(fetch)
        except KeyError:
            constructors = {
                'A': lambda: self.__seqToRE(self.locale_time.f_weekday, fetch),
                'a': lambda: self.__seqToRE(self.locale_time.a_weekday, fetch),
                'B': lambda: self.__seqToRE(self.locale_time.f_month[1:],
                                            fetch),
                'b': lambda: self.__seqToRE(self.locale_time.a_month[1:],
                                            fetch),
                'c': lambda: self.pattern(self.locale_time.LC_date_time),
                'p': lambda: self.__seqToRE(self.locale_time.am_pm, fetch),
                'x': lambda: self.pattern(self.locale_time.LC_date),
                'X': lambda: self.pattern(self.locale_time.LC_time),
                'Z': lambda: self.__seqToRE(self.locale_time.timezone, fetch),
                '%': lambda: '%',
                }
            if fetch in constructors:
                self[fetch] = constructors[fetch]()
                return self[fetch]
            else:
                raise

    def __seqToRE(self, to_convert, directive):
        """Convert a list to a regex string for matching a directive."""
        def sorter(a, b):
            """Sort based on length.

            Done in case for some strange reason that names in the locale only
            differ by a suffix and thus want the name with the suffix to match
            first.
            """
            try:
                a_length = len(a)
            except TypeError:
                a_length = 0
            try:
                b_length = len(b)
            except TypeError:
                b_length = 0
            return cmp(b_length, a_length)

        to_convert = to_convert[:]  # Don't want to change value in-place.
        for value in to_convert:
            if value != '':
                break
        else:
            return ''
        to_convert.sort(sorter)
        regex = '|'.join(to_convert)
        regex = '(?P<%s>%s' % (directive, regex)
        return '%s)' % regex

    def pattern(self, format):
        """Return re pattern for the format string.

        Need to make sure that any characters that might be interpreted as
        regex syntax is escaped.

        """
        processed_format = ''
        # The sub() call escapes all characters that might be misconstrued
        # as regex syntax.
        regex_chars = re_compile(r"([\\.^$*+?{}\[\]|])")
        format = regex_chars.sub(r"\\\1", format)
        whitespace_replacement = re_compile('\s+')
        format = whitespace_replacement.sub('\s*', format)
        while format.find('%') != -1:
            directive_index = format.index('%')+1
            processed_format = "%s%s%s" % (processed_format,
                                           format[:directive_index-1],
                                           self[format[directive_index]])
            format = format[directive_index+1:]
        return "%s%s" % (processed_format, format)

    def compile(self, format):
        """Return a compiled re object for the format string."""
        return re_compile(self.pattern(format), IGNORECASE)


def strptime(data_string, format="%a %b %d %H:%M:%S %Y"):
    """Return a time struct based on the input data and the format string."""
    time_re = TimeRE()
    locale_time = time_re.locale_time
    format_regex = time_re.compile(format)
    found = format_regex.match(data_string)
    if not found:
        raise ValueError("time data did not match format:  data=%s  fmt=%s" %
                         (data_string, format))
    if len(data_string) != found.end():
        raise ValueError("unconverted data remains: %s" %
                          data_string[found.end():])
    year = 1900
    month = day = 1
    hour = minute = second = 0
    tz = -1
    # weekday and julian defaulted to -1 so as to signal need to calculate values
    weekday = julian = -1
    found_dict = found.groupdict()
    for group_key in found_dict.iterkeys():
        if group_key == 'y':
            year = int(found_dict['y'])
            # Open Group specification for strptime() states that a %y
            #value in the range of [00, 68] is in the century 2000, while
            #[69,99] is in the century 1900
            if year <= 68:
                year += 2000
            else:
                year += 1900
        elif group_key == 'Y':
            year = int(found_dict['Y'])
        elif group_key == 'm':
            month = int(found_dict['m'])
        elif group_key == 'B':
            month = _insensitiveindex(locale_time.f_month, found_dict['B'])
        elif group_key == 'b':
            month = _insensitiveindex(locale_time.a_month, found_dict['b'])
        elif group_key == 'd':
            day = int(found_dict['d'])
        elif group_key == 'H':
            hour = int(found_dict['H'])
        elif group_key == 'I':
            hour = int(found_dict['I'])
            ampm = found_dict.get('p', '').lower()
            # If there was no AM/PM indicator, we'll treat this like AM
            if ampm in ('', locale_time.am_pm[0].lower()):
                # We're in AM so the hour is correct unless we're
                # looking at 12 midnight.
                # 12 midnight == 12 AM == hour 0
                if hour == 12:
                    hour = 0
            elif ampm == locale_time.am_pm[1].lower():
                # We're in PM so we need to add 12 to the hour unless
                # we're looking at 12 noon.
                # 12 noon == 12 PM == hour 12
                if hour != 12:
                    hour += 12
        elif group_key == 'M':
            minute = int(found_dict['M'])
        elif group_key == 'S':
            second = int(found_dict['S'])
        elif group_key == 'A':
            weekday = _insensitiveindex(locale_time.f_weekday,
                                        found_dict['A'])
        elif group_key == 'a':
            weekday = _insensitiveindex(locale_time.a_weekday,
                                        found_dict['a'])
        elif group_key == 'w':
            weekday = int(found_dict['w'])
            if weekday == 0:
                weekday = 6
            else:
                weekday -= 1
        elif group_key == 'j':
            julian = int(found_dict['j'])
        elif group_key == 'Z':
            # Since -1 is default value only need to worry about setting tz if
            # it can be something other than -1.
            found_zone = found_dict['Z'].lower()
            if locale_time.timezone[0] == locale_time.timezone[1] and \
               time.daylight:
                pass #Deals with bad locale setup where timezone info is
                     # the same; first found on FreeBSD 4.4.
            elif found_zone in ("utc", "gmt"):
                tz = 0
            elif locale_time.timezone[2].lower() == found_zone:
                tz = 0
            elif time.daylight and \
                locale_time.timezone[3].lower() == found_zone:
                tz = 1

    # Cannot pre-calculate datetime_date() since can change in Julian
    #calculation and thus could have different value for the day of the week
    #calculation
    if julian == -1:
        # Need to add 1 to result since first day of the year is 1, not 0.
        julian = datetime_date(year, month, day).toordinal() - \
                  datetime_date(year, 1, 1).toordinal() + 1
    else:  # Assume that if they bothered to include Julian day it will
           #be accurate
        datetime_result = datetime_date.fromordinal((julian - 1) + datetime_date(year, 1, 1).toordinal())
        year = datetime_result.year
        month = datetime_result.month
        day = datetime_result.day
    if weekday == -1:
        weekday = datetime_date(year, month, day).weekday()
    return time.struct_time((year, month, day,
                             hour, minute, second,
                             weekday, julian, tz))

def _insensitiveindex(lst, findme):
    # Perform a case-insensitive index search.

    #XXX <bc>: If LocaleTime is not exposed, then consider removing this and
    #          just lowercase when LocaleTime sets its vars and lowercasing
    #          search values.
    findme = findme.lower()
    for key,item in enumerate(lst):
        if item.lower() == findme:
            return key
    else:
        raise ValueError("value not in list")
