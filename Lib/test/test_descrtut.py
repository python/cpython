# This contains most of the executable examples from Guido's descr
# tutorial, once at
#
#     http://www.python.org/2.2/descrintro.html
#
# A few examples left implicit in the writeup were fleshed out, a few were
# skipped due to lack of interest (e.g., faking super() by hand isn't
# of much interest anymore), and a few were fiddled to make the output
# deterministic.

from test.test_support import sortdict
import pprint

class defaultdict(dict):
    def __init__(self, default=None):
        dict.__init__(self)
        self.default = default

    def __getitem__(self, key):
        try:
            return dict.__getitem__(self, key)
        except KeyError:
            return self.default

    def get(self, key, *args):
        if not args:
            args = (self.default,)
        return dict.get(self, key, *args)

    def merge(self, other):
        for key in other:
            if key not in self:
                self[key] = other[key]

test_1 = """

Here's the new type at work:

    >>> print(defaultdict)              # show our type
    <class 'test.test_descrtut.defaultdict'>
    >>> print(type(defaultdict))        # its metatype
    <type 'type'>
    >>> a = defaultdict(default=0.0)    # create an instance
    >>> print(a)                        # show the instance
    {}
    >>> print(type(a))                  # show its type
    <class 'test.test_descrtut.defaultdict'>
    >>> print(a.__class__)              # show its class
    <class 'test.test_descrtut.defaultdict'>
    >>> print(type(a) is a.__class__)   # its type is its class
    True
    >>> a[1] = 3.25                     # modify the instance
    >>> print(a)                        # show the new value
    {1: 3.25}
    >>> print(a[1])                     # show the new item
    3.25
    >>> print(a[0])                     # a non-existant item
    0.0
    >>> a.merge({1:100, 2:200})         # use a dict method
    >>> print(sortdict(a))              # show the result
    {1: 3.25, 2: 200}
    >>>

We can also use the new type in contexts where classic only allows "real"
dictionaries, such as the locals/globals dictionaries for the exec
statement or the built-in function eval():

    >>> print(sorted(a.keys()))
    [1, 2]
    >>> a['print'] = print              # need the print function here
    >>> exec("x = 3; print(x)", a)
    3
    >>> print(sorted(a.keys(), key=lambda x: (str(type(x)), x)))
    [1, 2, '__builtins__', 'print', 'x']
    >>> print(a['x'])
    3
    >>>

Now I'll show that defaultdict instances have dynamic instance variables,
just like classic classes:

    >>> a.default = -1
    >>> print(a["noway"])
    -1
    >>> a.default = -1000
    >>> print(a["noway"])
    -1000
    >>> 'default' in dir(a)
    True
    >>> a.x1 = 100
    >>> a.x2 = 200
    >>> print(a.x1)
    100
    >>> d = dir(a)
    >>> 'default' in d and 'x1' in d and 'x2' in d
    True
    >>> print(sortdict(a.__dict__))
    {'default': -1000, 'x1': 100, 'x2': 200}
    >>>
"""

class defaultdict2(dict):
    __slots__ = ['default']

    def __init__(self, default=None):
        dict.__init__(self)
        self.default = default

    def __getitem__(self, key):
        try:
            return dict.__getitem__(self, key)
        except KeyError:
            return self.default

    def get(self, key, *args):
        if not args:
            args = (self.default,)
        return dict.get(self, key, *args)

    def merge(self, other):
        for key in other:
            if key not in self:
                self[key] = other[key]

test_2 = """

The __slots__ declaration takes a list of instance variables, and reserves
space for exactly these in the instance. When __slots__ is used, other
instance variables cannot be assigned to:

    >>> a = defaultdict2(default=0.0)
    >>> a[1]
    0.0
    >>> a.default = -1
    >>> a[1]
    -1
    >>> a.x1 = 1
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
    AttributeError: 'defaultdict2' object has no attribute 'x1'
    >>>

"""

