# Test descriptor-related enhancements

from test_support import verify, verbose
from copy import deepcopy

def testunop(a, res, expr="len(a)", meth="__len__"):
    if verbose: print "checking", expr
    dict = {'a': a}
    verify(eval(expr, dict) == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    verify(m(a) == res)
    bm = getattr(a, meth)
    verify(bm() == res)

def testbinop(a, b, res, expr="a+b", meth="__add__"):
    if verbose: print "checking", expr
    dict = {'a': a, 'b': b}
    verify(eval(expr, dict) == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    verify(m(a, b) == res)
    bm = getattr(a, meth)
    verify(bm(b) == res)

def testternop(a, b, c, res, expr="a[b:c]", meth="__getslice__"):
    if verbose: print "checking", expr
    dict = {'a': a, 'b': b, 'c': c}
    verify(eval(expr, dict) == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    verify(m(a, b, c) == res)
    bm = getattr(a, meth)
    verify(bm(b, c) == res)

def testsetop(a, b, res, stmt="a+=b", meth="__iadd__"):
    if verbose: print "checking", stmt
    dict = {'a': deepcopy(a), 'b': b}
    exec stmt in dict
    verify(dict['a'] == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b)
    verify(dict['a'] == res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b)
    verify(dict['a'] == res)

def testset2op(a, b, c, res, stmt="a[b]=c", meth="__setitem__"):
    if verbose: print "checking", stmt
    dict = {'a': deepcopy(a), 'b': b, 'c': c}
    exec stmt in dict
    verify(dict['a'] == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b, c)
    verify(dict['a'] == res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b, c)
    verify(dict['a'] == res)

def testset3op(a, b, c, d, res, stmt="a[b:c]=d", meth="__setslice__"):
    if verbose: print "checking", stmt
    dict = {'a': deepcopy(a), 'b': b, 'c': c, 'd': d}
    exec stmt in dict
    verify(dict['a'] == res)
    t = type(a)
    m = getattr(t, meth)
    verify(m == t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b, c, d)
    verify(dict['a'] == res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b, c, d)
    verify(dict['a'] == res)

def lists():
    if verbose: print "Testing list operations..."
    testbinop([1], [2], [1,2], "a+b", "__add__")
    testbinop([1,2,3], 2, 1, "b in a", "__contains__")
    testbinop([1,2,3], 4, 0, "b in a", "__contains__")
    testbinop([1,2,3], 1, 2, "a[b]", "__getitem__")
    testternop([1,2,3], 0, 2, [1,2], "a[b:c]", "__getslice__")
    testsetop([1], [2], [1,2], "a+=b", "__iadd__")
    testsetop([1,2], 3, [1,2,1,2,1,2], "a*=b", "__imul__")
    testunop([1,2,3], 3, "len(a)", "__len__")
    testbinop([1,2], 3, [1,2,1,2,1,2], "a*b", "__mul__")
    testbinop([1,2], 3, [1,2,1,2,1,2], "b*a", "__rmul__")
    testset2op([1,2], 1, 3, [1,3], "a[b]=c", "__setitem__")
    testset3op([1,2,3,4], 1, 3, [5,6], [1,5,6,4], "a[b:c]=d", "__setslice__")

def dicts():
    if verbose: print "Testing dict operations..."
    testbinop({1:2}, {2:1}, -1, "cmp(a,b)", "__cmp__")
    testbinop({1:2,3:4}, 1, 1, "b in a", "__contains__")
    testbinop({1:2,3:4}, 2, 0, "b in a", "__contains__")
    testbinop({1:2,3:4}, 1, 2, "a[b]", "__getitem__")
    d = {1:2,3:4}
    l1 = []
    for i in d.keys(): l1.append(i)
    l = []
    for i in iter(d): l.append(i)
    verify(l == l1)
    l = []
    for i in d.__iter__(): l.append(i)
    verify(l == l1)
    l = []
    for i in dictionary.__iter__(d): l.append(i)
    verify(l == l1)
    d = {1:2, 3:4}
    testunop(d, 2, "len(a)", "__len__")
    verify(eval(repr(d), {}) == d)
    verify(eval(d.__repr__(), {}) == d)
    testset2op({1:2,3:4}, 2, 3, {1:2,2:3,3:4}, "a[b]=c", "__setitem__")

binops = {
    'add': '+',
    'sub': '-',
    'mul': '*',
    'div': '/',
    'mod': '%',
    'divmod': 'divmod',
    'pow': '**',
    'lshift': '<<',
    'rshift': '>>',
    'and': '&',
    'xor': '^',
    'or': '|',
    'cmp': 'cmp',
    'lt': '<',
    'le': '<=',
    'eq': '==',
    'ne': '!=',
    'gt': '>',
    'ge': '>=',
    }

for name, expr in binops.items():
    if expr.islower():
        expr = expr + "(a, b)"
    else:
        expr = 'a %s b' % expr
    binops[name] = expr

unops = {
    'pos': '+',
    'neg': '-',
    'abs': 'abs',
    'invert': '~',
    'int': 'int',
    'long': 'long',
    'float': 'float',
    'oct': 'oct',
    'hex': 'hex',
    }

for name, expr in unops.items():
    if expr.islower():
        expr = expr + "(a)"
    else:
        expr = '%s a' % expr
    unops[name] = expr

def numops(a, b, skip=[]):
    dict = {'a': a, 'b': b}
    for name, expr in binops.items():
        if name not in skip:
            name = "__%s__" % name
            if hasattr(a, name):
                res = eval(expr, dict)
                testbinop(a, b, res, expr, name)
    for name, expr in unops.items():
        name = "__%s__" % name
        if hasattr(a, name):
            res = eval(expr, dict)
            testunop(a, res, expr, name)

def ints():
    if verbose: print "Testing int operations..."
    numops(100, 3)

def longs():
    if verbose: print "Testing long operations..."
    numops(100L, 3L)

def floats():
    if verbose: print "Testing float operations..."
    numops(100.0, 3.0)

def complexes():
    if verbose: print "Testing complex operations..."
    numops(100.0j, 3.0j, skip=['lt', 'le', 'gt', 'ge'])
    class Number(complex):
        __slots__ = ['prec']
        def __init__(self, *args, **kwds):
            self.prec = kwds.get('prec', 12)
        def __repr__(self):
            prec = self.prec
            if self.imag == 0.0:
                return "%.*g" % (prec, self.real)
            if self.real == 0.0:
                return "%.*gj" % (prec, self.imag)
            return "(%.*g+%.*gj)" % (prec, self.real, prec, self.imag)
        __str__ = __repr__
    a = Number(3.14, prec=6)
    verify(`a` == "3.14")
    verify(a.prec == 6)

def spamlists():
    if verbose: print "Testing spamlist operations..."
    import copy, xxsubtype as spam
    def spamlist(l, memo=None):
        import xxsubtype as spam
        return spam.spamlist(l)
    # This is an ugly hack:
    copy._deepcopy_dispatch[spam.spamlist] = spamlist

    testbinop(spamlist([1]), spamlist([2]), spamlist([1,2]), "a+b", "__add__")
    testbinop(spamlist([1,2,3]), 2, 1, "b in a", "__contains__")
    testbinop(spamlist([1,2,3]), 4, 0, "b in a", "__contains__")
    testbinop(spamlist([1,2,3]), 1, 2, "a[b]", "__getitem__")
    testternop(spamlist([1,2,3]), 0, 2, spamlist([1,2]),
               "a[b:c]", "__getslice__")
    testsetop(spamlist([1]), spamlist([2]), spamlist([1,2]),
              "a+=b", "__iadd__")
    testsetop(spamlist([1,2]), 3, spamlist([1,2,1,2,1,2]), "a*=b", "__imul__")
    testunop(spamlist([1,2,3]), 3, "len(a)", "__len__")
    testbinop(spamlist([1,2]), 3, spamlist([1,2,1,2,1,2]), "a*b", "__mul__")
    testbinop(spamlist([1,2]), 3, spamlist([1,2,1,2,1,2]), "b*a", "__rmul__")
    testset2op(spamlist([1,2]), 1, 3, spamlist([1,3]), "a[b]=c", "__setitem__")
    testset3op(spamlist([1,2,3,4]), 1, 3, spamlist([5,6]),
               spamlist([1,5,6,4]), "a[b:c]=d", "__setslice__")
    # Test subclassing
    class C(spam.spamlist):
        def foo(self): return 1
    a = C()
    verify(a == [])
    verify(a.foo() == 1)
    a.append(100)
    verify(a == [100])
    verify(a.getstate() == 0)
    a.setstate(42)
    verify(a.getstate() == 42)

def spamdicts():
    if verbose: print "Testing spamdict operations..."
    import copy, xxsubtype as spam
    def spamdict(d, memo=None):
        import xxsubtype as spam
        sd = spam.spamdict()
        for k, v in d.items(): sd[k] = v
        return sd
    # This is an ugly hack:
    copy._deepcopy_dispatch[spam.spamdict] = spamdict

    testbinop(spamdict({1:2}), spamdict({2:1}), -1, "cmp(a,b)", "__cmp__")
    testbinop(spamdict({1:2,3:4}), 1, 1, "b in a", "__contains__")
    testbinop(spamdict({1:2,3:4}), 2, 0, "b in a", "__contains__")
    testbinop(spamdict({1:2,3:4}), 1, 2, "a[b]", "__getitem__")
    d = spamdict({1:2,3:4})
    l1 = []
    for i in d.keys(): l1.append(i)
    l = []
    for i in iter(d): l.append(i)
    verify(l == l1)
    l = []
    for i in d.__iter__(): l.append(i)
    verify(l == l1)
    l = []
    for i in type(spamdict({})).__iter__(d): l.append(i)
    verify(l == l1)
    straightd = {1:2, 3:4}
    spamd = spamdict(straightd)
    testunop(spamd, 2, "len(a)", "__len__")
    testunop(spamd, repr(straightd), "repr(a)", "__repr__")
    testset2op(spamdict({1:2,3:4}), 2, 3, spamdict({1:2,2:3,3:4}),
               "a[b]=c", "__setitem__")
    # Test subclassing
    class C(spam.spamdict):
        def foo(self): return 1
    a = C()
    verify(a.items() == [])
    verify(a.foo() == 1)
    a['foo'] = 'bar'
    verify(a.items() == [('foo', 'bar')])
    verify(a.getstate() == 0)
    a.setstate(100)
    verify(a.getstate() == 100)

def pydicts():
    if verbose: print "Testing Python subclass of dict..."
    verify(issubclass(dictionary, dictionary))
    verify(isinstance({}, dictionary))
    d = dictionary()
    verify(d == {})
    verify(d.__class__ is dictionary)
    verify(isinstance(d, dictionary))
    class C(dictionary):
        state = -1
        def __init__(self, *a, **kw):
            if a:
                assert len(a) == 1
                self.state = a[0]
            if kw:
                for k, v in kw.items(): self[v] = k
        def __getitem__(self, key):
            return self.get(key, 0)
        def __setitem__(self, key, value):
            assert isinstance(key, type(0))
            dictionary.__setitem__(self, key, value)
        def setstate(self, state):
            self.state = state
        def getstate(self):
            return self.state
    verify(issubclass(C, dictionary))
    a1 = C(12)
    verify(a1.state == 12)
    a2 = C(foo=1, bar=2)
    verify(a2[1] == 'foo' and a2[2] == 'bar')
    a = C()
    verify(a.state == -1)
    verify(a.getstate() == -1)
    a.setstate(0)
    verify(a.state == 0)
    verify(a.getstate() == 0)
    a.setstate(10)
    verify(a.state == 10)
    verify(a.getstate() == 10)
    verify(a[42] == 0)
    a[42] = 24
    verify(a[42] == 24)
    if verbose: print "pydict stress test ..."
    N = 50
    for i in range(N):
        a[i] = C()
        for j in range(N):
            a[i][j] = i*j
    for i in range(N):
        for j in range(N):
            verify(a[i][j] == i*j)

def pylists():
    if verbose: print "Testing Python subclass of list..."
    class C(list):
        def __getitem__(self, i):
            return list.__getitem__(self, i) + 100
        def __getslice__(self, i, j):
            return (i, j)
    a = C()
    a.extend([0,1,2])
    verify(a[0] == 100)
    verify(a[1] == 101)
    verify(a[2] == 102)
    verify(a[100:200] == (100,200))

def metaclass():
    if verbose: print "Testing __metaclass__..."
    global C
    class C:
        __metaclass__ = type
        def __init__(self):
            self.__state = 0
        def getstate(self):
            return self.__state
        def setstate(self, state):
            self.__state = state
    a = C()
    verify(a.getstate() == 0)
    a.setstate(10)
    verify(a.getstate() == 10)
    class D:
        class __metaclass__(type):
            def myself(cls): return cls
    verify(D.myself() == D)

import sys
MT = type(sys)

def pymods():
    if verbose: print "Testing Python subclass of module..."
    global log
    log = []
    class MM(MT):
        def __init__(self):
            MT.__init__(self)
        def __getattr__(self, name):
            log.append(("getattr", name))
            return MT.__getattr__(self, name)
        def __setattr__(self, name, value):
            log.append(("setattr", name, value))
            MT.__setattr__(self, name, value)
        def __delattr__(self, name):
            log.append(("delattr", name))
            MT.__delattr__(self, name)
    a = MM()
    a.foo = 12
    x = a.foo
    del a.foo
    verify(log == [('getattr', '__init__'),
                   ('getattr', '__setattr__'),
                   ("setattr", "foo", 12),
                   ("getattr", "foo"),
                   ('getattr', '__delattr__'),
                   ("delattr", "foo")], log)

def multi():
    if verbose: print "Testing multiple inheritance..."
    global C
    class C(object):
        def __init__(self):
            self.__state = 0
        def getstate(self):
            return self.__state
        def setstate(self, state):
            self.__state = state
    a = C()
    verify(a.getstate() == 0)
    a.setstate(10)
    verify(a.getstate() == 10)
    class D(dictionary, C):
        def __init__(self):
            type({}).__init__(self)
            C.__init__(self)
    d = D()
    verify(d.keys() == [])
    d["hello"] = "world"
    verify(d.items() == [("hello", "world")])
    verify(d["hello"] == "world")
    verify(d.getstate() == 0)
    d.setstate(10)
    verify(d.getstate() == 10)
    verify(D.__mro__ == (D, dictionary, C, object))

def diamond():
    if verbose: print "Testing multiple inheritance special cases..."
    class A(object):
        def spam(self): return "A"
    verify(A().spam() == "A")
    class B(A):
        def boo(self): return "B"
        def spam(self): return "B"
    verify(B().spam() == "B")
    verify(B().boo() == "B")
    class C(A):
        def boo(self): return "C"
    verify(C().spam() == "A")
    verify(C().boo() == "C")
    class D(B, C): pass
    verify(D().spam() == "B")
    verify(D().boo() == "B")
    verify(D.__mro__ == (D, B, C, A, object))
    class E(C, B): pass
    verify(E().spam() == "B")
    verify(E().boo() == "C")
    verify(E.__mro__ == (E, C, B, A, object))
    class F(D, E): pass
    verify(F().spam() == "B")
    verify(F().boo() == "B")
    verify(F.__mro__ == (F, D, E, B, C, A, object))
    class G(E, D): pass
    verify(G().spam() == "B")
    verify(G().boo() == "C")
    verify(G.__mro__ == (G, E, D, C, B, A, object))

def objects():
    if verbose: print "Testing object class..."
    a = object()
    verify(a.__class__ == object == type(a))
    b = object()
    verify(a is not b)
    verify(not hasattr(a, "foo"))
    try:
        a.foo = 12
    except TypeError:
        pass
    else:
        verify(0, "object() should not allow setting a foo attribute")
    verify(not hasattr(object(), "__dict__"))

    class Cdict(object):
        pass
    x = Cdict()
    verify(x.__dict__ is None)
    x.foo = 1
    verify(x.foo == 1)
    verify(x.__dict__ == {'foo': 1})

def slots():
    if verbose: print "Testing __slots__..."
    class C0(object):
        __slots__ = []
    x = C0()
    verify(not hasattr(x, "__dict__"))
    verify(not hasattr(x, "foo"))

    class C1(object):
        __slots__ = ['a']
    x = C1()
    verify(not hasattr(x, "__dict__"))
    verify(x.a == None)
    x.a = 1
    verify(x.a == 1)
    del x.a
    verify(x.a == None)

    class C3(object):
        __slots__ = ['a', 'b', 'c']
    x = C3()
    verify(not hasattr(x, "__dict__"))
    verify(x.a is None)
    verify(x.b is None)
    verify(x.c is None)
    x.a = 1
    x.b = 2
    x.c = 3
    verify(x.a == 1)
    verify(x.b == 2)
    verify(x.c == 3)

def dynamics():
    if verbose: print "Testing __dynamic__..."
    verify(object.__dynamic__ == 0)
    verify(list.__dynamic__ == 0)
    class S1:
        __metaclass__ = type
    verify(S1.__dynamic__ == 0)
    class S(object):
        pass
    verify(C.__dynamic__ == 0)
    class D(object):
        __dynamic__ = 1
    verify(D.__dynamic__ == 1)
    class E(D, S):
        pass
    verify(E.__dynamic__ == 1)
    class F(S, D):
        pass
    verify(F.__dynamic__ == 1)
    try:
        S.foo = 1
    except (AttributeError, TypeError):
        pass
    else:
        verify(0, "assignment to a static class attribute should be illegal")
    D.foo = 1
    verify(D.foo == 1)
    # Test that dynamic attributes are inherited
    verify(E.foo == 1)
    verify(F.foo == 1)
    class SS(D):
        __dynamic__ = 0
    verify(SS.__dynamic__ == 0)
    verify(SS.foo == 1)
    try:
        SS.foo = 1
    except (AttributeError, TypeError):
        pass
    else:
        verify(0, "assignment to SS.foo should be illegal")

def errors():
    if verbose: print "Testing errors..."

    try:
        class C(list, dictionary):
            pass
    except TypeError:
        pass
    else:
        verify(0, "inheritance from both list and dict should be illegal")

    try:
        class C(object, None):
            pass
    except TypeError:
        pass
    else:
        verify(0, "inheritance from non-type should be illegal")
    class Classic:
        pass

    try:
        class C(object, Classic):
            pass
    except TypeError:
        pass
    else:
        verify(0, "inheritance from object and Classic should be illegal")

    try:
        class C(int):
            pass
    except TypeError:
        pass
    else:
        verify(0, "inheritance from int should be illegal")

    try:
        class C(object):
            __slots__ = 1
    except TypeError:
        pass
    else:
        verify(0, "__slots__ = 1 should be illegal")

    try:
        class C(object):
            __slots__ = [1]
    except TypeError:
        pass
    else:
        verify(0, "__slots__ = [1] should be illegal")

def classmethods():
    if verbose: print "Testing class methods..."
    class C(object):
        def foo(*a): return a
        goo = classmethod(foo)
    c = C()
    verify(C.goo(1) == (C, 1))
    verify(c.goo(1) == (C, 1))
    verify(c.foo(1) == (c, 1))
    class D(C):
        pass
    d = D()
    verify(D.goo(1) == (D, 1))
    verify(d.goo(1) == (D, 1))
    verify(d.foo(1) == (d, 1))
    verify(D.foo(d, 1) == (d, 1))

def staticmethods():
    if verbose: print "Testing static methods..."
    class C(object):
        def foo(*a): return a
        goo = staticmethod(foo)
    c = C()
    verify(C.goo(1) == (1,))
    verify(c.goo(1) == (1,))
    verify(c.foo(1) == (c, 1,))
    class D(C):
        pass
    d = D()
    verify(D.goo(1) == (1,))
    verify(d.goo(1) == (1,))
    verify(d.foo(1) == (d, 1))
    verify(D.foo(d, 1) == (d, 1))

def classic():
    if verbose: print "Testing classic classes..."
    class C:
        def foo(*a): return a
        goo = classmethod(foo)
    c = C()
    verify(C.goo(1) == (C, 1))
    verify(c.goo(1) == (C, 1))
    verify(c.foo(1) == (c, 1))
    class D(C):
        pass
    d = D()
    verify(D.goo(1) == (D, 1))
    verify(d.goo(1) == (D, 1))
    verify(d.foo(1) == (d, 1))
    verify(D.foo(d, 1) == (d, 1))

def compattr():
    if verbose: print "Testing computed attributes..."
    class C(object):
        class computed_attribute(object):
            def __init__(self, get, set=None):
                self.__get = get
                self.__set = set
            def __get__(self, obj, type=None):
                return self.__get(obj)
            def __set__(self, obj, value):
                return self.__set(obj, value)
        def __init__(self):
            self.__x = 0
        def __get_x(self):
            x = self.__x
            self.__x = x+1
            return x
        def __set_x(self, x):
            self.__x = x
        x = computed_attribute(__get_x, __set_x)
    a = C()
    verify(a.x == 0)
    verify(a.x == 1)
    a.x = 10
    verify(a.x == 10)
    verify(a.x == 11)

def newslot():
    if verbose: print "Testing __new__ slot override..."
    class C(list):
        def __new__(cls):
            self = list.__new__(cls)
            self.foo = 1
            return self
        def __init__(self):
            self.foo = self.foo + 2
    a = C()
    verify(a.foo == 3)
    verify(a.__class__ is C)
    class D(C):
        pass
    b = D()
    verify(b.foo == 3)
    verify(b.__class__ is D)

class PerverseMetaType(type):
    def mro(cls):
        L = type.mro(cls)
        L.reverse()
        return L

def altmro():
    if verbose: print "Testing mro() and overriding it..."
    class A(object):
        def f(self): return "A"
    class B(A):
        pass
    class C(A):
        def f(self): return "C"
    class D(B, C):
        pass
    verify(D.mro() == [D, B, C, A, object] == list(D.__mro__))
    verify(D().f() == "C")
    class X(A,B,C,D):
        __metaclass__ = PerverseMetaType
    verify(X.__mro__ == (object, A, C, B, D, X))
    verify(X().f() == "A")

def overloading():
    if verbose: print "testing operator overloading..."

    class B(object):
        "Intermediate class because object doesn't have a __setattr__"

    class C(B):

        def __getattr__(self, name):
            if name == "foo":
                return ("getattr", name)
            else:
                return B.__getattr__(self, name)
        def __setattr__(self, name, value):
            if name == "foo":
                self.setattr = (name, value)
            else:
                return B.__setattr__(self, name, value)
        def __delattr__(self, name):
            if name == "foo":
                self.delattr = name
            else:
                return B.__delattr__(self, name)

        def __getitem__(self, key):
            return ("getitem", key)
        def __setitem__(self, key, value):
            self.setitem = (key, value)
        def __delitem__(self, key):
            self.delitem = key

        def __getslice__(self, i, j):
            return ("getslice", i, j)
        def __setslice__(self, i, j, value):
            self.setslice = (i, j, value)
        def __delslice__(self, i, j):
            self.delslice = (i, j)

    a = C()
    verify(a.foo == ("getattr", "foo"))
    a.foo = 12
    verify(a.setattr == ("foo", 12))
    del a.foo
    verify(a.delattr == "foo")

    verify(a[12] == ("getitem", 12))
    a[12] = 21
    verify(a.setitem == (12, 21))
    del a[12]
    verify(a.delitem == 12)

    verify(a[0:10] == ("getslice", 0, 10))
    a[0:10] = "foo"
    verify(a.setslice == (0, 10, "foo"))
    del a[0:10]
    verify(a.delslice == (0, 10))

def all():
    lists()
    dicts()
    ints()
    longs()
    floats()
    complexes()
    spamlists()
    spamdicts()
    pydicts()
    pylists()
    metaclass()
    pymods()
    multi()
    diamond()
    objects()
    slots()
    dynamics()
    errors()
    classmethods()
    staticmethods()
    classic()
    compattr()
    newslot()
    altmro()
    overloading()

all()

if verbose: print "All OK"
