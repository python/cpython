##############################
# Calendar support functions #
##############################

# Revision 2: uses funtions from built-in time module where possible.

# Import functions and variables from time module
from time import gmtime, localtime, mktime
from time import timezone, altzone, daylight, tzname

# Exception raised for bad input (with string parameter for details)
error = 'calendar.error'

# Abbreviated names of months (1-based arrays!!!)
month_abbr =       ['   ', 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', \
		           'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

# Turn calendar time as returned by localtime() into a string
def asctime(arg):
	year, month, day, hours, mins, secs, wday, yday, isdst = arg
	return '%s %s %02d %02d:%02d:%02d %04d' % (
		day_abbr[wday], month_abbr[month], day,
		hours, mins, secs, year)

# UNIX-style ctime (except it doesn't append '\n'!)
def ctime(secs):
	return asctime(localtime(secs))

######################
# Non-UNIX additions #
######################

# Calendar printing etc.

# Note when comparing these calendars to the ones printed by cal(1):
# My calendars have Monday as the first day of the week, and Sunday as
# the last!  (I believe this is the European convention.)

# Constants for months referenced later
January = 1
February = 2

# Number of days per month (except for February in leap years)
mdays = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

# Full and abbreviated names of weekdays
day_name = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', \
	    'Friday', 'Saturday', 'Sunday']
day_abbr = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

# Full names of months (1-based arrays!!!)
month_name =          ['', 'January',   'February', 'March',    'April', \
		           'May',       'June',     'July',     'August', \
			   'September', 'October',  'November', 'December']

# Return 1 for leap years, 0 for non-leap years
def isleap(year):
	return year % 4 == 0 and (year % 100 <> 0 or year % 400 == 0)

# Return number of leap years in range [y1, y2)
# Assume y1 <= y2 and no funny (non-leap century) years
def leapdays(y1, y2):
	return (y2+3)/4 - (y1+3)/4

# Return weekday (0-6 ~ Mon-Sun) for year (1970-...), month (1-12), day (1-31)
def weekday(year, month, day):
	secs = mktime((year, month, day, 0, 0, 0, 0, 0, 0))
	tuple = localtime(secs)
	return tuple[6]

# Return weekday (0-6 ~ Mon-Sun) and number of days (28-31) for year, month
def monthrange(year, month):
	if not 1 <= month <= 12: raise ValueError, 'bad month number'
	day1 = weekday(year, month, 1)
	ndays = mdays[month] + (month == February and isleap(year))
	return day1, ndays

# Return a matrix representing a month's calendar
# Each row represents a week; days outside this month are zero
def _monthcalendar(year, month):
	day1, ndays = monthrange(year, month)
	rows = []
	r7 = range(7)
	day = 1 - day1
	while day <= ndays:
		row = [0, 0, 0, 0, 0, 0, 0]
		for i in r7:
			if 1 <= day <= ndays: row[i] = day
			day = day + 1
		rows.append(row)
	return rows

# Caching interface to _monthcalendar
_mc_cache = {}
def monthcalendar(year, month):
	key = (year, month)
	if _mc_cache.has_key(key):
		return _mc_cache[key]
	else:
		_mc_cache[key] = ret = _monthcalendar(year, month)
		return ret

# Center a string in a field
def _center(str, width):
	n = width - len(str)
	if n <= 0: return str
	return ' '*((n+1)/2) + str + ' '*((n)/2)

# XXX The following code knows that print separates items with space!

# Print a single week (no newline)
def prweek(week, width):
	for day in week:
		if day == 0: s = ''
		else: s = `day`
		print _center(s, width),

# Return a header for a week
def weekheader(width):
	str = ''
	if width >= 9: names = day_name
	else: names = day_abbr
	for i in range(7):
		if str: str = str + ' '
		str = str + _center(names[i%7][:width], width)
	return str

# Print a month's calendar
def prmonth(year, month, *rest):
	if rest[2:]: raise TypeError, 'too many args'
	w = 0
	l = 0
	if rest[0:]: w = rest[0]
	if rest[1:]: l = rest[1]
	w = max(2, w)
	l = max(1, l)
	print _center(month_name[month] + ' ' + `year`, 7*(w+1) - 1),
	print '\n'*l,
	print weekheader(w),
	print '\n'*l,
	for week in monthcalendar(year, month):
		prweek(week, w)
		print '\n'*l,

# Spacing of month columns
_colwidth = 7*3 - 1			# Amount printed by prweek()
_spacing = ' '*4			# Spaces between columns

# 3-column formatting for year calendars
def format3c(a, b, c):
	print _center(a, _colwidth),
	print _spacing,
	print _center(b, _colwidth),
	print _spacing,
	print _center(c, _colwidth)

# Print a year's calendar
def prcal(year):
	header = weekheader(2)
	format3c('', `year`, '')
	for q in range(January, January+12, 3):
		print
		format3c(month_name[q], month_name[q+1], month_name[q+2])
		format3c(header, header, header)
		data = []
		height = 0
		for month in range(q, q+3):
			cal = monthcalendar(year, month)
			if len(cal) > height: height = len(cal)
			data.append(cal)
		for i in range(height):
			for cal in data:
				if i >= len(cal):
					print ' '*_colwidth,
				else:
					prweek(cal[i], 2)
				print _spacing,
			print
