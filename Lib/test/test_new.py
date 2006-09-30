from test.test_support import verbose, verify, TestFailed
import sys
import new

class Eggs:
    def get_yolks(self):
        return self.yolks

print 'new.module()'
m = new.module('Spam')
if verbose:
    print m
m.Eggs = Eggs
sys.modules['Spam'] = m
import Spam

def get_more_yolks(self):
    return self.yolks + 3

print 'new.classobj()'
C = new.classobj('Spam', (Spam.Eggs,), {'get_more_yolks': get_more_yolks})
if verbose:
    print C
print 'new.instance()'
c = new.instance(C, {'yolks': 3})
if verbose:
    print c
o = new.instance(C)
verify(o.__dict__ == {},
       "new __dict__ should be empty")
del o
o = new.instance(C, None)
verify(o.__dict__ == {},
       "new __dict__ should be empty")
del o

def break_yolks(self):
    self.yolks = self.yolks - 2
print 'new.instancemethod()'
im = new.instancemethod(break_yolks, c, C)
if verbose:
    print im

verify(c.get_yolks() == 3 and c.get_more_yolks() == 6,
       'Broken call of hand-crafted class instance')
im()
verify(c.get_yolks() == 1 and c.get_more_yolks() == 4,
       'Broken call of hand-crafted instance method')

im = new.instancemethod(break_yolks, c)
im()
verify(c.get_yolks() == -1)
try:
    new.instancemethod(break_yolks, None)
except TypeError:
    pass
else:
    raise TestFailed, "dangerous instance method creation allowed"

# Verify that instancemethod() doesn't allow keyword args
try:
    new.instancemethod(break_yolks, c, kw=1)
except TypeError:
    pass
else:
    raise TestFailed, "instancemethod shouldn't accept keyword args"

# It's unclear what the semantics should be for a code object compiled at
# module scope, but bound and run in a function.  In CPython, `c' is global
# (by accident?) while in Jython, `c' is local.  The intent of the test
# clearly is to make `c' global, so let's be explicit about it.
codestr = '''
global c
a = 1
b = 2
c = a + b
'''

ccode = compile(codestr, '<string>', 'exec')
# Jython doesn't have a __builtins__, so use a portable alternative
import __builtin__
g = {'c': 0, '__builtins__': __builtin__}
# this test could be more robust
print 'new.function()'
func = new.function(ccode, g)
if verbose:
    print func
func()
verify(g['c'] == 3,
       'Could not create a proper function object')

# test the various extended flavors of function.new
def f(x):
    def g(y):
        return x + y
    return g
g = f(4)
new.function(f.func_code, {}, "blah")
g2 = new.function(g.func_code, {}, "blah", (2,), g.func_closure)
verify(g2() == 6)
g3 = new.function(g.func_code, {}, "blah", None, g.func_closure)
verify(g3(5) == 9)
def test_closure(func, closure, exc):
    try:
        new.function(func.func_code, {}, "", None, closure)
    except exc:
        pass
    else:
        print "corrupt closure accepted"

test_closure(g, None, TypeError) # invalid closure
test_closure(g, (1,), TypeError) # non-cell in closure
test_closure(g, (1, 1), ValueError) # closure is wrong size
test_closure(f, g.func_closure, ValueError) # no closure needed

print 'new.code()'
# bogus test of new.code()
# Note: Jython will never have new.code()
if hasattr(new, 'code'):
    def f(a): pass

    c = f.func_code
    argcount = c.co_argcount
    nlocals = c.co_nlocals
    stacksize = c.co_stacksize
    flags = c.co_flags
    codestring = c.co_code
    constants = c.co_consts
    names = c.co_names
    varnames = c.co_varnames
    filename = c.co_filename
    name = c.co_name
    firstlineno = c.co_firstlineno
    lnotab = c.co_lnotab
    freevars = c.co_freevars
    cellvars = c.co_cellvars

    d = new.code(argcount, nlocals, stacksize, flags, codestring,
                 constants, names, varnames, filename, name,
                 firstlineno, lnotab, freevars, cellvars)

    # test backwards-compatibility version with no freevars or cellvars
    d = new.code(argcount, nlocals, stacksize, flags, codestring,
                 constants, names, varnames, filename, name,
                 firstlineno, lnotab)

    try: # this used to trigger a SystemError
        d = new.code(-argcount, nlocals, stacksize, flags, codestring,
                     constants, names, varnames, filename, name,
                     firstlineno, lnotab)
    except ValueError:
        pass
    else:
        raise TestFailed, "negative co_argcount didn't trigger an exception"

    try: # this used to trigger a SystemError
        d = new.code(argcount, -nlocals, stacksize, flags, codestring,
                     constants, names, varnames, filename, name,
                     firstlineno, lnotab)
    except ValueError:
        pass
    else:
        raise TestFailed, "negative co_nlocals didn't trigger an exception"

    try: # this used to trigger a Py_FatalError!
        d = new.code(argcount, nlocals, stacksize, flags, codestring,
                     constants, (5,), varnames, filename, name,
                     firstlineno, lnotab)
    except TypeError:
        pass
    else:
        raise TestFailed, "non-string co_name didn't trigger an exception"

    # new.code used to be a way to mutate a tuple...
    class S(str): pass
    t = (S("ab"),)
    d = new.code(argcount, nlocals, stacksize, flags, codestring,
                 constants, t, varnames, filename, name,
                 firstlineno, lnotab)
    verify(type(t[0]) is S, "eek, tuple changed under us!")

    if verbose:
        print d
