# -*- coding: utf-8 -*-

import collections
import contextlib
import dataclasses
import io
import itertools
import pprint
import random
import re
import types
import unittest
from collections.abc import ItemsView, KeysView, Mapping, MappingView, ValuesView

from test.support import cpython_only
from test.support.import_helper import ensure_lazy_imports

# list, tuple and dict subclasses that do or don't overwrite __repr__
class list2(list):
    pass

class list3(list):
    def __repr__(self):
        return list.__repr__(self)

class list_custom_repr(list):
    def __repr__(self):
        return '*'*len(list.__repr__(self))

class tuple2(tuple):
    pass

class tuple3(tuple):
    def __repr__(self):
        return tuple.__repr__(self)

class tuple_custom_repr(tuple):
    def __repr__(self):
        return '*'*len(tuple.__repr__(self))

class set2(set):
    pass

class set3(set):
    def __repr__(self):
        return set.__repr__(self)

class set_custom_repr(set):
    def __repr__(self):
        return '*'*len(set.__repr__(self))

class frozenset2(frozenset):
    pass

class frozenset3(frozenset):
    def __repr__(self):
        return frozenset.__repr__(self)

class frozenset_custom_repr(frozenset):
    def __repr__(self):
        return '*'*len(frozenset.__repr__(self))

class dict2(dict):
    pass

class dict3(dict):
    def __repr__(self):
        return dict.__repr__(self)

class dict_custom_repr(dict):
    def __repr__(self):
        return '*'*len(dict.__repr__(self))

class mappingview_custom_repr(MappingView):
    def __repr__(self):
        return '*'*len(MappingView.__repr__(self))

class keysview_custom_repr(KeysView):
    def __repr__(self):
        return '*'*len(KeysView.__repr__(self))

@dataclasses.dataclass
class dataclass1:
    field1: str
    field2: int
    field3: bool = False
    field4: int = dataclasses.field(default=1, repr=False)

@dataclasses.dataclass
class dataclass2:
    a: int = 1
    def __repr__(self):
        return "custom repr that doesn't fit within pprint width"

@dataclasses.dataclass(repr=False)
class dataclass3:
    a: int = 1

@dataclasses.dataclass
class dataclass4:
    a: "dataclass4"
    b: int = 1

@dataclasses.dataclass
class dataclass5:
    a: "dataclass6"
    b: int = 1

@dataclasses.dataclass
class dataclass6:
    c: "dataclass5"
    d: int = 1

class Unorderable:
    def __repr__(self):
        return str(id(self))

# Class Orderable is orderable with any type
class Orderable:
    def __init__(self, hash):
        self._hash = hash
    def __lt__(self, other):
        return False
    def __gt__(self, other):
        return self != other
    def __le__(self, other):
        return self == other
    def __ge__(self, other):
        return True
    def __eq__(self, other):
        return self is other
    def __ne__(self, other):
        return self is not other
    def __hash__(self):
        return self._hash

