"""Calendar printing functions

Note when comparing these calendars to the ones printed by cal(1): By
default, these calendars have Monday as the first day of the week, and
Sunday as the last (the European convention). Use setfirstweekday() to
set the first day of the week (0=Monday, 6=Sunday)."""

# Revision 2: uses functions from built-in time module

# Import functions and variables from time module
from time import localtime, mktime

# Exception raised for bad input (with string parameter for details)
error = ValueError

# Constants for months referenced later
January = 1
February = 2

# Number of days per month (except for February in leap years)
mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

# Full and abbreviated names of weekdays
day_name = ['Monday', 'Tuesday', 'Wednesday', 'Thursday',
            'Friday', 'Saturday', 'Sunday']
day_abbr = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

# Full and abbreviated names of months (1-based arrays!!!)
month_name = ['', 'January', 'February', 'March', 'April',
              'May', 'June', 'July', 'August',
              'September', 'October',  'November', 'December']
month_abbr = ['   ', 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
              'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

# Constants for weekdays
(MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY) = range(7)

_firstweekday = 0                       # 0 = Monday, 6 = Sunday

def firstweekday():
    return _firstweekday

def setfirstweekday(weekday):
    """Set weekday (Monday=0, Sunday=6) to start each week."""
    global _firstweekday
    if not MONDAY <= weekday <= SUNDAY:
        raise ValueError, \
              'bad weekday number; must be 0 (Monday) to 6 (Sunday)'
    _firstweekday = weekday

def isleap(year):
    """Return 1 for leap years, 0 for non-leap years."""
    return year % 4 == 0 and (year % 100 <> 0 or year % 400 == 0)

def leapdays(y1, y2):
    """Return number of leap years in range [y1, y2).
       Assume y1 <= y2."""
    y1 -= 1
    y2 -= 1
    return (y2/4 - y1/4) - (y2/100 - y1/100) + (y2/400 - y1/400)

def weekday(year, month, day):
    """Return weekday (0-6 ~ Mon-Sun) for year (1970-...), month (1-12),
       day (1-31)."""
    secs = mktime((year, month, day, 0, 0, 0, 0, 0, 0))
    tuple = localtime(secs)
    return tuple[6]

def monthrange(year, month):
    """Return weekday (0-6 ~ Mon-Sun) and number of days (28-31) for
       year, month."""
    if not 1 <= month <= 12:
        raise ValueError, 'bad month number'
    day1 = weekday(year, month, 1)
    ndays = mdays[month] + (month == February and isleap(year))
    return day1, ndays

def monthcalendar(year, month):
    """Return a matrix representing a month's calendar.
       Each row represents a week; days outside this month are zero."""
    day1, ndays = monthrange(year, month)
    rows = []
    r7 = range(7)
    day = (_firstweekday - day1 + 6) % 7 - 5   # for leading 0's in first week
    while day <= ndays:
        row = [0, 0, 0, 0, 0, 0, 0]
        for i in r7:
            if 1 <= day <= ndays: row[i] = day
            day = day + 1
        rows.append(row)
    return rows

def _center(str, width):
    """Center a string in a field."""
    n = width - len(str)
    if n <= 0:
        return str
    return ' '*((n+1)/2) + str + ' '*((n)/2)

def prweek(theweek, width):
    """Print a single week (no newline)."""
    print week(theweek, width),

def week(theweek, width):
    """Returns a single week in a string (no newline)."""
    days = []
    for day in theweek:
        if day == 0:
            s = ''
        else:
            s = '%2i' % day             # right-align single-digit days
        days.append(_center(s, width))
    return ' '.join(days)

def weekheader(width):
    """Return a header for a week."""
    if width >= 9:
        names = day_name
    else:
        names = day_abbr
    days = []
    for i in range(_firstweekday, _firstweekday + 7):
        days.append(_center(names[i%7][:width], width))
    return ' '.join(days)

def prmonth(theyear, themonth, w=0, l=0):
    """Print a month's calendar."""
    print month(theyear, themonth, w, l),

def month(theyear, themonth, w=0, l=0):
    """Return a month's calendar string (multi-line)."""
    w = max(2, w)
    l = max(1, l)
    s = (_center(month_name[themonth] + ' ' + `theyear`, 
                 7 * (w + 1) - 1).rstrip() +
         '\n' * l + weekheader(w).rstrip() + '\n' * l)
    for aweek in monthcalendar(theyear, themonth):
        s = s + week(aweek, w).rstrip() + '\n' * l
    return s[:-l] + '\n'

# Spacing of month columns for 3-column year calendar
_colwidth = 7*3 - 1         # Amount printed by prweek()
_spacing = 6                # Number of spaces between columns

def format3c(a, b, c, colwidth=_colwidth, spacing=_spacing):
    """Prints 3-column formatting for year calendars"""
    print format3cstring(a, b, c, colwidth, spacing)

def format3cstring(a, b, c, colwidth=_colwidth, spacing=_spacing):
    """Returns a string formatted from 3 strings, centered within 3 columns."""
    return (_center(a, colwidth) + ' ' * spacing + _center(b, colwidth) +
            ' ' * spacing + _center(c, colwidth))

def prcal(year, w=0, l=0, c=_spacing):
    """Print a year's calendar."""
    print calendar(year, w, l, c),

def calendar(year, w=0, l=0, c=_spacing):
    """Returns a year's calendar as a multi-line string."""
    w = max(2, w)
    l = max(1, l)
    c = max(2, c)
    colwidth = (w + 1) * 7 - 1
    s = _center(`year`, colwidth * 3 + c * 2).rstrip() + '\n' * l
    header = weekheader(w)
    header = format3cstring(header, header, header, colwidth, c).rstrip()
    for q in range(January, January+12, 3):
        s = (s + '\n' * l +
             format3cstring(month_name[q], month_name[q+1], month_name[q+2],
                            colwidth, c).rstrip() + 
             '\n' * l + header + '\n' * l)
        data = []
        height = 0
        for amonth in range(q, q + 3):
            cal = monthcalendar(year, amonth)
            if len(cal) > height:
                height = len(cal)
            data.append(cal)
        for i in range(height):
            weeks = []
            for cal in data:
                if i >= len(cal):
                    weeks.append('')
                else:
                    weeks.append(week(cal[i], w))
            s = s + format3cstring(weeks[0], weeks[1], weeks[2], 
                                   colwidth, c).rstrip() + '\n' * l
    return s[:-l] + '\n'

EPOCH = 1970
def timegm(tuple):
    """Unrelated but handy function to calculate Unix timestamp from GMT."""
    year, month, day, hour, minute, second = tuple[:6]
    assert year >= EPOCH
    assert 1 <= month <= 12
    days = 365*(year-EPOCH) + leapdays(EPOCH, year)
    for i in range(1, month):
        days = days + mdays[i]
    if month > 2 and isleap(year):
        days = days + 1
    days = days + day - 1
    hours = days*24 + hour
    minutes = hours*60 + minute
    seconds = minutes*60 + second
    return seconds
