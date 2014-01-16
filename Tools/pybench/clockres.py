#!/usr/bin/env python

""" clockres - calculates the resolution in seconds of a given timer.

    Copyright (c) 2006, Marc-Andre Lemburg (mal@egenix.com). See the
    documentation for further information on copyrights, or contact
    the author. All Rights Reserved.

"""
import time

TEST_TIME = 1.0

def clockres(timer):
    d = {}
    wallclock = time.time
    start = wallclock()
    stop = wallclock() + TEST_TIME
    spin_loops = range(1000)
    while 1:
        now = wallclock()
        if now >= stop:
            break
        for i in spin_loops:
            d[timer()] = 1
    values = d.keys()
    values.sort()
    min_diff = TEST_TIME
    for i in range(len(values) - 1):
        diff = values[i+1] - values[i]
        if diff < min_diff:
            min_diff = diff
    return min_diff

if __name__ == '__main__':
    print 'Clock resolution of various timer implementations:'
    print 'time.clock:           %10.3fus' % (clockres(time.clock) * 1e6)
    print 'time.time:            %10.3fus' % (clockres(time.time) * 1e6)
    try:
        import systimes
        print 'systimes.processtime: %10.3fus' % (clockres(systimes.processtime) * 1e6)
    except ImportError:
        pass
