#! /usr/bin/env python

# Calculate your unbirthday count (see Alice in Wonderland).
# This is defined as the number of days from your birth until today
# that weren't your birthday.  (The day you were born is not counted).
# Leap years make it interesting.

import sys
import time
import calendar

def main():
    # Note that the range checks below also check for bad types,
    # e.g. 3.14 or ().  However syntactically invalid replies
    # will raise an exception.
    if sys.argv[1:]:
        year = int(sys.argv[1])
    else:
        year = int(raw_input('In which year were you born? '))
    if 0<=year<100:
        print "I'll assume that by", year,
        year = year + 1900
        print 'you mean', year, 'and not the early Christian era'
    elif not (1850<=year<=2002):
        print "It's hard to believe you were born in", year
        return
    #
    if sys.argv[2:]:
        month = int(sys.argv[2])
    else:
        month = int(raw_input('And in which month? (1-12) '))
    if not (1<=month<=12):
        print 'There is no month numbered', month
        return
    #
    if sys.argv[3:]:
        day = int(sys.argv[3])
    else:
        day = int(raw_input('And on what day of that month? (1-31) '))
    if month == 2 and calendar.isleap(year):
        maxday = 29
    else:
        maxday = calendar.mdays[month]
    if not (1<=day<=maxday):
        print 'There are no', day, 'days in that month!'
        return
    #
    bdaytuple = (year, month, day)
    bdaydate = mkdate(bdaytuple)
    print 'You were born on', format(bdaytuple)
    #
    todaytuple = time.localtime()[:3]
    todaydate = mkdate(todaytuple)
    print 'Today is', format(todaytuple)
    #
    if bdaytuple > todaytuple:
        print 'You are a time traveler.  Go back to the future!'
        return
    #
    if bdaytuple == todaytuple:
        print 'You were born today.  Have a nice life!'
        return
    #
    days = todaydate - bdaydate
    print 'You have lived', days, 'days'
    #
    age = 0
    for y in range(year, todaytuple[0] + 1):
        if bdaytuple < (y, month, day) <= todaytuple:
            age = age + 1
    #
    print 'You are', age, 'years old'
    #
    if todaytuple[1:] == bdaytuple[1:]:
        print 'Congratulations!  Today is your', nth(age), 'birthday'
        print 'Yesterday was your',
    else:
        print 'Today is your',
    print nth(days - age), 'unbirthday'

def format((year, month, day)):
    return '%d %s %d' % (day, calendar.month_name[month], year)

def nth(n):
    if n == 1: return '1st'
    if n == 2: return '2nd'
    if n == 3: return '3rd'
    return '%dth' % n

def mkdate((year, month, day)):
    # Januari 1st, in 0 A.D. is arbitrarily defined to be day 1,
    # even though that day never actually existed and the calendar
    # was different then...
    days = year*365                 # years, roughly
    days = days + (year+3)/4        # plus leap years, roughly
    days = days - (year+99)/100     # minus non-leap years every century
    days = days + (year+399)/400    # plus leap years every 4 centirues
    for i in range(1, month):
        if i == 2 and calendar.isleap(year):
            days = days + 29
        else:
            days = days + calendar.mdays[i]
    days = days + day
    return days

if __name__ == "__main__":
    main()