test_3 = """

Introspecting instances of built-in types

For instance of built-in types, x.__class__ is now the same as type(x):

    >>> type([])
    <type 'list'>
    >>> [].__class__
    <type 'list'>
    >>> list
    <type 'list'>
    >>> isinstance([], list)
    True
    >>> isinstance([], dict)
    False
    >>> isinstance([], object)
    True
    >>>

You can get the information from the list type:

    >>> pprint.pprint(dir(list))    # like list.__dict__.keys(), but sorted
    ['__add__',
     '__class__',
     '__contains__',
     '__delattr__',
     '__delitem__',
     '__delslice__',
     '__doc__',
     '__eq__',
     '__format__',
     '__ge__',
     '__getattribute__',
     '__getitem__',
     '__getslice__',
     '__gt__',
     '__hash__',
     '__iadd__',
     '__imul__',
     '__init__',
     '__iter__',
     '__le__',
     '__len__',
     '__lt__',
     '__mul__',
     '__ne__',
     '__new__',
     '__reduce__',
     '__reduce_ex__',
     '__repr__',
     '__reversed__',
     '__rmul__',
     '__setattr__',
     '__setitem__',
     '__setslice__',
     '__str__',
     'append',
     'count',
     'extend',
     'index',
     'insert',
     'pop',
     'remove',
     'reverse',
     'sort']

The new introspection API gives more information than the old one:  in
addition to the regular methods, it also shows the methods that are
normally invoked through special notations, e.g. __iadd__ (+=), __len__
(len), __ne__ (!=). You can invoke any method from this list directly:

    >>> a = ['tic', 'tac']
    >>> list.__len__(a)          # same as len(a)
    2
    >>> a.__len__()              # ditto
    2
    >>> list.append(a, 'toe')    # same as a.append('toe')
    >>> a
    ['tic', 'tac', 'toe']
    >>>

This is just like it is for user-defined classes.
"""

test_4 = """

Static methods and class methods

The new introspection API makes it possible to add static methods and class
methods. Static methods are easy to describe: they behave pretty much like
static methods in C++ or Java. Here's an example:

    >>> class C:
    ...
    ...     @staticmethod
    ...     def foo(x, y):
    ...         print("staticmethod", x, y)

    >>> C.foo(1, 2)
    staticmethod 1 2
    >>> c = C()
    >>> c.foo(1, 2)
    staticmethod 1 2

Class methods use a similar pattern to declare methods that receive an
implicit first argument that is the *class* for which they are invoked.

    >>> class C:
    ...     @classmethod
    ...     def foo(cls, y):
    ...         print("classmethod", cls, y)

    >>> C.foo(1)
    classmethod <class 'test.test_descrtut.C'> 1
    >>> c = C()
    >>> c.foo(1)
    classmethod <class 'test.test_descrtut.C'> 1

    >>> class D(C):
    ...     pass

    >>> D.foo(1)
    classmethod <class 'test.test_descrtut.D'> 1
    >>> d = D()
    >>> d.foo(1)
    classmethod <class 'test.test_descrtut.D'> 1

This prints "classmethod __main__.D 1" both times; in other words, the
class passed as the first argument of foo() is the class involved in the
call, not the class involved in the definition of foo().

But notice this:

    >>> class E(C):
    ...     @classmethod
    ...     def foo(cls, y): # override C.foo
    ...         print("E.foo() called")
    ...         C.foo(y)

    >>> E.foo(1)
    E.foo() called
    classmethod <class 'test.test_descrtut.C'> 1
    >>> e = E()
    >>> e.foo(1)
    E.foo() called
    classmethod <class 'test.test_descrtut.C'> 1

In this example, the call to C.foo() from E.foo() will see class C as its
first argument, not class E. This is to be expected, since the call
specifies the class C. But it stresses the difference between these class
methods and methods defined in metaclasses (where an upcall to a metamethod
would pass the target class as an explicit first argument).
"""

