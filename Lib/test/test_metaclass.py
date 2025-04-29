import doctest
import unittest


doctests = """

Basic class construction.

    >>> class C:
    ...     def meth(self): print("Hello")
    ...
    >>> C.__class__ is type
    True
    >>> a = C()
    >>> a.__class__ is C
    True
    >>> a.meth()
    Hello
    >>>

Use *args notation for the bases.

    >>> class A: pass
    >>> class B: pass
    >>> bases = (A, B)
    >>> class C(*bases): pass
    >>> C.__bases__ == bases
    True
    >>>

Use a trivial metaclass.

    >>> class M(type):
    ...     pass
    ...
    >>> class C(metaclass=M):
    ...    def meth(self): print("Hello")
    ...
    >>> C.__class__ is M
    True
    >>> a = C()
    >>> a.__class__ is C
    True
    >>> a.meth()
    Hello
    >>>

Use **kwds notation for the metaclass keyword.

    >>> kwds = {'metaclass': M}
    >>> class C(**kwds): pass
    ...
    >>> C.__class__ is M
    True
    >>> a = C()
    >>> a.__class__ is C
    True
    >>>

Use a metaclass with a __prepare__ static method.

    >>> class M(type):
    ...    @staticmethod
    ...    def __prepare__(*args, **kwds):
    ...        print("Prepare called:", args, kwds)
    ...        return dict()
    ...    def __new__(cls, name, bases, namespace, **kwds):
    ...        print("New called:", kwds)
    ...        return type.__new__(cls, name, bases, namespace)
    ...    def __init__(cls, *args, **kwds):
    ...        pass
    ...
    >>> class C(metaclass=M):
    ...     def meth(self): print("Hello")
    ...
    Prepare called: ('C', ()) {}
    New called: {}
    >>>

Also pass another keyword.

    >>> class C(object, metaclass=M, other="haha"):
    ...     pass
    ...
    Prepare called: ('C', (<class 'object'>,)) {'other': 'haha'}
    New called: {'other': 'haha'}
    >>> C.__class__ is M
    True
    >>> C.__bases__ == (object,)
    True
    >>> a = C()
    >>> a.__class__ is C
    True
    >>>

Check that build_class doesn't mutate the kwds dict.

    >>> kwds = {'metaclass': type}
    >>> class C(**kwds): pass
    ...
    >>> kwds == {'metaclass': type}
    True
    >>>

Use various combinations of explicit keywords and **kwds.

    >>> bases = (object,)
    >>> kwds = {'metaclass': M, 'other': 'haha'}
    >>> class C(*bases, **kwds): pass
    ...
    Prepare called: ('C', (<class 'object'>,)) {'other': 'haha'}
    New called: {'other': 'haha'}
    >>> C.__class__ is M
    True
    >>> C.__bases__ == (object,)
    True
    >>> class B: pass
    >>> kwds = {'other': 'haha'}
    >>> class C(B, metaclass=M, *bases, **kwds): pass
    ...
    Prepare called: ('C', (<class 'test.test_metaclass.B'>, <class 'object'>)) {'other': 'haha'}
    New called: {'other': 'haha'}
    >>> C.__class__ is M
    True
    >>> C.__bases__ == (B, object)
    True
    >>>

Check for duplicate keywords.

    >>> class C(metaclass=type, metaclass=type): pass
    ...
    Traceback (most recent call last):
    [...]
    SyntaxError: keyword argument repeated: metaclass
    >>>

Another way.

    >>> kwds = {'metaclass': type}
    >>> class C(metaclass=type, **kwds): pass
    ...
    Traceback (most recent call last):
    [...]
    TypeError: __build_class__() got multiple values for keyword argument 'metaclass'
    >>>

Use a __prepare__ method that returns an instrumented dict.

    >>> class LoggingDict(dict):
    ...     def __setitem__(self, key, value):
    ...         print("d[%r] = %r" % (key, value))
    ...         dict.__setitem__(self, key, value)
    ...
    >>> class Meta(type):
    ...    @staticmethod
    ...    def __prepare__(name, bases):
    ...        return LoggingDict()
    ...
    >>> class C(metaclass=Meta):
    ...     foo = 2+2
    ...     foo = 42
    ...     bar = 123
    ...
    d['__module__'] = 'test.test_metaclass'
    d['__qualname__'] = 'C'
    d['__firstlineno__'] = 1
    d['foo'] = 4
    d['foo'] = 42
    d['bar'] = 123
    d['__static_attributes__'] = ()
    >>>

Use a metaclass that doesn't derive from type.

    >>> def meta(name, bases, namespace, **kwds):
    ...     print("meta:", name, bases)
    ...     print("ns:", sorted(namespace.items()))
    ...     print("kw:", sorted(kwds.items()))
    ...     return namespace
    ...
    >>> class C(metaclass=meta):
    ...     a = 42
    ...     b = 24
    ...
    meta: C ()
    ns: [('__firstlineno__', 1), ('__module__', 'test.test_metaclass'), ('__qualname__', 'C'), ('__static_attributes__', ()), ('a', 42), ('b', 24)]
    kw: []
    >>> type(C) is dict
    True
    >>> print(sorted(C.items()))
    [('__firstlineno__', 1), ('__module__', 'test.test_metaclass'), ('__qualname__', 'C'), ('__static_attributes__', ()), ('a', 42), ('b', 24)]
    >>>

And again, with a __prepare__ attribute.

    >>> def prepare(name, bases, **kwds):
    ...     print("prepare:", name, bases, sorted(kwds.items()))
    ...     return LoggingDict()
    ...
    >>> meta.__prepare__ = prepare
    >>> class C(metaclass=meta, other="booh"):
    ...    a = 1
    ...    a = 2
    ...    b = 3
    ...
    prepare: C () [('other', 'booh')]
    d['__module__'] = 'test.test_metaclass'
    d['__qualname__'] = 'C'
    d['__firstlineno__'] = 1
    d['a'] = 1
    d['a'] = 2
    d['b'] = 3
    d['__static_attributes__'] = ()
    meta: C ()
    ns: [('__firstlineno__', 1), ('__module__', 'test.test_metaclass'), ('__qualname__', 'C'), ('__static_attributes__', ()), ('a', 2), ('b', 3)]
    kw: [('other', 'booh')]
    >>>

The default metaclass must define a __prepare__() method.

    >>> type.__prepare__()
    {}
    >>>

Make sure it works with subclassing.

    >>> class M(type):
    ...     @classmethod
    ...     def __prepare__(cls, *args, **kwds):
    ...         d = super().__prepare__(*args, **kwds)
    ...         d["hello"] = 42
    ...         return d
    ...
    >>> class C(metaclass=M):
    ...     print(hello)
    ...
    42
    >>> print(C.hello)
    42
    >>>

Test failures in looking up the __prepare__ method work.
    >>> class ObscureException(Exception):
    ...     pass
    >>> class FailDescr:
    ...     def __get__(self, instance, owner):
    ...        raise ObscureException
    >>> class Meta(type):
    ...     __prepare__ = FailDescr()
    >>> class X(metaclass=Meta):
    ...     pass
    Traceback (most recent call last):
    [...]
    test.test_metaclass.ObscureException

Test setting attributes with a non-base type in mro() (gh-127773).

    >>> class Base:
    ...     value = 1
    ...
    >>> class Meta(type):
    ...     def mro(cls):
    ...         return (cls, Base, object)
    ...
    >>> class WeirdClass(metaclass=Meta):
    ...     pass
    ...
    >>> Base.value
    1
    >>> WeirdClass.value
    1
    >>> Base.value = 2
    >>> Base.value
    2
    >>> WeirdClass.value
    2
    >>> Base.value = 3
    >>> Base.value
    3
    >>> WeirdClass.value
    3

"""

import sys

# Trace function introduces __locals__ which causes various tests to fail.
if hasattr(sys, 'gettrace') and sys.gettrace():
    __test__ = {}
else:
    __test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    # set __name__ to match doctest expectations
    __name__ = "test.test_metaclass"
    unittest.main()
