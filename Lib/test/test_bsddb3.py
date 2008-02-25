# Test driver for bsddb package.
"""
Run all test cases.
"""
import os
import sys
import tempfile
import time
import unittest
from test.test_support import requires, verbose, run_unittest, unlink, rmtree

# When running as a script instead of within the regrtest framework, skip the
# requires test, since it's obvious we want to run them.
if __name__ != '__main__':
    requires('bsddb')

import bsddb.test.test_all
if 'verbose' in sys.argv:
    bsddb.test.test_all.verbose = 1
    sys.argv.remove('verbose')

if 'silent' in sys.argv:  # take care of old flag, just in case
    bsddb.test.test_all.verbose = 0
    sys.argv.remove('silent')


class TimingCheck(unittest.TestCase):

    """This class is not a real test.  Its purpose is to print a message
    periodically when the test runs slowly.  This will prevent the buildbots
    from timing out on slow machines."""

    # How much time in seconds before printing a 'Still working' message.
    # Since this is run at most once between each test module, use a smaller
    # interval than other tests.
    _PRINT_WORKING_MSG_INTERVAL = 4 * 60

    # next_time is used as a global variable that survives each instance.
    # This is necessary since a new instance will be created for each test.
    next_time = time.time() + _PRINT_WORKING_MSG_INTERVAL

    def testCheckElapsedTime(self):
        # Print still working message since these tests can be really slow.
        now = time.time()
        if self.next_time <= now:
            TimingCheck.next_time = now + self._PRINT_WORKING_MSG_INTERVAL
            sys.__stdout__.write('  test_bsddb3 still working, be patient...\n')
            sys.__stdout__.flush()


def suite():
    try:
        # this is special, it used to segfault the interpreter
        import bsddb.test.test_1413192
    finally:
        for f in ['xxx.db','__db.001','__db.002','__db.003','log.0000000001']:
            unlink(f)

    test_modules = [
        'test_associate',
        'test_basics',
        'test_compat',
        'test_compare',
        'test_dbobj',
        'test_dbshelve',
        'test_dbtables',
        'test_env_close',
        'test_get_none',
        'test_join',
        'test_lock',
        'test_misc',
        'test_pickle',
        'test_queue',
        'test_recno',
        'test_thread',
        'test_sequence',
        'test_cursor_pget_bug',
        ]

    alltests = unittest.TestSuite()
    for name in test_modules:
        module = __import__("bsddb.test."+name, globals(), locals(), name)
        #print module,name
        alltests.addTest(module.test_suite())
        alltests.addTest(unittest.makeSuite(TimingCheck))
    return alltests


# For invocation through regrtest
def test_main():
    run_unittest(suite())
    db_home = os.path.join(tempfile.gettempdir(), 'db_home')
    # The only reason to remove db_home is in case if there is an old
    # one lying around.  This might be by a different user, so just
    # ignore errors.  We should always make a unique name now.
    try:
        rmtree(db_home)
    except:
        pass
    rmtree('db_home%d' % os.getpid())

# For invocation as a script
if __name__ == '__main__':
    from bsddb import db
    print('-=' * 38)
    print(db.DB_VERSION_STRING)
    print('bsddb.db.version():   %s' % (db.version(),))
    print('bsddb.db.__version__: %s' % db.__version__)
    print('bsddb.db.cvsid:       %s' % db.cvsid)
    print('python version:        %s' % sys.version)
    print('-=' * 38)

    test_main()
