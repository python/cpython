# module calendar

##############################
# Calendar support functions #
##############################

# This is based on UNIX ctime() et al. (also Standard C and POSIX)
# Subtle but crucial differences:
# - the order of the elements of a 'struct tm' differs, to ease sorting
# - months numbers are 1-12, not 0-11; month arrays have a dummy element 0
# - Monday is the first day of the week (numbered 0)

# These are really parameters of the 'time' module:
epoch = 1970		# Time began on January 1 of this year (00:00:00 UCT)
day_0 = 3		# The epoch begins on a Thursday (Monday = 0)

# Return 1 for leap years, 0 for non-leap years
def isleap(year):
	return year % 4 = 0 and (year % 100 <> 0 or year % 400 = 0)

# Constants for months referenced later
January = 1
February = 2

# Number of days per month (except for February in leap years)
mdays = (0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31)

# Exception raised for bad input (with string parameter for details)
error = 'calendar error'

# Turn seconds since epoch into calendar time
def gmtime(secs):
	if secs < 0: raise error, 'negative input to gmtime()'
	mins, secs = divmod(secs, 60)
	hours, mins = divmod(mins, 60)
	days, hours = divmod(hours, 24)
	wday = (days + day_0) % 7
	year = epoch
	# XXX Most of the following loop can be replaced by one division
	while 1:
		yd = 365 + isleap(year)
		if days < yd: break
		days = days - yd
		year = year + 1
	yday = days
	month = January
	while 1:
		md = mdays[month] + (month = February and isleap(year))
		if days < md: break
		days = days - md
		month = month + 1
	return year, month, days + 1, hours, mins, secs, yday, wday
	# XXX Week number also?

# Return number of leap years in range [y1, y2)
# Assume y1 <= y2 and no funny (non-leap century) years
def leapdays(y1, y2):
	return (y2+3)/4 - (y1+3)/4

# Inverse of gmtime():
# Turn UCT calendar time (less yday, wday) into seconds since epoch
def mktime(year, month, day, hours, mins, secs):
	days = day - 1
	for m in range(January, month): days = days + mdays[m]
	if isleap(year) and month > February: days = days+1
	days = days + (year-epoch)*365 + leapdays(epoch, year)
	return ((days*24 + hours)*60 + mins)*60 + secs

# Full and abbreviated names of weekdays
day_name = ('Monday', 'Tuesday', 'Wednesday', 'Thursday')
day_name = day_name + ('Friday', 'Saturday', 'Sunday')
day_abbr = ('Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun')

# Full and abbreviated of months (1-based arrays!!!)
month_name =          ('', 'January',   'February', 'March',    'April')
month_name = month_name + ('May',       'June',     'July',     'August')
month_name = month_name + ('September', 'October',  'November', 'December')
month_abbr =       ('   ', 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun')
month_abbr = month_abbr + ('Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec')

# Zero-fill string to two positions (helper for asctime())
def dd(s):
	while len(s) < 2: s = '0' + s
	return s

# Blank-fill string to two positions (helper for asctime())
def zd(s):
	while len(s) < 2: s = ' ' + s
	return s

# Turn calendar time as returned by gmtime() into a string
# (the yday parameter is for compatibility with gmtime())
def asctime(year, month, day, hours, mins, secs, yday, wday):
	s = day_abbr[wday] + ' ' + month_abbr[month] + ' ' + zd(`day`)
	s = s + ' ' + dd(`hours`) + ':' + dd(`mins`) + ':' + dd(`secs`)
	return s + ' ' + `year`

# Localization: Minutes West from Greenwich
# timezone = -2*60	# Middle-European time with DST on
timezone = 5*60		# EST (sigh -- THINK time() doesn't return UCT)

# Local time ignores DST issues for now -- adjust 'timezone' to fake it
def localtime(secs):
	return gmtime(secs - timezone*60)

# UNIX-style ctime (except it doesn't append '\n'!)
def ctime(secs):
	return asctime(localtime(secs))

######################
# Non-UNIX additions #
######################

# Calendar printing etc.

# Return weekday (0-6 ~ Mon-Sun) for year (1970-...), month (1-12), day (1-31)
def weekday(year, month, day):
	secs = mktime(year, month, day, 0, 0, 0)
	days = secs / (24*60*60)
	return (days + day_0) % 7

# Return weekday (0-6 ~ Mon-Sun) and number of days (28-31) for year, month
def monthrange(year, month):
	day1 = weekday(year, month, 1)
	ndays = mdays[month] + (month = February and isleap(year))
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
mc_cache = {}
def monthcalendar(year, month):
	key = `year` + month_abbr[month]
	try:
		return mc_cache[key]
	except RuntimeError:
		mc_cache[key] = ret = _monthcalendar(year, month)
		return ret

# Center a string in a field
def center(str, width):
	n = width - len(str)
	if n < 0: return str
	return ' '*(n/2) + str + ' '*(n-n/2)

# XXX The following code knows that print separates items with space!

# Print a single week (no newline)
def prweek(week, width):
	for day in week:
		if day = 0: print ' '*width,
		else:
			if width > 2: print ' '*(width-3),
			if day < 10: print '',
			print day,

# Return a header for a week
def weekheader(width):
	str = ''
	for i in range(7):
		if str: str = str + ' '
		str = str + day_abbr[i%7][:width]
	return str

# Print a month's calendar
def prmonth(year, month):
	print weekheader(3)
	for week in monthcalendar(year, month):
		prweek(week, 3)
		print

# Spacing between month columns
spacing = '    '

# 3-column formatting for year calendars
def format3c(a, b, c):
	print center(a, 20), spacing, center(b, 20), spacing, center(c, 20)

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
					print ' '*20,
				else:
					prweek(cal[i], 2)
				print spacing,
			print
