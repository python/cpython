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
    TypeError: __build_class__() got multiple values for keyword argument 'metaclass'
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
    d['foo'] = 4
    d['foo'] = 42
    d['bar'] = 123
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
    ns: [('__module__', 'test.test_metaclass'), ('a', 42), ('b', 24)]
    kw: []
    >>> type(C) is dict
    True
    >>> print(sorted(C.items()))
    [('__module__', 'test.test_metaclass'), ('a', 42), ('b', 24)]
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
    d['a'] = 1
    d['a'] = 2
    d['b'] = 3
    meta: C ()
    ns: [('__module__', 'test.test_metaclass'), ('a', 2), ('b', 3)]
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

"""

__test__ = {'doctests' : doctests}

def test_main(verbose=False):
    from test import test_support
    from test import test_metaclass
    test_support.run_doctest(test_metaclass, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
