# Class Date supplies date objects that support date arithmetic.
#
# Date(month,day,year) returns a Date object.  An instance prints as,
# e.g., 'Mon 16 Aug 1993'.
#
# Addition, subtraction, comparison operators, min, max, and sorting
# all work as expected for date objects:  int+date or date+int returns
# the date `int' days from `date'; date+date raises an exception;
# date-int returns the date `int' days before `date'; date2-date1 returns
# an integer, the number of days from date1 to date2; int-date raises an
# exception; date1 < date2 is true iff date1 occurs before date2 (&
# similarly for other comparisons); min(date1,date2) is the earlier of
# the two dates and max(date1,date2) the later; and date objects can be
# used as dictionary keys.
#
# Date objects support one visible method, date.weekday().  This returns
# the day of the week the date falls on, as a string.
#
# Date objects also have 4 read-only data attributes:
#   .month  in 1..12
#   .day    in 1..31
#   .year   int or long int
#   .ord    the ordinal of the date relative to an arbitrary staring point
#
# The Dates module also supplies function today(), which returns the
# current date as a date object.
#
# Those entranced by calendar trivia will be disappointed, as no attempt
# has been made to accommodate the Julian (etc) system.  On the other
# hand, at least this package knows that 2000 is a leap year but 2100
# isn't, and works fine for years with a hundred decimal digits <wink>.

# Tim Peters   tim@ksr.com
# not speaking for Kendall Square Research Corp

# Adapted to Python 1.1 (where some hacks to overcome coercion are unnecessary)
# by Guido van Rossum

# Note that as of Python 2.3, a datetime module is included in the stardard
# library.

# vi:set tabsize=8:

_MONTH_NAMES = [ 'January', 'February', 'March', 'April', 'May',
                 'June', 'July', 'August', 'September', 'October',
                 'November', 'December' ]

_DAY_NAMES = [ 'Friday', 'Saturday', 'Sunday', 'Monday',
               'Tuesday', 'Wednesday', 'Thursday' ]

_DAYS_IN_MONTH = [ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 ]

_DAYS_BEFORE_MONTH = []
dbm = 0
for dim in _DAYS_IN_MONTH:
    _DAYS_BEFORE_MONTH.append(dbm)
    dbm = dbm + dim
del dbm, dim

_INT_TYPES = type(1), type(1L)

def _is_leap(year):           # 1 if leap year, else 0
    if year % 4 != 0: return 0
    if year % 400 == 0: return 1
    return year % 100 != 0

def _days_in_year(year):      # number of days in year
    return 365 + _is_leap(year)

def _days_before_year(year):  # number of days before year
    return year*365L + (year+3)//4 - (year+99)//100 + (year+399)//400

def _days_in_month(month, year):      # number of days in month of year
    if month == 2 and _is_leap(year): return 29
    return _DAYS_IN_MONTH[month-1]

def _days_before_month(month, year):  # number of days in year before month
    return _DAYS_BEFORE_MONTH[month-1] + (month > 2 and _is_leap(year))

def _date2num(date):          # compute ordinal of date.month,day,year
    return _days_before_year(date.year) + \
           _days_before_month(date.month, date.year) + \
           date.day

_DI400Y = _days_before_year(400)      # number of days in 400 years

