import pprint
import test.test_support
import unittest

try:
    uni = unicode
except NameError:
    def uni(x):
        return x

# list, tuple and dict subclasses that do or don't overwrite __repr__
class list2(list):
    pass
class list3(list):
    def __repr__(self):
        return list.__repr__(self)
class tuple2(tuple):
    pass
class tuple3(tuple):
    def __repr__(self):
        return tuple.__repr__(self)
class dict2(dict):
    pass
class dict3(dict):
    def __repr__(self):
        return dict.__repr__(self)

class QueryTestCase(unittest.TestCase):

    def setUp(self):
        self.a = range(100)
        self.b = range(200)
        self.a[-12] = self.b

    def test_basic(self):
        # Verify .isrecursive() and .isreadable() w/o recursion
        verify = self.assert_
        pp = pprint.PrettyPrinter()
        for safe in (2, 2.0, 2j, "abc", [3], (2,2), {3: 3}, uni("yaddayadda"),
                     self.a, self.b):
            # module-level convenience functions
            verify(not pprint.isrecursive(safe),
                   "expected not isrecursive for %r" % (safe,))
            verify(pprint.isreadable(safe),
                   "expected isreadable for %r" % (safe,))
            # PrettyPrinter methods
            verify(not pp.isrecursive(safe),
                   "expected not isrecursive for %r" % (safe,))
            verify(pp.isreadable(safe),
                   "expected isreadable for %r" % (safe,))

    def test_knotted(self):
        # Verify .isrecursive() and .isreadable() w/ recursion
        # Tie a knot.
        self.b[67] = self.a
        # Messy dict.
        self.d = {}
        self.d[0] = self.d[1] = self.d[2] = self.d

        verify = self.assert_
        pp = pprint.PrettyPrinter()

        for icky in self.a, self.b, self.d, (self.d, self.d):
            verify(pprint.isrecursive(icky), "expected isrecursive")
            verify(not pprint.isreadable(icky),  "expected not isreadable")
            verify(pp.isrecursive(icky), "expected isrecursive")
            verify(not pp.isreadable(icky),  "expected not isreadable")

        # Break the cycles.
        self.d.clear()
        del self.a[:]
        del self.b[:]

        for safe in self.a, self.b, self.d, (self.d, self.d):
            # module-level convenience functions
            verify(not pprint.isrecursive(safe),
                   "expected not isrecursive for %r" % (safe,))
            verify(pprint.isreadable(safe),
                   "expected isreadable for %r" % (safe,))
            # PrettyPrinter methods
            verify(not pp.isrecursive(safe),
                   "expected not isrecursive for %r" % (safe,))
            verify(pp.isreadable(safe),
                   "expected isreadable for %r" % (safe,))

    def test_unreadable(self):
        # Not recursive but not readable anyway
        verify = self.assert_
        pp = pprint.PrettyPrinter()
        for unreadable in type(3), pprint, pprint.isrecursive:
            # module-level convenience functions
            verify(not pprint.isrecursive(unreadable),
                   "expected not isrecursive for %r" % (unreadable,))
            verify(not pprint.isreadable(unreadable),
                   "expected not isreadable for %r" % (unreadable,))
            # PrettyPrinter methods
            verify(not pp.isrecursive(unreadable),
                   "expected not isrecursive for %r" % (unreadable,))
            verify(not pp.isreadable(unreadable),
                   "expected not isreadable for %r" % (unreadable,))

    def test_same_as_repr(self):
        # Simple objects, small containers and classes that overwrite __repr__
        # For those the result should be the same as repr()
        verify = self.assert_
        for simple in (0, 0L, 0+0j, 0.0, "", uni(""),
                       (), tuple2(), tuple3(),
                       [], list2(), list3(),
                       {}, dict2(), dict3(),
                       verify, pprint,
                       -6, -6L, -6-6j, -1.5, "x", uni("x"), (3,), [3], {3: 6},
                       (1,2), [3,4], {5: 6, 7: 8},
                       tuple2((1,2)), tuple3((1,2)), tuple3(range(100)),
                       [3,4], list2([3,4]), list3([3,4]), list3(range(100)),
                       {5: 6, 7: 8}, dict2({5: 6, 7: 8}), dict3({5: 6, 7: 8}),
                       dict3([(x,x) for x in range(100)]),
                       {"xy\tab\n": (3,), 5: [[]], (): {}},
                       range(10, -11, -1)
                      ):
            native = repr(simple)
            for function in "pformat", "saferepr":
                f = getattr(pprint, function)
                got = f(simple)
                verify(native == got, "expected %s got %s from pprint.%s" %
                                      (native, got, function))

    def test_basic_line_wrap(self):
        # verify basic line-wrapping operation
        o = {'RPM_cal': 0,
             'RPM_cal2': 48059,
             'Speed_cal': 0,
             'controldesk_runtime_us': 0,
             'main_code_runtime_us': 0,
             'read_io_runtime_us': 0,
             'write_io_runtime_us': 43690}
        exp = """\
{'RPM_cal': 0,
 'RPM_cal2': 48059,
 'Speed_cal': 0,
 'controldesk_runtime_us': 0,
 'main_code_runtime_us': 0,
 'read_io_runtime_us': 0,
 'write_io_runtime_us': 43690}"""
        for type in [dict, dict2]:
            self.assertEqual(pprint.pformat(type(o)), exp)

        o = range(100)
        exp = '[%s]' % ',\n '.join(map(str, o))
        for type in [list, list2]:
            self.assertEqual(pprint.pformat(type(o)), exp)

        o = tuple(range(100))
        exp = '(%s)' % ',\n '.join(map(str, o))
        for type in [tuple, tuple2]:
            self.assertEqual(pprint.pformat(type(o)), exp)

        # indent parameter
        o = range(100)
        exp = '[   %s]' % ',\n    '.join(map(str, o))
        for type in [list, list2]:
            self.assertEqual(pprint.pformat(type(o), indent=4), exp)

    def test_subclassing(self):
        o = {'names with spaces': 'should be presented using repr()',
             'others.should.not.be': 'like.this'}
        exp = """\
{'names with spaces': 'should be presented using repr()',
 others.should.not.be: like.this}"""
        self.assertEqual(DottedPrettyPrinter().pformat(o), exp)


class DottedPrettyPrinter(pprint.PrettyPrinter):

    def format(self, object, context, maxlevels, level):
        if isinstance(object, str):
            if ' ' in object:
                return repr(object), 1, 0
            else:
                return object, 0, 0
        else:
            return pprint.PrettyPrinter.format(
                self, object, context, maxlevels, level)


def test_main():
    test.test_support.run_unittest(QueryTestCase)


if __name__ == "__main__":
    test_main()
