"""Strptime-related classes and functions.

CLASSES:
    LocaleTime -- Discovers and/or stores locale-specific time information
    TimeRE -- Creates regexes for pattern matching string of text containing 
                time information as is returned by time.strftime()

FUNCTIONS:
    firstjulian -- Calculates the Julian date up to the first of the specified
                    year
    gregorian -- Calculates the Gregorian date based on the Julian day and
                    year
    julianday -- Calculates the Julian day since the first of the year based 
                    on the Gregorian date
    dayofweek -- Calculates the day of the week from the Gregorian date.
    strptime -- Calculates the time struct represented by the passed-in string

Requires Python 2.2.1 or higher.
Can be used in Python 2.2 if the following line is added:
    >>> True = 1; False = 0

"""
import time
import locale
import calendar
from re import compile as re_compile
from re import IGNORECASE
from string import whitespace as whitespace_string

__version__ = (2,1,5)
__author__ = "Brett Cannon"
__email__ = "drifty@bigfoot.com"

__all__ = ['strptime']

class LocaleTime(object):
    """Stores and handles locale-specific information related to time.

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
    a_month=None, am_pm=None, LC_date_time=None, LC_time=None, LC_date=None, 
    timezone=None, lang=None):
        """Optionally set attributes with passed-in values."""
        if f_weekday is None: self.__f_weekday = None
        elif len(f_weekday) == 7: self.__f_weekday = list(f_weekday)
        else:
            raise TypeError("full weekday names must be a 7-item sequence")
        if a_weekday is None: self.__a_weekday = None
        elif len(a_weekday) == 7: self.__a_weekday = list(a_weekday)
        else:
            raise TypeError(
                    "abbreviated weekday names must be a 7-item  sequence")
        if f_month is None: self.__f_month = None
        elif len(f_month) == 12:
            self.__f_month = self.__pad(f_month, True)
        else:
            raise TypeError("full month names must be a 12-item sequence")
        if a_month is None: self.__a_month = None
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
        self.__lang = lang

    def __pad(self, seq, front):
        """Add '' to seq to either front (is True), else the back."""
        seq = list(seq)
        if front: seq.insert(0, '')
        else: seq.append('')
        return seq

    def __set_nothing(self, stuff):
        """Raise TypeError when trying to set an attribute."""
        raise TypeError("attribute does not support assignment")

    def __get_f_weekday(self):
        """Fetch self.f_weekday."""
        if not self.__f_weekday: self.__calc_weekday()
        return self.__f_weekday

    def __get_a_weekday(self):
        """Fetch self.a_weekday."""
        if not self.__a_weekday: self.__calc_weekday()
        return self.__a_weekday

    f_weekday = property(__get_f_weekday, __set_nothing, 
                        doc="Full weekday names")
    a_weekday = property(__get_a_weekday, __set_nothing, 
                        doc="Abbreviated weekday names")

    def __get_f_month(self):
        """Fetch self.f_month."""
        if not self.__f_month: self.__calc_month()
        return self.__f_month

    def __get_a_month(self):
        """Fetch self.a_month."""
        if not self.__a_month: self.__calc_month()
        return self.__a_month

    f_month = property(__get_f_month, __set_nothing,
                        doc="Full month names (dummy value at index 0)")
    a_month = property(__get_a_month, __set_nothing,
                        doc="Abbreviated month names (dummy value at index 0)")

    def __get_am_pm(self):
        """Fetch self.am_pm."""
        if not self.__am_pm: self.__calc_am_pm()
        return self.__am_pm

    am_pm = property(__get_am_pm, __set_nothing, doc="AM/PM representation")

    def __get_timezone(self):
        """Fetch self.timezone."""
        if not self.__timezone: self.__calc_timezone()
        return self.__timezone

    timezone = property(__get_timezone, __set_nothing,
                        doc="Timezone representation (dummy value at index 2)")

    def __get_LC_date_time(self):
        """Fetch self.LC_date_time."""
        if not self.__LC_date_time: self.__calc_date_time()
        return self.__LC_date_time

    def __get_LC_date(self):
        """Fetch self.LC_date."""
        if not self.__LC_date: self.__calc_date_time()
        return self.__LC_date

    def __get_LC_time(self):
        """Fetch self.LC_time."""
        if not self.__LC_time: self.__calc_date_time()
        return self.__LC_time

    LC_date_time = property(__get_LC_date_time, __set_nothing,
        doc="Format string for locale's date/time representation ('%c' format)")
    LC_date = property(__get_LC_date, __set_nothing,
        doc="Format string for locale's date representation ('%x' format)")
    LC_time = property(__get_LC_time, __set_nothing,
        doc="Format string for locale's time representation ('%X' format)")

    def __get_lang(self):
        """Fetch self.lang."""
        if not self.__lang: self.__calc_lang()
        return self.__lang

    lang = property(__get_lang, __set_nothing, doc="Language used for instance")

    def __calc_weekday(self):
        """Set self.__a_weekday and self.__f_weekday using the calendar module."""
        a_weekday = [calendar.day_abbr[i] for i in range(7)]
        f_weekday = [calendar.day_name[i] for i in range(7)]
        if not self.__a_weekday: self.__a_weekday = a_weekday
        if not self.__f_weekday: self.__f_weekday = f_weekday
        
    def __calc_month(self):
        """Set self.__f_month and self.__a_month using the calendar module."""
        a_month = [calendar.month_abbr[i] for i in range(13)]
        f_month = [calendar.month_name[i] for i in range(13)]
        if not self.__a_month: self.__a_month = a_month
        if not self.__f_month: self.__f_month = f_month

    def __calc_am_pm(self):
        """Set self.__am_pm by using time.strftime().
        
        The magic date (2002, 3, 17, hour, 44, 44, 2, 76, 0) is not really 
        that magical; just happened to have used it everywhere else where a 
        static date was needed.
        
        """
        am_pm = []
        for hour in (01,22):
            time_tuple = time.struct_time((1999,3,17,hour,44,55,2,76,0))
            am_pm.append(time.strftime("%p", time_tuple))
        self.__am_pm = am_pm

    def __calc_date_time(self):
        """Set self.__date_time, self.__date, & self.__time by using time.strftime().
        
        Use (1999,3,17,22,44,55,2,76,0) for magic date because the amount of 
        overloaded numbers is minimized.  The order in which searches for 
        values within the format string is very important; it eliminates 
        possible ambiguity for what something represents.

        """
        time_tuple = time.struct_time((1999,3,17,22,44,55,2,76,0))
        date_time = [None, None, None]
        date_time[0] = time.strftime("%c", time_tuple)
        date_time[1] = time.strftime("%x", time_tuple)
        date_time[2] = time.strftime("%X", time_tuple)
        for offset,directive in ((0,'%c'), (1,'%x'), (2,'%X')):
            current_format = date_time[offset]
            current_format = current_format.replace('%', '%%')
            current_format = current_format.replace(self.f_weekday[2], '%A')
            current_format = current_format.replace(self.f_month[3], '%B')
            current_format = current_format.replace(self.a_weekday[2], '%a')
            current_format = current_format.replace(self.a_month[3], '%b')
            current_format = current_format.replace(self.am_pm[1], '%p')
            current_format = current_format.replace(self.timezone[0], '%Z')
            current_format = current_format.replace(self.timezone[1], '%Z')
            current_format = current_format.replace('1999', '%Y')
            current_format = current_format.replace('99', '%y')
            current_format = current_format.replace('22', '%H')
            current_format = current_format.replace('44', '%M')
            current_format = current_format.replace('55', '%S')
            current_format = current_format.replace('76', '%j')
            current_format = current_format.replace('17', '%d')
            current_format = current_format.replace('03', '%m')
            current_format = current_format.replace('2', '%w')
            current_format = current_format.replace('10', '%I')
            time_tuple = time.struct_time((1999,1,3,1,1,1,6,3,0))
            if time.strftime(directive, time_tuple).find('00'):
                U_W = '%U'
            else:
                U_W = '%W'
            date_time[offset] = current_format.replace('11', U_W)
        if not self.__LC_date_time: self.__LC_date_time = date_time[0]
        if not self.__LC_date: self.__LC_date = date_time[1]
        if not self.__LC_time: self.__LC_time = date_time[2]

    def __calc_timezone(self):
        """Set self.__timezone by using time.tzname.
        
        Empty string used for matching when timezone is not used/needed such 
        as with UTC.

        """
        self.__timezone = self.__pad(time.tzname, 0)

    def __calc_lang(self):
        """Set self.lang by using locale.getlocale() or 
        locale.getdefaultlocale().
        
        """
        current_lang = locale.getlocale(locale.LC_TIME)[0]
        if current_lang: self.__lang = current_lang
        else: self.__lang = locale.getdefaultlocale()[0]

class TimeRE(dict):
    """Handle conversion from format directives to regexes."""

    def __init__(self, locale_time=LocaleTime()):
        """Initialize instance with non-locale regexes and store LocaleTime object."""
        super(TimeRE,self).__init__({
            'd': r"(?P<d>3[0-1]|[0-2]\d|\d| \d)",  #The " \d" option is 
                                                         #to make %c from ANSI
                                                         #C work
            'H': r"(?P<H>2[0-3]|[0-1]\d|\d)",
            'I': r"(?P<I>0\d|1[0-2]|\d)",
            'j': r"(?P<j>(?:3[0-5]\d|6[0-6])|[0-2]\d\d|\d)",
            'm': r"(?P<m>0\d|1[0-2]|\d)",
            'M': r"(?P<M>[0-5]\d|\d)",
            'S': r"(?P<S>6[0-1]|[0-5]\d|\d)",
            'U': r"(?P<U>5[0-3]|[0-4]\d|\d)",
            'w': r"(?P<w>[0-6])",
            'W': r"(?P<W>5[0-3]|[0-4]\d|\d)",  #Same as U
            'y': r"(?P<y>\d\d)",
            'Y': r"(?P<Y>\d\d\d\d)"})
        self.locale_time = locale_time

    def __getitem__(self, fetch):
        """Try to fetch regex; if it does not exist, construct it."""
        try:
            return super(TimeRE,self).__getitem__(fetch)
        except KeyError:
            if fetch == 'A':
                self[fetch] = self.__seqToRE(self.locale_time.f_weekday, 
                                                fetch)
            elif fetch == 'a':
                self[fetch] = self.__seqToRE(self.locale_time.a_weekday, 
                                                fetch)
            elif fetch == 'B':
                self[fetch] = self.__seqToRE(self.locale_time.f_month[1:], 
                                                fetch)
            elif fetch == 'b':
                self[fetch] = self.__seqToRE(self.locale_time.a_month[1:], 
                                                fetch)
            elif fetch == 'c':
                self[fetch] = self.pattern(self.locale_time.LC_date_time)
            elif fetch == 'p':
                self[fetch] = self.__seqToRE(self.locale_time.am_pm, fetch)
            elif fetch == 'x':
                self[fetch] = self.pattern(self.locale_time.LC_date)
            elif fetch == 'X':
                self[fetch] = self.pattern(self.locale_time.LC_time)
            elif fetch == 'Z':
                self[fetch] = self.__seqToRE(self.locale_time.timezone, 
                                                fetch)
            elif fetch == '%':
                return '%'
            return super(TimeRE,self).__getitem__(fetch)
        
    def __seqToRE(self, to_convert, directive):
        """Convert a list to a regex string for matching directive."""
        def sorter(a, b):
            """Sort based on length.
            
            Done in case for some strange reason that names in the locale only
            differ by a suffix and thus want the name with the suffix to match
            first.
            
            """
            try: a_length = len(a)
            except TypeError: a_length = 0
            try: b_length = len(b)
            except TypeError: b_length = 0
            return cmp(b_length, a_length)
        
        to_convert = to_convert[:]  #Don't want to change value in-place.
        to_convert.sort(sorter)
        regex = '(?P<%s>' % directive
        for item in to_convert:
            regex = "%s(?:%s)|" % (regex, item)
        else:
            regex = regex[:-1]
        return '%s)' % regex

    def pattern(self, format):
        """Return re pattern for the format string."""
        processed_format = ''
        for whitespace in whitespace_string:
            format = format.replace(whitespace, r'\s*')
        while format.find('%') != -1:
            directive_index = format.index('%')+1
            processed_format = "%s%s%s" % (processed_format, 
                                format[:directive_index-1],
                                self[format[directive_index]])
            format = format[directive_index+1:]
        return "%s%s" % (processed_format, format)

    def compile(self, format):
        """Return a compiled re object for the format string."""
        format = "(?#%s)%s" % (self.locale_time.lang,format)
        return re_compile(self.pattern(format), IGNORECASE)


def strptime(data_string, format="%a %b %d %H:%M:%S %Y"):
    """Convert data_string to a time struct based on the format string or re object; will return an re object for format if data_string is False.
    
    The object passed in for format may either be a re object compiled by 
    strptime() or a format string.  If False is passed in for data_string 
    then an re object for format will be returned.  The re object 
    must be used with the same language as used to compile the re object.
    
    """
    locale_time = LocaleTime()
    if isinstance(format, type(re_compile(''))):
        if format.pattern.find(locale_time.lang) == -1:
            raise TypeError("re object not created with same language as \
            LocaleTime instance")
        else:
            compiled_re = format
    else:
        compiled_re = TimeRE(locale_time).compile(format)
    if data_string is False:
        return compiled_re
    else:
        found = compiled_re.match(data_string)
        if not found:
            raise ValueError("time data did not match format")
        year = month = day = hour = minute = second = weekday = julian = tz = -1
        found_dict = found.groupdict()
        for group_key in found_dict.iterkeys():
            if group_key in 'yY':
                if group_key is 'y':
                    year = int("%s%s" % (time.strftime("%Y")[:-2], found_dict['y']))
                else:
                    year = int(found_dict['Y'])
            elif group_key in 'Bbm':
                if group_key is 'm':
                    month = int(found_dict['m'])
                elif group_key is 'B':
                    month = locale_time.f_month.index(found_dict['B'])
                else:
                    month = locale_time.a_month.index(found_dict['b'])
            elif group_key is 'd':
                day = int(found_dict['d'])
            elif group_key in 'HI':
                if group_key is 'H':
                    hour = int(found_dict['H'])
                else:
                    hour = int(found_dict['I'])
                    if found_dict.has_key('p'):
                        if found_dict['p'] == locale_time.am_pm[1]:
                            hour += 12
                        else:
                            if hour is 12:
                                hour = 0
            elif group_key is 'M':
                minute = int(found_dict['M'])
            elif group_key is 'S':
                second = int(found_dict['S'])
            elif group_key in 'Aaw':
                if group_key is 'A':
                    weekday = locale_time.f_weekday.index(found_dict['A'])
                elif group_key is 'a':
                    weekday = locale_time.a_weekday.index(found_dict['a'])
                else:
                    weekday = int(found_dict['w'])
                    if weekday == 0:
                        weekday = 6
                    else:
                        weekday -= 1
            elif group_key is 'j':
                julian = int(found_dict['j'])
            elif group_key is 'Z':
                if locale_time.timezone[0] == found_dict['Z']:
                    tz = 0
                elif locale_time.timezone[1] == found_dict['Z']:
                    tz = 1
                elif locale_time.timezone[2] == found_dict['Z']:
                    tz = 0
        if julian == -1 and year != -1 and month != -1 and day != -1:
            julian = julianday(year, month, day)
        if (month == -1 or day == -1) and julian != -1 and year != -1:
            year,month,day = gregorian(julian, year)
        if weekday == -1 and year != -1 and month != -1 and day != -1:
            weekday = dayofweek(year, month, day)
        return time.struct_time((year,month,day,hour,minute,second,weekday,
                                julian,tz))

def firstjulian(year):
    """Calculate the Julian date up until the first of the year."""
    return ((146097*(year+4799))//400)-31738

def julianday(year, month, day):
    """Calculate the Julian day since the beginning of the year from the Gregorian date."""
    a = (14-month)//12
    return (day-32045+(((153*(month+(12*a)-3))+2)//5)+\
    ((146097*(year+4800-a))//400))-firstjulian(year)+1

def gregorian(julian, year):
    """Return a 3-item list containing the Gregorian date based on the Julian day."""
    a = 32043+julian+firstjulian(year)
    b = ((4*a)+3)//146097
    c = a-((146097*b)//4)
    d = ((4*c)+3)//1461
    e = c-((1461*d)//4)
    m = ((5*e)+2)//153
    day = 1+e-(((153*m)+2)//5)
    month = m+3-(12*(m//10))
    year = (100*b)+d-4800+(m//10)
    return [year, month, day]

def dayofweek(year, month, day):
    """Calculate the day of the week (Monday is 0)."""
    a = (14-month)//12
    y = year-a
    weekday = (day+y+((97*y)//400)+((31*(month+(12*a)-2))//12))%7
    if weekday == 0:
        return 6
    else:
        return weekday-1
