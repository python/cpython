# testall.py -- a regression test for the Python interpreter.
# To run the tests, execute "import testall" in a clean interpreter.
# It is a good idea to do this whenever you build a new interpreter.
# Remember to add new tests when new features are added!

from test_support import *
		  
print 'test_grammar'
forget('test_grammar')
import test_grammar

print 'test_opcodes'
unload('test_opcodes')
import test_opcodes

print 'test_operations'
unload('test_operations')
import test_operations

print 'test_builtin'
unload('test_builtin')
import test_builtin

print 'test_exceptions'
unload('test_exceptions')
import test_exceptions

print 'test_types'
unload('test_types')
import test_types

print 'test_math'
unload('test_math')
import test_math

print 'test_time'
unload('test_time')
import test_time

print 'test_array'
unload('test_array')
import test_array

print 'test_strop'
unload('test_strop')
import test_strop

print 'Passed all tests.'
