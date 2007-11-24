from test.test_support import verbose, TestFailed, verify
import types

class F:
    def a(self):
        pass

def b():
    'my docstring'
    pass

# __module__ is a special attribute
verify(b.__module__ == __name__)
verify(verify.__module__ == "test.test_support")

# setting attributes on functions
try:
    b.publish
except AttributeError: pass
else: raise TestFailed, 'expected AttributeError'

if b.__dict__ <> {}:
    raise TestFailed, 'expected unassigned func.__dict__ to be {}'

b.publish = 1
if b.publish <> 1:
    raise TestFailed, 'function attribute not set to expected value'

docstring = 'its docstring'
b.__doc__ = docstring
if b.__doc__ <> docstring:
    raise TestFailed, 'problem with setting __doc__ attribute'

if 'publish' not in dir(b):
    raise TestFailed, 'attribute not in dir()'

try:
    del b.__dict__
except TypeError: pass
else: raise TestFailed, 'del func.__dict__ expected TypeError'

b.publish = 1
try:
    b.__dict__ = None
except TypeError: pass
else: raise TestFailed, 'func.__dict__ = None expected TypeError'

d = {'hello': 'world'}
b.__dict__ = d
if b.func_dict is not d:
    raise TestFailed, 'func.__dict__ assignment to dictionary failed'
if b.hello <> 'world':
    raise TestFailed, 'attribute after func.__dict__ assignment failed'

f1 = F()
f2 = F()

try:
    F.a.publish
except AttributeError: pass
else: raise TestFailed, 'expected AttributeError'

try:
    f1.a.publish
except AttributeError: pass
else: raise TestFailed, 'expected AttributeError'

# In Python 2.1 beta 1, we disallowed setting attributes on unbound methods
# (it was already disallowed on bound methods).  See the PEP for details.
try:
    F.a.publish = 1
except (AttributeError, TypeError): pass
else: raise TestFailed, 'expected AttributeError or TypeError'

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
except (AttributeError, TypeError): pass
else: raise TestFailed, 'expected AttributeError or TypeError'

# See the comment above about the change in semantics for Python 2.1b1
try:
    F.a.myclass = F
except (AttributeError, TypeError): pass
else: raise TestFailed, 'expected AttributeError or TypeError'

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
except (AttributeError, TypeError): pass
else: raise TestFailed, 'expected TypeError or AttributeError'

F.a.im_func.__dict__ = {'one': 11, 'two': 22, 'three': 33}

if f1.a.two <> 22:
    raise TestFailed, 'setting __dict__'

from UserDict import UserDict
d = UserDict({'four': 44, 'five': 55})

try:
    F.a.__dict__ = d
except (AttributeError, TypeError): pass
else: raise TestFailed

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
except (AttributeError, TypeError): pass
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
except (AttributeError, TypeError): pass
else: raise TestFailed

try:
    eff.id.foo
except AttributeError: pass
else: raise TestFailed

# Regression test for a crash in pre-2.1a1
def another():
    pass

try:
    del another.__dict__
except TypeError: pass
else: raise TestFailed

try:
    del another.func_dict
except TypeError: pass
else: raise TestFailed

try:
    another.func_dict = None
except TypeError: pass
else: raise TestFailed

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

if foo==bar:
    raise TestFailed

d={}
d[foo] = 1

foo.func_code = temp.func_code

d[foo]

# Test all predefined function attributes systematically

def cantset(obj, name, value, exception=(AttributeError, TypeError)):
    verify(hasattr(obj, name)) # Otherwise it's probably a typo
    try:
        setattr(obj, name, value)
    except exception:
        pass
    else:
        raise TestFailed, "shouldn't be able to set %s to %r" % (name, value)
    try:
        delattr(obj, name)
    except (AttributeError, TypeError):
        pass
    else:
        raise TestFailed, "shouldn't be able to del %s" % name

def test_func_closure():
    a = 12
    def f(): print a
    c = f.func_closure
    verify(isinstance(c, tuple))
    verify(len(c) == 1)
    verify(c[0].__class__.__name__ == "cell") # don't have a type object handy
    cantset(f, "func_closure", c)

def test_empty_cell():
    def f(): print a
    try:
        f.func_closure[0].cell_contents
    except ValueError:
        pass
    else:
        raise TestFailed, "shouldn't be able to read an empty cell"

    a = 12

