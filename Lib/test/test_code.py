"""This module includes tests of the code object representation.

>>> def f(x):
...     def g(y):
...         return x + y
...     return g
...

>>> dump(f.func_code)
name: f
argcount: 1
names: ()
varnames: ('x', 'g')
cellvars: ('x',)
freevars: ()
nlocals: 2
flags: 3
consts: ('None', '<code object g>')

>>> dump(f(4).func_code)
name: g
argcount: 1
names: ()
varnames: ('y',)
cellvars: ()
freevars: ('x',)
nlocals: 1
flags: 19
consts: ('None',)

>>> def h(x, y):
...     a = x + y
...     b = x - y
...     c = a * b
...     return c
...
>>> dump(h.func_code)
name: h
argcount: 2
names: ()
varnames: ('x', 'y', 'a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 5
flags: 67
consts: ('None',)

>>> def attrs(obj):
...     print obj.attr1
...     print obj.attr2
...     print obj.attr3

>>> dump(attrs.func_code)
name: attrs
argcount: 1
names: ('attr1', 'attr2', 'attr3')
varnames: ('obj',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 67
consts: ('None',)

>>> def optimize_away():
...     'doc string'
...     'not a docstring'
...     53
...     53L

>>> dump(optimize_away.func_code)
name: optimize_away
argcount: 0
names: ()
varnames: ()
cellvars: ()
freevars: ()
nlocals: 0
flags: 67
consts: ("'doc string'", 'None')

"""

def consts(t):
    """Yield a doctest-safe sequence of object reprs."""
    for elt in t:
        r = repr(elt)
        if r.startswith("<code object"):
            yield "<code object %s>" % elt.co_name
        else:
            yield r

def dump(co):
    """Print out a text representation of a code object."""
    for attr in ["name", "argcount", "names", "varnames", "cellvars",
                 "freevars", "nlocals", "flags"]:
        print "%s: %s" % (attr, getattr(co, "co_" + attr))
    print "consts:", tuple(consts(co.co_consts))

def test_main(verbose=None):
    from test.test_support import run_doctest
    from test import test_code
    run_doctest(test_code, verbose)


if __name__ == '__main__':
    test_main()