class QueryTestCase(unittest.TestCase):

    def setUp(self):
        self.a = list(range(100))
        self.b = list(range(200))
        self.a[-12] = self.b

    @cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("pprint", {"dataclasses", "re"})

    def test_init(self):
        pp = pprint.PrettyPrinter()
        pp = pprint.PrettyPrinter(indent=4, width=40, depth=5,
                                  stream=io.StringIO(), compact=True)
        pp = pprint.PrettyPrinter(4, 40, 5, io.StringIO())
        pp = pprint.PrettyPrinter(sort_dicts=False)
        with self.assertRaises(TypeError):
            pp = pprint.PrettyPrinter(4, 40, 5, io.StringIO(), True)
        self.assertRaises(ValueError, pprint.PrettyPrinter, indent=-1)
        self.assertRaises(ValueError, pprint.PrettyPrinter, depth=0)
        self.assertRaises(ValueError, pprint.PrettyPrinter, depth=-1)
        self.assertRaises(ValueError, pprint.PrettyPrinter, width=0)

    def test_basic(self):
        # Verify .isrecursive() and .isreadable() w/o recursion
        pp = pprint.PrettyPrinter()
        for safe in (2, 2.0, 2j, "abc", [3], (2,2), {3: 3}, b"def",
                     bytearray(b"ghi"), True, False, None, ...,
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

    def test_stdout_is_None(self):
        with contextlib.redirect_stdout(None):
            # smoke test - there is no output to check
            value = 'this should not fail'
            pprint.pprint(value)
            pprint.PrettyPrinter().pprint(value)

    def test_knotted(self):
        # Verify .isrecursive() and .isreadable() w/ recursion
        # Tie a knot.
        self.b[67] = self.a
        # Messy dict.
        self.d = {}
        self.d[0] = self.d[1] = self.d[2] = self.d
        self.e = {}
        self.v = ValuesView(self.e)
        self.m = MappingView(self.e)
        self.dv = self.e.values()
        self.e["v"] = self.v
        self.e["m"] = self.m
        self.e["dv"] = self.dv

        pp = pprint.PrettyPrinter()

        for icky in self.a, self.b, self.d, (self.d, self.d), self.e, self.v, self.m, self.dv:
            self.assertTrue(pprint.isrecursive(icky), "expected isrecursive")
            self.assertFalse(pprint.isreadable(icky), "expected not isreadable")
            self.assertTrue(pp.isrecursive(icky), "expected isrecursive")
            self.assertFalse(pp.isreadable(icky), "expected not isreadable")

        # Break the cycles.
        self.d.clear()
        self.e.clear()
        del self.a[:]
        del self.b[:]

        for safe in self.a, self.b, self.d, (self.d, self.d), self.e, self.v, self.m, self.dv:
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
        for unreadable in object(), int, pprint, pprint.isrecursive:
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
        # Simple objects, small containers and classes that override __repr__
        # to directly call super's __repr__.
        # For those the result should be the same as repr().
        # Ahem.  The docs don't say anything about that -- this appears to
        # be testing an implementation quirk.  Starting in Python 2.5, it's
        # not true for dicts:  pprint always sorts dicts by key now; before,
        # it sorted a dict display if and only if the display required
        # multiple lines.  For that reason, dicts with more than one element
        # aren't tested here.
        for simple in (0, 0, 0+0j, 0.0, "", b"", bytearray(),
                       (), tuple2(), tuple3(),
                       [], list2(), list3(),
                       set(), set2(), set3(),
                       frozenset(), frozenset2(), frozenset3(),
                       {}, dict2(), dict3(),
                       {}.keys(), {}.values(), {}.items(),
                       MappingView({}), KeysView({}), ItemsView({}), ValuesView({}),
                       self.assertTrue, pprint,
                       -6, -6, -6-6j, -1.5, "x", b"x", bytearray(b"x"),
                       (3,), [3], {3: 6},
                       (1,2), [3,4], {5: 6},
                       tuple2((1,2)), tuple3((1,2)), tuple3(range(100)),
                       [3,4], list2([3,4]), list3([3,4]), list3(range(100)),
                       set({7}), set2({7}), set3({7}),
                       frozenset({8}), frozenset2({8}), frozenset3({8}),
                       dict2({5: 6}), dict3({5: 6}),
                       {5: 6}.keys(), {5: 6}.values(), {5: 6}.items(),
                       MappingView({5: 6}), KeysView({5: 6}),
                       ItemsView({5: 6}), ValuesView({5: 6}),
                       range(10, -11, -1),
                       True, False, None, ...,
                      ):
            native = repr(simple)
            self.assertEqual(pprint.pformat(simple), native)
            self.assertEqual(pprint.pformat(simple, width=1, indent=0)
                             .replace('\n', ' '), native)
            self.assertEqual(pprint.pformat(simple, underscore_numbers=True), native)
            self.assertEqual(pprint.saferepr(simple), native)

    def test_container_repr_override_called(self):
        N = 1000
        # Ensure that __repr__ override is called for subclasses of containers

        for cont in (list_custom_repr(),
                     list_custom_repr([1,2,3]),
                     list_custom_repr(range(N)),
                     tuple_custom_repr(),
                     tuple_custom_repr([1,2,3]),
                     tuple_custom_repr(range(N)),
                     set_custom_repr(),
                     set_custom_repr([1,2,3]),
                     set_custom_repr(range(N)),
                     frozenset_custom_repr(),
                     frozenset_custom_repr([1,2,3]),
                     frozenset_custom_repr(range(N)),
                     dict_custom_repr(),
                     dict_custom_repr({5: 6}),
                     dict_custom_repr(zip(range(N),range(N))),
                     mappingview_custom_repr({}),
                     mappingview_custom_repr({5: 6}),
                     mappingview_custom_repr(dict(zip(range(N),range(N)))),
                     keysview_custom_repr({}),
                     keysview_custom_repr({5: 6}),
                     keysview_custom_repr(dict(zip(range(N),range(N)))),
                    ):
            native = repr(cont)
            expected = '*' * len(native)
            self.assertEqual(pprint.pformat(cont), expected)
            self.assertEqual(pprint.pformat(cont, width=1, indent=0), expected)
            self.assertEqual(pprint.saferepr(cont), expected)

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
        exp = 'dict_keys([%s])' % ',\n '.join(map(str, o))
        keys = dict.fromkeys(o).keys()
        self.assertEqual(pprint.pformat(keys), exp)

        o = range(100)
        exp = 'dict_values([%s])' % ',\n '.join(map(str, o))
        values = {v: v for v in o}.values()
        self.assertEqual(pprint.pformat(values), exp)

        o = range(100)
        exp = 'dict_items([%s])' % ',\n '.join("(%s, %s)" % (i, i) for i in o)
        items = {v: v for v in o}.items()
        self.assertEqual(pprint.pformat(items), exp)

        o = range(100)
        exp = 'odict_keys([%s])' % ',\n '.join(map(str, o))
        keys = collections.OrderedDict.fromkeys(o).keys()
        self.assertEqual(pprint.pformat(keys), exp)

        o = range(100)
        exp = 'odict_values([%s])' % ',\n '.join(map(str, o))
        values = collections.OrderedDict({v: v for v in o}).values()
        self.assertEqual(pprint.pformat(values), exp)

        o = range(100)
        exp = 'odict_items([%s])' % ',\n '.join("(%s, %s)" % (i, i) for i in o)
        items = collections.OrderedDict({v: v for v in o}).items()
        self.assertEqual(pprint.pformat(items), exp)

        o = range(100)
        exp = 'KeysView({%s})' % (': None,\n '.join(map(str, o)) + ': None')
        keys_view = KeysView(dict.fromkeys(o))
        self.assertEqual(pprint.pformat(keys_view), exp)

        o = range(100)
        exp = 'ItemsView({%s})' % (': None,\n '.join(map(str, o)) + ': None')
        items_view = ItemsView(dict.fromkeys(o))
        self.assertEqual(pprint.pformat(items_view), exp)

        o = range(100)
        exp = 'MappingView({%s})' % (': None,\n '.join(map(str, o)) + ': None')
        mapping_view = MappingView(dict.fromkeys(o))
        self.assertEqual(pprint.pformat(mapping_view), exp)

        o = range(100)
        exp = 'ValuesView({%s})' % (': None,\n '.join(map(str, o)) + ': None')
        values_view = ValuesView(dict.fromkeys(o))
        self.assertEqual(pprint.pformat(values_view), exp)

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
    {'first': 1, 'second': 2, 'third': 3}]"""
        self.assertEqual(pprint.pformat(o, indent=4, width=42), expected)
        expected = """\
