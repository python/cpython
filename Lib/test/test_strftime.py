#! /usr/bin/env python

# Sanity checker for time.strftime

import time, calendar, sys, string, os
from test_support import verbose

if verbose:
    now = time.time()
    fp = os.popen('date')
    fromdate = string.strip(fp.readline())
    fp.close()
else:
    now = 850499890.282			      # time.time()
    fromdate = 'Fri Dec 13 12:58:10 EST 1996' # os.popen('date')

nowsecs = int(now)
gmt = time.gmtime(now)
now = time.localtime(now)

if gmt[3] < 12: ampm='AM'
else: ampm='PM'

jan1 = time.localtime(time.mktime((now[0], 1, 1) + (0,)*6))
wk1offset = jan1[6] - 6

if now[8]: tz = time.tzname[1]
else: tz = time.tzname[0]

if now[3] > 12: clock12 = now[3] - 12
else: clock12 = now[3]

# descriptions are a mixture of those from the BSD/OS v2.0 man page
# (known to be incorrect in some instances) and the documentation for
# Python's time module

expectations = (
    ('%A', calendar.day_name[now[6]], 'full weekday name'),
    ('%a', calendar.day_abbr[now[6]], 'abbreviated weekday name'),
    ('%B', calendar.month_name[now[1]], 'full month name'),
    ('%b', calendar.month_abbr[now[1]], 'abbreviated month name'),
    ('%h', calendar.month_abbr[now[1]], 'abbreviated month name'),
    ('%c', time.asctime(now), 'asctime() format'),
    ('%D', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)), 'mm/dd/yy'),
    ('%d', '%02d' % now[2], 'day of month as number (00-31)'),
    ('%e', '%2d' % now[2], 'day of month as number, blank padded ( 0-31)'),
    ('%H', '%02d' % now[3], 'hour (00-23)'),
    ('%I', '%02d' % clock12, 'hour (01-12)'),
    ('%j', '%03d' % now[7], 'julian day (001-366)'),
    ('%M', '%02d' % now[4], 'minute, (00-59)'),
    ('%m', '%02d' % now[1], 'month as number (01-12)'),
    ('%p', ampm, 'AM or PM as appropriate'),
    ('%R', '%02d:%02d' % (now[3], now[4]), '%H:%M'),
    ('%r', '%02d:%02d:%02d %s' % (clock12, now[4], now[5], ampm),
     '%I:%M:%S %p'),
    ('%S', '%02d' % now[5], 'seconds of current time (00-60)'),
    ('%T', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
    ('%X', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
    ('%U', '%02d' % (1+(wk1offset+now[7])/7),
     'week number of the year (Sun 1st)'),
    ('%W', '%02d' % (1+now[7]/7), 'week number of the year (Mon 1st)'),
    ('%w', '%d' % (1+now[6]), 'weekday as a number (Sun 1st)'),
    ('%x', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)),
     '%m/%d/%y %H:%M:%S'),
    ('%Y', '%d' % now[0], 'year with century'),
    ('%y', '%02d' % (now[0]%100), 'year without century'),
    ('%Z', tz, 'time zone name'),
    ('%%', '%', 'single percent sign'),
    )

nonstandard_expectations = (
    ('%C', fromdate, 'date(1) format'),
    ('%k', '%2d' % now[3], 'hour, blank padded ( 0-23)'),
    ('%s', '%d' % nowsecs, 'seconds since the Epoch in UCT'),
    ('%3y', '%03d' % (now[0]%100),
     'year without century rendered using fieldwidth'),
    ('%n', '\n', 'newline character'),
    ('%t', '\t', 'tab character'),
    )

if verbose:
    print "Strftime test, platform: %s, Python version: %s" % \
	  (sys.platform, string.split(sys.version)[0])

for e in expectations:
    result = time.strftime(e[0], now)
    if result == e[1]: continue
    if result[0] == '%':
	print "Does not support standard '%s' format (%s)" % (e[0], e[2])
    else:
	print "Conflict for %s (%s):" % (e[0], e[2])
	print "  Expected %s, but got %s" % (e[1], result)

for e in nonstandard_expectations:
    result = time.strftime(e[0], now)
    if result == e[1]:
	if verbose:
	    print "Supports nonstandard '%s' format (%s)" % (e[0], e[2])
    elif result[0] == '%':
	if verbose:
	    print "Does not appear to support '%s' format" % e[0]
    else:
	if verbose:
	    print "Conflict for %s (%s):" % (e[0], e[2])
	    print "  Expected %s, but got %s" % (e[1], result)
