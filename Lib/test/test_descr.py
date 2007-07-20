# Test enhancements related to descriptors and new-style classes

from test.test_support import verify, vereq, verbose, TestFailed, TESTFN, get_original_stdout
from copy import deepcopy
import warnings
import types

warnings.filterwarnings("ignore",
         r'complex divmod\(\), // and % are deprecated$',
         DeprecationWarning, r'(<string>|%s)$' % __name__)

def veris(a, b):
    if a is not b:
        raise TestFailed, "%r is %r" % (a, b)

def testunop(a, res, expr="len(a)", meth="__len__"):
    if verbose: print("checking", expr)
    dict = {'a': a}
    vereq(eval(expr, dict), res)
    t = type(a)
    m = getattr(t, meth)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    vereq(m, t.__dict__[meth])
    vereq(m(a), res)
    bm = getattr(a, meth)
    vereq(bm(), res)

def testbinop(a, b, res, expr="a+b", meth="__add__"):
    if verbose: print("checking", expr)
    dict = {'a': a, 'b': b}

    vereq(eval(expr, dict), res)
    t = type(a)
    m = getattr(t, meth)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    vereq(m, t.__dict__[meth])
    vereq(m(a, b), res)
    bm = getattr(a, meth)
    vereq(bm(b), res)

def testternop(a, b, c, res, expr="a[b:c]", meth="__getslice__"):
    if verbose: print("checking", expr)
    dict = {'a': a, 'b': b, 'c': c}
    vereq(eval(expr, dict), res)
    t = type(a)
    m = getattr(t, meth)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    vereq(m, t.__dict__[meth])
    vereq(m(a, b, c), res)
    bm = getattr(a, meth)
    vereq(bm(b, c), res)

def testsetop(a, b, res, stmt="a+=b", meth="__iadd__"):
    if verbose: print("checking", stmt)
    dict = {'a': deepcopy(a), 'b': b}
    exec(stmt, dict)
    vereq(dict['a'], res)
    t = type(a)
    m = getattr(t, meth)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    vereq(m, t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b)
    vereq(dict['a'], res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b)
    vereq(dict['a'], res)

def testset2op(a, b, c, res, stmt="a[b]=c", meth="__setitem__"):
    if verbose: print("checking", stmt)
    dict = {'a': deepcopy(a), 'b': b, 'c': c}
    exec(stmt, dict)
    vereq(dict['a'], res)
    t = type(a)
    m = getattr(t, meth)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    vereq(m, t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b, c)
    vereq(dict['a'], res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b, c)
    vereq(dict['a'], res)

def testset3op(a, b, c, d, res, stmt="a[b:c]=d", meth="__setslice__"):
    if verbose: print("checking", stmt)
    dict = {'a': deepcopy(a), 'b': b, 'c': c, 'd': d}
    exec(stmt, dict)
    vereq(dict['a'], res)
    t = type(a)
    while meth not in t.__dict__:
        t = t.__bases__[0]
    m = getattr(t, meth)
    vereq(m, t.__dict__[meth])
    dict['a'] = deepcopy(a)
    m(dict['a'], b, c, d)
    vereq(dict['a'], res)
    dict['a'] = deepcopy(a)
    bm = getattr(dict['a'], meth)
    bm(b, c, d)
    vereq(dict['a'], res)

def class_docstrings():
    class Classic:
        "A classic docstring."
    vereq(Classic.__doc__, "A classic docstring.")
    vereq(Classic.__dict__['__doc__'], "A classic docstring.")

    class Classic2:
        pass
    verify(Classic2.__doc__ is None)

    class NewStatic(object):
        "Another docstring."
    vereq(NewStatic.__doc__, "Another docstring.")
    vereq(NewStatic.__dict__['__doc__'], "Another docstring.")

    class NewStatic2(object):
        pass
    verify(NewStatic2.__doc__ is None)

    class NewDynamic(object):
        "Another docstring."
    vereq(NewDynamic.__doc__, "Another docstring.")
    vereq(NewDynamic.__dict__['__doc__'], "Another docstring.")

    class NewDynamic2(object):
        pass
    verify(NewDynamic2.__doc__ is None)

def lists():
    if verbose: print("Testing list operations...")
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
    if verbose: print("Testing dict operations...")
    ##testbinop({1:2}, {2:1}, -1, "cmp(a,b)", "__cmp__")
    testbinop({1:2,3:4}, 1, 1, "b in a", "__contains__")
    testbinop({1:2,3:4}, 2, 0, "b in a", "__contains__")
    testbinop({1:2,3:4}, 1, 2, "a[b]", "__getitem__")
    d = {1:2,3:4}
    l1 = []
    for i in d.keys(): l1.append(i)
    l = []
    for i in iter(d): l.append(i)
    vereq(l, l1)
    l = []
    for i in d.__iter__(): l.append(i)
    vereq(l, l1)
    l = []
    for i in dict.__iter__(d): l.append(i)
    vereq(l, l1)
    d = {1:2, 3:4}
    testunop(d, 2, "len(a)", "__len__")
    vereq(eval(repr(d), {}), d)
    vereq(eval(d.__repr__(), {}), d)
    testset2op({1:2,3:4}, 2, 3, {1:2,2:3,3:4}, "a[b]=c", "__setitem__")

def dict_constructor():
    if verbose:
        print("Testing dict constructor ...")
    d = dict()
    vereq(d, {})
    d = dict({})
    vereq(d, {})
    d = dict({1: 2, 'a': 'b'})
    vereq(d, {1: 2, 'a': 'b'})
    vereq(d, dict(d.items()))
    vereq(d, dict(d.items()))
    d = dict({'one':1, 'two':2})
    vereq(d, dict(one=1, two=2))
    vereq(d, dict(**d))
    vereq(d, dict({"one": 1}, two=2))
    vereq(d, dict([("two", 2)], one=1))
    vereq(d, dict([("one", 100), ("two", 200)], **d))
    verify(d is not dict(**d))
    for badarg in 0, 0, 0j, "0", [0], (0,):
        try:
            dict(badarg)
        except TypeError:
            pass
        except ValueError:
            if badarg == "0":
                # It's a sequence, and its elements are also sequences (gotta
                # love strings <wink>), but they aren't of length 2, so this
                # one seemed better as a ValueError than a TypeError.
                pass
            else:
                raise TestFailed("no TypeError from dict(%r)" % badarg)
        else:
            raise TestFailed("no TypeError from dict(%r)" % badarg)

    try:
        dict({}, {})
    except TypeError:
        pass
    else:
        raise TestFailed("no TypeError from dict({}, {})")

    class Mapping:
        # Lacks a .keys() method; will be added later.
        dict = {1:2, 3:4, 'a':1j}

    try:
        dict(Mapping())
    except TypeError:
        pass
    else:
        raise TestFailed("no TypeError from dict(incomplete mapping)")

    Mapping.keys = lambda self: self.dict.keys()
    Mapping.__getitem__ = lambda self, i: self.dict[i]
    d = dict(Mapping())
    vereq(d, Mapping.dict)

    # Init from sequence of iterable objects, each producing a 2-sequence.
    class AddressBookEntry:
        def __init__(self, first, last):
            self.first = first
            self.last = last
        def __iter__(self):
            return iter([self.first, self.last])

    d = dict([AddressBookEntry('Tim', 'Warsaw'),
              AddressBookEntry('Barry', 'Peters'),
              AddressBookEntry('Tim', 'Peters'),
              AddressBookEntry('Barry', 'Warsaw')])
    vereq(d, {'Barry': 'Warsaw', 'Tim': 'Peters'})

    d = dict(zip(range(4), range(1, 5)))
    vereq(d, dict([(i, i+1) for i in range(4)]))

    # Bad sequence lengths.
    for bad in [('tooshort',)], [('too', 'long', 'by 1')]:
        try:
            dict(bad)
        except ValueError:
            pass
        else:
            raise TestFailed("no ValueError from dict(%r)" % bad)

def test_dir():
    if verbose:
        print("Testing dir() ...")
    junk = 12
    vereq(dir(), ['junk'])
    del junk

    # Just make sure these don't blow up!
    for arg in 2, 2, 2j, 2e0, [2], "2", b"2", (2,), {2:2}, type, test_dir:
        dir(arg)

    # Test dir on custom classes. Since these have object as a
    # base class, a lot of stuff gets sucked in.
    def interesting(strings):
        return [s for s in strings if not s.startswith('_')]

    class C(object):
        Cdata = 1
        def Cmethod(self): pass

    cstuff = ['Cdata', 'Cmethod']
    vereq(interesting(dir(C)), cstuff)

    c = C()
    vereq(interesting(dir(c)), cstuff)
    verify('im_self' in dir(C.Cmethod))

    c.cdata = 2
    c.cmethod = lambda self: 0
    vereq(interesting(dir(c)), cstuff + ['cdata', 'cmethod'])
    verify('im_self' in dir(c.Cmethod))

    class A(C):
        Adata = 1
        def Amethod(self): pass

    astuff = ['Adata', 'Amethod'] + cstuff
    vereq(interesting(dir(A)), astuff)
    verify('im_self' in dir(A.Amethod))
    a = A()
    vereq(interesting(dir(a)), astuff)
    a.adata = 42
    a.amethod = lambda self: 3
    vereq(interesting(dir(a)), astuff + ['adata', 'amethod'])
    verify('im_self' in dir(a.Amethod))

    # Try a module subclass.
    import sys
    class M(type(sys)):
        pass
    minstance = M("m")
    minstance.b = 2
    minstance.a = 1
    names = [x for x in dir(minstance) if x not in ["__name__", "__doc__"]]
    vereq(names, ['a', 'b'])

    class M2(M):
        def getdict(self):
            return "Not a dict!"
        __dict__ = property(getdict)

    m2instance = M2("m2")
    m2instance.b = 2
    m2instance.a = 1
    vereq(m2instance.__dict__, "Not a dict!")
    try:
        dir(m2instance)
    except TypeError:
        pass

    # Two essentially featureless objects, just inheriting stuff from
    # object.
    vereq(dir(None), dir(Ellipsis))

    # Nasty test case for proxied objects
    class Wrapper(object):
        def __init__(self, obj):
            self.__obj = obj
        def __repr__(self):
            return "Wrapper(%s)" % repr(self.__obj)
        def __getitem__(self, key):
            return Wrapper(self.__obj[key])
        def __len__(self):
            return len(self.__obj)
        def __getattr__(self, name):
            return Wrapper(getattr(self.__obj, name))

    class C(object):
        def __getclass(self):
            return Wrapper(type(self))
        __class__ = property(__getclass)

    dir(C()) # This used to segfault

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
        if name not in skip:
            name = "__%s__" % name
            if hasattr(a, name):
                res = eval(expr, dict)
                testunop(a, res, expr, name)

def ints():
    if verbose: print("Testing int operations...")
    numops(100, 3)
    # The following crashes in Python 2.2
    vereq((1).__bool__(), True)
    vereq((0).__bool__(), False)
    # This returns 'NotImplemented' in Python 2.2
    class C(int):
        def __add__(self, other):
            return NotImplemented
    vereq(C(5), 5)
    try:
        C() + ""
    except TypeError:
        pass
    else:
        raise TestFailed, "NotImplemented should have caused TypeError"

def longs():
    if verbose: print("Testing long operations...")
    numops(100, 3)

def floats():
    if verbose: print("Testing float operations...")
    numops(100.0, 3.0)

def complexes():
    if verbose: print("Testing complex operations...")
    numops(100.0j, 3.0j, skip=['lt', 'le', 'gt', 'ge', 'int', 'long', 'float'])
    class Number(complex):
        __slots__ = ['prec']
        def __new__(cls, *args, **kwds):
            result = complex.__new__(cls, *args)
            result.prec = kwds.get('prec', 12)
            return result
        def __repr__(self):
            prec = self.prec
            if self.imag == 0.0:
                return "%.*g" % (prec, self.real)
            if self.real == 0.0:
                return "%.*gj" % (prec, self.imag)
            return "(%.*g+%.*gj)" % (prec, self.real, prec, self.imag)
        __str__ = __repr__

    a = Number(3.14, prec=6)
    vereq(repr(a), "3.14")
    vereq(a.prec, 6)

    a = Number(a, prec=2)
    vereq(repr(a), "3.1")
    vereq(a.prec, 2)

    a = Number(234.5)
    vereq(repr(a), "234.5")
    vereq(a.prec, 12)

def spamlists():
    if verbose: print("Testing spamlist operations...")
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
    vereq(a, [])
    vereq(a.foo(), 1)
    a.append(100)
    vereq(a, [100])
    vereq(a.getstate(), 0)
    a.setstate(42)
    vereq(a.getstate(), 42)

def spamdicts():
    if verbose: print("Testing spamdict operations...")
    import copy, xxsubtype as spam
    def spamdict(d, memo=None):
        import xxsubtype as spam
        sd = spam.spamdict()
        for k, v in d.items(): sd[k] = v
        return sd
    # This is an ugly hack:
    copy._deepcopy_dispatch[spam.spamdict] = spamdict

    ##testbinop(spamdict({1:2}), spamdict({2:1}), -1, "cmp(a,b)", "__cmp__")
    testbinop(spamdict({1:2,3:4}), 1, 1, "b in a", "__contains__")
    testbinop(spamdict({1:2,3:4}), 2, 0, "b in a", "__contains__")
    testbinop(spamdict({1:2,3:4}), 1, 2, "a[b]", "__getitem__")
    d = spamdict({1:2,3:4})
    l1 = []
    for i in d.keys(): l1.append(i)
    l = []
    for i in iter(d): l.append(i)
    vereq(l, l1)
    l = []
    for i in d.__iter__(): l.append(i)
    vereq(l, l1)
    l = []
    for i in type(spamdict({})).__iter__(d): l.append(i)
    vereq(l, l1)
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
    vereq(list(a.items()), [])
    vereq(a.foo(), 1)
    a['foo'] = 'bar'
    vereq(list(a.items()), [('foo', 'bar')])
    vereq(a.getstate(), 0)
    a.setstate(100)
    vereq(a.getstate(), 100)

