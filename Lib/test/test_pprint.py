import pprint
import test.support
import unittest
import test.test_set
import random
import collections
import itertools

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

class Unorderable:
    def __repr__(self):
        return str(id(self))

class QueryTestCase(unittest.TestCase):

    def setUp(self):
        self.a = list(range(100))
        self.b = list(range(200))
        self.a[-12] = self.b

    def test_basic(self):
        # Verify .isrecursive() and .isreadable() w/o recursion
        pp = pprint.PrettyPrinter()
        for safe in (2, 2.0, 2j, "abc", [3], (2,2), {3: 3}, "yaddayadda",
                     self.a, self.b):
            # module-level convenience functions
            self.assertFalse(pprint.isrecursive(safe),
                             "expected not isrecursive for %r" % (safe,))
            self.assertTrue(pprint.isreadable(safe),
                            "expected isreadable for %r" % (safe,))
            # PrettyPrinter methods
            self.assertFalse(pp.isrecursive(safe),
                             "expected not isrecursive for %r" % (safe,))
            self.assertTrue(pp.isreadable(safe),
                            "expected isreadable for %r" % (safe,))

    def test_knotted(self):
        # Verify .isrecursive() and .isreadable() w/ recursion
        # Tie a knot.
        self.b[67] = self.a
        # Messy dict.
        self.d = {}
        self.d[0] = self.d[1] = self.d[2] = self.d

        pp = pprint.PrettyPrinter()

        for icky in self.a, self.b, self.d, (self.d, self.d):
            self.assertTrue(pprint.isrecursive(icky), "expected isrecursive")
            self.assertFalse(pprint.isreadable(icky), "expected not isreadable")
            self.assertTrue(pp.isrecursive(icky), "expected isrecursive")
            self.assertFalse(pp.isreadable(icky), "expected not isreadable")

        # Break the cycles.
        self.d.clear()
        del self.a[:]
        del self.b[:]

        for safe in self.a, self.b, self.d, (self.d, self.d):
            # module-level convenience functions
            self.assertFalse(pprint.isrecursive(safe),
                             "expected not isrecursive for %r" % (safe,))
            self.assertTrue(pprint.isreadable(safe),
                            "expected isreadable for %r" % (safe,))
            # PrettyPrinter methods
            self.assertFalse(pp.isrecursive(safe),
                             "expected not isrecursive for %r" % (safe,))
            self.assertTrue(pp.isreadable(safe),
                            "expected isreadable for %r" % (safe,))

    def test_unreadable(self):
        # Not recursive but not readable anyway
        pp = pprint.PrettyPrinter()
        for unreadable in type(3), pprint, pprint.isrecursive:
            # module-level convenience functions
            self.assertFalse(pprint.isrecursive(unreadable),
                             "expected not isrecursive for %r" % (unreadable,))
            self.assertFalse(pprint.isreadable(unreadable),
                             "expected not isreadable for %r" % (unreadable,))
            # PrettyPrinter methods
            self.assertFalse(pp.isrecursive(unreadable),
                             "expected not isrecursive for %r" % (unreadable,))
            self.assertFalse(pp.isreadable(unreadable),
                             "expected not isreadable for %r" % (unreadable,))

    def test_same_as_repr(self):
        # Simple objects, small containers and classes that overwrite __repr__
        # For those the result should be the same as repr().
        # Ahem.  The docs don't say anything about that -- this appears to
        # be testing an implementation quirk.  Starting in Python 2.5, it's
        # not true for dicts:  pprint always sorts dicts by key now; before,
        # it sorted a dict display if and only if the display required
        # multiple lines.  For that reason, dicts with more than one element
        # aren't tested here.
        for simple in (0, 0, 0+0j, 0.0, "", b"",
                       (), tuple2(), tuple3(),
                       [], list2(), list3(),
                       {}, dict2(), dict3(),
                       self.assertTrue, pprint,
                       -6, -6, -6-6j, -1.5, "x", b"x", (3,), [3], {3: 6},
                       (1,2), [3,4], {5: 6},
                       tuple2((1,2)), tuple3((1,2)), tuple3(range(100)),
                       [3,4], list2([3,4]), list3([3,4]), list3(range(100)),
                       dict2({5: 6}), dict3({5: 6}),
                       range(10, -11, -1)
                      ):
            native = repr(simple)
            for function in "pformat", "saferepr":
                f = getattr(pprint, function)
                got = f(simple)
                self.assertEqual(native, got,
                                 "expected %s got %s from pprint.%s" %
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

    def test_nested_indentations(self):
        o1 = list(range(10))
        o2 = dict(first=1, second=2, third=3)
        o = [o1, o2]
        expected = """\
[   [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
    {   'first': 1,
        'second': 2,
        'third': 3}]"""
        self.assertEqual(pprint.pformat(o, indent=4, width=42), expected)

    def test_sorted_dict(self):
        # Starting in Python 2.5, pprint sorts dict displays by key regardless
        # of how small the dictionary may be.
        # Before the change, on 32-bit Windows pformat() gave order
        # 'a', 'c', 'b' here, so this test failed.
        d = {'a': 1, 'b': 1, 'c': 1}
        self.assertEqual(pprint.pformat(d), "{'a': 1, 'b': 1, 'c': 1}")
        self.assertEqual(pprint.pformat([d, d]),
            "[{'a': 1, 'b': 1, 'c': 1}, {'a': 1, 'b': 1, 'c': 1}]")

        # The next one is kind of goofy.  The sorted order depends on the
        # alphabetic order of type names:  "int" < "str" < "tuple".  Before
        # Python 2.5, this was in the test_same_as_repr() test.  It's worth
        # keeping around for now because it's one of few tests of pprint
        # against a crazy mix of types.
        self.assertEqual(pprint.pformat({"xy\tab\n": (3,), 5: [[]], (): {}}),
            r"{5: [[]], 'xy\tab\n': (3,), (): {}}")

    def test_ordered_dict(self):
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.OrderedDict(zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
{'the': 0,
 'quick': 1,
 'brown': 2,
 'fox': 3,
 'jumped': 4,
 'over': 5,
 'a': 6,
 'lazy': 7,
 'dog': 8}""")
    def test_subclassing(self):
        o = {'names with spaces': 'should be presented using repr()',
             'others.should.not.be': 'like.this'}
        exp = """\
{'names with spaces': 'should be presented using repr()',
 others.should.not.be: like.this}"""
        self.assertEqual(DottedPrettyPrinter().pformat(o), exp)

    @unittest.expectedFailure
    #See http://bugs.python.org/issue13907
    @test.support.cpython_only
    def test_set_reprs(self):
        # This test creates a complex arrangement of frozensets and
        # compares the pretty-printed repr against a string hard-coded in
        # the test.  The hard-coded repr depends on the sort order of
        # frozensets.
        #
        # However, as the docs point out: "Since sets only define
        # partial ordering (subset relationships), the output of the
        # list.sort() method is undefined for lists of sets."
        #
        # In a nutshell, the test assumes frozenset({0}) will always
        # sort before frozenset({1}), but:
        #
        # >>> frozenset({0}) < frozenset({1})
        # False
        # >>> frozenset({1}) < frozenset({0})
        # False
        #
        # Consequently, this test is fragile and
        # implementation-dependent.  Small changes to Python's sort
        # algorithm cause the test to fail when it should pass.
        # XXX Or changes to the dictionary implmentation...

        self.assertEqual(pprint.pformat(set()), 'set()')
        self.assertEqual(pprint.pformat(set(range(3))), '{0, 1, 2}')
        self.assertEqual(pprint.pformat(frozenset()), 'frozenset()')

        self.assertEqual(pprint.pformat(frozenset(range(3))), 'frozenset({0, 1, 2})')
        cube_repr_tgt = """\
{frozenset(): frozenset({frozenset({2}), frozenset({0}), frozenset({1})}),
 frozenset({0}): frozenset({frozenset(),
                            frozenset({0, 2}),
                            frozenset({0, 1})}),
 frozenset({1}): frozenset({frozenset(),
                            frozenset({1, 2}),
                            frozenset({0, 1})}),
 frozenset({2}): frozenset({frozenset(),
                            frozenset({1, 2}),
                            frozenset({0, 2})}),
 frozenset({1, 2}): frozenset({frozenset({2}),
                               frozenset({1}),
                               frozenset({0, 1, 2})}),
 frozenset({0, 2}): frozenset({frozenset({2}),
                               frozenset({0}),
                               frozenset({0, 1, 2})}),
 frozenset({0, 1}): frozenset({frozenset({0}),
                               frozenset({1}),
                               frozenset({0, 1, 2})}),
 frozenset({0, 1, 2}): frozenset({frozenset({1, 2}),
                                  frozenset({0, 2}),
                                  frozenset({0, 1})})}"""
        cube = test.test_set.cube(3)
        self.assertEqual(pprint.pformat(cube), cube_repr_tgt)
        cubo_repr_tgt = """\
{frozenset({frozenset({0, 2}), frozenset({0})}): frozenset({frozenset({frozenset({0,
                                                                                  2}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({0}),
                                                                       frozenset({0,
                                                                                  1})}),
                                                            frozenset({frozenset(),
                                                                       frozenset({0})}),
                                                            frozenset({frozenset({2}),
                                                                       frozenset({0,
                                                                                  2})})}),
 frozenset({frozenset({0, 1}), frozenset({1})}): frozenset({frozenset({frozenset({0,
                                                                                  1}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({0}),
                                                                       frozenset({0,
                                                                                  1})}),
                                                            frozenset({frozenset({1}),
                                                                       frozenset({1,
                                                                                  2})}),
                                                            frozenset({frozenset(),
                                                                       frozenset({1})})}),
 frozenset({frozenset({1, 2}), frozenset({1})}): frozenset({frozenset({frozenset({1,
                                                                                  2}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({2}),
                                                                       frozenset({1,
                                                                                  2})}),
                                                            frozenset({frozenset(),
                                                                       frozenset({1})}),
                                                            frozenset({frozenset({1}),
                                                                       frozenset({0,
                                                                                  1})})}),
 frozenset({frozenset({1, 2}), frozenset({2})}): frozenset({frozenset({frozenset({1,
                                                                                  2}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({1}),
                                                                       frozenset({1,
                                                                                  2})}),
                                                            frozenset({frozenset({2}),
                                                                       frozenset({0,
                                                                                  2})}),
                                                            frozenset({frozenset(),
                                                                       frozenset({2})})}),
 frozenset({frozenset(), frozenset({0})}): frozenset({frozenset({frozenset({0}),
                                                                 frozenset({0,
                                                                            1})}),
                                                      frozenset({frozenset({0}),
                                                                 frozenset({0,
                                                                            2})}),
                                                      frozenset({frozenset(),
                                                                 frozenset({1})}),
                                                      frozenset({frozenset(),
                                                                 frozenset({2})})}),
 frozenset({frozenset(), frozenset({1})}): frozenset({frozenset({frozenset(),
                                                                 frozenset({0})}),
                                                      frozenset({frozenset({1}),
                                                                 frozenset({1,
                                                                            2})}),
                                                      frozenset({frozenset(),
                                                                 frozenset({2})}),
                                                      frozenset({frozenset({1}),
                                                                 frozenset({0,
                                                                            1})})}),
 frozenset({frozenset({2}), frozenset()}): frozenset({frozenset({frozenset({2}),
                                                                 frozenset({1,
                                                                            2})}),
                                                      frozenset({frozenset(),
                                                                 frozenset({0})}),
                                                      frozenset({frozenset(),
                                                                 frozenset({1})}),
                                                      frozenset({frozenset({2}),
                                                                 frozenset({0,
                                                                            2})})}),
 frozenset({frozenset({0, 1, 2}), frozenset({0, 1})}): frozenset({frozenset({frozenset({1,
                                                                                        2}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({0,
                                                                                        2}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({0}),
                                                                             frozenset({0,
                                                                                        1})}),
                                                                  frozenset({frozenset({1}),
                                                                             frozenset({0,
                                                                                        1})})}),
 frozenset({frozenset({0}), frozenset({0, 1})}): frozenset({frozenset({frozenset(),
                                                                       frozenset({0})}),
                                                            frozenset({frozenset({0,
                                                                                  1}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({0}),
                                                                       frozenset({0,
                                                                                  2})}),
                                                            frozenset({frozenset({1}),
                                                                       frozenset({0,
                                                                                  1})})}),
 frozenset({frozenset({2}), frozenset({0, 2})}): frozenset({frozenset({frozenset({0,
                                                                                  2}),
                                                                       frozenset({0,
                                                                                  1,
                                                                                  2})}),
                                                            frozenset({frozenset({2}),
                                                                       frozenset({1,
                                                                                  2})}),
                                                            frozenset({frozenset({0}),
                                                                       frozenset({0,
                                                                                  2})}),
                                                            frozenset({frozenset(),
                                                                       frozenset({2})})}),
 frozenset({frozenset({0, 1, 2}), frozenset({0, 2})}): frozenset({frozenset({frozenset({1,
                                                                                        2}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({0,
                                                                                        1}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({0}),
                                                                             frozenset({0,
                                                                                        2})}),
                                                                  frozenset({frozenset({2}),
                                                                             frozenset({0,
                                                                                        2})})}),
 frozenset({frozenset({1, 2}), frozenset({0, 1, 2})}): frozenset({frozenset({frozenset({0,
                                                                                        2}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({0,
                                                                                        1}),
                                                                             frozenset({0,
                                                                                        1,
                                                                                        2})}),
                                                                  frozenset({frozenset({2}),
                                                                             frozenset({1,
                                                                                        2})}),
                                                                  frozenset({frozenset({1}),
                                                                             frozenset({1,
                                                                                        2})})})}"""

        cubo = test.test_set.linegraph(cube)
        self.assertEqual(pprint.pformat(cubo), cubo_repr_tgt)

    def test_depth(self):
        nested_tuple = (1, (2, (3, (4, (5, 6)))))
        nested_dict = {1: {2: {3: {4: {5: {6: 6}}}}}}
        nested_list = [1, [2, [3, [4, [5, [6, []]]]]]]
        self.assertEqual(pprint.pformat(nested_tuple), repr(nested_tuple))
        self.assertEqual(pprint.pformat(nested_dict), repr(nested_dict))
        self.assertEqual(pprint.pformat(nested_list), repr(nested_list))

        lv1_tuple = '(1, (...))'
        lv1_dict = '{1: {...}}'
        lv1_list = '[1, [...]]'
        self.assertEqual(pprint.pformat(nested_tuple, depth=1), lv1_tuple)
        self.assertEqual(pprint.pformat(nested_dict, depth=1), lv1_dict)
        self.assertEqual(pprint.pformat(nested_list, depth=1), lv1_list)

    def test_sort_unorderable_values(self):
        # Issue 3976:  sorted pprints fail for unorderable values.
        n = 20
        keys = [Unorderable() for i in range(n)]
        random.shuffle(keys)
        skeys = sorted(keys, key=id)
        clean = lambda s: s.replace(' ', '').replace('\n','')

        self.assertEqual(clean(pprint.pformat(set(keys))),
            '{' + ','.join(map(repr, skeys)) + '}')
        self.assertEqual(clean(pprint.pformat(frozenset(keys))),
            'frozenset({' + ','.join(map(repr, skeys)) + '})')
        self.assertEqual(clean(pprint.pformat(dict.fromkeys(keys))),
            '{' + ','.join('%r:None' % k for k in skeys) + '}')

        # Issue 10017: TypeError on user-defined types as dict keys.
        self.assertEqual(pprint.pformat({Unorderable: 0, 1: 0}),
                         '{1: 0, ' + repr(Unorderable) +': 0}')

        # Issue 14998: TypeError on tuples with NoneTypes as dict keys.
        self.assertEqual(pprint.pformat({(1,): 0, (None,): 0}),
                         '{(1,): 0, (None,): 0}')


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
    test.support.run_unittest(QueryTestCase)


if __name__ == "__main__":
    test_main()