def test_func_doc():
    def f(): pass
    verify(f.__doc__ is None)
    verify(f.func_doc is None)
    f.__doc__ = "hello"
    verify(f.__doc__ == "hello")
    verify(f.func_doc == "hello")
    del f.__doc__
    verify(f.__doc__ is None)
    verify(f.func_doc is None)
    f.func_doc = "world"
    verify(f.__doc__ == "world")
    verify(f.func_doc == "world")
    del f.func_doc
    verify(f.func_doc is None)
    verify(f.__doc__ is None)

def test_func_globals():
    def f(): pass
    verify(f.func_globals is globals())
    cantset(f, "func_globals", globals())

def test_func_name():
    def f(): pass
    verify(f.__name__ == "f")
    verify(f.func_name == "f")
    f.__name__ = "g"
    verify(f.__name__ == "g")
    verify(f.func_name == "g")
    f.func_name = "h"
    verify(f.__name__ == "h")
    verify(f.func_name == "h")
    cantset(f, "func_globals", 1)
    cantset(f, "__name__", 1)
    # test that you can access func.__name__ in restricted mode
    s = """def f(): pass\nf.__name__"""
    exec s in {'__builtins__':{}}


def test_func_code():
    a = b = 24
    def f(): pass
    def g(): print 12
    def f1(): print a
    def g1(): print b
    def f2(): print a, b
    verify(type(f.func_code) is types.CodeType)
    f.func_code = g.func_code
    cantset(f, "func_code", None)
    # can't change the number of free vars
    cantset(f,  "func_code", f1.func_code, exception=ValueError)
    cantset(f1, "func_code",  f.func_code, exception=ValueError)
    cantset(f1, "func_code", f2.func_code, exception=ValueError)
    f1.func_code = g1.func_code

def test_func_defaults():
    def f(a, b): return (a, b)
    verify(f.func_defaults is None)
    f.func_defaults = (1, 2)
    verify(f.func_defaults == (1, 2))
    verify(f(10) == (10, 2))
    def g(a=1, b=2): return (a, b)
    verify(g.func_defaults == (1, 2))
    del g.func_defaults
    verify(g.func_defaults is None)
    try:
        g()
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't be allowed to call g() w/o defaults"

def test_func_dict():
    def f(): pass
    a = f.__dict__
    b = f.func_dict
    verify(a == {})
    verify(a is b)
    f.hello = 'world'
    verify(a == {'hello': 'world'})
    verify(f.func_dict is a is f.__dict__)
    f.func_dict = {}
    verify(not hasattr(f, "hello"))
    f.__dict__ = {'world': 'hello'}
    verify(f.world == "hello")
    verify(f.__dict__ is f.func_dict == {'world': 'hello'})
    cantset(f, "func_dict", None)
    cantset(f, "__dict__", None)

def test_im_class():
    class C:
        def foo(self): pass
    verify(C.foo.im_class is C)
    verify(C().foo.im_class is C)
    cantset(C.foo, "im_class", C)
    cantset(C().foo, "im_class", C)

def test_im_func():
    def foo(self): pass
    class C:
        pass
    C.foo = foo
    verify(C.foo.im_func is foo)
    verify(C().foo.im_func is foo)
    cantset(C.foo, "im_func", foo)
    cantset(C().foo, "im_func", foo)

def test_im_self():
    class C:
        def foo(self): pass
    verify(C.foo.im_self is None)
    c = C()
    verify(c.foo.im_self is c)
    cantset(C.foo, "im_self", None)
    cantset(c.foo, "im_self", c)

def test_im_dict():
    class C:
        def foo(self): pass
        foo.bar = 42
    verify(C.foo.__dict__ == {'bar': 42})
    verify(C().foo.__dict__ == {'bar': 42})
    cantset(C.foo, "__dict__", C.foo.__dict__)
    cantset(C().foo, "__dict__", C.foo.__dict__)

def test_im_doc():
    class C:
        def foo(self): "hello"
    verify(C.foo.__doc__ == "hello")
    verify(C().foo.__doc__ == "hello")
    cantset(C.foo, "__doc__", "hello")
    cantset(C().foo, "__doc__", "hello")

def test_im_name():
    class C:
        def foo(self): pass
    verify(C.foo.__name__ == "foo")
    verify(C().foo.__name__ == "foo")
    cantset(C.foo, "__name__", "foo")
    cantset(C().foo, "__name__", "foo")

def testmore():
    test_func_closure()
    test_empty_cell()
    test_func_doc()
    test_func_globals()
    test_func_name()
    test_func_code()
    test_func_defaults()
    test_func_dict()
    # Tests for instance method attributes
    test_im_class()
    test_im_func()
    test_im_self()
    test_im_dict()
    test_im_doc()
    test_im_name()

testmore()
