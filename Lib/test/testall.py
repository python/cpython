# testall.py -- a regression test for the Python interpreter.
# To run the tests, execute "import testall" in a clean interpreter.
# It is a good idea to do this whenever you build a new interpreter.
# Remember to add new tests when new features are added!

import sys
from test_support import *

print 'test_grammar'
forget('test_grammar')
import test_grammar

for t in ['test_opcodes', 'test_operations', 'test_builtin',
	  'test_exceptions', 'test_types', 'test_math', 'test_time',
	  'test_array', 'test_strop', 'test_md5', 'test_cmath',
	  'test_crypt', 'test_dbm', 'test_new',
	  ]:
    print t
    unload(t)
    try:
	__import__(t, globals(), locals())
    except ImportError, msg:
	sys.stderr.write('%s.  Uninstalled optional module?\n' % msg)

print 'Passed all tests.'
