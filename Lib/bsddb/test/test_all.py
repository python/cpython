"""Run all test cases.
"""

import sys
import os
import unittest
try:
    # For Pythons w/distutils pybsddb
    from bsddb3 import db
except ImportError:
    # For Python 2.3
    from bsddb import db

try:
    from bsddb3 import test_support
except ImportError:
    from test import test_support

verbose = 0
if 'verbose' in sys.argv:
    verbose = 1
    sys.argv.remove('verbose')

if 'silent' in sys.argv:  # take care of old flag, just in case
    verbose = 0
    sys.argv.remove('silent')


def print_versions():
    print
    print '-=' * 38
    print db.DB_VERSION_STRING
    print 'bsddb.db.version():   %s' % (db.version(), )
    print 'bsddb.db.__version__: %s' % db.__version__
    print 'bsddb.db.cvsid:       %s' % db.cvsid
    print 'python version:       %s' % sys.version
    print 'My pid:               %s' % os.getpid()
    print '-=' * 38


def get_new_path(name) :
    get_new_path.mutex.acquire()
    try :
        import os
        path=os.path.join(get_new_path.prefix,
                name+"_"+str(os.getpid())+"_"+str(get_new_path.num))
        get_new_path.num+=1
    finally :
        get_new_path.mutex.release()
    return path

def get_new_environment_path() :
    path=get_new_path("environment")
    import os
    try:
        os.makedirs(path,mode=0700)
    except os.error:
        test_support.rmtree(path)
        os.makedirs(path)
    return path

def get_new_database_path() :
    path=get_new_path("database")
    import os
    if os.path.exists(path) :
        os.remove(path)
    return path


get_new_path.prefix="/tmp/z-Berkeley_DB"
get_new_path.num=0

try :
    import threading
    get_new_path.mutex=threading.Lock()
    del threading
except ImportError:
    class Lock(object) :
        def acquire(self) :
            pass
        def release(self) :
            pass
    get_new_path.mutex=Lock()
    del Lock



class PrintInfoFakeTest(unittest.TestCase):
    def testPrintVersions(self):
        print_versions()


# This little hack is for when this module is run as main and all the
# other modules import it so they will still be able to get the right
# verbose setting.  It's confusing but it works.
import test_all
test_all.verbose = verbose


def suite():
    try:
        # this is special, it used to segfault the interpreter
        import test_1413192
    except:
        pass

    test_modules = [
        'test_associate',
        'test_basics',
        'test_compat',
        'test_compare',
        'test_dbobj',
        'test_dbshelve',
        'test_dbtables',
        'test_early_close',
        'test_distributed_transactions',
        'test_replication',
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
        module = __import__(name)
        alltests.addTest(module.test_suite())
    return alltests


def test_suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(PrintInfoFakeTest))
    return suite


if __name__ == '__main__':
    print_versions()
    unittest.main(defaultTest='suite')