test_5 = """

Attributes defined by get/set methods


    >>> class property(object):
    ...
    ...     def __init__(self, get, set=None):
    ...         self.__get = get
    ...         self.__set = set
    ...
    ...     def __get__(self, inst, type=None):
    ...         return self.__get(inst)
    ...
    ...     def __set__(self, inst, value):
    ...         if self.__set is None:
    ...             raise AttributeError, "this attribute is read-only"
    ...         return self.__set(inst, value)

Now let's define a class with an attribute x defined by a pair of methods,
getx() and and setx():

    >>> class C(object):
    ...
    ...     def __init__(self):
    ...         self.__x = 0
    ...
    ...     def getx(self):
    ...         return self.__x
    ...
    ...     def setx(self, x):
    ...         if x < 0: x = 0
    ...         self.__x = x
    ...
    ...     x = property(getx, setx)

Here's a small demonstration:

    >>> a = C()
    >>> a.x = 10
    >>> print(a.x)
    10
    >>> a.x = -10
    >>> print(a.x)
    0
    >>>

Hmm -- property is builtin now, so let's try it that way too.

    >>> del property  # unmask the builtin
    >>> property
    <type 'property'>

    >>> class C(object):
    ...     def __init__(self):
    ...         self.__x = 0
    ...     def getx(self):
    ...         return self.__x
    ...     def setx(self, x):
    ...         if x < 0: x = 0
    ...         self.__x = x
    ...     x = property(getx, setx)


    >>> a = C()
    >>> a.x = 10
    >>> print(a.x)
    10
    >>> a.x = -10
    >>> print(a.x)
    0
    >>>
"""

test_6 = """

Method resolution order

This example is implicit in the writeup.

>>> class A:    # implicit new-style class
...     def save(self):
...         print("called A.save()")
>>> class B(A):
...     pass
>>> class C(A):
...     def save(self):
...         print("called C.save()")
>>> class D(B, C):
...     pass

>>> D().save()
called C.save()

>>> class A(object):  # explicit new-style class
...     def save(self):
...         print("called A.save()")
>>> class B(A):
...     pass
>>> class C(A):
...     def save(self):
...         print("called C.save()")
>>> class D(B, C):
...     pass

>>> D().save()
called C.save()
"""

class A(object):
    def m(self):
        return "A"

class B(A):
    def m(self):
        return "B" + super(B, self).m()

class C(A):
    def m(self):
        return "C" + super(C, self).m()

class D(C, B):
    def m(self):
        return "D" + super(D, self).m()


test_7 = """

Cooperative methods and "super"

>>> print(D().m()) # "DCBA"
DCBA
"""

test_8 = """

Backwards incompatibilities

>>> class A:
...     def foo(self):
...         print("called A.foo()")

>>> class B(A):
...     pass

>>> class C(A):
...     def foo(self):
...         B.foo(self)

>>> C().foo()
Traceback (most recent call last):
 ...
TypeError: unbound method foo() must be called with B instance as first argument (got C instance instead)

>>> class C(A):
...     def foo(self):
...         A.foo(self)
>>> C().foo()
called A.foo()
"""

__test__ = {"tut1": test_1,
            "tut2": test_2,
            "tut3": test_3,
            "tut4": test_4,
            "tut5": test_5,
            "tut6": test_6,
            "tut7": test_7,
            "tut8": test_8}

# Magic test name that regrtest.py invokes *after* importing this module.
# This worms around a bootstrap problem.
# Note that doctest and regrtest both look in sys.argv for a "-v" argument,
# so this works as expected in both ways of running regrtest.
def test_main(verbose=None):
    # Obscure:  import this module as test.test_descrtut instead of as
    # plain test_descrtut because the name of this module works its way
    # into the doctest examples, and unless the full test.test_descrtut
    # business is used the name can change depending on how the test is
    # invoked.
    from test import test_support, test_descrtut
    test_support.run_doctest(test_descrtut, verbose)

# This part isn't needed for regrtest, but for running the test directly.
if __name__ == "__main__":
    test_main(1)