def _num2date(n):             # return date with ordinal n
    if type(n) not in _INT_TYPES:
        raise TypeError, 'argument must be integer: %r' % type(n)

    ans = Date(1,1,1)   # arguments irrelevant; just getting a Date obj
    del ans.ord, ans.month, ans.day, ans.year # un-initialize it
    ans.ord = n

    n400 = (n-1)//_DI400Y                # # of 400-year blocks preceding
    year, n = 400 * n400, n - _DI400Y * n400
    more = n // 365
    dby = _days_before_year(more)
    if dby >= n:
        more = more - 1
        dby = dby - _days_in_year(more)
    year, n = year + more, int(n - dby)

    try: year = int(year)               # chop to int, if it fits
    except (ValueError, OverflowError): pass

    month = min(n//29 + 1, 12)
    dbm = _days_before_month(month, year)
    if dbm >= n:
        month = month - 1
        dbm = dbm - _days_in_month(month, year)

    ans.month, ans.day, ans.year = month, n-dbm, year
    return ans

def _num2day(n):      # return weekday name of day with ordinal n
    return _DAY_NAMES[ int(n % 7) ]


class Date:
    def __init__(self, month, day, year):
        if not 1 <= month <= 12:
            raise ValueError, 'month must be in 1..12: %r' % (month,)
        dim = _days_in_month(month, year)
        if not 1 <= day <= dim:
            raise ValueError, 'day must be in 1..%r: %r' % (dim, day)
        self.month, self.day, self.year = month, day, year
        self.ord = _date2num(self)

    # don't allow setting existing attributes
    def __setattr__(self, name, value):
        if self.__dict__.has_key(name):
            raise AttributeError, 'read-only attribute ' + name
        self.__dict__[name] = value

    def __cmp__(self, other):
        return cmp(self.ord, other.ord)

    # define a hash function so dates can be used as dictionary keys
    def __hash__(self):
        return hash(self.ord)

    # print as, e.g., Mon 16 Aug 1993
    def __repr__(self):
        return '%.3s %2d %.3s %r' % (
              self.weekday(),
              self.day,
              _MONTH_NAMES[self.month-1],
              self.year)

    # Python 1.1 coerces neither int+date nor date+int
    def __add__(self, n):
        if type(n) not in _INT_TYPES:
            raise TypeError, 'can\'t add %r to date' % type(n)
        return _num2date(self.ord + n)
    __radd__ = __add__ # handle int+date

    # Python 1.1 coerces neither date-int nor date-date
    def __sub__(self, other):
        if type(other) in _INT_TYPES:           # date-int
            return _num2date(self.ord - other)
        else:
            return self.ord - other.ord         # date-date

    # complain about int-date
    def __rsub__(self, other):
        raise TypeError, 'Can\'t subtract date from integer'

    def weekday(self):
        return _num2day(self.ord)

def today():
    import time
    local = time.localtime(time.time())
    return Date(local[1], local[2], local[0])

class DateTestError(Exception):
    pass

def test(firstyear, lastyear):
    a = Date(9,30,1913)
    b = Date(9,30,1914)
    if repr(a) != 'Tue 30 Sep 1913':
        raise DateTestError, '__repr__ failure'
    if (not a < b) or a == b or a > b or b != b:
        raise DateTestError, '__cmp__ failure'
    if a+365 != b or 365+a != b:
        raise DateTestError, '__add__ failure'
    if b-a != 365 or b-365 != a:
        raise DateTestError, '__sub__ failure'
    try:
        x = 1 - a
        raise DateTestError, 'int-date should have failed'
    except TypeError:
        pass
    try:
        x = a + b
        raise DateTestError, 'date+date should have failed'
    except TypeError:
        pass
    if a.weekday() != 'Tuesday':
        raise DateTestError, 'weekday() failure'
    if max(a,b) is not b or min(a,b) is not a:
        raise DateTestError, 'min/max failure'
    d = {a-1:b, b:a+1}
    if d[b-366] != b or d[a+(b-a)] != Date(10,1,1913):
        raise DateTestError, 'dictionary failure'

    # verify date<->number conversions for first and last days for
    # all years in firstyear .. lastyear

    lord = _days_before_year(firstyear)
    y = firstyear
    while y <= lastyear:
        ford = lord + 1
        lord = ford + _days_in_year(y) - 1
        fd, ld = Date(1,1,y), Date(12,31,y)
        if (fd.ord,ld.ord) != (ford,lord):
            raise DateTestError, ('date->num failed', y)
        fd, ld = _num2date(ford), _num2date(lord)
        if (1,1,y,12,31,y) != \
           (fd.month,fd.day,fd.year,ld.month,ld.day,ld.year):
            raise DateTestError, ('num->date failed', y)
        y = y + 1

if __name__ == '__main__':
    test(1850, 2150)
