#------------------------------------------------------------------------
#
# In my performance tests, using this (as in dbtest.py test4) is
# slightly slower than simply compiling _db.c with MYDB_THREAD
# undefined to prevent multithreading support in the C module.
# Using NoDeadlockDb also prevent deadlocks from mutliple processes
# accessing the same database.
#
# Copyright (C) 2000 Autonomous Zone Industries
#
# License:      This is free software.  You may use this software for any
#               purpose including modification/redistribution, so long as
#               this header remains intact and that you do not claim any
#               rights of ownership or authorship of this software.  This
#               software has been tested, but no warranty is expressed or
#               implied.
#
# Author: Gregory P. Smith <greg@electricrain.com>
#
# Note: I don't know how useful this is in reality since when a
#       DBDeadlockError happens the current transaction is supposed to be
#       aborted.  If it doesn't then when the operation is attempted again
#       the deadlock is still happening...
#       --Robin
#
#------------------------------------------------------------------------


#
# import the time.sleep function in a namespace safe way to allow
# "from bsddb3.db import *"
#
from time import sleep
_sleep = sleep
del sleep

import _db

_deadlock_MinSleepTime = 1.0/64  # always sleep at least N seconds between retrys
_deadlock_MaxSleepTime = 1.0     # never sleep more than N seconds between retrys


def DeadlockWrap(function, *_args, **_kwargs):
    """DeadlockWrap(function, *_args, **_kwargs) - automatically retries
    function in case of a database deadlock.

    This is a DeadlockWrapper method which DB calls can be made using to
    preform infinite retrys with sleeps in between when a DBLockDeadlockError
    exception is raised in a database call:

        d = DB(...)
        d.open(...)
        DeadlockWrap(d.put, "foo", data="bar")  # set key "foo" to "bar"
    """
    sleeptime = _deadlock_MinSleepTime
    while (1) :
        try:
            return apply(function, _args, _kwargs)
        except _db.DBLockDeadlockError:
            print 'DeadlockWrap sleeping ', sleeptime
            _sleep(sleeptime)
            # exponential backoff in the sleep time
            sleeptime = sleeptime * 2
            if sleeptime > _deadlock_MaxSleepTime :
                sleeptime = _deadlock_MaxSleepTime


#------------------------------------------------------------------------

