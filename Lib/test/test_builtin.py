# Python test set -- part 4, built-in functions

from test.test_support import unload

print '4. Built-in functions'

print 'test_b1'
unload('test_b1')
from test import test_b1

print 'test_b2'
unload('test_b2')
from test import test_b2
