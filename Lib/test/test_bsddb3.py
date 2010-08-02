# Test driver for bsddb package.
"""
Run all test cases.
"""
import os
import sys
import tempfile
import time
import unittest
from test.test_support import requires, run_unittest, import_module

# Skip test if _bsddb module was not built.
import_module('_bsddb')
# Silence Py3k warning
import_module('bsddb', deprecated=True)

# When running as a script instead of within the regrtest framework, skip the
# requires test, since it's obvious we want to run them.
if __name__ != '__main__':
    requires('bsddb')

verbose = False
if 'verbose' in sys.argv:
    verbose = True
    sys.argv.remove('verbose')

if 'silent' in sys.argv:  # take care of old flag, just in case
    verbose = False
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


# For invocation through regrtest
def test_main():
    from bsddb import db
    from bsddb.test import test_all
    test_all.set_test_path_prefix(os.path.join(tempfile.gettempdir(),
                                 'z-test_bsddb3-%s' %
                                 os.getpid()))
    # Please leave this print in, having this show up in the buildbots
    # makes diagnosing problems a lot easier.
    print >>sys.stderr, db.DB_VERSION_STRING
    print >>sys.stderr, 'Test path prefix: ', test_all.get_test_path_prefix()
    try:
        run_unittest(test_all.suite(module_prefix='bsddb.test.',
                                    timing_check=TimingCheck))
    finally:
        # The only reason to remove db_home is in case if there is an old
        # one lying around.  This might be by a different user, so just
        # ignore errors.  We should always make a unique name now.
        try:
            test_all.remove_test_path_directory()
        except:
            pass


if __name__ == '__main__':
    test_main()