def pydicts():
    if verbose: print("Testing Python subclass of dict...")
    verify(issubclass(dict, dict))
    verify(isinstance({}, dict))
    d = dict()
    vereq(d, {})
    verify(d.__class__ is dict)
    verify(isinstance(d, dict))
    class C(dict):
        state = -1
        def __init__(self, *a, **kw):
            if a:
                vereq(len(a), 1)
                self.state = a[0]
            if kw:
                for k, v in kw.items(): self[v] = k
        def __getitem__(self, key):
            return self.get(key, 0)
        def __setitem__(self, key, value):
            verify(isinstance(key, type(0)))
            dict.__setitem__(self, key, value)
        def setstate(self, state):
            self.state = state
        def getstate(self):
            return self.state
    verify(issubclass(C, dict))
    a1 = C(12)
    vereq(a1.state, 12)
    a2 = C(foo=1, bar=2)
    vereq(a2[1] == 'foo' and a2[2], 'bar')
    a = C()
    vereq(a.state, -1)
    vereq(a.getstate(), -1)
    a.setstate(0)
    vereq(a.state, 0)
    vereq(a.getstate(), 0)
    a.setstate(10)
    vereq(a.state, 10)
    vereq(a.getstate(), 10)
    vereq(a[42], 0)
    a[42] = 24
    vereq(a[42], 24)
    if verbose: print("pydict stress test ...")
    N = 50
    for i in range(N):
        a[i] = C()
        for j in range(N):
            a[i][j] = i*j
    for i in range(N):
        for j in range(N):
            vereq(a[i][j], i*j)

def pylists():
    if verbose: print("Testing Python subclass of list...")
    class C(list):
        def __getitem__(self, i):
            return list.__getitem__(self, i) + 100
        def __getslice__(self, i, j):
            return (i, j)
    a = C()
    a.extend([0,1,2])
    vereq(a[0], 100)
    vereq(a[1], 101)
    vereq(a[2], 102)
    vereq(a[100:200], (100,200))

def metaclass():
    if verbose: print("Testing metaclass...")
    class C(metaclass=type):
        def __init__(self):
            self.__state = 0
        def getstate(self):
            return self.__state
        def setstate(self, state):
            self.__state = state
    a = C()
    vereq(a.getstate(), 0)
    a.setstate(10)
    vereq(a.getstate(), 10)
    class _metaclass(type):
        def myself(cls): return cls
    class D(metaclass=_metaclass):
        pass
    vereq(D.myself(), D)
    d = D()
    verify(d.__class__ is D)
    class M1(type):
        def __new__(cls, name, bases, dict):
            dict['__spam__'] = 1
            return type.__new__(cls, name, bases, dict)
    class C(metaclass=M1):
        pass
    vereq(C.__spam__, 1)
    c = C()
    vereq(c.__spam__, 1)

    class _instance(object):
        pass
    class M2(object):
        @staticmethod
        def __new__(cls, name, bases, dict):
            self = object.__new__(cls)
            self.name = name
            self.bases = bases
            self.dict = dict
            return self
        def __call__(self):
            it = _instance()
            # Early binding of methods
            for key in self.dict:
                if key.startswith("__"):
                    continue
                setattr(it, key, self.dict[key].__get__(it, self))
            return it
    class C(metaclass=M2):
        def spam(self):
            return 42
    vereq(C.name, 'C')
    vereq(C.bases, ())
    verify('spam' in C.dict)
    c = C()
    vereq(c.spam(), 42)

    # More metaclass examples

    class autosuper(type):
        # Automatically add __super to the class
        # This trick only works for dynamic classes
        def __new__(metaclass, name, bases, dict):
            cls = super(autosuper, metaclass).__new__(metaclass,
                                                      name, bases, dict)
            # Name mangling for __super removes leading underscores
            while name[:1] == "_":
                name = name[1:]
            if name:
                name = "_%s__super" % name
            else:
                name = "__super"
            setattr(cls, name, super(cls))
            return cls
    class A(metaclass=autosuper):
        def meth(self):
            return "A"
    class B(A):
        def meth(self):
            return "B" + self.__super.meth()
    class C(A):
        def meth(self):
            return "C" + self.__super.meth()
    class D(C, B):
        def meth(self):
            return "D" + self.__super.meth()
    vereq(D().meth(), "DCBA")
    class E(B, C):
        def meth(self):
            return "E" + self.__super.meth()
    vereq(E().meth(), "EBCA")

    class autoproperty(type):
        # Automatically create property attributes when methods
        # named _get_x and/or _set_x are found
        def __new__(metaclass, name, bases, dict):
            hits = {}
            for key, val in dict.items():
                if key.startswith("_get_"):
                    key = key[5:]
                    get, set = hits.get(key, (None, None))
                    get = val
                    hits[key] = get, set
                elif key.startswith("_set_"):
                    key = key[5:]
                    get, set = hits.get(key, (None, None))
                    set = val
                    hits[key] = get, set
            for key, (get, set) in hits.items():
                dict[key] = property(get, set)
            return super(autoproperty, metaclass).__new__(metaclass,
                                                        name, bases, dict)
    class A(metaclass=autoproperty):
        def _get_x(self):
            return -self.__x
        def _set_x(self, x):
            self.__x = -x
    a = A()
    verify(not hasattr(a, "x"))
    a.x = 12
    vereq(a.x, 12)
    vereq(a._A__x, -12)

    class multimetaclass(autoproperty, autosuper):
        # Merge of multiple cooperating metaclasses
        pass
    class A(metaclass=multimetaclass):
        def _get_x(self):
            return "A"
    class B(A):
        def _get_x(self):
            return "B" + self.__super._get_x()
    class C(A):
        def _get_x(self):
            return "C" + self.__super._get_x()
    class D(C, B):
        def _get_x(self):
            return "D" + self.__super._get_x()
    vereq(D().x, "DCBA")

    # Make sure type(x) doesn't call x.__class__.__init__
    class T(type):
        counter = 0
        def __init__(self, *args):
            T.counter += 1
    class C(metaclass=T):
        pass
    vereq(T.counter, 1)
    a = C()
    vereq(type(a), C)
    vereq(T.counter, 1)

    class C(object): pass
    c = C()
    try: c()
    except TypeError: pass
    else: raise TestFailed, "calling object w/o call method should raise TypeError"

    # Testing code to find most derived baseclass
    class A(type):
        def __new__(*args, **kwargs):
            return type.__new__(*args, **kwargs)

    class B(object):
        pass

    class C(object, metaclass=A):
        pass

    # The most derived metaclass of D is A rather than type.
    class D(B, C):
        pass


def pymods():
    if verbose: print("Testing Python subclass of module...")
    log = []
    import sys
    MT = type(sys)
    class MM(MT):
        def __init__(self, name):
            MT.__init__(self, name)
        def __getattribute__(self, name):
            log.append(("getattr", name))
            return MT.__getattribute__(self, name)
        def __setattr__(self, name, value):
            log.append(("setattr", name, value))
            MT.__setattr__(self, name, value)
        def __delattr__(self, name):
            log.append(("delattr", name))
            MT.__delattr__(self, name)
    a = MM("a")
    a.foo = 12
    x = a.foo
    del a.foo
    vereq(log, [("setattr", "foo", 12),
                ("getattr", "foo"),
                ("delattr", "foo")])

    # http://python.org/sf/1174712
    try:
        class Module(types.ModuleType, str):
            pass
    except TypeError:
        pass
    else:
        raise TestFailed("inheriting from ModuleType and str at the "
                          "same time should fail")

def multi():
    if verbose: print("Testing multiple inheritance...")
    class C(object):
        def __init__(self):
            self.__state = 0
        def getstate(self):
            return self.__state
        def setstate(self, state):
            self.__state = state
    a = C()
    vereq(a.getstate(), 0)
    a.setstate(10)
    vereq(a.getstate(), 10)
    class D(dict, C):
        def __init__(self):
            type({}).__init__(self)
            C.__init__(self)
    d = D()
    vereq(list(d.keys()), [])
    d["hello"] = "world"
    vereq(list(d.items()), [("hello", "world")])
    vereq(d["hello"], "world")
    vereq(d.getstate(), 0)
    d.setstate(10)
    vereq(d.getstate(), 10)
    vereq(D.__mro__, (D, dict, C, object))

    # SF bug #442833
    class Node(object):
        def __int__(self):
            return int(self.foo())
        def foo(self):
            return "23"
    class Frag(Node, list):
        def foo(self):
            return "42"
    vereq(Node().__int__(), 23)
    vereq(int(Node()), 23)
    vereq(Frag().__int__(), 42)
    vereq(int(Frag()), 42)

def diamond():
    if verbose: print("Testing multiple inheritance special cases...")
    class A(object):
        def spam(self): return "A"
    vereq(A().spam(), "A")
    class B(A):
        def boo(self): return "B"
        def spam(self): return "B"
    vereq(B().spam(), "B")
    vereq(B().boo(), "B")
    class C(A):
        def boo(self): return "C"
    vereq(C().spam(), "A")
    vereq(C().boo(), "C")
    class D(B, C): pass
    vereq(D().spam(), "B")
    vereq(D().boo(), "B")
    vereq(D.__mro__, (D, B, C, A, object))
    class E(C, B): pass
    vereq(E().spam(), "B")
    vereq(E().boo(), "C")
    vereq(E.__mro__, (E, C, B, A, object))
    # MRO order disagreement
    try:
        class F(D, E): pass
    except TypeError:
        pass
    else:
        raise TestFailed, "expected MRO order disagreement (F)"
    try:
        class G(E, D): pass
    except TypeError:
        pass
    else:
        raise TestFailed, "expected MRO order disagreement (G)"


# see thread python-dev/2002-October/029035.html
def ex5():
    if verbose: print("Testing ex5 from C3 switch discussion...")
    class A(object): pass
    class B(object): pass
    class C(object): pass
    class X(A): pass
    class Y(A): pass
    class Z(X,B,Y,C): pass
    vereq(Z.__mro__, (Z, X, B, Y, A, C, object))

# see "A Monotonic Superclass Linearization for Dylan",
# by Kim Barrett et al. (OOPSLA 1996)
def monotonicity():
    if verbose: print("Testing MRO monotonicity...")
    class Boat(object): pass
    class DayBoat(Boat): pass
    class WheelBoat(Boat): pass
    class EngineLess(DayBoat): pass
    class SmallMultihull(DayBoat): pass
    class PedalWheelBoat(EngineLess,WheelBoat): pass
    class SmallCatamaran(SmallMultihull): pass
    class Pedalo(PedalWheelBoat,SmallCatamaran): pass

    vereq(PedalWheelBoat.__mro__,
          (PedalWheelBoat, EngineLess, DayBoat, WheelBoat, Boat,
           object))
    vereq(SmallCatamaran.__mro__,
          (SmallCatamaran, SmallMultihull, DayBoat, Boat, object))

    vereq(Pedalo.__mro__,
          (Pedalo, PedalWheelBoat, EngineLess, SmallCatamaran,
           SmallMultihull, DayBoat, WheelBoat, Boat, object))

# see "A Monotonic Superclass Linearization for Dylan",
# by Kim Barrett et al. (OOPSLA 1996)
def consistency_with_epg():
    if verbose: print("Testing consistentcy with EPG...")
    class Pane(object): pass
    class ScrollingMixin(object): pass
    class EditingMixin(object): pass
    class ScrollablePane(Pane,ScrollingMixin): pass
    class EditablePane(Pane,EditingMixin): pass
    class EditableScrollablePane(ScrollablePane,EditablePane): pass

    vereq(EditableScrollablePane.__mro__,
          (EditableScrollablePane, ScrollablePane, EditablePane,
           Pane, ScrollingMixin, EditingMixin, object))

mro_err_msg = """Cannot create a consistent method resolution
order (MRO) for bases """

def mro_disagreement():
    if verbose: print("Testing error messages for MRO disagreement...")
    def raises(exc, expected, callable, *args):
        try:
            callable(*args)
        except exc as msg:
            if not str(msg).startswith(expected):
                raise TestFailed, "Message %r, expected %r" % (str(msg),
                                                               expected)
        else:
            raise TestFailed, "Expected %s" % exc
    class A(object): pass
    class B(A): pass
    class C(object): pass
    # Test some very simple errors
    raises(TypeError, "duplicate base class A",
           type, "X", (A, A), {})
    raises(TypeError, mro_err_msg,
           type, "X", (A, B), {})
    raises(TypeError, mro_err_msg,
           type, "X", (A, C, B), {})
    # Test a slightly more complex error
    class GridLayout(object): pass
    class HorizontalGrid(GridLayout): pass
    class VerticalGrid(GridLayout): pass
    class HVGrid(HorizontalGrid, VerticalGrid): pass
    class VHGrid(VerticalGrid, HorizontalGrid): pass
    raises(TypeError, mro_err_msg,
           type, "ConfusedGrid", (HVGrid, VHGrid), {})

def objects():
    if verbose: print("Testing object class...")
    a = object()
    vereq(a.__class__, object)
    vereq(type(a), object)
    b = object()
    verify(a is not b)
    verify(not hasattr(a, "foo"))
    try:
        a.foo = 12
    except (AttributeError, TypeError):
        pass
    else:
        verify(0, "object() should not allow setting a foo attribute")
    verify(not hasattr(object(), "__dict__"))

    class Cdict(object):
        pass
    x = Cdict()
    vereq(x.__dict__, {})
    x.foo = 1
    vereq(x.foo, 1)
    vereq(x.__dict__, {'foo': 1})

