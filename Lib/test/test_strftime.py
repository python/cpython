#! /usr/bin/env python

# Sanity checker for time.strftime

import time, calendar, sys, string, os
from test_support import verbose

def main():
    global verbose
    now = time.time()
    strftest(now)
    verbose = 0
    # Try a bunch of dates and times,  chosen to vary through time of
    # day and daylight saving time
    for j in range(-5, 5):
	for i in range(25):
	    strftest(now + (i + j*100)*23*3603)

def strftest(now):
    if verbose:
	print "strftime test for", time.ctime(now)
    nowsecs = str(long(now))[:-1]
    gmt = time.gmtime(now)
    now = time.localtime(now)

    if now[3] < 12: ampm='AM'
    else: ampm='PM'

    jan1 = time.localtime(time.mktime((now[0], 1, 1) + (0,)*6))

    try:
	if now[8]: tz = time.tzname[1]
	else: tz = time.tzname[0]
    except AttributeError:
    	tz = ''

    if now[3] > 12: clock12 = now[3] - 12
    elif now[3] > 0: clock12 = now[3]
    else: clock12 = 12

    expectations = (
	('%A', calendar.day_name[now[6]], 'full weekday name'),
	('%a', calendar.day_abbr[now[6]], 'abbreviated weekday name'),
	('%B', calendar.month_name[now[1]], 'full month name'),
	('%b', calendar.month_abbr[now[1]], 'abbreviated month name'),
	('%h', calendar.month_abbr[now[1]], 'abbreviated month name'),
	('%c', fixasctime(time.asctime(now)), 'near-asctime() format'),
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
	('%U', '%02d' % ((now[7] + jan1[6])/7),
	 'week number of the year (Sun 1st)'),
	('%W', '%02d' % ((now[7] + (jan1[6] - 1)%7)/7),
	 'week number of the year (Mon 1st)'),
	('%w', '%d' % ((1+now[6]) % 7), 'weekday as a number (Sun 1st)'),
	('%x', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)),
	 '%m/%d/%y %H:%M:%S'),
	('%Y', '%d' % now[0], 'year with century'),
	('%y', '%02d' % (now[0]%100), 'year without century'),
	('%%', '%', 'single percent sign'),
	)

    nonstandard_expectations = (
	('%C', '%02d' % (now[0]/100), 'century'),
	    # This is for IRIX; on Solaris, %C yields date(1) format.
	    # Tough.
	('%k', '%2d' % now[3], 'hour, blank padded ( 0-23)'),
	('%s', nowsecs, 'seconds since the Epoch in UCT'),
	('%3y', '%03d' % (now[0]%100),
	 'year without century rendered using fieldwidth'),
	('%n', '\n', 'newline character'),
	('%t', '\t', 'tab character'),
	('%Z', tz, 'time zone name'),
	)

    if verbose:
	print "Strftime test, platform: %s, Python version: %s" % \
	      (sys.platform, string.split(sys.version)[0])

    for e in expectations:
	try:
	    result = time.strftime(e[0], now)
	except ValueError, error:
	    print "Standard '%s' format gave error:" % e[0], error
	    continue
	if result == e[1]: continue
	if result[0] == '%':
	    print "Does not support standard '%s' format (%s)" % (e[0], e[2])
	else:
	    print "Conflict for %s (%s):" % (e[0], e[2])
	    print "  Expected %s, but got %s" % (e[1], result)

    for e in nonstandard_expectations:
	try:
	    result = time.strftime(e[0], now)
	except ValueError, result:
	    if verbose:
		print "Error for nonstandard '%s' format (%s): %s" % \
		      (e[0], e[2], str(result))
	    continue
	if result == e[1]:
	    if verbose:
		print "Supports nonstandard '%s' format (%s)" % (e[0], e[2])
	elif result[0] == '%':
	    if verbose:
		print "Does not appear to support '%s' format (%s)" % (e[0],
								       e[2])
	else:
	    if verbose:
		print "Conflict for nonstandard '%s' format (%s):" % (e[0],
								      e[2])
		print "  Expected %s, but got %s" % (e[1], result)

def fixasctime(s):
    if s[8] == ' ':
	s = s[:8] + '0' + s[9:]
    return s

main()
