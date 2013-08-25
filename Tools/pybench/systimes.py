#!/usr/bin/env python

""" systimes() user and system timer implementations for use by
    pybench.

    This module implements various different strategies for measuring
    performance timings. It tries to choose the best available method
    based on the platform and available tools.

    On Windows, it is recommended to have the Mark Hammond win32
    package installed. Alternatively, the Thomas Heller ctypes
    packages can also be used.

    On Unix systems, the standard resource module provides the highest
    resolution timings. Unfortunately, it is not available on all Unix
    platforms.

    If no supported timing methods based on process time can be found,
    the module reverts to the highest resolution wall-clock timer
    instead. The system time part will then always be 0.0.

    The module exports one public API:

    def systimes():

        Return the current timer values for measuring user and system
        time as tuple of seconds (user_time, system_time).

    Copyright (c) 2006, Marc-Andre Lemburg (mal@egenix.com). See the
    documentation for further information on copyrights, or contact
    the author. All Rights Reserved.

"""
import time, sys

#
# Note: Please keep this module compatible to Python 1.5.2.
#
# TODOs:
#
# * Add ctypes wrapper for new clock_gettime() real-time POSIX APIs;
#   these will then provide nano-second resolution where available.
#
# * Add a function that returns the resolution of systimes()
#   values, ie. systimesres().
#

### Choose an implementation

SYSTIMES_IMPLEMENTATION = None
USE_CTYPES_GETPROCESSTIMES = 'ctypes GetProcessTimes() wrapper'
USE_WIN32PROCESS_GETPROCESSTIMES = 'win32process.GetProcessTimes()'
USE_RESOURCE_GETRUSAGE = 'resource.getrusage()'
USE_PROCESS_TIME_CLOCK = 'time.clock() (process time)'
USE_WALL_TIME_CLOCK = 'time.clock() (wall-clock)'
USE_WALL_TIME_TIME = 'time.time() (wall-clock)'

if sys.platform[:3] == 'win':
    # Windows platform
    try:
        import win32process
    except ImportError:
        try:
            import ctypes
        except ImportError:
            # Use the wall-clock implementation time.clock(), since this
            # is the highest resolution clock available on Windows
            SYSTIMES_IMPLEMENTATION = USE_WALL_TIME_CLOCK
        else:
            SYSTIMES_IMPLEMENTATION = USE_CTYPES_GETPROCESSTIMES
    else:
        SYSTIMES_IMPLEMENTATION = USE_WIN32PROCESS_GETPROCESSTIMES
else:
    # All other platforms
    try:
        import resource
    except ImportError:
        pass
    else:
        SYSTIMES_IMPLEMENTATION = USE_RESOURCE_GETRUSAGE

# Fall-back solution
if SYSTIMES_IMPLEMENTATION is None:
    # Check whether we can use time.clock() as approximation
    # for systimes()
    start = time.clock()
    time.sleep(0.1)
    stop = time.clock()
    if stop - start < 0.001:
        # Looks like time.clock() is usable (and measures process
        # time)
        SYSTIMES_IMPLEMENTATION = USE_PROCESS_TIME_CLOCK
    else:
        # Use wall-clock implementation time.time() since this provides
        # the highest resolution clock on most systems
        SYSTIMES_IMPLEMENTATION = USE_WALL_TIME_TIME

### Implementations

def getrusage_systimes():
    return resource.getrusage(resource.RUSAGE_SELF)[:2]

def process_time_clock_systimes():
    return (time.clock(), 0.0)

def wall_clock_clock_systimes():
    return (time.clock(), 0.0)

def wall_clock_time_systimes():
    return (time.time(), 0.0)

# Number of clock ticks per second for the values returned
# by GetProcessTimes() on Windows.
#
# Note: Ticks returned by GetProcessTimes() are 100ns intervals on
# Windows XP. However, the process times are only updated with every
# clock tick and the frequency of these is somewhat lower: depending
# on the OS version between 10ms and 15ms. Even worse, the process
# time seems to be allocated to process currently running when the
# clock interrupt arrives, ie. it is possible that the current time
# slice gets accounted to a different process.

WIN32_PROCESS_TIMES_TICKS_PER_SECOND = 1e7

def win32process_getprocesstimes_systimes():
    d = win32process.GetProcessTimes(win32process.GetCurrentProcess())
    return (d['UserTime'] / WIN32_PROCESS_TIMES_TICKS_PER_SECOND,
            d['KernelTime'] / WIN32_PROCESS_TIMES_TICKS_PER_SECOND)

def ctypes_getprocesstimes_systimes():
    creationtime = ctypes.c_ulonglong()
    exittime = ctypes.c_ulonglong()
    kerneltime = ctypes.c_ulonglong()
    usertime = ctypes.c_ulonglong()
    rc = ctypes.windll.kernel32.GetProcessTimes(
        ctypes.windll.kernel32.GetCurrentProcess(),
        ctypes.byref(creationtime),
        ctypes.byref(exittime),
        ctypes.byref(kerneltime),
        ctypes.byref(usertime))
    if not rc:
        raise TypeError('GetProcessTimes() returned an error')
    return (usertime.value / WIN32_PROCESS_TIMES_TICKS_PER_SECOND,
            kerneltime.value / WIN32_PROCESS_TIMES_TICKS_PER_SECOND)

# Select the default for the systimes() function

if SYSTIMES_IMPLEMENTATION is USE_RESOURCE_GETRUSAGE:
    systimes = getrusage_systimes

elif SYSTIMES_IMPLEMENTATION is USE_PROCESS_TIME_CLOCK:
    systimes = process_time_clock_systimes

elif SYSTIMES_IMPLEMENTATION is USE_WALL_TIME_CLOCK:
    systimes = wall_clock_clock_systimes

elif SYSTIMES_IMPLEMENTATION is USE_WALL_TIME_TIME:
    systimes = wall_clock_time_systimes

elif SYSTIMES_IMPLEMENTATION is USE_WIN32PROCESS_GETPROCESSTIMES:
    systimes = win32process_getprocesstimes_systimes

elif SYSTIMES_IMPLEMENTATION is USE_CTYPES_GETPROCESSTIMES:
    systimes = ctypes_getprocesstimes_systimes

else:
    raise TypeError('no suitable systimes() implementation found')

def processtime():

    """ Return the total time spent on the process.

        This is the sum of user and system time as returned by
        systimes().

    """
    user, system = systimes()
    return user + system

### Testing

def some_workload():
    x = 0L
    for i in xrange(10000000L):
        x = x + 1L

def test_workload():
    print 'Testing systimes() under load conditions'
    t0 = systimes()
    some_workload()
    t1 = systimes()
    print 'before:', t0
    print 'after:', t1
    print 'differences:', (t1[0] - t0[0], t1[1] - t0[1])
    print

def test_idle():
    print 'Testing systimes() under idle conditions'
    t0 = systimes()
    time.sleep(1)
    t1 = systimes()
    print 'before:', t0
    print 'after:', t1
    print 'differences:', (t1[0] - t0[0], t1[1] - t0[1])
    print

if __name__ == '__main__':
    print 'Using %s as timer' % SYSTIMES_IMPLEMENTATION
    print
    test_workload()
    test_idle()