def slots():
    if verbose: print("Testing __slots__...")
    class C0(object):
        __slots__ = []
    x = C0()
    verify(not hasattr(x, "__dict__"))
    verify(not hasattr(x, "foo"))

    class C1(object):
        __slots__ = ['a']
    x = C1()
    verify(not hasattr(x, "__dict__"))
    verify(not hasattr(x, "a"))
    x.a = 1
    vereq(x.a, 1)
    x.a = None
    veris(x.a, None)
    del x.a
    verify(not hasattr(x, "a"))

    class C3(object):
        __slots__ = ['a', 'b', 'c']
    x = C3()
    verify(not hasattr(x, "__dict__"))
    verify(not hasattr(x, 'a'))
    verify(not hasattr(x, 'b'))
    verify(not hasattr(x, 'c'))
    x.a = 1
    x.b = 2
    x.c = 3
    vereq(x.a, 1)
    vereq(x.b, 2)
    vereq(x.c, 3)

    class C4(object):
        """Validate name mangling"""
        __slots__ = ['__a']
        def __init__(self, value):
            self.__a = value
        def get(self):
            return self.__a
    x = C4(5)
    verify(not hasattr(x, '__dict__'))
    verify(not hasattr(x, '__a'))
    vereq(x.get(), 5)
    try:
        x.__a = 6
    except AttributeError:
        pass
    else:
        raise TestFailed, "Double underscored names not mangled"

    # Make sure slot names are proper identifiers
    try:
        class C(object):
            __slots__ = [None]
    except TypeError:
        pass
    else:
        raise TestFailed, "[None] slots not caught"
    try:
        class C(object):
            __slots__ = ["foo bar"]
    except TypeError:
        pass
    else:
        raise TestFailed, "['foo bar'] slots not caught"
    try:
        class C(object):
            __slots__ = ["foo\0bar"]
    except TypeError:
        pass
    else:
        raise TestFailed, "['foo\\0bar'] slots not caught"
    try:
        class C(object):
            __slots__ = ["foo\u1234bar"]
    except TypeError:
        pass
    else:
        raise TestFailed, "['foo\\u1234bar'] slots not caught"
    try:
        class C(object):
            __slots__ = ["1"]
    except TypeError:
        pass
    else:
        raise TestFailed, "['1'] slots not caught"
    try:
        class C(object):
            __slots__ = [""]
    except TypeError:
        pass
    else:
        raise TestFailed, "[''] slots not caught"
    class C(object):
        __slots__ = ["a", "a_b", "_a", "A0123456789Z"]
    # XXX(nnorwitz): was there supposed to be something tested
    # from the class above?

    # Test a single string is not expanded as a sequence.
    class C(object):
        __slots__ = "abc"
    c = C()
    c.abc = 5
    vereq(c.abc, 5)

    # Test unicode slot names
    # Test a single unicode string is not expanded as a sequence.
    class C(object):
        __slots__ = "abc"
    c = C()
    c.abc = 5
    vereq(c.abc, 5)

    # _unicode_to_string used to modify slots in certain circumstances
    slots = ("foo", "bar")
    class C(object):
        __slots__ = slots
    x = C()
    x.foo = 5
    vereq(x.foo, 5)
    veris(type(slots[0]), str)
    # this used to leak references
    try:
        class C(object):
            __slots__ = [chr(128)]
    except (TypeError, UnicodeEncodeError):
        pass
    else:
        raise TestFailed, "[unichr(128)] slots not caught"

    # Test leaks
    class Counted(object):
        counter = 0    # counts the number of instances alive
        def __init__(self):
            Counted.counter += 1
        def __del__(self):
            Counted.counter -= 1
    class C(object):
        __slots__ = ['a', 'b', 'c']
    x = C()
    x.a = Counted()
    x.b = Counted()
    x.c = Counted()
    vereq(Counted.counter, 3)
    del x
    vereq(Counted.counter, 0)
    class D(C):
        pass
    x = D()
    x.a = Counted()
    x.z = Counted()
    vereq(Counted.counter, 2)
    del x
    vereq(Counted.counter, 0)
    class E(D):
        __slots__ = ['e']
    x = E()
    x.a = Counted()
    x.z = Counted()
    x.e = Counted()
    vereq(Counted.counter, 3)
    del x
    vereq(Counted.counter, 0)

    # Test cyclical leaks [SF bug 519621]
    class F(object):
        __slots__ = ['a', 'b']
    log = []
    s = F()
    s.a = [Counted(), s]
    vereq(Counted.counter, 1)
    s = None
    import gc
    gc.collect()
    vereq(Counted.counter, 0)

    # Test lookup leaks [SF bug 572567]
    import sys,gc
    class G(object):
        def __cmp__(self, other):
            return 0
    g = G()
    orig_objects = len(gc.get_objects())
    for i in range(10):
        g==g
    new_objects = len(gc.get_objects())
    vereq(orig_objects, new_objects)
    class H(object):
        __slots__ = ['a', 'b']
        def __init__(self):
            self.a = 1
            self.b = 2
        def __del__(self):
            assert self.a == 1
            assert self.b == 2

    save_stderr = sys.stderr
    sys.stderr = sys.stdout
    h = H()
    try:
        del h
    finally:
        sys.stderr = save_stderr

def slotspecials():
    if verbose: print("Testing __dict__ and __weakref__ in __slots__...")

    class D(object):
        __slots__ = ["__dict__"]
    a = D()
    verify(hasattr(a, "__dict__"))
    verify(not hasattr(a, "__weakref__"))
    a.foo = 42
    vereq(a.__dict__, {"foo": 42})

    class W(object):
        __slots__ = ["__weakref__"]
    a = W()
    verify(hasattr(a, "__weakref__"))
    verify(not hasattr(a, "__dict__"))
    try:
        a.foo = 42
    except AttributeError:
        pass
    else:
        raise TestFailed, "shouldn't be allowed to set a.foo"

    class C1(W, D):
        __slots__ = []
    a = C1()
    verify(hasattr(a, "__dict__"))
    verify(hasattr(a, "__weakref__"))
    a.foo = 42
    vereq(a.__dict__, {"foo": 42})

    class C2(D, W):
        __slots__ = []
    a = C2()
    verify(hasattr(a, "__dict__"))
    verify(hasattr(a, "__weakref__"))
    a.foo = 42
    vereq(a.__dict__, {"foo": 42})

# MRO order disagreement
#
#    class C3(C1, C2):
#        __slots__ = []
#
#    class C4(C2, C1):
#        __slots__ = []

def dynamics():
    if verbose: print("Testing class attribute propagation...")
    class D(object):
        pass
    class E(D):
        pass
    class F(D):
        pass
    D.foo = 1
    vereq(D.foo, 1)
    # Test that dynamic attributes are inherited
    vereq(E.foo, 1)
    vereq(F.foo, 1)
    # Test dynamic instances
    class C(object):
        pass
    a = C()
    verify(not hasattr(a, "foobar"))
    C.foobar = 2
    vereq(a.foobar, 2)
    C.method = lambda self: 42
    vereq(a.method(), 42)
    C.__repr__ = lambda self: "C()"
    vereq(repr(a), "C()")
    C.__int__ = lambda self: 100
    vereq(int(a), 100)
    vereq(a.foobar, 2)
    verify(not hasattr(a, "spam"))
    def mygetattr(self, name):
        if name == "spam":
            return "spam"
        raise AttributeError
    C.__getattr__ = mygetattr
    vereq(a.spam, "spam")
    a.new = 12
    vereq(a.new, 12)
    def mysetattr(self, name, value):
        if name == "spam":
            raise AttributeError
        return object.__setattr__(self, name, value)
    C.__setattr__ = mysetattr
    try:
        a.spam = "not spam"
    except AttributeError:
        pass
    else:
        verify(0, "expected AttributeError")
    vereq(a.spam, "spam")
    class D(C):
        pass
    d = D()
    d.foo = 1
    vereq(d.foo, 1)

    # Test handling of int*seq and seq*int
    class I(int):
        pass
    vereq("a"*I(2), "aa")
    vereq(I(2)*"a", "aa")
    vereq(2*I(3), 6)
    vereq(I(3)*2, 6)
    vereq(I(3)*I(2), 6)

    # Test handling of long*seq and seq*long
    class L(int):
        pass
    vereq("a"*L(2), "aa")
    vereq(L(2)*"a", "aa")
    vereq(2*L(3), 6)
    vereq(L(3)*2, 6)
    vereq(L(3)*L(2), 6)

    # Test comparison of classes with dynamic metaclasses
    class dynamicmetaclass(type):
        pass
    class someclass(metaclass=dynamicmetaclass):
        pass
    verify(someclass != object)

def errors():
    if verbose: print("Testing errors...")

    try:
        class C(list, dict):
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
        class C(type(len)):
            pass
    except TypeError:
        pass
    else:
        verify(0, "inheritance from CFunction should be illegal")

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

    class M1(type):
        pass
    class M2(type):
        pass
    class A1(object, metaclass=M1):
        pass
    class A2(object, metaclass=M2):
        pass
    try:
        class B(A1, A2):
            pass
    except TypeError:
        pass
    else:
        verify(0, "finding the most derived metaclass should have failed")

def classmethods():
    if verbose: print("Testing class methods...")
    class C(object):
        def foo(*a): return a
        goo = classmethod(foo)
    c = C()
    vereq(C.goo(1), (C, 1))
    vereq(c.goo(1), (C, 1))
    vereq(c.foo(1), (c, 1))
    class D(C):
        pass
    d = D()
    vereq(D.goo(1), (D, 1))
    vereq(d.goo(1), (D, 1))
    vereq(d.foo(1), (d, 1))
    vereq(D.foo(d, 1), (d, 1))
    # Test for a specific crash (SF bug 528132)
    def f(cls, arg): return (cls, arg)
    ff = classmethod(f)
    vereq(ff.__get__(0, int)(42), (int, 42))
    vereq(ff.__get__(0)(42), (int, 42))

    # Test super() with classmethods (SF bug 535444)
    veris(C.goo.im_self, C)
    veris(D.goo.im_self, D)
    veris(super(D,D).goo.im_self, D)
    veris(super(D,d).goo.im_self, D)
    vereq(super(D,D).goo(), (D,))
    vereq(super(D,d).goo(), (D,))

    # Verify that argument is checked for callability (SF bug 753451)
    try:
        classmethod(1).__get__(1)
    except TypeError:
        pass
    else:
        raise TestFailed, "classmethod should check for callability"

    # Verify that classmethod() doesn't allow keyword args
    try:
        classmethod(f, kw=1)
    except TypeError:
        pass
    else:
        raise TestFailed, "classmethod shouldn't accept keyword args"

def classmethods_in_c():
    if verbose: print("Testing C-based class methods...")
    import xxsubtype as spam
    a = (1, 2, 3)
    d = {'abc': 123}
    x, a1, d1 = spam.spamlist.classmeth(*a, **d)
    veris(x, spam.spamlist)
    vereq(a, a1)
    vereq(d, d1)
    x, a1, d1 = spam.spamlist().classmeth(*a, **d)
    veris(x, spam.spamlist)
    vereq(a, a1)
    vereq(d, d1)

def staticmethods():
    if verbose: print("Testing static methods...")
    class C(object):
        def foo(*a): return a
        goo = staticmethod(foo)
    c = C()
    vereq(C.goo(1), (1,))
    vereq(c.goo(1), (1,))
    vereq(c.foo(1), (c, 1,))
    class D(C):
        pass
    d = D()
    vereq(D.goo(1), (1,))
    vereq(d.goo(1), (1,))
    vereq(d.foo(1), (d, 1))
    vereq(D.foo(d, 1), (d, 1))

def staticmethods_in_c():
    if verbose: print("Testing C-based static methods...")
    import xxsubtype as spam
    a = (1, 2, 3)
    d = {"abc": 123}
    x, a1, d1 = spam.spamlist.staticmeth(*a, **d)
    veris(x, None)
    vereq(a, a1)
    vereq(d, d1)
    x, a1, d2 = spam.spamlist().staticmeth(*a, **d)
    veris(x, None)
    vereq(a, a1)
    vereq(d, d1)

def classic():
    if verbose: print("Testing classic classes...")
    class C:
        def foo(*a): return a
        goo = classmethod(foo)
    c = C()
    vereq(C.goo(1), (C, 1))
    vereq(c.goo(1), (C, 1))
    vereq(c.foo(1), (c, 1))
    class D(C):
        pass
    d = D()
    vereq(D.goo(1), (D, 1))
    vereq(d.goo(1), (D, 1))
    vereq(d.foo(1), (d, 1))
    vereq(D.foo(d, 1), (d, 1))
    class E: # *not* subclassing from C
        foo = C.foo
    vereq(E().foo, C.foo) # i.e., unbound
    verify(repr(C.foo.__get__(C())).startswith("<bound method "))

def compattr():
    if verbose: print("Testing computed attributes...")
    class C(object):
        class computed_attribute(object):
            def __init__(self, get, set=None, delete=None):
                self.__get = get
                self.__set = set
                self.__delete = delete
            def __get__(self, obj, type=None):
                return self.__get(obj)
            def __set__(self, obj, value):
                return self.__set(obj, value)
            def __delete__(self, obj):
                return self.__delete(obj)
        def __init__(self):
            self.__x = 0
        def __get_x(self):
            x = self.__x
            self.__x = x+1
            return x
        def __set_x(self, x):
            self.__x = x
        def __delete_x(self):
            del self.__x
        x = computed_attribute(__get_x, __set_x, __delete_x)
    a = C()
    vereq(a.x, 0)
    vereq(a.x, 1)
    a.x = 10
    vereq(a.x, 10)
    vereq(a.x, 11)
    del a.x
    vereq(hasattr(a, 'x'), 0)

def newslot():
    if verbose: print("Testing __new__ slot override...")
    class C(list):
        def __new__(cls):
            self = list.__new__(cls)
            self.foo = 1
            return self
        def __init__(self):
            self.foo = self.foo + 2
    a = C()
    vereq(a.foo, 3)
    verify(a.__class__ is C)
    class D(C):
        pass
    b = D()
    vereq(b.foo, 3)
    verify(b.__class__ is D)

def altmro():
    if verbose: print("Testing mro() and overriding it...")
    class A(object):
        def f(self): return "A"
    class B(A):
        pass
    class C(A):
        def f(self): return "C"
    class D(B, C):
        pass
    vereq(D.mro(), [D, B, C, A, object])
    vereq(D.__mro__, (D, B, C, A, object))
    vereq(D().f(), "C")

    class PerverseMetaType(type):
        def mro(cls):
            L = type.mro(cls)
            L.reverse()
            return L
    class X(D,B,C,A, metaclass=PerverseMetaType):
        pass
    vereq(X.__mro__, (object, A, C, B, D, X))
    vereq(X().f(), "A")

    try:
        class _metaclass(type):
            def mro(self):
                return [self, dict, object]
        class X(object, metaclass=_metaclass):
            pass
    except TypeError:
        pass
    else:
        raise TestFailed, "devious mro() return not caught"

    try:
        class _metaclass(type):
            def mro(self):
                return [1]
        class X(object, metaclass=_metaclass):
            pass
    except TypeError:
        pass
    else:
        raise TestFailed, "non-class mro() return not caught"

    try:
        class _metaclass(type):
            def mro(self):
                return 1
        class X(object, metaclass=_metaclass):
            pass
    except TypeError:
        pass
    else:
        raise TestFailed, "non-sequence mro() return not caught"


def overloading():
    if verbose: print("Testing operator overloading...")

    class B(object):
        "Intermediate class because object doesn't have a __setattr__"

    class C(B):

        def __getattr__(self, name):
            if name == "foo":
                return ("getattr", name)
            else:
                raise AttributeError
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
    vereq(a.foo, ("getattr", "foo"))
    a.foo = 12
    vereq(a.setattr, ("foo", 12))
    del a.foo
    vereq(a.delattr, "foo")

    vereq(a[12], ("getitem", 12))
    a[12] = 21
    vereq(a.setitem, (12, 21))
    del a[12]
    vereq(a.delitem, 12)

    vereq(a[0:10], ("getslice", 0, 10))
    a[0:10] = "foo"
    vereq(a.setslice, (0, 10, "foo"))
    del a[0:10]
    vereq(a.delslice, (0, 10))

