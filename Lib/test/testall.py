# This file now contains only the list of separate regression tests.
# All of the testing harness is now contained in autotest.py.

tests = ['test_grammar',
	 'test_opcodes',
	 'test_operations',
	 'test_builtin',
	 'test_exceptions',
	 'test_types',
	 'test_math',
	 'test_time',
	 'test_array',
	 'test_strop',
	 'test_md5',
	 'test_cmath',
	 'test_crypt',
	 'test_dbm',
	 'test_new',
	 'test_nis',
	 ]

if __name__ == '__main__':
    # low-overhead testing, for cases where autotest.py harness
    # doesn't even work!
    import sys
    from test_support import *

    for t in tests:
        print t
	unload(t)
	try:
	    __import__(t, globals(), locals())
	except ImportError, msg:
## 	    sys.stderr.write('%s.  Uninstalled optional module?\n' % msg)
	    pass

    print 'Passed all tests.'
