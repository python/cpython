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

b.publish = 1
if b.publish <> 1:
    raise TestFailed, 'function attribute not set to expected value'

docstring = 'its docstring'
b.__doc__ = docstring
if b.__doc__ <> docstring:
    raise TestFailed, 'problem with setting __doc__ attribute'

if 'publish' not in dir(b):
    raise TestFailed, 'attribute not in dir()'

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


F.a.publish = 1
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

F.a.myclass = F
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

F.a.__dict__ = {'one': 11, 'two': 22, 'three': 33}
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