def methods():
    if verbose: print("Testing methods...")
    class C(object):
        def __init__(self, x):
            self.x = x
        def foo(self):
            return self.x
    c1 = C(1)
    vereq(c1.foo(), 1)
    class D(C):
        boo = C.foo
        goo = c1.foo
    d2 = D(2)
    vereq(d2.foo(), 2)
    vereq(d2.boo(), 2)
    vereq(d2.goo(), 1)
    class E(object):
        foo = C.foo
    vereq(E().foo, C.foo) # i.e., unbound
    verify(repr(C.foo.__get__(C(1))).startswith("<bound method "))

def specials():
    # Test operators like __hash__ for which a built-in default exists
    if verbose: print("Testing special operators...")
    # Test the default behavior for static classes
    class C(object):
        def __getitem__(self, i):
            if 0 <= i < 10: return i
            raise IndexError
    c1 = C()
    c2 = C()
    verify(not not c1)
    verify(id(c1) != id(c2))
    hash(c1)
    hash(c2)
    ##vereq(cmp(c1, c2), cmp(id(c1), id(c2)))
    vereq(c1, c1)
    verify(c1 != c2)
    verify(not c1 != c1)
    verify(not c1 == c2)
    # Note that the module name appears in str/repr, and that varies
    # depending on whether this test is run standalone or from a framework.
    verify(str(c1).find('C object at ') >= 0)
    vereq(str(c1), repr(c1))
    verify(-1 not in c1)
    for i in range(10):
        verify(i in c1)
    verify(10 not in c1)
    # Test the default behavior for dynamic classes
    class D(object):
        def __getitem__(self, i):
            if 0 <= i < 10: return i
            raise IndexError
    d1 = D()
    d2 = D()
    verify(not not d1)
    verify(id(d1) != id(d2))
    hash(d1)
    hash(d2)
    ##vereq(cmp(d1, d2), cmp(id(d1), id(d2)))
    vereq(d1, d1)
    verify(d1 != d2)
    verify(not d1 != d1)
    verify(not d1 == d2)
    # Note that the module name appears in str/repr, and that varies
    # depending on whether this test is run standalone or from a framework.
    verify(str(d1).find('D object at ') >= 0)
    vereq(str(d1), repr(d1))
    verify(-1 not in d1)
    for i in range(10):
        verify(i in d1)
    verify(10 not in d1)
    # Test overridden behavior for static classes
    class Proxy(object):
        def __init__(self, x):
            self.x = x
        def __bool__(self):
            return not not self.x
        def __hash__(self):
            return hash(self.x)
        def __eq__(self, other):
            return self.x == other
        def __ne__(self, other):
            return self.x != other
        def __cmp__(self, other):
            return cmp(self.x, other.x)
        def __str__(self):
            return "Proxy:%s" % self.x
        def __repr__(self):
            return "Proxy(%r)" % self.x
        def __contains__(self, value):
            return value in self.x
    p0 = Proxy(0)
    p1 = Proxy(1)
    p_1 = Proxy(-1)
    verify(not p0)
    verify(not not p1)
    vereq(hash(p0), hash(0))
    vereq(p0, p0)
    verify(p0 != p1)
    verify(not p0 != p0)
    vereq(not p0, p1)
    vereq(cmp(p0, p1), -1)
    vereq(cmp(p0, p0), 0)
    vereq(cmp(p0, p_1), 1)
    vereq(str(p0), "Proxy:0")
    vereq(repr(p0), "Proxy(0)")
    p10 = Proxy(range(10))
    verify(-1 not in p10)
    for i in range(10):
        verify(i in p10)
    verify(10 not in p10)
    # Test overridden behavior for dynamic classes
    class DProxy(object):
        def __init__(self, x):
            self.x = x
        def __bool__(self):
            return not not self.x
        def __hash__(self):
            return hash(self.x)
        def __eq__(self, other):
            return self.x == other
        def __ne__(self, other):
            return self.x != other
        def __cmp__(self, other):
            return cmp(self.x, other.x)
        def __str__(self):
            return "DProxy:%s" % self.x
        def __repr__(self):
            return "DProxy(%r)" % self.x
        def __contains__(self, value):
            return value in self.x
    p0 = DProxy(0)
    p1 = DProxy(1)
    p_1 = DProxy(-1)
    verify(not p0)
    verify(not not p1)
    vereq(hash(p0), hash(0))
    vereq(p0, p0)
    verify(p0 != p1)
    verify(not p0 != p0)
    vereq(not p0, p1)
    vereq(cmp(p0, p1), -1)
    vereq(cmp(p0, p0), 0)
    vereq(cmp(p0, p_1), 1)
    vereq(str(p0), "DProxy:0")
    vereq(repr(p0), "DProxy(0)")
    p10 = DProxy(range(10))
    verify(-1 not in p10)
    for i in range(10):
        verify(i in p10)
    verify(10 not in p10)
##     # Safety test for __cmp__
##     def unsafecmp(a, b):
##         try:
##             a.__class__.__cmp__(a, b)
##         except TypeError:
##             pass
##         else:
##             raise TestFailed, "shouldn't allow %s.__cmp__(%r, %r)" % (
##                 a.__class__, a, b)
##     unsafecmp(u"123", "123")
##     unsafecmp("123", u"123")
##     unsafecmp(1, 1.0)
##     unsafecmp(1.0, 1)
##     unsafecmp(1, 1L)
##     unsafecmp(1L, 1)

##     class Letter(str):
##         def __new__(cls, letter):
##             if letter == 'EPS':
##                 return str.__new__(cls)
##             return str.__new__(cls, letter)
##         def __str__(self):
##             if not self:
##                 return 'EPS'
##             return self

##     # sys.stdout needs to be the original to trigger the recursion bug
##     import sys
##     test_stdout = sys.stdout
##     sys.stdout = get_original_stdout()
##     try:
##         # nothing should actually be printed, this should raise an exception
##         print(Letter('w'))
##     except RuntimeError:
##         pass
##     else:
##         raise TestFailed, "expected a RuntimeError for print recursion"
##     sys.stdout = test_stdout

def weakrefs():
    if verbose: print("Testing weak references...")
    import weakref
    class C(object):
        pass
    c = C()
    r = weakref.ref(c)
    verify(r() is c)
    del c
    verify(r() is None)
    del r
    class NoWeak(object):
        __slots__ = ['foo']
    no = NoWeak()
    try:
        weakref.ref(no)
    except TypeError as msg:
        verify(str(msg).find("weak reference") >= 0)
    else:
        verify(0, "weakref.ref(no) should be illegal")
    class Weak(object):
        __slots__ = ['foo', '__weakref__']
    yes = Weak()
    r = weakref.ref(yes)
    verify(r() is yes)
    del yes
    verify(r() is None)
    del r

def properties():
    if verbose: print("Testing property...")
    class C(object):
        def getx(self):
            return self.__x
        def setx(self, value):
            self.__x = value
        def delx(self):
            del self.__x
        x = property(getx, setx, delx, doc="I'm the x property.")
    a = C()
    verify(not hasattr(a, "x"))
    a.x = 42
    vereq(a._C__x, 42)
    vereq(a.x, 42)
    del a.x
    verify(not hasattr(a, "x"))
    verify(not hasattr(a, "_C__x"))
    C.x.__set__(a, 100)
    vereq(C.x.__get__(a), 100)
    C.x.__delete__(a)
    verify(not hasattr(a, "x"))

    raw = C.__dict__['x']
    verify(isinstance(raw, property))

    attrs = dir(raw)
    verify("__doc__" in attrs)
    verify("fget" in attrs)
    verify("fset" in attrs)
    verify("fdel" in attrs)

    vereq(raw.__doc__, "I'm the x property.")
    verify(raw.fget is C.__dict__['getx'])
    verify(raw.fset is C.__dict__['setx'])
    verify(raw.fdel is C.__dict__['delx'])

    for attr in "__doc__", "fget", "fset", "fdel":
        try:
            setattr(raw, attr, 42)
        except AttributeError as msg:
            if str(msg).find('readonly') < 0:
                raise TestFailed("when setting readonly attr %r on a "
                                 "property, got unexpected AttributeError "
                                 "msg %r" % (attr, str(msg)))
        else:
            raise TestFailed("expected AttributeError from trying to set "
                             "readonly %r attr on a property" % attr)

    class D(object):
        __getitem__ = property(lambda s: 1/0)

    d = D()
    try:
        for i in d:
            str(i)
    except ZeroDivisionError:
        pass
    else:
        raise TestFailed, "expected ZeroDivisionError from bad property"

    class E(object):
        def getter(self):
            "getter method"
            return 0
        def setter(self, value):
            "setter method"
            pass
        prop = property(getter)
        vereq(prop.__doc__, "getter method")
        prop2 = property(fset=setter)
        vereq(prop2.__doc__, None)

    # this segfaulted in 2.5b2
    try:
        import _testcapi
    except ImportError:
        pass
    else:
        class X(object):
            p = property(_testcapi.test_with_docstring)


def supers():
    if verbose: print("Testing super...")

    class A(object):
        def meth(self, a):
            return "A(%r)" % a

    vereq(A().meth(1), "A(1)")

    class B(A):
        def __init__(self):
            self.__super = super(B, self)
        def meth(self, a):
            return "B(%r)" % a + self.__super.meth(a)

    vereq(B().meth(2), "B(2)A(2)")

    class C(A):
        def meth(self, a):
            return "C(%r)" % a + self.__super.meth(a)
    C._C__super = super(C)

    vereq(C().meth(3), "C(3)A(3)")

    class D(C, B):
        def meth(self, a):
            return "D(%r)" % a + super(D, self).meth(a)

    vereq(D().meth(4), "D(4)C(4)B(4)A(4)")

    # Test for subclassing super

    class mysuper(super):
        def __init__(self, *args):
            return super(mysuper, self).__init__(*args)

    class E(D):
        def meth(self, a):
            return "E(%r)" % a + mysuper(E, self).meth(a)

    vereq(E().meth(5), "E(5)D(5)C(5)B(5)A(5)")

    class F(E):
        def meth(self, a):
            s = self.__super # == mysuper(F, self)
            return "F(%r)[%s]" % (a, s.__class__.__name__) + s.meth(a)
    F._F__super = mysuper(F)

    vereq(F().meth(6), "F(6)[mysuper]E(6)D(6)C(6)B(6)A(6)")

    # Make sure certain errors are raised

    try:
        super(D, 42)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't allow super(D, 42)"

    try:
        super(D, C())
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't allow super(D, C())"

    try:
        super(D).__get__(12)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't allow super(D).__get__(12)"

    try:
        super(D).__get__(C())
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't allow super(D).__get__(C())"

    # Make sure data descriptors can be overridden and accessed via super
    # (new feature in Python 2.3)

    class DDbase(object):
        def getx(self): return 42
        x = property(getx)

    class DDsub(DDbase):
        def getx(self): return "hello"
        x = property(getx)

    dd = DDsub()
    vereq(dd.x, "hello")
    vereq(super(DDsub, dd).x, 42)

    # Ensure that super() lookup of descriptor from classmethod
    # works (SF ID# 743627)

    class Base(object):
        aProp = property(lambda self: "foo")

    class Sub(Base):
        @classmethod
        def test(klass):
            return super(Sub,klass).aProp

    veris(Sub.test(), Base.aProp)

    # Verify that super() doesn't allow keyword args
    try:
        super(Base, kw=1)
    except TypeError:
        pass
    else:
        raise TestFailed, "super shouldn't accept keyword args"

