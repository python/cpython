from test_support import verbose, TestFailed

class F:
    def a(self):
        pass

def b():
    'my docstring'
    pass

# setting attributes on functions
try:
    b.publish
except AttributeError:
    pass
else:
    raise TestFailed, 'expected AttributeError'

if b.__dict__ <> None:
    raise TestFailed, 'expected unassigned func.__dict__ to be None'

b.publish = 1
if b.publish <> 1:
    raise TestFailed, 'function attribute not set to expected value'

docstring = 'its docstring'
b.__doc__ = docstring
if b.__doc__ <> docstring:
    raise TestFailed, 'problem with setting __doc__ attribute'

if 'publish' not in dir(b):
    raise TestFailed, 'attribute not in dir()'

del b.__dict__
if b.__dict__ <> None:
    raise TestFailed, 'del func.__dict__ did not result in __dict__ == None'

b.publish = 1
b.__dict__ = None
if b.__dict__ <> None:
    raise TestFailed, 'func.__dict__ = None did not result in __dict__ == None'


f1 = F()
f2 = F()

try:
    F.a.publish
except AttributeError:
    pass
else:
    raise TestFailed, 'expected AttributeError'

try:
    f1.a.publish
except AttributeError:
    pass
else:
    raise TestFailed, 'expected AttributeError'

# In Python 2.1 beta 1, we disallowed setting attributes on unbound methods
# (it was already disallowed on bound methods).  See the PEP for details.
try:
    F.a.publish = 1
except TypeError:
    pass
else:
    raise TestFailed, 'expected TypeError'

# But setting it explicitly on the underlying function object is okay.
F.a.im_func.publish = 1

if F.a.publish <> 1:
    raise TestFailed, 'unbound method attribute not set to expected value'

if f1.a.publish <> 1:
    raise TestFailed, 'bound method attribute access did not work'

if f2.a.publish <> 1:
    raise TestFailed, 'bound method attribute access did not work'

if 'publish' not in dir(F.a):
    raise TestFailed, 'attribute not in dir()'

try:
    f1.a.publish = 0
except TypeError:
    pass
else:
    raise TestFailed, 'expected TypeError'

# See the comment above about the change in semantics for Python 2.1b1
try:
    F.a.myclass = F
except TypeError:
    pass
else:
    raise TestFailed, 'expected TypeError'

F.a.im_func.myclass = F

f1.a.myclass
f2.a.myclass
f1.a.myclass
F.a.myclass

if f1.a.myclass is not f2.a.myclass or \
   f1.a.myclass is not F.a.myclass:
    raise TestFailed, 'attributes were not the same'

# try setting __dict__
try:
    F.a.__dict__ = (1, 2, 3)
except TypeError:
    pass
else:
    raise TestFailed, 'expected TypeError'

F.a.im_func.__dict__ = {'one': 11, 'two': 22, 'three': 33}

if f1.a.two <> 22:
    raise TestFailed, 'setting __dict__'

from UserDict import UserDict
d = UserDict({'four': 44, 'five': 55})

try:
    F.a.__dict__ = d
except TypeError:
    pass
else:
    raise TestFailed

if f2.a.one <> f1.a.one <> F.a.one <> 11:
    raise TestFailed

# im_func may not be a Python method!
import new
F.id = new.instancemethod(id, None, F)

eff = F()
if eff.id() <> id(eff):
    raise TestFailed

try:
    F.id.foo
except AttributeError: pass
else: raise TestFailed

try:
    F.id.foo = 12
except TypeError: pass
else: raise TestFailed

try:
    F.id.foo
except AttributeError: pass
else: raise TestFailed

try:
    eff.id.foo
except AttributeError: pass
else: raise TestFailed

try:
    eff.id.foo = 12
except TypeError: pass
else: raise TestFailed

try:
    eff.id.foo
except AttributeError: pass
else: raise TestFailed

# Regression test for a crash in pre-2.1a1
def another():
    pass
del another.__dict__
del another.func_dict
another.func_dict = None

try:
    del another.bar
except AttributeError: pass
else: raise TestFailed

# This isn't specifically related to function attributes, but it does test a
# core dump regression in funcobject.c
del another.func_defaults

def foo():
    pass

def bar():
    pass

def temp():
    print 1

if foo==bar: raise TestFailed

d={}
d[foo] = 1

foo.func_code = temp.func_code

d[foo]
