# Test driver for bsddb package.
"""
Run all test cases.
"""

import sys
import unittest
from test.test_support import requires, verbose, run_suite
requires('bsddb')

verbose = 0
if 'verbose' in sys.argv:
    verbose = 1
    sys.argv.remove('verbose')

if 'silent' in sys.argv:  # take care of old flag, just in case
    verbose = 0
    sys.argv.remove('silent')


def suite():
    test_modules = [ 'test_compat',
                     'test_basics',
                     'test_misc',
                     'test_dbobj',
                     'test_recno',
                     'test_queue',
                     'test_get_none',
                     'test_dbshelve',
                     'test_dbtables',
                     'test_thread',
                     'test_lock',
                     'test_associate',
                   ]

    alltests = unittest.TestSuite()
    for name in test_modules:
        module = __import__("bsddb.test."+name, globals(), locals(), name)
        print module,name
        alltests.addTest(module.suite())
    return alltests

# For invocation through regrtest
def test_main():
    tests = suite()
    run_suite(tests)

# For invocation as a script
if __name__ == '__main__':
    from bsddb import db
    print '-=' * 38
    print db.DB_VERSION_STRING
    print 'bsddb3.db.version():   %s' % (db.version(), )
    print 'bsddb3.db.__version__: %s' % db.__version__
    print 'bsddb3.db.cvsid:       %s' % db.cvsid
    print 'python version:        %s' % sys.version
    print '-=' * 38

    unittest.main( defaultTest='suite' )

