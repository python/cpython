#! /usr/bin/env python

# Sanity checker for time.strftime

import time, calendar, sys, os, re
from test_support import verbose

def main():
    global verbose
    # For C Python, these tests expect C locale, so we try to set that
    # explicitly.  For Jython, Finn says we need to be in the US locale; my
    # understanding is that this is the closest Java gets to C's "C" locale.
    # Jython ought to supply an _locale module which Does The Right Thing, but
    # this is the best we can do given today's state of affairs.
    try:
        import java
        java.util.Locale.setDefault(java.util.Locale.US)
    except ImportError:
        # Can't do this first because it will succeed, even in Jython
        import locale
        locale.setlocale(locale.LC_TIME, 'C')
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

    if now[3] < 12: ampm='(AM|am)'
    else: ampm='(PM|pm)'

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
        ('%a', calendar.day_abbr[now[6]], 'abbreviated weekday name'),
        ('%A', calendar.day_name[now[6]], 'full weekday name'),
        ('%b', calendar.month_abbr[now[1]], 'abbreviated month name'),
        ('%B', calendar.month_name[now[1]], 'full month name'),
        # %c see below
        ('%d', '%02d' % now[2], 'day of month as number (00-31)'),
        ('%H', '%02d' % now[3], 'hour (00-23)'),
        ('%I', '%02d' % clock12, 'hour (01-12)'),
        ('%j', '%03d' % now[7], 'julian day (001-366)'),
        ('%m', '%02d' % now[1], 'month as number (01-12)'),
        ('%M', '%02d' % now[4], 'minute, (00-59)'),
        ('%p', ampm, 'AM or PM as appropriate'),
        ('%S', '%02d' % now[5], 'seconds of current time (00-60)'),
        ('%U', '%02d' % ((now[7] + jan1[6])//7),
         'week number of the year (Sun 1st)'),
        ('%w', '0?%d' % ((1+now[6]) % 7), 'weekday as a number (Sun 1st)'),
        ('%W', '%02d' % ((now[7] + (jan1[6] - 1)%7)//7),
         'week number of the year (Mon 1st)'),
        # %x see below
        ('%X', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
        ('%y', '%02d' % (now[0]%100), 'year without century'),
        ('%Y', '%d' % now[0], 'year with century'),
        # %Z see below
        ('%%', '%', 'single percent sign'),
        )

    nonstandard_expectations = (
        # These are standard but don't have predictable output
        ('%c', fixasctime(time.asctime(now)), 'near-asctime() format'),
        ('%x', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)),
         '%m/%d/%y %H:%M:%S'),
        ('%Z', '%s' % tz, 'time zone name'),

        # These are some platform specific extensions
        ('%D', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)), 'mm/dd/yy'),
        ('%e', '%2d' % now[2], 'day of month as number, blank padded ( 0-31)'),
        ('%h', calendar.month_abbr[now[1]], 'abbreviated month name'),
        ('%k', '%2d' % now[3], 'hour, blank padded ( 0-23)'),
        ('%n', '\n', 'newline character'),
        ('%r', '%02d:%02d:%02d %s' % (clock12, now[4], now[5], ampm),
         '%I:%M:%S %p'),
        ('%R', '%02d:%02d' % (now[3], now[4]), '%H:%M'),
        ('%s', nowsecs, 'seconds since the Epoch in UCT'),
        ('%t', '\t', 'tab character'),
        ('%T', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
        ('%3y', '%03d' % (now[0]%100),
         'year without century rendered using fieldwidth'),
        )

    if verbose:
        print "Strftime test, platform: %s, Python version: %s" % \
              (sys.platform, sys.version.split()[0])

    for e in expectations:
        try:
            result = time.strftime(e[0], now)
        except ValueError, error:
            print "Standard '%s' format gave error:" % e[0], error
            continue
        if re.match(e[1], result): continue
        if not result or result[0] == '%':
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
        if re.match(e[1], result):
            if verbose:
                print "Supports nonstandard '%s' format (%s)" % (e[0], e[2])
        elif not result or result[0] == '%':
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