[   [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
    {   'first': 1,
        'second': 2,
        'third': 3}]"""
        self.assertEqual(pprint.pformat(o, indent=4, width=41), expected)

    def test_width(self):
        expected = """\
[[[[[[1, 2, 3],
     '1 2']]]],
 {1: [1, 2, 3],
  2: [12, 34]},
 'abc def ghi',
 ('ab cd ef',),
 set2({1, 23}),
 [[[[[1, 2, 3],
     '1 2']]]]]"""
        o = eval(expected)
        self.assertEqual(pprint.pformat(o, width=15), expected)
        self.assertEqual(pprint.pformat(o, width=16), expected)
        self.assertEqual(pprint.pformat(o, width=25), expected)
        self.assertEqual(pprint.pformat(o, width=14), """\
[[[[[[1,
      2,
      3],
     '1 '
     '2']]]],
 {1: [1,
      2,
      3],
  2: [12,
      34]},
 'abc def '
 'ghi',
 ('ab cd '
  'ef',),
 set2({1,
       23}),
 [[[[[1,
      2,
      3],
     '1 '
     '2']]]]]""")

    def test_integer(self):
        self.assertEqual(pprint.pformat(1234567), '1234567')
        self.assertEqual(pprint.pformat(1234567, underscore_numbers=True), '1_234_567')

        class Temperature(int):
            def __new__(cls, celsius_degrees):
                return super().__new__(Temperature, celsius_degrees)
            def __repr__(self):
                kelvin_degrees = self + 273.15
                return f"{kelvin_degrees}°K"
        self.assertEqual(pprint.pformat(Temperature(1000)), '1273.15°K')

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

    def test_sort_dict(self):
        d = dict.fromkeys('cba')
        self.assertEqual(pprint.pformat(d, sort_dicts=False), "{'c': None, 'b': None, 'a': None}")
        self.assertEqual(pprint.pformat([d, d], sort_dicts=False),
            "[{'c': None, 'b': None, 'a': None}, {'c': None, 'b': None, 'a': None}]")

    def test_ordered_dict(self):
        d = collections.OrderedDict()
        self.assertEqual(pprint.pformat(d, width=1), 'OrderedDict()')
        d = collections.OrderedDict([])
        self.assertEqual(pprint.pformat(d, width=1), 'OrderedDict()')
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.OrderedDict(zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
OrderedDict([('the', 0),
             ('quick', 1),
             ('brown', 2),
             ('fox', 3),
             ('jumped', 4),
             ('over', 5),
             ('a', 6),
             ('lazy', 7),
             ('dog', 8)])""")
        self.assertEqual(pprint.pformat(d.keys(), sort_dicts=False),
"""\
odict_keys(['the',
 'quick',
 'brown',
 'fox',
 'jumped',
 'over',
 'a',
 'lazy',
 'dog'])""")
        self.assertEqual(pprint.pformat(d.items(), sort_dicts=False),
"""\
odict_items([('the', 0),
 ('quick', 1),
 ('brown', 2),
 ('fox', 3),
 ('jumped', 4),
 ('over', 5),
 ('a', 6),
 ('lazy', 7),
 ('dog', 8)])""")
        self.assertEqual(pprint.pformat(d.values(), sort_dicts=False),
                         "odict_values([0, 1, 2, 3, 4, 5, 6, 7, 8])")

    def test_mapping_proxy(self):
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = dict(zip(words, itertools.count()))
        m = types.MappingProxyType(d)
        self.assertEqual(pprint.pformat(m), """\
mappingproxy({'a': 6,
              'brown': 2,
              'dog': 8,
              'fox': 3,
              'jumped': 4,
              'lazy': 7,
              'over': 5,
              'quick': 1,
              'the': 0})""")
        d = collections.OrderedDict(zip(words, itertools.count()))
        m = types.MappingProxyType(d)
        self.assertEqual(pprint.pformat(m), """\
mappingproxy(OrderedDict([('the', 0),
                          ('quick', 1),
                          ('brown', 2),
                          ('fox', 3),
                          ('jumped', 4),
                          ('over', 5),
                          ('a', 6),
                          ('lazy', 7),
                          ('dog', 8)]))""")

    def test_dict_views(self):
        for dict_class in (dict, collections.OrderedDict, collections.Counter):
            empty = dict_class({})
            short = dict_class(dict(zip('edcba', 'edcba')))
            long = dict_class(dict((chr(x), chr(x)) for x in range(90, 64, -1)))
            lengths = {"empty": empty, "short": short, "long": long}
            prefix = "odict" if dict_class is collections.OrderedDict else "dict"
            for name, d in lengths.items():
                with self.subTest(length=name, prefix=prefix):
                    is_short = len(d) < 6
                    joiner = ", " if is_short else ",\n "
                    k = d.keys()
                    v = d.values()
                    i = d.items()
                    self.assertEqual(pprint.pformat(k, sort_dicts=True),
                                     prefix + "_keys([%s])" %
                                     joiner.join(repr(key) for key in sorted(k)))
                    self.assertEqual(pprint.pformat(v, sort_dicts=True),
                                     prefix + "_values([%s])" %
                                     joiner.join(repr(val) for val in sorted(v)))
                    self.assertEqual(pprint.pformat(i, sort_dicts=True),
                                     prefix + "_items([%s])" %
                                     joiner.join(repr(item) for item in sorted(i)))
                    self.assertEqual(pprint.pformat(k, sort_dicts=False),
                                     prefix + "_keys([%s])" %
                                     joiner.join(repr(key) for key in k))
                    self.assertEqual(pprint.pformat(v, sort_dicts=False),
                                     prefix + "_values([%s])" %
                                     joiner.join(repr(val) for val in v))
                    self.assertEqual(pprint.pformat(i, sort_dicts=False),
                                     prefix + "_items([%s])" %
                                     joiner.join(repr(item) for item in i))

    def test_abc_views(self):
        empty = {}
        short = dict(zip('edcba', 'edcba'))
        long = dict((chr(x), chr(x)) for x in range(90, 64, -1))
        lengths = {"empty": empty, "short": short, "long": long}
        # Test that a subclass that doesn't replace __repr__ works with different lengths
        class MV(MappingView): pass

        for name, d in lengths.items():
            with self.subTest(length=name, name="Views"):
                is_short = len(d) < 6
                joiner = ", " if is_short else ",\n "
                i = d.items()
                s = sorted(i)
                joined_items = "({%s})" % joiner.join(["%r: %r" % (k, v) for (k, v) in i])
                sorted_items = "({%s})" % joiner.join(["%r: %r" % (k, v) for (k, v) in s])
                self.assertEqual(pprint.pformat(KeysView(d), sort_dicts=True),
                                 KeysView.__name__ + sorted_items)
                self.assertEqual(pprint.pformat(ItemsView(d), sort_dicts=True),
                                 ItemsView.__name__ + sorted_items)
                self.assertEqual(pprint.pformat(MappingView(d), sort_dicts=True),
                                 MappingView.__name__ + sorted_items)
                self.assertEqual(pprint.pformat(MV(d), sort_dicts=True),
                                 MV.__name__ + sorted_items)
                self.assertEqual(pprint.pformat(ValuesView(d), sort_dicts=True),
                                 ValuesView.__name__ + sorted_items)
                self.assertEqual(pprint.pformat(KeysView(d), sort_dicts=False),
                                 KeysView.__name__ + joined_items)
                self.assertEqual(pprint.pformat(ItemsView(d), sort_dicts=False),
                                 ItemsView.__name__ + joined_items)
                self.assertEqual(pprint.pformat(MappingView(d), sort_dicts=False),
                                 MappingView.__name__ + joined_items)
                self.assertEqual(pprint.pformat(MV(d), sort_dicts=False),
                                 MV.__name__ + joined_items)
                self.assertEqual(pprint.pformat(ValuesView(d), sort_dicts=False),
                                 ValuesView.__name__ + joined_items)

    def test_nested_views(self):
        d = {1: MappingView({1: MappingView({1: MappingView({1: 2})})})}
        self.assertEqual(repr(d),
                         "{1: MappingView({1: MappingView({1: MappingView({1: 2})})})}")
        self.assertEqual(pprint.pformat(d),
                         "{1: MappingView({1: MappingView({1: MappingView({1: 2})})})}")
        self.assertEqual(pprint.pformat(d, depth=2),
                         "{1: MappingView({1: {...}})}")
        d = {}
        d1 = {1: d.values()}
        d2 = {1: d1.values()}
        d3 = {1: d2.values()}
        self.assertEqual(pprint.pformat(d3),
                         "{1: dict_values([dict_values([dict_values([])])])}")
        self.assertEqual(pprint.pformat(d3, depth=2),
                         "{1: dict_values([{...}])}")

    def test_unorderable_items_views(self):
        """Check that views with unorderable items have stable sorting."""
        d = dict((((3+1j), 3), ((1+1j), (1+0j)), (1j, 0j), (500, None), (499, None)))
        iv = ItemsView(d)
        self.assertEqual(pprint.pformat(iv),
                         pprint.pformat(iv))
        self.assertTrue(pprint.pformat(iv).endswith(", 499: None, 500: None})"),
                        pprint.pformat(iv))
        self.assertEqual(pprint.pformat(d.items()),  # Won't be equal unless _safe_tuple
                         pprint.pformat(d.items()))  # is used in _safe_repr
        self.assertTrue(pprint.pformat(d.items()).endswith(", (499, None), (500, None)])"))

    def test_mapping_view_subclass_no_mapping(self):
        class BMV(MappingView):
            def __init__(self, d):
                super().__init__(d)
                self.mapping = self._mapping
                del self._mapping

        self.assertRaises(AttributeError, pprint.pformat, BMV({}))

    def test_mapping_subclass_repr(self):
        """Test that mapping ABC views use their ._mapping's __repr__."""
        class MyMapping(Mapping):
            def __init__(self, keys=None):
                self._keys = {} if keys is None else dict.fromkeys(keys)

            def __getitem__(self, item):
                return self._keys[item]

            def __len__(self):
                return len(self._keys)

            def __iter__(self):
                return iter(self._keys)

            def __repr__(self):
                return f"{self.__class__.__name__}([{', '.join(map(repr, self._keys.keys()))}])"

        m = MyMapping(["test", 1])
        self.assertEqual(repr(m), "MyMapping(['test', 1])")
        short_view_repr = "%s(MyMapping(['test', 1]))"
        self.assertEqual(repr(m.keys()), short_view_repr % "KeysView")
        self.assertEqual(pprint.pformat(m.items()), short_view_repr % "ItemsView")
        self.assertEqual(pprint.pformat(m.keys()), short_view_repr % "KeysView")
        self.assertEqual(pprint.pformat(MappingView(m)), short_view_repr % "MappingView")
        self.assertEqual(pprint.pformat(m.values()), short_view_repr % "ValuesView")

        alpha = "abcdefghijklmnopqrstuvwxyz"
        m = MyMapping(alpha)
        alpha_repr = ", ".join(map(repr, list(alpha)))
        long_view_repr = "%%s(MyMapping([%s]))" % alpha_repr
        self.assertEqual(repr(m), "MyMapping([%s])" % alpha_repr)
        self.assertEqual(repr(m.keys()), long_view_repr % "KeysView")
        self.assertEqual(pprint.pformat(m.items()), long_view_repr % "ItemsView")
        self.assertEqual(pprint.pformat(m.keys()), long_view_repr % "KeysView")
        self.assertEqual(pprint.pformat(MappingView(m)), long_view_repr % "MappingView")
        self.assertEqual(pprint.pformat(m.values()), long_view_repr % "ValuesView")

    def test_empty_simple_namespace(self):
        ns = types.SimpleNamespace()
        formatted = pprint.pformat(ns)
        self.assertEqual(formatted, "namespace()")

    def test_small_simple_namespace(self):
        ns = types.SimpleNamespace(a=1, b=2)
        formatted = pprint.pformat(ns)
        self.assertEqual(formatted, "namespace(a=1, b=2)")

    def test_simple_namespace(self):
        ns = types.SimpleNamespace(
            the=0,
            quick=1,
            brown=2,
            fox=3,
            jumped=4,
            over=5,
            a=6,
            lazy=7,
            dog=8,
        )
        formatted = pprint.pformat(ns, width=60, indent=4)
        self.assertEqual(formatted, """\
namespace(the=0,
          quick=1,
          brown=2,
          fox=3,
          jumped=4,
          over=5,
          a=6,
          lazy=7,
          dog=8)""")

    def test_simple_namespace_subclass(self):
        class AdvancedNamespace(types.SimpleNamespace): pass
        ns = AdvancedNamespace(
            the=0,
            quick=1,
            brown=2,
            fox=3,
            jumped=4,
            over=5,
            a=6,
            lazy=7,
            dog=8,
        )
        formatted = pprint.pformat(ns, width=60)
        self.assertEqual(formatted, """\
AdvancedNamespace(the=0,
                  quick=1,
                  brown=2,
                  fox=3,
                  jumped=4,
                  over=5,
                  a=6,
                  lazy=7,
                  dog=8)""")

    def test_empty_dataclass(self):
        dc = dataclasses.make_dataclass("MyDataclass", ())()
        formatted = pprint.pformat(dc)
        self.assertEqual(formatted, "MyDataclass()")

    def test_small_dataclass(self):
        dc = dataclass1("text", 123)
        formatted = pprint.pformat(dc)
        self.assertEqual(formatted, "dataclass1(field1='text', field2=123, field3=False)")

    def test_larger_dataclass(self):
        dc = dataclass1("some fairly long text", int(1e10), True)
        formatted = pprint.pformat([dc, dc], width=60, indent=4)
        self.assertEqual(formatted, """\
[   dataclass1(field1='some fairly long text',
               field2=10000000000,
               field3=True),
    dataclass1(field1='some fairly long text',
               field2=10000000000,
               field3=True)]""")

    def test_dataclass_with_repr(self):
        dc = dataclass2()
        formatted = pprint.pformat(dc, width=20)
        self.assertEqual(formatted, "custom repr that doesn't fit within pprint width")

    def test_dataclass_no_repr(self):
        dc = dataclass3()
        formatted = pprint.pformat(dc, width=10)
        self.assertRegex(
            formatted,
            fr"<{re.escape(__name__)}.dataclass3 object at \w+>",
        )

    def test_recursive_dataclass(self):
        dc = dataclass4(None)
        dc.a = dc
        formatted = pprint.pformat(dc, width=10)
        self.assertEqual(formatted, """\
dataclass4(a=...,
           b=1)""")

    def test_cyclic_dataclass(self):
        dc5 = dataclass5(None)
        dc6 = dataclass6(None)
        dc5.a = dc6
        dc6.c = dc5
        formatted = pprint.pformat(dc5, width=10)
        self.assertEqual(formatted, """\
dataclass5(a=dataclass6(c=...,
                        d=1),
           b=1)""")

    def test_subclassing(self):
        # length(repr(obj)) > width
        o = {'names with spaces': 'should be presented using repr()',
             'others.should.not.be': 'like.this'}
        exp = """\
{'names with spaces': 'should be presented using repr()',
 others.should.not.be: like.this}"""

        dotted_printer = DottedPrettyPrinter()
        self.assertEqual(dotted_printer.pformat(o), exp)

        # length(repr(obj)) < width
        o1 = ['with space']
        exp1 = "['with space']"
        self.assertEqual(dotted_printer.pformat(o1), exp1)
        o2 = ['without.space']
        exp2 = "[without.space]"
        self.assertEqual(dotted_printer.pformat(o2), exp2)

    def test_set_reprs(self):
        self.assertEqual(pprint.pformat(set()), 'set()')
        self.assertEqual(pprint.pformat(set(range(3))), '{0, 1, 2}')
        self.assertEqual(pprint.pformat(set(range(7)), width=20), '''\
{0,
 1,
 2,
 3,
 4,
 5,
 6}''')
        self.assertEqual(pprint.pformat(set2(range(7)), width=20), '''\
set2({0,
      1,
      2,
      3,
      4,
      5,
      6})''')
        self.assertEqual(pprint.pformat(set3(range(7)), width=20),
                         'set3({0, 1, 2, 3, 4, 5, 6})')

        self.assertEqual(pprint.pformat(frozenset()), 'frozenset()')
        self.assertEqual(pprint.pformat(frozenset(range(3))),
                         'frozenset({0, 1, 2})')
        self.assertEqual(pprint.pformat(frozenset(range(7)), width=20), '''\
frozenset({0,
           1,
           2,
           3,
           4,
           5,
           6})''')
        self.assertEqual(pprint.pformat(frozenset2(range(7)), width=20), '''\
frozenset2({0,
            1,
            2,
            3,
            4,
            5,
            6})''')
        self.assertEqual(pprint.pformat(frozenset3(range(7)), width=20),
                         'frozenset3({0, 1, 2, 3, 4, 5, 6})')

    def test_set_of_sets_reprs(self):
        # This test creates a complex arrangement of frozensets and
        # compares the pretty-printed repr against a string hard-coded in
        # the test.  The hard-coded repr depends on the sort order of
        # frozensets.
        #
        # However, as the docs point out: "Since sets only define
        # partial ordering (subset relationships), the output of the
        # list.sort() method is undefined for lists of sets."
        #
        # >>> frozenset({0}) < frozenset({1})
        # False
        # >>> frozenset({1}) < frozenset({0})
        # False
        #
        # In this test we list all possible invariants of the result
        # for unordered frozensets.
        #
        # This test has a long history, see:
        # - https://github.com/python/cpython/commit/969fe57baa0eb80332990f9cda936a33e13fabef
        # - https://github.com/python/cpython/issues/58115
        # - https://github.com/python/cpython/issues/111147

        import textwrap

        # Single-line, always ordered:
        fs0 = frozenset()
        fs1 = frozenset(('abc', 'xyz'))
        data = frozenset((fs0, fs1))
        self.assertEqual(pprint.pformat(data),
                         'frozenset({%r, %r})' % (fs0, fs1))
        self.assertEqual(pprint.pformat(data), repr(data))

        fs2 = frozenset(('one', 'two'))
        data = {fs2: frozenset((fs0, fs1))}
        self.assertEqual(pprint.pformat(data),
                         "{%r: frozenset({%r, %r})}" % (fs2, fs0, fs1))
        self.assertEqual(pprint.pformat(data), repr(data))

        # Single-line, unordered:
        fs1 = frozenset(("xyz", "qwerty"))
        fs2 = frozenset(("abcd", "spam"))
        fs = frozenset((fs1, fs2))
        self.assertEqual(pprint.pformat(fs), repr(fs))

        # Multiline, unordered:
        def check(res, invariants):
            self.assertIn(res, [textwrap.dedent(i).strip() for i in invariants])

        # Inner-most frozensets are singleline, result is multiline, unordered:
        fs1 = frozenset(('regular string', 'other string'))
        fs2 = frozenset(('third string', 'one more string'))
        check(
            pprint.pformat(frozenset((fs1, fs2))),
            [
                """
                frozenset({%r,
                           %r})
                """ % (fs1, fs2),
                """
                frozenset({%r,
                           %r})
                """ % (fs2, fs1),
            ],
        )

        # Everything is multiline, unordered:
        check(
            pprint.pformat(
                frozenset((
                    frozenset((
                        "xyz very-very long string",
                        "qwerty is also absurdly long",
                    )),
                    frozenset((
                        "abcd is even longer that before",
                        "spam is not so long",
                    )),
                )),
            ),
            [
                """
                frozenset({frozenset({'abcd is even longer that before',
                                      'spam is not so long'}),
                           frozenset({'qwerty is also absurdly long',
                                      'xyz very-very long string'})})
                """,

                """
                frozenset({frozenset({'abcd is even longer that before',
                                      'spam is not so long'}),
                           frozenset({'xyz very-very long string',
                                      'qwerty is also absurdly long'})})
                """,

                """
                frozenset({frozenset({'qwerty is also absurdly long',
                                      'xyz very-very long string'}),
                           frozenset({'abcd is even longer that before',
                                      'spam is not so long'})})
                """,

                """
                frozenset({frozenset({'qwerty is also absurdly long',
                                      'xyz very-very long string'}),
                           frozenset({'spam is not so long',
                                      'abcd is even longer that before'})})
                """,
            ],
        )

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
        self.assertEqual(clean(pprint.pformat(dict.fromkeys(keys).keys())),
            'dict_keys([' + ','.join('%r' % k for k in skeys) + '])')
        self.assertEqual(clean(pprint.pformat(dict.fromkeys(keys).items())),
            'dict_items([' + ','.join('(%r,None)' % k for k in skeys) + '])')

        # Issue 10017: TypeError on user-defined types as dict keys.
        self.assertEqual(pprint.pformat({Unorderable: 0, 1: 0}),
                         '{1: 0, ' + repr(Unorderable) +': 0}')

        # Issue 14998: TypeError on tuples with NoneTypes as dict keys.
        keys = [(1,), (None,)]
        self.assertEqual(pprint.pformat(dict.fromkeys(keys, 0)),
                         '{%r: 0, %r: 0}' % tuple(sorted(keys, key=id)))

    def test_sort_orderable_and_unorderable_values(self):
        # Issue 22721:  sorted pprints is not stable
        a = Unorderable()
        b = Orderable(hash(a))  # should have the same hash value
        # self-test
        self.assertLess(a, b)
        self.assertLess(str(type(b)), str(type(a)))
        self.assertEqual(sorted([b, a]), [a, b])
        self.assertEqual(sorted([a, b]), [a, b])
        # set
        self.assertEqual(pprint.pformat(set([b, a]), width=1),
                         '{%r,\n %r}' % (a, b))
        self.assertEqual(pprint.pformat(set([a, b]), width=1),
                         '{%r,\n %r}' % (a, b))
        # dict
        self.assertEqual(pprint.pformat(dict.fromkeys([b, a]), width=1),
                         '{%r: None,\n %r: None}' % (a, b))
        self.assertEqual(pprint.pformat(dict.fromkeys([a, b]), width=1),
                         '{%r: None,\n %r: None}' % (a, b))

    def test_str_wrap(self):
        # pprint tries to wrap strings intelligently
        fox = 'the quick brown fox jumped over a lazy dog'
        self.assertEqual(pprint.pformat(fox, width=19), """\
('the quick brown '
 'fox jumped over '
 'a lazy dog')""")
        self.assertEqual(pprint.pformat({'a': 1, 'b': fox, 'c': 2},
                                        width=25), """\
{'a': 1,
 'b': 'the quick brown '
      'fox jumped over '
      'a lazy dog',
 'c': 2}""")
        # With some special characters
        # - \n always triggers a new line in the pprint
        # - \t and \n are escaped
        # - non-ASCII is allowed
        # - an apostrophe doesn't disrupt the pprint
        special = "Portons dix bons \"whiskys\"\nà l'avocat goujat\t qui fumait au zoo"
        self.assertEqual(pprint.pformat(special, width=68), repr(special))
        self.assertEqual(pprint.pformat(special, width=31), """\
('Portons dix bons "whiskys"\\n'
 "à l'avocat goujat\\t qui "
 'fumait au zoo')""")
        self.assertEqual(pprint.pformat(special, width=20), """\
('Portons dix bons '
 '"whiskys"\\n'
 "à l'avocat "
 'goujat\\t qui '
 'fumait au zoo')""")
        self.assertEqual(pprint.pformat([[[[[special]]]]], width=35), """\
[[[[['Portons dix bons "whiskys"\\n'
     "à l'avocat goujat\\t qui "
     'fumait au zoo']]]]]""")
        self.assertEqual(pprint.pformat([[[[[special]]]]], width=25), """\
[[[[['Portons dix bons '
     '"whiskys"\\n'
     "à l'avocat "
     'goujat\\t qui '
     'fumait au zoo']]]]]""")
        self.assertEqual(pprint.pformat([[[[[special]]]]], width=23), """\
[[[[['Portons dix '
     'bons "whiskys"\\n'
     "à l'avocat "
     'goujat\\t qui '
     'fumait au '
     'zoo']]]]]""")
        # An unwrappable string is formatted as its repr
        unwrappable = "x" * 100
        self.assertEqual(pprint.pformat(unwrappable, width=80), repr(unwrappable))
        self.assertEqual(pprint.pformat(''), "''")
        # Check that the pprint is a usable repr
        special *= 10
        for width in range(3, 40):
            formatted = pprint.pformat(special, width=width)
            self.assertEqual(eval(formatted), special)
            formatted = pprint.pformat([special] * 2, width=width)
            self.assertEqual(eval(formatted), [special] * 2)

    def test_compact(self):
        o = ([list(range(i * i)) for i in range(5)] +
             [list(range(i)) for i in range(6)])
        expected = """\
[[], [0], [0, 1, 2, 3],
 [0, 1, 2, 3, 4, 5, 6, 7, 8],
 [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  14, 15],
 [], [0], [0, 1], [0, 1, 2], [0, 1, 2, 3],
 [0, 1, 2, 3, 4]]"""
        self.assertEqual(pprint.pformat(o, width=47, compact=True), expected)

    def test_compact_width(self):
        levels = 20
        number = 10
        o = [0] * number
        for i in range(levels - 1):
            o = [o]
        for w in range(levels * 2 + 1, levels + 3 * number - 1):
            lines = pprint.pformat(o, width=w, compact=True).splitlines()
            maxwidth = max(map(len, lines))
            self.assertLessEqual(maxwidth, w)
            self.assertGreater(maxwidth, w - 3)

    def test_bytes_wrap(self):
        self.assertEqual(pprint.pformat(b'', width=1), "b''")
        self.assertEqual(pprint.pformat(b'abcd', width=1), "b'abcd'")
        letters = b'abcdefghijklmnopqrstuvwxyz'
        self.assertEqual(pprint.pformat(letters, width=29), repr(letters))
        self.assertEqual(pprint.pformat(letters, width=19), """\
(b'abcdefghijkl'
 b'mnopqrstuvwxyz')""")
        self.assertEqual(pprint.pformat(letters, width=18), """\
(b'abcdefghijkl'
 b'mnopqrstuvwx'
 b'yz')""")
        self.assertEqual(pprint.pformat(letters, width=16), """\
(b'abcdefghijkl'
 b'mnopqrstuvwx'
 b'yz')""")
        special = bytes(range(16))
        self.assertEqual(pprint.pformat(special, width=61), repr(special))
        self.assertEqual(pprint.pformat(special, width=48), """\
(b'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b'
 b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat(special, width=32), """\
(b'\\x00\\x01\\x02\\x03'
 b'\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b'
 b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat(special, width=1), """\
(b'\\x00\\x01\\x02\\x03'
 b'\\x04\\x05\\x06\\x07'
 b'\\x08\\t\\n\\x0b'
 b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat({'a': 1, 'b': letters, 'c': 2},
                                        width=21), """\
{'a': 1,
 'b': b'abcdefghijkl'
      b'mnopqrstuvwx'
      b'yz',
 'c': 2}""")
        self.assertEqual(pprint.pformat({'a': 1, 'b': letters, 'c': 2},
                                        width=20), """\
{'a': 1,
 'b': b'abcdefgh'
      b'ijklmnop'
      b'qrstuvwxyz',
 'c': 2}""")
        self.assertEqual(pprint.pformat([[[[[[letters]]]]]], width=25), """\
[[[[[[b'abcdefghijklmnop'
      b'qrstuvwxyz']]]]]]""")
        self.assertEqual(pprint.pformat([[[[[[special]]]]]], width=41), """\
[[[[[[b'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07'
      b'\\x08\\t\\n\\x0b\\x0c\\r\\x0e\\x0f']]]]]]""")
        # Check that the pprint is a usable repr
        for width in range(1, 64):
            formatted = pprint.pformat(special, width=width)
            self.assertEqual(eval(formatted), special)
            formatted = pprint.pformat([special] * 2, width=width)
            self.assertEqual(eval(formatted), [special] * 2)

    def test_bytearray_wrap(self):
        self.assertEqual(pprint.pformat(bytearray(), width=1), "bytearray(b'')")
        letters = bytearray(b'abcdefghijklmnopqrstuvwxyz')
        self.assertEqual(pprint.pformat(letters, width=40), repr(letters))
        self.assertEqual(pprint.pformat(letters, width=28), """\
bytearray(b'abcdefghijkl'
          b'mnopqrstuvwxyz')""")
        self.assertEqual(pprint.pformat(letters, width=27), """\
bytearray(b'abcdefghijkl'
          b'mnopqrstuvwx'
          b'yz')""")
        self.assertEqual(pprint.pformat(letters, width=25), """\
bytearray(b'abcdefghijkl'
          b'mnopqrstuvwx'
          b'yz')""")
        special = bytearray(range(16))
        self.assertEqual(pprint.pformat(special, width=72), repr(special))
        self.assertEqual(pprint.pformat(special, width=57), """\
bytearray(b'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b'
          b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat(special, width=41), """\
bytearray(b'\\x00\\x01\\x02\\x03'
          b'\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b'
          b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat(special, width=1), """\
bytearray(b'\\x00\\x01\\x02\\x03'
          b'\\x04\\x05\\x06\\x07'
          b'\\x08\\t\\n\\x0b'
          b'\\x0c\\r\\x0e\\x0f')""")
        self.assertEqual(pprint.pformat({'a': 1, 'b': letters, 'c': 2},
                                        width=31), """\
{'a': 1,
 'b': bytearray(b'abcdefghijkl'
                b'mnopqrstuvwx'
                b'yz'),
 'c': 2}""")
        self.assertEqual(pprint.pformat([[[[[letters]]]]], width=37), """\
[[[[[bytearray(b'abcdefghijklmnop'
               b'qrstuvwxyz')]]]]]""")
        self.assertEqual(pprint.pformat([[[[[special]]]]], width=50), """\
[[[[[bytearray(b'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07'
               b'\\x08\\t\\n\\x0b\\x0c\\r\\x0e\\x0f')]]]]]""")

    def test_default_dict(self):
        d = collections.defaultdict(int)
        self.assertEqual(pprint.pformat(d, width=1), "defaultdict(<class 'int'>, {})")
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.defaultdict(int, zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
defaultdict(<class 'int'>,
            {'a': 6,
             'brown': 2,
             'dog': 8,
             'fox': 3,
             'jumped': 4,
             'lazy': 7,
             'over': 5,
             'quick': 1,
             'the': 0})""")

    def test_counter(self):
        d = collections.Counter()
        self.assertEqual(pprint.pformat(d, width=1), "Counter()")
        d = collections.Counter('senselessness')
        self.assertEqual(pprint.pformat(d, width=40),
"""\
Counter({'s': 6,
         'e': 4,
         'n': 2,
         'l': 1})""")

    def test_chainmap(self):
        d = collections.ChainMap()
        self.assertEqual(pprint.pformat(d, width=1), "ChainMap({})")
        words = 'the quick brown fox jumped over a lazy dog'.split()
        items = list(zip(words, itertools.count()))
        d = collections.ChainMap(dict(items))
        self.assertEqual(pprint.pformat(d),
"""\
ChainMap({'a': 6,
          'brown': 2,
          'dog': 8,
          'fox': 3,
          'jumped': 4,
          'lazy': 7,
          'over': 5,
          'quick': 1,
          'the': 0})""")
        d = collections.ChainMap(dict(items), collections.OrderedDict(items))
        self.assertEqual(pprint.pformat(d),
"""\
ChainMap({'a': 6,
          'brown': 2,
          'dog': 8,
          'fox': 3,
          'jumped': 4,
          'lazy': 7,
          'over': 5,
          'quick': 1,
          'the': 0},
         OrderedDict([('the', 0),
                      ('quick', 1),
                      ('brown', 2),
                      ('fox', 3),
                      ('jumped', 4),
                      ('over', 5),
                      ('a', 6),
                      ('lazy', 7),
                      ('dog', 8)]))""")
        self.assertEqual(pprint.pformat(d.keys()),
"""\
KeysView(ChainMap({'a': 6,
          'brown': 2,
          'dog': 8,
          'fox': 3,
          'jumped': 4,
          'lazy': 7,
          'over': 5,
          'quick': 1,
          'the': 0},
         OrderedDict([('the', 0),
                      ('quick', 1),
                      ('brown', 2),
                      ('fox', 3),
                      ('jumped', 4),
                      ('over', 5),
                      ('a', 6),
                      ('lazy', 7),
                      ('dog', 8)])))""")
        self.assertEqual(pprint.pformat(d.items()),
 """\
ItemsView(ChainMap({'a': 6,
          'brown': 2,
          'dog': 8,
          'fox': 3,
          'jumped': 4,
          'lazy': 7,
          'over': 5,
          'quick': 1,
          'the': 0},
         OrderedDict([('the', 0),
                      ('quick', 1),
                      ('brown', 2),
                      ('fox', 3),
                      ('jumped', 4),
                      ('over', 5),
                      ('a', 6),
                      ('lazy', 7),
                      ('dog', 8)])))""")
        self.assertEqual(pprint.pformat(d.values()),
 """\
ValuesView(ChainMap({'a': 6,
          'brown': 2,
          'dog': 8,
          'fox': 3,
          'jumped': 4,
          'lazy': 7,
          'over': 5,
          'quick': 1,
          'the': 0},
         OrderedDict([('the', 0),
                      ('quick', 1),
                      ('brown', 2),
                      ('fox', 3),
                      ('jumped', 4),
                      ('over', 5),
                      ('a', 6),
                      ('lazy', 7),
                      ('dog', 8)])))""")

    def test_deque(self):
        d = collections.deque()
        self.assertEqual(pprint.pformat(d, width=1), "deque([])")
        d = collections.deque(maxlen=7)
        self.assertEqual(pprint.pformat(d, width=1), "deque([], maxlen=7)")
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.deque(zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
deque([('the', 0),
       ('quick', 1),
       ('brown', 2),
       ('fox', 3),
       ('jumped', 4),
       ('over', 5),
       ('a', 6),
       ('lazy', 7),
       ('dog', 8)])""")
        d = collections.deque(zip(words, itertools.count()), maxlen=7)
        self.assertEqual(pprint.pformat(d),
"""\
deque([('brown', 2),
       ('fox', 3),
       ('jumped', 4),
       ('over', 5),
       ('a', 6),
       ('lazy', 7),
       ('dog', 8)],
      maxlen=7)""")

    def test_user_dict(self):
        d = collections.UserDict()
        self.assertEqual(pprint.pformat(d, width=1), "{}")
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.UserDict(zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
{'a': 6,
 'brown': 2,
 'dog': 8,
 'fox': 3,
 'jumped': 4,
 'lazy': 7,
 'over': 5,
 'quick': 1,
 'the': 0}""")
        self.assertEqual(pprint.pformat(d.keys()), """\
KeysView({'a': 6,
 'brown': 2,
 'dog': 8,
 'fox': 3,
 'jumped': 4,
 'lazy': 7,
 'over': 5,
 'quick': 1,
 'the': 0})""")
        self.assertEqual(pprint.pformat(d.items()), """\
ItemsView({'a': 6,
 'brown': 2,
 'dog': 8,
 'fox': 3,
 'jumped': 4,
 'lazy': 7,
 'over': 5,
 'quick': 1,
 'the': 0})""")
        self.assertEqual(pprint.pformat(d.values()), """\
ValuesView({'a': 6,
 'brown': 2,
 'dog': 8,
 'fox': 3,
 'jumped': 4,
 'lazy': 7,
 'over': 5,
 'quick': 1,
 'the': 0})""")

    def test_user_list(self):
        d = collections.UserList()
        self.assertEqual(pprint.pformat(d, width=1), "[]")
        words = 'the quick brown fox jumped over a lazy dog'.split()
        d = collections.UserList(zip(words, itertools.count()))
        self.assertEqual(pprint.pformat(d),
"""\
[('the', 0),
 ('quick', 1),
 ('brown', 2),
 ('fox', 3),
 ('jumped', 4),
 ('over', 5),
 ('a', 6),
 ('lazy', 7),
 ('dog', 8)]""")

    def test_user_string(self):
        d = collections.UserString('')
        self.assertEqual(pprint.pformat(d, width=1), "''")
        d = collections.UserString('the quick brown fox jumped over a lazy dog')
        self.assertEqual(pprint.pformat(d, width=20),
"""\
('the quick brown '
 'fox jumped over '
 'a lazy dog')""")
        self.assertEqual(pprint.pformat({1: d}, width=20),
"""\
{1: 'the quick '
    'brown fox '
    'jumped over a '
    'lazy dog'}""")


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


if __name__ == "__main__":
    unittest.main()