def inherits():
    if verbose: print("Testing inheritance from basic types...")

    class hexint(int):
        def __repr__(self):
            return hex(self)
        def __add__(self, other):
            return hexint(int.__add__(self, other))
        # (Note that overriding __radd__ doesn't work,
        # because the int type gets first dibs.)
    vereq(repr(hexint(7) + 9), "0x10")
    vereq(repr(hexint(1000) + 7), "0x3ef")
    a = hexint(12345)
    vereq(a, 12345)
    vereq(int(a), 12345)
    verify(int(a).__class__ is int)
    vereq(hash(a), hash(12345))
    verify((+a).__class__ is int)
    verify((a >> 0).__class__ is int)
    verify((a << 0).__class__ is int)
    verify((hexint(0) << 12).__class__ is int)
    verify((hexint(0) >> 12).__class__ is int)

    class octlong(int):
        __slots__ = []
        def __str__(self):
            return oct(self)
        def __add__(self, other):
            return self.__class__(super(octlong, self).__add__(other))
        __radd__ = __add__
    vereq(str(octlong(3) + 5), "0o10")
    # (Note that overriding __radd__ here only seems to work
    # because the example uses a short int left argument.)
    vereq(str(5 + octlong(3000)), "0o5675")
    a = octlong(12345)
    vereq(a, 12345)
    vereq(int(a), 12345)
    vereq(hash(a), hash(12345))
    verify(int(a).__class__ is int)
    verify((+a).__class__ is int)
    verify((-a).__class__ is int)
    verify((-octlong(0)).__class__ is int)
    verify((a >> 0).__class__ is int)
    verify((a << 0).__class__ is int)
    verify((a - 0).__class__ is int)
    verify((a * 1).__class__ is int)
    verify((a ** 1).__class__ is int)
    verify((a // 1).__class__ is int)
    verify((1 * a).__class__ is int)
    verify((a | 0).__class__ is int)
    verify((a ^ 0).__class__ is int)
    verify((a & -1).__class__ is int)
    verify((octlong(0) << 12).__class__ is int)
    verify((octlong(0) >> 12).__class__ is int)
    verify(abs(octlong(0)).__class__ is int)

    # Because octlong overrides __add__, we can't check the absence of +0
    # optimizations using octlong.
    class longclone(int):
        pass
    a = longclone(1)
    verify((a + 0).__class__ is int)
    verify((0 + a).__class__ is int)

    # Check that negative clones don't segfault
    a = longclone(-1)
    vereq(a.__dict__, {})
    vereq(int(a), -1)  # verify PyNumber_Long() copies the sign bit

    class precfloat(float):
        __slots__ = ['prec']
        def __init__(self, value=0.0, prec=12):
            self.prec = int(prec)
        def __repr__(self):
            return "%.*g" % (self.prec, self)
    vereq(repr(precfloat(1.1)), "1.1")
    a = precfloat(12345)
    vereq(a, 12345.0)
    vereq(float(a), 12345.0)
    verify(float(a).__class__ is float)
    vereq(hash(a), hash(12345.0))
    verify((+a).__class__ is float)

    class madcomplex(complex):
        def __repr__(self):
            return "%.17gj%+.17g" % (self.imag, self.real)
    a = madcomplex(-3, 4)
    vereq(repr(a), "4j-3")
    base = complex(-3, 4)
    veris(base.__class__, complex)
    vereq(a, base)
    vereq(complex(a), base)
    veris(complex(a).__class__, complex)
    a = madcomplex(a)  # just trying another form of the constructor
    vereq(repr(a), "4j-3")
    vereq(a, base)
    vereq(complex(a), base)
    veris(complex(a).__class__, complex)
    vereq(hash(a), hash(base))
    veris((+a).__class__, complex)
    veris((a + 0).__class__, complex)
    vereq(a + 0, base)
    veris((a - 0).__class__, complex)
    vereq(a - 0, base)
    veris((a * 1).__class__, complex)
    vereq(a * 1, base)
    veris((a / 1).__class__, complex)
    vereq(a / 1, base)

    class madtuple(tuple):
        _rev = None
        def rev(self):
            if self._rev is not None:
                return self._rev
            L = list(self)
            L.reverse()
            self._rev = self.__class__(L)
            return self._rev
    a = madtuple((1,2,3,4,5,6,7,8,9,0))
    vereq(a, (1,2,3,4,5,6,7,8,9,0))
    vereq(a.rev(), madtuple((0,9,8,7,6,5,4,3,2,1)))
    vereq(a.rev().rev(), madtuple((1,2,3,4,5,6,7,8,9,0)))
    for i in range(512):
        t = madtuple(range(i))
        u = t.rev()
        v = u.rev()
        vereq(v, t)
    a = madtuple((1,2,3,4,5))
    vereq(tuple(a), (1,2,3,4,5))
    verify(tuple(a).__class__ is tuple)
    vereq(hash(a), hash((1,2,3,4,5)))
    verify(a[:].__class__ is tuple)
    verify((a * 1).__class__ is tuple)
    verify((a * 0).__class__ is tuple)
    verify((a + ()).__class__ is tuple)
    a = madtuple(())
    vereq(tuple(a), ())
    verify(tuple(a).__class__ is tuple)
    verify((a + a).__class__ is tuple)
    verify((a * 0).__class__ is tuple)
    verify((a * 1).__class__ is tuple)
    verify((a * 2).__class__ is tuple)
    verify(a[:].__class__ is tuple)

    class madstring(str):
        _rev = None
        def rev(self):
            if self._rev is not None:
                return self._rev
            L = list(self)
            L.reverse()
            self._rev = self.__class__("".join(L))
            return self._rev
    s = madstring("abcdefghijklmnopqrstuvwxyz")
    vereq(s, "abcdefghijklmnopqrstuvwxyz")
    vereq(s.rev(), madstring("zyxwvutsrqponmlkjihgfedcba"))
    vereq(s.rev().rev(), madstring("abcdefghijklmnopqrstuvwxyz"))
    for i in range(256):
        s = madstring("".join(map(chr, range(i))))
        t = s.rev()
        u = t.rev()
        vereq(u, s)
    s = madstring("12345")
    vereq(str(s), "12345")
    verify(str(s).__class__ is str)

    base = "\x00" * 5
    s = madstring(base)
    vereq(s, base)
    vereq(str(s), base)
    verify(str(s).__class__ is str)
    vereq(hash(s), hash(base))
    vereq({s: 1}[base], 1)
    vereq({base: 1}[s], 1)
    verify((s + "").__class__ is str)
    vereq(s + "", base)
    verify(("" + s).__class__ is str)
    vereq("" + s, base)
    verify((s * 0).__class__ is str)
    vereq(s * 0, "")
    verify((s * 1).__class__ is str)
    vereq(s * 1, base)
    verify((s * 2).__class__ is str)
    vereq(s * 2, base + base)
    verify(s[:].__class__ is str)
    vereq(s[:], base)
    verify(s[0:0].__class__ is str)
    vereq(s[0:0], "")
    verify(s.strip().__class__ is str)
    vereq(s.strip(), base)
    verify(s.lstrip().__class__ is str)
    vereq(s.lstrip(), base)
    verify(s.rstrip().__class__ is str)
    vereq(s.rstrip(), base)
    identitytab = {}
    verify(s.translate(identitytab).__class__ is str)
    vereq(s.translate(identitytab), base)
    verify(s.replace("x", "x").__class__ is str)
    vereq(s.replace("x", "x"), base)
    verify(s.ljust(len(s)).__class__ is str)
    vereq(s.ljust(len(s)), base)
    verify(s.rjust(len(s)).__class__ is str)
    vereq(s.rjust(len(s)), base)
    verify(s.center(len(s)).__class__ is str)
    vereq(s.center(len(s)), base)
    verify(s.lower().__class__ is str)
    vereq(s.lower(), base)

    class madunicode(str):
        _rev = None
        def rev(self):
            if self._rev is not None:
                return self._rev
            L = list(self)
            L.reverse()
            self._rev = self.__class__("".join(L))
            return self._rev
    u = madunicode("ABCDEF")
    vereq(u, "ABCDEF")
    vereq(u.rev(), madunicode("FEDCBA"))
    vereq(u.rev().rev(), madunicode("ABCDEF"))
    base = "12345"
    u = madunicode(base)
    vereq(str(u), base)
    verify(str(u).__class__ is str)
    vereq(hash(u), hash(base))
    vereq({u: 1}[base], 1)
    vereq({base: 1}[u], 1)
    verify(u.strip().__class__ is str)
    vereq(u.strip(), base)
    verify(u.lstrip().__class__ is str)
    vereq(u.lstrip(), base)
    verify(u.rstrip().__class__ is str)
    vereq(u.rstrip(), base)
    verify(u.replace("x", "x").__class__ is str)
    vereq(u.replace("x", "x"), base)
    verify(u.replace("xy", "xy").__class__ is str)
    vereq(u.replace("xy", "xy"), base)
    verify(u.center(len(u)).__class__ is str)
    vereq(u.center(len(u)), base)
    verify(u.ljust(len(u)).__class__ is str)
    vereq(u.ljust(len(u)), base)
    verify(u.rjust(len(u)).__class__ is str)
    vereq(u.rjust(len(u)), base)
    verify(u.lower().__class__ is str)
    vereq(u.lower(), base)
    verify(u.upper().__class__ is str)
    vereq(u.upper(), base)
    verify(u.capitalize().__class__ is str)
    vereq(u.capitalize(), base)
    verify(u.title().__class__ is str)
    vereq(u.title(), base)
    verify((u + "").__class__ is str)
    vereq(u + "", base)
    verify(("" + u).__class__ is str)
    vereq("" + u, base)
    verify((u * 0).__class__ is str)
    vereq(u * 0, "")
    verify((u * 1).__class__ is str)
    vereq(u * 1, base)
    verify((u * 2).__class__ is str)
    vereq(u * 2, base + base)
    verify(u[:].__class__ is str)
    vereq(u[:], base)
    verify(u[0:0].__class__ is str)
    vereq(u[0:0], "")

    class sublist(list):
        pass
    a = sublist(range(5))
    vereq(a, list(range(5)))
    a.append("hello")
    vereq(a, list(range(5)) + ["hello"])
    a[5] = 5
    vereq(a, list(range(6)))
    a.extend(range(6, 20))
    vereq(a, list(range(20)))
    a[-5:] = []
    vereq(a, list(range(15)))
    del a[10:15]
    vereq(len(a), 10)
    vereq(a, list(range(10)))
    vereq(list(a), list(range(10)))
    vereq(a[0], 0)
    vereq(a[9], 9)
    vereq(a[-10], 0)
    vereq(a[-1], 9)
    vereq(a[:5], list(range(5)))

##     class CountedInput(file):
##         """Counts lines read by self.readline().

##         self.lineno is the 0-based ordinal of the last line read, up to
##         a maximum of one greater than the number of lines in the file.

##         self.ateof is true if and only if the final "" line has been read,
##         at which point self.lineno stops incrementing, and further calls
##         to readline() continue to return "".
##         """

##         lineno = 0
##         ateof = 0
##         def readline(self):
##             if self.ateof:
##                 return ""
##             s = file.readline(self)
##             # Next line works too.
##             # s = super(CountedInput, self).readline()
##             self.lineno += 1
##             if s == "":
##                 self.ateof = 1
##             return s

##     f = open(name=TESTFN, mode='w')
##     lines = ['a\n', 'b\n', 'c\n']
##     try:
##         f.writelines(lines)
##         f.close()
##         f = CountedInput(TESTFN)
##         for (i, expected) in zip(list(range(1, 5)) + [4], lines + 2 * [""]):
##             got = f.readline()
##             vereq(expected, got)
##             vereq(f.lineno, i)
##             vereq(f.ateof, (i > len(lines)))
##         f.close()
##     finally:
##         try:
##             f.close()
##         except:
##             pass
##         try:
##             import os
##             os.unlink(TESTFN)
##         except:
##             pass

def keywords():
    if verbose:
        print("Testing keyword args to basic type constructors ...")
    vereq(int(x=1), 1)
    vereq(float(x=2), 2.0)
    vereq(int(x=3), 3)
    vereq(complex(imag=42, real=666), complex(666, 42))
    vereq(str(object=500), '500')
    vereq(str(object=b'abc', errors='strict'), 'abc')
    vereq(tuple(sequence=range(3)), (0, 1, 2))
    vereq(list(sequence=(0, 1, 2)), list(range(3)))
    # note: as of Python 2.3, dict() no longer has an "items" keyword arg

    for constructor in (int, float, int, complex, str, str, tuple, list):
        try:
            constructor(bogus_keyword_arg=1)
        except TypeError:
            pass
        else:
            raise TestFailed("expected TypeError from bogus keyword "
                             "argument to %r" % constructor)

def str_subclass_as_dict_key():
    if verbose:
        print("Testing a str subclass used as dict key ..")

    class cistr(str):
        """Sublcass of str that computes __eq__ case-insensitively.

        Also computes a hash code of the string in canonical form.
        """

        def __init__(self, value):
            self.canonical = value.lower()
            self.hashcode = hash(self.canonical)

        def __eq__(self, other):
            if not isinstance(other, cistr):
                other = cistr(other)
            return self.canonical == other.canonical

        def __hash__(self):
            return self.hashcode

    vereq(cistr('ABC'), 'abc')
    vereq('aBc', cistr('ABC'))
    vereq(str(cistr('ABC')), 'ABC')

    d = {cistr('one'): 1, cistr('two'): 2, cistr('tHree'): 3}
    vereq(d[cistr('one')], 1)
    vereq(d[cistr('tWo')], 2)
    vereq(d[cistr('THrEE')], 3)
    verify(cistr('ONe') in d)
    vereq(d.get(cistr('thrEE')), 3)

def classic_comparisons():
    if verbose: print("Testing classic comparisons...")
    class classic:
        pass
    for base in (classic, int, object):
        if verbose: print("        (base = %s)" % base)
        class C(base):
            def __init__(self, value):
                self.value = int(value)
            def __eq__(self, other):
                if isinstance(other, C):
                    return self.value == other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value == other
                return NotImplemented
            def __ne__(self, other):
                if isinstance(other, C):
                    return self.value != other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value != other
                return NotImplemented
            def __lt__(self, other):
                if isinstance(other, C):
                    return self.value < other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value < other
                return NotImplemented
            def __le__(self, other):
                if isinstance(other, C):
                    return self.value <= other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value <= other
                return NotImplemented
            def __gt__(self, other):
                if isinstance(other, C):
                    return self.value > other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value > other
                return NotImplemented
            def __ge__(self, other):
                if isinstance(other, C):
                    return self.value >= other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value >= other
                return NotImplemented

        c1 = C(1)
        c2 = C(2)
        c3 = C(3)
        vereq(c1, 1)
        c = {1: c1, 2: c2, 3: c3}
        for x in 1, 2, 3:
            for y in 1, 2, 3:
                ##verify(cmp(c[x], c[y]) == cmp(x, y), "x=%d, y=%d" % (x, y))
                for op in "<", "<=", "==", "!=", ">", ">=":
                    verify(eval("c[x] %s c[y]" % op) == eval("x %s y" % op),
                           "x=%d, y=%d" % (x, y))
                ##verify(cmp(c[x], y) == cmp(x, y), "x=%d, y=%d" % (x, y))
                ##verify(cmp(x, c[y]) == cmp(x, y), "x=%d, y=%d" % (x, y))

def rich_comparisons():
    if verbose:
        print("Testing rich comparisons...")
    class Z(complex):
        pass
    z = Z(1)
    vereq(z, 1+0j)
    vereq(1+0j, z)
    class ZZ(complex):
        def __eq__(self, other):
            try:
                return abs(self - other) <= 1e-6
            except:
                return NotImplemented
    zz = ZZ(1.0000003)
    vereq(zz, 1+0j)
    vereq(1+0j, zz)

    class classic:
        pass
    for base in (classic, int, object, list):
        if verbose: print("        (base = %s)" % base)
        class C(base):
            def __init__(self, value):
                self.value = int(value)
            def __cmp__(self, other):
                raise TestFailed, "shouldn't call __cmp__"
            def __eq__(self, other):
                if isinstance(other, C):
                    return self.value == other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value == other
                return NotImplemented
            def __ne__(self, other):
                if isinstance(other, C):
                    return self.value != other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value != other
                return NotImplemented
            def __lt__(self, other):
                if isinstance(other, C):
                    return self.value < other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value < other
                return NotImplemented
            def __le__(self, other):
                if isinstance(other, C):
                    return self.value <= other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value <= other
                return NotImplemented
            def __gt__(self, other):
                if isinstance(other, C):
                    return self.value > other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value > other
                return NotImplemented
            def __ge__(self, other):
                if isinstance(other, C):
                    return self.value >= other.value
                if isinstance(other, int) or isinstance(other, int):
                    return self.value >= other
                return NotImplemented
        c1 = C(1)
        c2 = C(2)
        c3 = C(3)
        vereq(c1, 1)
        c = {1: c1, 2: c2, 3: c3}
        for x in 1, 2, 3:
            for y in 1, 2, 3:
                for op in "<", "<=", "==", "!=", ">", ">=":
                    verify(eval("c[x] %s c[y]" % op) == eval("x %s y" % op),
                           "x=%d, y=%d" % (x, y))
                    verify(eval("c[x] %s y" % op) == eval("x %s y" % op),
                           "x=%d, y=%d" % (x, y))
                    verify(eval("x %s c[y]" % op) == eval("x %s y" % op),
                           "x=%d, y=%d" % (x, y))

def descrdoc():
    if verbose: print("Testing descriptor doc strings...")
    from _fileio import _FileIO
    def check(descr, what):
        vereq(descr.__doc__, what)
    check(_FileIO.closed, "True if the file is closed") # getset descriptor
    check(complex.real, "the real part of a complex number") # member descriptor

def setclass():
    if verbose: print("Testing __class__ assignment...")
    class C(object): pass
    class D(object): pass
    class E(object): pass
    class F(D, E): pass
    for cls in C, D, E, F:
        for cls2 in C, D, E, F:
            x = cls()
            x.__class__ = cls2
            verify(x.__class__ is cls2)
            x.__class__ = cls
            verify(x.__class__ is cls)
    def cant(x, C):
        try:
            x.__class__ = C
        except TypeError:
            pass
        else:
            raise TestFailed, "shouldn't allow %r.__class__ = %r" % (x, C)
        try:
            delattr(x, "__class__")
        except TypeError:
            pass
        else:
            raise TestFailed, "shouldn't allow del %r.__class__" % x
    cant(C(), list)
    cant(list(), C)
    cant(C(), 1)
    cant(C(), object)
    cant(object(), list)
    cant(list(), object)
    class Int(int): __slots__ = []
    cant(2, Int)
    cant(Int(), int)
    cant(True, int)
    cant(2, bool)
    o = object()
    cant(o, type(1))
    cant(o, type(None))
    del o
    class G(object):
        __slots__ = ["a", "b"]
    class H(object):
        __slots__ = ["b", "a"]
    class I(object):
        __slots__ = ["a", "b"]
    class J(object):
        __slots__ = ["c", "b"]
    class K(object):
        __slots__ = ["a", "b", "d"]
    class L(H):
        __slots__ = ["e"]
    class M(I):
        __slots__ = ["e"]
    class N(J):
        __slots__ = ["__weakref__"]
    class P(J):
        __slots__ = ["__dict__"]
    class Q(J):
        pass
    class R(J):
        __slots__ = ["__dict__", "__weakref__"]

    for cls, cls2 in ((G, H), (G, I), (I, H), (Q, R), (R, Q)):
        x = cls()
        x.a = 1
        x.__class__ = cls2
        verify(x.__class__ is cls2,
               "assigning %r as __class__ for %r silently failed" % (cls2, x))
        vereq(x.a, 1)
        x.__class__ = cls
        verify(x.__class__ is cls,
               "assigning %r as __class__ for %r silently failed" % (cls, x))
        vereq(x.a, 1)
    for cls in G, J, K, L, M, N, P, R, list, Int:
        for cls2 in G, J, K, L, M, N, P, R, list, Int:
            if cls is cls2:
                continue
            cant(cls(), cls2)

def setdict():
    if verbose: print("Testing __dict__ assignment...")
    class C(object): pass
    a = C()
    a.__dict__ = {'b': 1}
    vereq(a.b, 1)
    def cant(x, dict):
        try:
            x.__dict__ = dict
        except (AttributeError, TypeError):
            pass
        else:
            raise TestFailed, "shouldn't allow %r.__dict__ = %r" % (x, dict)
    cant(a, None)
    cant(a, [])
    cant(a, 1)
    del a.__dict__ # Deleting __dict__ is allowed

    class Base(object):
        pass
    def verify_dict_readonly(x):
        """
        x has to be an instance of a class inheriting from Base.
        """
        cant(x, {})
        try:
            del x.__dict__
        except (AttributeError, TypeError):
            pass
        else:
            raise TestFailed, "shouldn't allow del %r.__dict__" % x
        dict_descr = Base.__dict__["__dict__"]
        try:
            dict_descr.__set__(x, {})
        except (AttributeError, TypeError):
            pass
        else:
            raise TestFailed, "dict_descr allowed access to %r's dict" % x

    # Classes don't allow __dict__ assignment and have readonly dicts
    class Meta1(type, Base):
        pass
    class Meta2(Base, type):
        pass
    class D(object):
        __metaclass__ = Meta1
    class E(object):
        __metaclass__ = Meta2
    for cls in C, D, E:
        verify_dict_readonly(cls)
        class_dict = cls.__dict__
        try:
            class_dict["spam"] = "eggs"
        except TypeError:
            pass
        else:
            raise TestFailed, "%r's __dict__ can be modified" % cls

    # Modules also disallow __dict__ assignment
    class Module1(types.ModuleType, Base):
        pass
    class Module2(Base, types.ModuleType):
        pass
    for ModuleType in Module1, Module2:
        mod = ModuleType("spam")
        verify_dict_readonly(mod)
        mod.__dict__["spam"] = "eggs"

    # Exception's __dict__ can be replaced, but not deleted
    class Exception1(Exception, Base):
        pass
    class Exception2(Base, Exception):
        pass
    for ExceptionType in Exception, Exception1, Exception2:
        e = ExceptionType()
        e.__dict__ = {"a": 1}
        vereq(e.a, 1)
        try:
            del e.__dict__
        except (TypeError, AttributeError):
            pass
        else:
            raise TestFaied, "%r's __dict__ can be deleted" % e


def pickles():
    if verbose:
        print("Testing pickling and copying new-style classes and objects...")
    import pickle

    def sorteditems(d):
        return sorted(d.items())

    global C
    class C(object):
        def __init__(self, a, b):
            super(C, self).__init__()
            self.a = a
            self.b = b
        def __repr__(self):
            return "C(%r, %r)" % (self.a, self.b)

    global C1
    class C1(list):
        def __new__(cls, a, b):
            return super(C1, cls).__new__(cls)
        def __getnewargs__(self):
            return (self.a, self.b)
        def __init__(self, a, b):
            self.a = a
            self.b = b
        def __repr__(self):
            return "C1(%r, %r)<%r>" % (self.a, self.b, list(self))

    global C2
    class C2(int):
        def __new__(cls, a, b, val=0):
            return super(C2, cls).__new__(cls, val)
        def __getnewargs__(self):
            return (self.a, self.b, int(self))
        def __init__(self, a, b, val=0):
            self.a = a
            self.b = b
        def __repr__(self):
            return "C2(%r, %r)<%r>" % (self.a, self.b, int(self))

    global C3
    class C3(object):
        def __init__(self, foo):
            self.foo = foo
        def __getstate__(self):
            return self.foo
        def __setstate__(self, foo):
            self.foo = foo

    global C4classic, C4
    class C4classic: # classic
        pass
    class C4(C4classic, object): # mixed inheritance
        pass

    for p in [pickle]:
        for bin in 0, 1:
            if verbose:
                print(p.__name__, ["text", "binary"][bin])

            for cls in C, C1, C2:
                s = p.dumps(cls, bin)
                cls2 = p.loads(s)
                verify(cls2 is cls)

            a = C1(1, 2); a.append(42); a.append(24)
            b = C2("hello", "world", 42)
            s = p.dumps((a, b), bin)
            x, y = p.loads(s)
            vereq(x.__class__, a.__class__)
            vereq(sorteditems(x.__dict__), sorteditems(a.__dict__))
            vereq(y.__class__, b.__class__)
            vereq(sorteditems(y.__dict__), sorteditems(b.__dict__))
            vereq(repr(x), repr(a))
            vereq(repr(y), repr(b))
            if verbose:
                print("a = x =", a)
                print("b = y =", b)
            # Test for __getstate__ and __setstate__ on new style class
            u = C3(42)
            s = p.dumps(u, bin)
            v = p.loads(s)
            veris(u.__class__, v.__class__)
            vereq(u.foo, v.foo)
            # Test for picklability of hybrid class
            u = C4()
            u.foo = 42
            s = p.dumps(u, bin)
            v = p.loads(s)
            veris(u.__class__, v.__class__)
            vereq(u.foo, v.foo)

    # Testing copy.deepcopy()
    if verbose:
        print("deepcopy")
    import copy
    for cls in C, C1, C2:
        cls2 = copy.deepcopy(cls)
        verify(cls2 is cls)

    a = C1(1, 2); a.append(42); a.append(24)
    b = C2("hello", "world", 42)
    x, y = copy.deepcopy((a, b))
    vereq(x.__class__, a.__class__)
    vereq(sorteditems(x.__dict__), sorteditems(a.__dict__))
    vereq(y.__class__, b.__class__)
    vereq(sorteditems(y.__dict__), sorteditems(b.__dict__))
    vereq(repr(x), repr(a))
    vereq(repr(y), repr(b))
    if verbose:
        print("a = x =", a)
        print("b = y =", b)

def pickleslots():
    if verbose: print("Testing pickling of classes with __slots__ ...")
    import pickle
    # Pickling of classes with __slots__ but without __getstate__ should fail
    # (when using protocols 0 or 1)
    global B, C, D, E
    class B(object):
        pass
    for base in [object, B]:
        class C(base):
            __slots__ = ['a']
        class D(C):
            pass
        try:
            pickle.dumps(C(), 0)
        except TypeError:
            pass
        else:
            raise TestFailed, "should fail: pickle C instance - %s" % base
        try:
            pickle.dumps(C(), 0)
        except TypeError:
            pass
        else:
            raise TestFailed, "should fail: pickle D instance - %s" % base
        # Give C a nice generic __getstate__ and __setstate__
        class C(base):
            __slots__ = ['a']
            def __getstate__(self):
                try:
                    d = self.__dict__.copy()
                except AttributeError:
                    d = {}
                for cls in self.__class__.__mro__:
                    for sn in cls.__dict__.get('__slots__', ()):
                        try:
                            d[sn] = getattr(self, sn)
                        except AttributeError:
                            pass
                return d
            def __setstate__(self, d):
                for k, v in d.items():
                    setattr(self, k, v)
        class D(C):
            pass
        # Now it should work
        x = C()
        y = pickle.loads(pickle.dumps(x))
        vereq(hasattr(y, 'a'), 0)
        x.a = 42
        y = pickle.loads(pickle.dumps(x))
        vereq(y.a, 42)
        x = D()
        x.a = 42
        x.b = 100
        y = pickle.loads(pickle.dumps(x))
        vereq(y.a + y.b, 142)
        # A subclass that adds a slot should also work
        class E(C):
            __slots__ = ['b']
        x = E()
        x.a = 42
        x.b = "foo"
        y = pickle.loads(pickle.dumps(x))
        vereq(y.a, x.a)
        vereq(y.b, x.b)

def copies():
    if verbose: print("Testing copy.copy() and copy.deepcopy()...")
    import copy
    class C(object):
        pass

    a = C()
    a.foo = 12
    b = copy.copy(a)
    vereq(b.__dict__, a.__dict__)

    a.bar = [1,2,3]
    c = copy.copy(a)
    vereq(c.bar, a.bar)
    verify(c.bar is a.bar)

    d = copy.deepcopy(a)
    vereq(d.__dict__, a.__dict__)
    a.bar.append(4)
    vereq(d.bar, [1,2,3])

def binopoverride():
    if verbose: print("Testing overrides of binary operations...")
    class I(int):
        def __repr__(self):
            return "I(%r)" % int(self)
        def __add__(self, other):
            return I(int(self) + int(other))
        __radd__ = __add__
        def __pow__(self, other, mod=None):
            if mod is None:
                return I(pow(int(self), int(other)))
            else:
                return I(pow(int(self), int(other), int(mod)))
        def __rpow__(self, other, mod=None):
            if mod is None:
                return I(pow(int(other), int(self), mod))
            else:
                return I(pow(int(other), int(self), int(mod)))

    vereq(repr(I(1) + I(2)), "I(3)")
    vereq(repr(I(1) + 2), "I(3)")
    vereq(repr(1 + I(2)), "I(3)")
    vereq(repr(I(2) ** I(3)), "I(8)")
    vereq(repr(2 ** I(3)), "I(8)")
    vereq(repr(I(2) ** 3), "I(8)")
    vereq(repr(pow(I(2), I(3), I(5))), "I(3)")
    class S(str):
        def __eq__(self, other):
            return self.lower() == other.lower()

def subclasspropagation():
    if verbose: print("Testing propagation of slot functions to subclasses...")
    class A(object):
        pass
    class B(A):
        pass
    class C(A):
        pass
    class D(B, C):
        pass
    d = D()
    orig_hash = hash(d) # related to id(d) in platform-dependent ways
    A.__hash__ = lambda self: 42
    vereq(hash(d), 42)
    C.__hash__ = lambda self: 314
    vereq(hash(d), 314)
    B.__hash__ = lambda self: 144
    vereq(hash(d), 144)
    D.__hash__ = lambda self: 100
    vereq(hash(d), 100)
    del D.__hash__
    vereq(hash(d), 144)
    del B.__hash__
    vereq(hash(d), 314)
    del C.__hash__
    vereq(hash(d), 42)
    del A.__hash__
    vereq(hash(d), orig_hash)
    d.foo = 42
    d.bar = 42
    vereq(d.foo, 42)
    vereq(d.bar, 42)
    def __getattribute__(self, name):
        if name == "foo":
            return 24
        return object.__getattribute__(self, name)
    A.__getattribute__ = __getattribute__
    vereq(d.foo, 24)
    vereq(d.bar, 42)
    def __getattr__(self, name):
        if name in ("spam", "foo", "bar"):
            return "hello"
        raise AttributeError, name
    B.__getattr__ = __getattr__
    vereq(d.spam, "hello")
    vereq(d.foo, 24)
    vereq(d.bar, 42)
    del A.__getattribute__
    vereq(d.foo, 42)
    del d.foo
    vereq(d.foo, "hello")
    vereq(d.bar, 42)
    del B.__getattr__
    try:
        d.foo
    except AttributeError:
        pass
    else:
        raise TestFailed, "d.foo should be undefined now"

    # Test a nasty bug in recurse_down_subclasses()
    import gc
    class A(object):
        pass
    class B(A):
        pass
    del B
    gc.collect()
    A.__setitem__ = lambda *a: None # crash

def buffer_inherit():
    import binascii
    # SF bug [#470040] ParseTuple t# vs subclasses.
    if verbose:
        print("Testing that buffer interface is inherited ...")

    class MyStr(str):
        pass
    base = 'abc'
    m = MyStr(base)
    # b2a_hex uses the buffer interface to get its argument's value, via
    # PyArg_ParseTuple 't#' code.
    vereq(binascii.b2a_hex(m), binascii.b2a_hex(base))

    # It's not clear that unicode will continue to support the character
    # buffer interface, and this test will fail if that's taken away.
    class MyUni(str):
        pass
    base = 'abc'
    m = MyUni(base)
    vereq(binascii.b2a_hex(m), binascii.b2a_hex(base))

    class MyInt(int):
        pass
    m = MyInt(42)
    try:
        binascii.b2a_hex(m)
        raise TestFailed('subclass of int should not have a buffer interface')
    except TypeError:
        pass

def str_of_str_subclass():
    import binascii
    import io

    if verbose:
        print("Testing __str__ defined in subclass of str ...")

    class octetstring(str):
        def __str__(self):
            return str(binascii.b2a_hex(self))
        def __repr__(self):
            return self + " repr"

    o = octetstring('A')
    vereq(type(o), octetstring)
    vereq(type(str(o)), str)
    vereq(type(repr(o)), str)
    vereq(ord(o), 0x41)
    vereq(str(o), '41')
    vereq(repr(o), 'A repr')
    vereq(o.__str__(), '41')
    vereq(o.__repr__(), 'A repr')

    capture = io.StringIO()
    # Calling str() or not exercises different internal paths.
    print(o, file=capture)
    print(str(o), file=capture)
    vereq(capture.getvalue(), '41\n41\n')
    capture.close()

def kwdargs():
    if verbose: print("Testing keyword arguments to __init__, __call__...")
    def f(a): return a
    vereq(f.__call__(a=42), 42)
    a = []
    list.__init__(a, sequence=[0, 1, 2])
    vereq(a, [0, 1, 2])

def recursive__call__():
    if verbose: print(("Testing recursive __call__() by setting to instance of "
                        "class ..."))
    class A(object):
        pass

    A.__call__ = A()
    try:
        A()()
    except RuntimeError:
        pass
    else:
        raise TestFailed("Recursion limit should have been reached for "
                         "__call__()")

def delhook():
    if verbose: print("Testing __del__ hook...")
    log = []
    class C(object):
        def __del__(self):
            log.append(1)
    c = C()
    vereq(log, [])
    del c
    vereq(log, [1])

    class D(object): pass
    d = D()
    try: del d[0]
    except TypeError: pass
    else: raise TestFailed, "invalid del() didn't raise TypeError"

def hashinherit():
    if verbose: print("Testing hash of mutable subclasses...")

    class mydict(dict):
        pass
    d = mydict()
    try:
        hash(d)
    except TypeError:
        pass
    else:
        raise TestFailed, "hash() of dict subclass should fail"

    class mylist(list):
        pass
    d = mylist()
    try:
        hash(d)
    except TypeError:
        pass
    else:
        raise TestFailed, "hash() of list subclass should fail"

def strops():
    try: 'a' + 5
    except TypeError: pass
    else: raise TestFailed, "'' + 5 doesn't raise TypeError"

    try: ''.split('')
    except ValueError: pass
    else: raise TestFailed, "''.split('') doesn't raise ValueError"

    try: ''.join([0])
    except TypeError: pass
    else: raise TestFailed, "''.join([0]) doesn't raise TypeError"

    try: ''.rindex('5')
    except ValueError: pass
    else: raise TestFailed, "''.rindex('5') doesn't raise ValueError"

    try: '%(n)s' % None
    except TypeError: pass
    else: raise TestFailed, "'%(n)s' % None doesn't raise TypeError"

    try: '%(n' % {}
    except ValueError: pass
    else: raise TestFailed, "'%(n' % {} '' doesn't raise ValueError"

    try: '%*s' % ('abc')
    except TypeError: pass
    else: raise TestFailed, "'%*s' % ('abc') doesn't raise TypeError"

    try: '%*.*s' % ('abc', 5)
    except TypeError: pass
    else: raise TestFailed, "'%*.*s' % ('abc', 5) doesn't raise TypeError"

    try: '%s' % (1, 2)
    except TypeError: pass
    else: raise TestFailed, "'%s' % (1, 2) doesn't raise TypeError"

    try: '%' % None
    except ValueError: pass
    else: raise TestFailed, "'%' % None doesn't raise ValueError"

    vereq('534253'.isdigit(), 1)
    vereq('534253x'.isdigit(), 0)
    vereq('%c' % 5, '\x05')
    vereq('%c' % '5', '5')

def deepcopyrecursive():
    if verbose: print("Testing deepcopy of recursive objects...")
    class Node:
        pass
    a = Node()
    b = Node()
    a.b = b
    b.a = a
    z = deepcopy(a) # This blew up before

def modules():
    if verbose: print("Testing uninitialized module objects...")
    from types import ModuleType as M
    m = M.__new__(M)
    str(m)
    vereq(hasattr(m, "__name__"), 0)
    vereq(hasattr(m, "__file__"), 0)
    vereq(hasattr(m, "foo"), 0)
    vereq(m.__dict__, None)
    m.foo = 1
    vereq(m.__dict__, {"foo": 1})

def dictproxyiterkeys():
    class C(object):
        def meth(self):
            pass
    if verbose: print("Testing dict-proxy iterkeys...")
    keys = [ key for key in C.__dict__.keys() ]
    keys.sort()
    vereq(keys, ['__dict__', '__doc__', '__module__', '__weakref__', 'meth'])

def dictproxyitervalues():
    class C(object):
        def meth(self):
            pass
    if verbose: print("Testing dict-proxy itervalues...")
    values = [ values for values in C.__dict__.values() ]
    vereq(len(values), 5)

def dictproxyiteritems():
    class C(object):
        def meth(self):
            pass
    if verbose: print("Testing dict-proxy iteritems...")
    keys = [ key for (key, value) in C.__dict__.items() ]
    keys.sort()
    vereq(keys, ['__dict__', '__doc__', '__module__', '__weakref__', 'meth'])

def funnynew():
    if verbose: print("Testing __new__ returning something unexpected...")
    class C(object):
        def __new__(cls, arg):
            if isinstance(arg, str): return [1, 2, 3]
            elif isinstance(arg, int): return object.__new__(D)
            else: return object.__new__(cls)
    class D(C):
        def __init__(self, arg):
            self.foo = arg
    vereq(C("1"), [1, 2, 3])
    vereq(D("1"), [1, 2, 3])
    d = D(None)
    veris(d.foo, None)
    d = C(1)
    vereq(isinstance(d, D), True)
    vereq(d.foo, 1)
    d = D(1)
    vereq(isinstance(d, D), True)
    vereq(d.foo, 1)

def imulbug():
    # SF bug 544647
    if verbose: print("Testing for __imul__ problems...")
    class C(object):
        def __imul__(self, other):
            return (self, other)
    x = C()
    y = x
    y *= 1.0
    vereq(y, (x, 1.0))
    y = x
    y *= 2
    vereq(y, (x, 2))
    y = x
    y *= 3
    vereq(y, (x, 3))
    y = x
    y *= 1<<100
    vereq(y, (x, 1<<100))
    y = x
    y *= None
    vereq(y, (x, None))
    y = x
    y *= "foo"
    vereq(y, (x, "foo"))

def docdescriptor():
    # SF bug 542984
    if verbose: print("Testing __doc__ descriptor...")
    class DocDescr(object):
        def __get__(self, object, otype):
            if object:
                object = object.__class__.__name__ + ' instance'
            if otype:
                otype = otype.__name__
            return 'object=%s; type=%s' % (object, otype)
    class OldClass:
        __doc__ = DocDescr()
    class NewClass(object):
        __doc__ = DocDescr()
    vereq(OldClass.__doc__, 'object=None; type=OldClass')
    vereq(OldClass().__doc__, 'object=OldClass instance; type=OldClass')
    vereq(NewClass.__doc__, 'object=None; type=NewClass')
    vereq(NewClass().__doc__, 'object=NewClass instance; type=NewClass')

def copy_setstate():
    if verbose:
        print("Testing that copy.*copy() correctly uses __setstate__...")
    import copy
    class C(object):
        def __init__(self, foo=None):
            self.foo = foo
            self.__foo = foo
        def setfoo(self, foo=None):
            self.foo = foo
        def getfoo(self):
            return self.__foo
        def __getstate__(self):
            return [self.foo]
        def __setstate__(self, lst):
            assert len(lst) == 1
            self.__foo = self.foo = lst[0]
    a = C(42)
    a.setfoo(24)
    vereq(a.foo, 24)
    vereq(a.getfoo(), 42)
    b = copy.copy(a)
    vereq(b.foo, 24)
    vereq(b.getfoo(), 24)
    b = copy.deepcopy(a)
    vereq(b.foo, 24)
    vereq(b.getfoo(), 24)

def slices():
    if verbose:
        print("Testing cases with slices and overridden __getitem__ ...")
    # Strings
    vereq("hello"[:4], "hell")
    vereq("hello"[slice(4)], "hell")
    vereq(str.__getitem__("hello", slice(4)), "hell")
    class S(str):
        def __getitem__(self, x):
            return str.__getitem__(self, x)
    vereq(S("hello")[:4], "hell")
    vereq(S("hello")[slice(4)], "hell")
    vereq(S("hello").__getitem__(slice(4)), "hell")
    # Tuples
    vereq((1,2,3)[:2], (1,2))
    vereq((1,2,3)[slice(2)], (1,2))
    vereq(tuple.__getitem__((1,2,3), slice(2)), (1,2))
    class T(tuple):
        def __getitem__(self, x):
            return tuple.__getitem__(self, x)
    vereq(T((1,2,3))[:2], (1,2))
    vereq(T((1,2,3))[slice(2)], (1,2))
    vereq(T((1,2,3)).__getitem__(slice(2)), (1,2))
    # Lists
    vereq([1,2,3][:2], [1,2])
    vereq([1,2,3][slice(2)], [1,2])
    vereq(list.__getitem__([1,2,3], slice(2)), [1,2])
    class L(list):
        def __getitem__(self, x):
            return list.__getitem__(self, x)
    vereq(L([1,2,3])[:2], [1,2])
    vereq(L([1,2,3])[slice(2)], [1,2])
    vereq(L([1,2,3]).__getitem__(slice(2)), [1,2])
    # Now do lists and __setitem__
    a = L([1,2,3])
    a[slice(1, 3)] = [3,2]
    vereq(a, [1,3,2])
    a[slice(0, 2, 1)] = [3,1]
    vereq(a, [3,1,2])
    a.__setitem__(slice(1, 3), [2,1])
    vereq(a, [3,2,1])
    a.__setitem__(slice(0, 2, 1), [2,3])
    vereq(a, [2,3,1])

def subtype_resurrection():
    if verbose:
        print("Testing resurrection of new-style instance...")

    class C(object):
        container = []

        def __del__(self):
            # resurrect the instance
            C.container.append(self)

    c = C()
    c.attr = 42
    # The most interesting thing here is whether this blows up, due to flawed
    #  GC tracking logic in typeobject.c's call_finalizer() (a 2.2.1 bug).
    del c

    # If that didn't blow up, it's also interesting to see whether clearing
    # the last container slot works:  that will attempt to delete c again,
    # which will cause c to get appended back to the container again "during"
    # the del.
    del C.container[-1]
    vereq(len(C.container), 1)
    vereq(C.container[-1].attr, 42)

    # Make c mortal again, so that the test framework with -l doesn't report
    # it as a leak.
    del C.__del__

def slottrash():
    # Deallocating deeply nested slotted trash caused stack overflows
    if verbose:
        print("Testing slot trash...")
    class trash(object):
        __slots__ = ['x']
        def __init__(self, x):
            self.x = x
    o = None
    for i in range(50000):
        o = trash(o)
    del o

def slotmultipleinheritance():
    # SF bug 575229, multiple inheritance w/ slots dumps core
    class A(object):
        __slots__=()
    class B(object):
        pass
    class C(A,B) :
        __slots__=()
    vereq(C.__basicsize__, B.__basicsize__)
    verify(hasattr(C, '__dict__'))
    verify(hasattr(C, '__weakref__'))
    C().x = 2

def testrmul():
    # SF patch 592646
    if verbose:
        print("Testing correct invocation of __rmul__...")
    class C(object):
        def __mul__(self, other):
            return "mul"
        def __rmul__(self, other):
            return "rmul"
    a = C()
    vereq(a*2, "mul")
    vereq(a*2.2, "mul")
    vereq(2*a, "rmul")
    vereq(2.2*a, "rmul")

def testipow():
    # [SF bug 620179]
    if verbose:
        print("Testing correct invocation of __ipow__...")
    class C(object):
        def __ipow__(self, other):
            pass
    a = C()
    a **= 2

def do_this_first():
    if verbose:
        print("Testing SF bug 551412 ...")
    # This dumps core when SF bug 551412 isn't fixed --
    # but only when test_descr.py is run separately.
    # (That can't be helped -- as soon as PyType_Ready()
    # is called for PyLong_Type, the bug is gone.)
    class UserLong(object):
        def __pow__(self, *args):
            pass
    try:
        pow(0, UserLong(), 0)
    except:
        pass

    if verbose:
        print("Testing SF bug 570483...")
    # Another segfault only when run early
    # (before PyType_Ready(tuple) is called)
    type.mro(tuple)

def test_mutable_bases():
    if verbose:
        print("Testing mutable bases...")
    # stuff that should work:
    class C(object):
        pass
    class C2(object):
        def __getattribute__(self, attr):
            if attr == 'a':
                return 2
            else:
                return super(C2, self).__getattribute__(attr)
        def meth(self):
            return 1
    class D(C):
        pass
    class E(D):
        pass
    d = D()
    e = E()
    D.__bases__ = (C,)
    D.__bases__ = (C2,)
    vereq(d.meth(), 1)
    vereq(e.meth(), 1)
    vereq(d.a, 2)
    vereq(e.a, 2)
    vereq(C2.__subclasses__(), [D])

    # stuff that shouldn't:
    class L(list):
        pass

    try:
        L.__bases__ = (dict,)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't turn list subclass into dict subclass"

    try:
        list.__bases__ = (dict,)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't be able to assign to list.__bases__"

    try:
        D.__bases__ = (C2, list)
    except TypeError:
        pass
    else:
        assert 0, "best_base calculation found wanting"

    try:
        del D.__bases__
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't be able to delete .__bases__"

    try:
        D.__bases__ = ()
    except TypeError as msg:
        if str(msg) == "a new-style class can't have only classic bases":
            raise TestFailed, "wrong error message for .__bases__ = ()"
    else:
        raise TestFailed, "shouldn't be able to set .__bases__ to ()"

    try:
        D.__bases__ = (D,)
    except TypeError:
        pass
    else:
        # actually, we'll have crashed by here...
        raise TestFailed, "shouldn't be able to create inheritance cycles"

    try:
        D.__bases__ = (C, C)
    except TypeError:
        pass
    else:
        raise TestFailed, "didn't detect repeated base classes"

    try:
        D.__bases__ = (E,)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't be able to create inheritance cycles"

def test_mutable_bases_with_failing_mro():
    if verbose:
        print("Testing mutable bases with failing mro...")
    class WorkOnce(type):
        def __new__(self, name, bases, ns):
            self.flag = 0
            return super(WorkOnce, self).__new__(WorkOnce, name, bases, ns)
        def mro(self):
            if self.flag > 0:
                raise RuntimeError, "bozo"
            else:
                self.flag += 1
                return type.mro(self)

    class WorkAlways(type):
        def mro(self):
            # this is here to make sure that .mro()s aren't called
            # with an exception set (which was possible at one point).
            # An error message will be printed in a debug build.
            # What's a good way to test for this?
            return type.mro(self)

    class C(object):
        pass

    class C2(object):
        pass

    class D(C):
        pass

    class E(D):
        pass

    class F(D, metaclass=WorkOnce):
        pass

    class G(D, metaclass=WorkAlways):
        pass

    # Immediate subclasses have their mro's adjusted in alphabetical
    # order, so E's will get adjusted before adjusting F's fails.  We
    # check here that E's gets restored.

    E_mro_before = E.__mro__
    D_mro_before = D.__mro__

    try:
        D.__bases__ = (C2,)
    except RuntimeError:
        vereq(E.__mro__, E_mro_before)
        vereq(D.__mro__, D_mro_before)
    else:
        raise TestFailed, "exception not propagated"

def test_mutable_bases_catch_mro_conflict():
    if verbose:
        print("Testing mutable bases catch mro conflict...")
    class A(object):
        pass

    class B(object):
        pass

    class C(A, B):
        pass

    class D(A, B):
        pass

    class E(C, D):
        pass

    try:
        C.__bases__ = (B, A)
    except TypeError:
        pass
    else:
        raise TestFailed, "didn't catch MRO conflict"

def mutable_names():
    if verbose:
        print("Testing mutable names...")
    class C(object):
        pass

    # C.__module__ could be 'test_descr' or '__main__'
    mod = C.__module__

    C.__name__ = 'D'
    vereq((C.__module__, C.__name__), (mod, 'D'))

    C.__name__ = 'D.E'
    vereq((C.__module__, C.__name__), (mod, 'D.E'))

def subclass_right_op():
    if verbose:
        print("Testing correct dispatch of subclass overloading __r<op>__...")

    # This code tests various cases where right-dispatch of a subclass
    # should be preferred over left-dispatch of a base class.

    # Case 1: subclass of int; this tests code in abstract.c::binary_op1()

    class B(int):
        def __floordiv__(self, other):
            return "B.__floordiv__"
        def __rfloordiv__(self, other):
            return "B.__rfloordiv__"

    vereq(B(1) // 1, "B.__floordiv__")
    vereq(1 // B(1), "B.__rfloordiv__")

    # Case 2: subclass of object; this is just the baseline for case 3

    class C(object):
        def __floordiv__(self, other):
            return "C.__floordiv__"
        def __rfloordiv__(self, other):
            return "C.__rfloordiv__"

    vereq(C() // 1, "C.__floordiv__")
    vereq(1 // C(), "C.__rfloordiv__")

    # Case 3: subclass of new-style class; here it gets interesting

    class D(C):
        def __floordiv__(self, other):
            return "D.__floordiv__"
        def __rfloordiv__(self, other):
            return "D.__rfloordiv__"

    vereq(D() // C(), "D.__floordiv__")
    vereq(C() // D(), "D.__rfloordiv__")

    # Case 4: this didn't work right in 2.2.2 and 2.3a1

    class E(C):
        pass

    vereq(E.__rfloordiv__, C.__rfloordiv__)

    vereq(E() // 1, "C.__floordiv__")
    vereq(1 // E(), "C.__rfloordiv__")
    vereq(E() // C(), "C.__floordiv__")
    vereq(C() // E(), "C.__floordiv__") # This one would fail

def dict_type_with_metaclass():
    if verbose:
        print("Testing type of __dict__ when metaclass set...")

    class B(object):
        pass
    class M(type):
        pass
    class C(metaclass=M):
        # In 2.3a1, C.__dict__ was a real dict rather than a dict proxy
        pass
    veris(type(C.__dict__), type(B.__dict__))

def meth_class_get():
    # Full coverage of descrobject.c::classmethod_get()
    if verbose:
        print("Testing __get__ method of METH_CLASS C methods...")
    # Baseline
    arg = [1, 2, 3]
    res = {1: None, 2: None, 3: None}
    vereq(dict.fromkeys(arg), res)
    vereq({}.fromkeys(arg), res)
    # Now get the descriptor
    descr = dict.__dict__["fromkeys"]
    # More baseline using the descriptor directly
    vereq(descr.__get__(None, dict)(arg), res)
    vereq(descr.__get__({})(arg), res)
    # Now check various error cases
    try:
        descr.__get__(None, None)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't have allowed descr.__get__(None, None)"
    try:
        descr.__get__(42)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't have allowed descr.__get__(42)"
    try:
        descr.__get__(None, 42)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't have allowed descr.__get__(None, 42)"
    try:
        descr.__get__(None, int)
    except TypeError:
        pass
    else:
        raise TestFailed, "shouldn't have allowed descr.__get__(None, int)"

def isinst_isclass():
    if verbose:
        print("Testing proxy isinstance() and isclass()...")
    class Proxy(object):
        def __init__(self, obj):
            self.__obj = obj
        def __getattribute__(self, name):
            if name.startswith("_Proxy__"):
                return object.__getattribute__(self, name)
            else:
                return getattr(self.__obj, name)
    # Test with a classic class
    class C:
        pass
    a = C()
    pa = Proxy(a)
    verify(isinstance(a, C))  # Baseline
    verify(isinstance(pa, C)) # Test
    # Test with a classic subclass
    class D(C):
        pass
    a = D()
    pa = Proxy(a)
    verify(isinstance(a, C))  # Baseline
    verify(isinstance(pa, C)) # Test
    # Test with a new-style class
    class C(object):
        pass
    a = C()
    pa = Proxy(a)
    verify(isinstance(a, C))  # Baseline
    verify(isinstance(pa, C)) # Test
    # Test with a new-style subclass
    class D(C):
        pass
    a = D()
    pa = Proxy(a)
    verify(isinstance(a, C))  # Baseline
    verify(isinstance(pa, C)) # Test

def proxysuper():
    if verbose:
        print("Testing super() for a proxy object...")
    class Proxy(object):
        def __init__(self, obj):
            self.__obj = obj
        def __getattribute__(self, name):
            if name.startswith("_Proxy__"):
                return object.__getattribute__(self, name)
            else:
                return getattr(self.__obj, name)

    class B(object):
        def f(self):
            return "B.f"

    class C(B):
        def f(self):
            return super(C, self).f() + "->C.f"

    obj = C()
    p = Proxy(obj)
    vereq(C.__dict__["f"](p), "B.f->C.f")

def carloverre():
    if verbose:
        print("Testing prohibition of Carlo Verre's hack...")
    try:
        object.__setattr__(str, "foo", 42)
    except TypeError:
        pass
    else:
        raise TestFailed, "Carlo Verre __setattr__ suceeded!"
    try:
        object.__delattr__(str, "lower")
    except TypeError:
        pass
    else:
        raise TestFailed, "Carlo Verre __delattr__ succeeded!"

def weakref_segfault():
    # SF 742911
    if verbose:
        print("Testing weakref segfault...")

    import weakref

    class Provoker:
        def __init__(self, referrent):
            self.ref = weakref.ref(referrent)

        def __del__(self):
            x = self.ref()

    class Oops(object):
        pass

    o = Oops()
    o.whatever = Provoker(o)
    del o

def wrapper_segfault():
    # SF 927248: deeply nested wrappers could cause stack overflow
    f = lambda:None
    for i in range(1000000):
        f = f.__call__
    f = None

# Fix SF #762455, segfault when sys.stdout is changed in getattr
def filefault():
    if verbose:
        print("Testing sys.stdout is changed in getattr...")
    import sys
    class StdoutGuard:
        def __getattr__(self, attr):
            sys.stdout = sys.__stdout__
            raise RuntimeError("Premature access to sys.stdout.%s" % attr)
    sys.stdout = StdoutGuard()
    try:
        print("Oops!")
    except RuntimeError:
        pass

def vicious_descriptor_nonsense():
    # A potential segfault spotted by Thomas Wouters in mail to
    # python-dev 2003-04-17, turned into an example & fixed by Michael
    # Hudson just less than four months later...
    if verbose:
        print("Testing vicious_descriptor_nonsense...")

    class Evil(object):
        def __hash__(self):
            return hash('attr')
        def __eq__(self, other):
            del C.attr
            return 0

    class Descr(object):
        def __get__(self, ob, type=None):
            return 1

    class C(object):
        attr = Descr()

    c = C()
    c.__dict__[Evil()] = 0

    vereq(c.attr, 1)
    # this makes a crash more likely:
    import gc; gc.collect()
    vereq(hasattr(c, 'attr'), False)

def test_init():
    # SF 1155938
    class Foo(object):
        def __init__(self):
            return 10
    try:
        Foo()
    except TypeError:
        pass
    else:
        raise TestFailed, "did not test __init__() for None return"

def methodwrapper():
    # <type 'method-wrapper'> did not support any reflection before 2.5
    if verbose:
        print("Testing method-wrapper objects...")

    return # XXX should methods really support __eq__?

    l = []
    vereq(l.__add__, l.__add__)
    vereq(l.__add__, [].__add__)
    verify(l.__add__ != [5].__add__)
    verify(l.__add__ != l.__mul__)
    verify(l.__add__.__name__ == '__add__')
    verify(l.__add__.__self__ is l)
    verify(l.__add__.__objclass__ is list)
    vereq(l.__add__.__doc__, list.__add__.__doc__)
    try:
        hash(l.__add__)
    except TypeError:
        pass
    else:
        raise TestFailed("no TypeError from hash([].__add__)")

    t = ()
    t += (7,)
    vereq(t.__add__, (7,).__add__)
    vereq(hash(t.__add__), hash((7,).__add__))

def notimplemented():
    # all binary methods should be able to return a NotImplemented
    if verbose:
        print("Testing NotImplemented...")

    import sys
    import types
    import operator

    def specialmethod(self, other):
        return NotImplemented

    def check(expr, x, y):
        try:
            exec(expr, {'x': x, 'y': y, 'operator': operator})
        except TypeError:
            pass
        else:
            raise TestFailed("no TypeError from %r" % (expr,))

    N1 = sys.maxint + 1    # might trigger OverflowErrors instead of TypeErrors
    N2 = sys.maxint         # if sizeof(int) < sizeof(long), might trigger
                            #   ValueErrors instead of TypeErrors
    if 1:
        metaclass = type
        for name, expr, iexpr in [
                ('__add__',      'x + y',                   'x += y'),
                ('__sub__',      'x - y',                   'x -= y'),
                ('__mul__',      'x * y',                   'x *= y'),
                ('__truediv__',  'x / y',                   None),
                ('__floordiv__', 'x // y',                  None),
                ('__mod__',      'x % y',                   'x %= y'),
                ('__divmod__',   'divmod(x, y)',            None),
                ('__pow__',      'x ** y',                  'x **= y'),
                ('__lshift__',   'x << y',                  'x <<= y'),
                ('__rshift__',   'x >> y',                  'x >>= y'),
                ('__and__',      'x & y',                   'x &= y'),
                ('__or__',       'x | y',                   'x |= y'),
                ('__xor__',      'x ^ y',                   'x ^= y'),
                ]:
            rname = '__r' + name[2:]
            A = metaclass('A', (), {name: specialmethod})
            B = metaclass('B', (), {rname: specialmethod})
            a = A()
            b = B()
            check(expr, a, a)
            check(expr, a, b)
            check(expr, b, a)
            check(expr, b, b)
            check(expr, a, N1)
            check(expr, a, N2)
            check(expr, N1, b)
            check(expr, N2, b)
            if iexpr:
                check(iexpr, a, a)
                check(iexpr, a, b)
                check(iexpr, b, a)
                check(iexpr, b, b)
                check(iexpr, a, N1)
                check(iexpr, a, N2)
                iname = '__i' + name[2:]
                C = metaclass('C', (), {iname: specialmethod})
                c = C()
                check(iexpr, c, a)
                check(iexpr, c, b)
                check(iexpr, c, N1)
                check(iexpr, c, N2)

def test_assign_slice():
    # ceval.c's assign_slice used to check for
    # tp->tp_as_sequence->sq_slice instead of
    # tp->tp_as_sequence->sq_ass_slice

    class C(object):
        def __setslice__(self, start, stop, value):
            self.value = value

    c = C()
    c[1:2] = 3
    vereq(c.value, 3)

def test_main():
    weakref_segfault() # Must be first, somehow
    wrapper_segfault()
    do_this_first()
    class_docstrings()
    lists()
    dicts()
    dict_constructor()
    test_dir()
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
    mro_disagreement()
    diamond()
    ex5()
    monotonicity()
    consistency_with_epg()
    objects()
    slots()
    slotspecials()
    dynamics()
    errors()
    classmethods()
    classmethods_in_c()
    staticmethods()
    staticmethods_in_c()
    classic()
    compattr()
    newslot()
    altmro()
    overloading()
    methods()
    specials()
    weakrefs()
    properties()
    supers()
    inherits()
    keywords()
    str_subclass_as_dict_key()
    classic_comparisons()
    rich_comparisons()
    descrdoc()
    setclass()
    setdict()
    pickles()
    copies()
    binopoverride()
    subclasspropagation()
    buffer_inherit()
    str_of_str_subclass()
    kwdargs()
    recursive__call__()
    delhook()
    hashinherit()
    strops()
    deepcopyrecursive()
    modules()
    dictproxyiterkeys()
    dictproxyitervalues()
    dictproxyiteritems()
    pickleslots()
    funnynew()
    imulbug()
    docdescriptor()
    copy_setstate()
    slices()
    subtype_resurrection()
    slottrash()
    slotmultipleinheritance()
    testrmul()
    testipow()
    test_mutable_bases()
    test_mutable_bases_with_failing_mro()
    test_mutable_bases_catch_mro_conflict()
    mutable_names()
    subclass_right_op()
    dict_type_with_metaclass()
    meth_class_get()
    isinst_isclass()
    proxysuper()
    carloverre()
    filefault()
    vicious_descriptor_nonsense()
    test_init()
    methodwrapper()
    notimplemented()
    test_assign_slice()

    if verbose: print("All OK")

if __name__ == "__main__":
    test_main()
