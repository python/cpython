# Test the module type

from test.test_support import verify, vereq, verbose, TestFailed

import sys
module = type(sys)

# An uninitialized module has no __dict__ or __name__, and __doc__ is None
foo = module.__new__(module)
verify(foo.__dict__ is None)
try:
    s = foo.__name__
except AttributeError:
    pass
else:
    raise TestFailed, "__name__ = %s" % repr(s)
vereq(foo.__doc__, module.__doc__)

# Regularly initialized module, no docstring
foo = module("foo")
vereq(foo.__name__, "foo")
vereq(foo.__doc__, None)
vereq(foo.__dict__, {"__name__": "foo", "__doc__": None})

# ASCII docstring
foo = module("foo", "foodoc")
vereq(foo.__name__, "foo")
vereq(foo.__doc__, "foodoc")
vereq(foo.__dict__, {"__name__": "foo", "__doc__": "foodoc"})

# Unicode docstring
foo = module("foo", u"foodoc\u1234")
vereq(foo.__name__, "foo")
vereq(foo.__doc__, u"foodoc\u1234")
vereq(foo.__dict__, {"__name__": "foo", "__doc__": u"foodoc\u1234"})

# Reinitialization should not replace the __dict__
foo.bar = 42
d = foo.__dict__
foo.__init__("foo", "foodoc")
vereq(foo.__name__, "foo")
vereq(foo.__doc__, "foodoc")
vereq(foo.bar, 42)
vereq(foo.__dict__, {"__name__": "foo", "__doc__": "foodoc", "bar": 42})
verify(foo.__dict__ is d)

if verbose:
    print "All OK"
