import unittest

from test import support
from io import StringIO
from pstats import SortKey
from enum import StrEnum, _test_simple_enum

import pstats
import tempfile
import cProfile
from os import remove, path
from functools import cmp_to_key

class AddCallersTestCase(unittest.TestCase):
    """Tests for pstats.add_callers helper."""

    def test_combine_results(self):
        # pstats.add_callers should combine the call results of both target
        # and source by adding the call time. See issue1269.
        # new format: used by the cProfile module
        target = {"a": (1, 2, 3, 4)}
        source = {"a": (1, 2, 3, 4), "b": (5, 6, 7, 8)}
        new_callers = pstats.add_callers(target, source)
        self.assertEqual(new_callers, {'a': (2, 4, 6, 8), 'b': (5, 6, 7, 8)})
        # old format: used by the profile module
        target = {"a": 1}
        source = {"a": 1, "b": 5}
        new_callers = pstats.add_callers(target, source)
        self.assertEqual(new_callers, {'a': 2, 'b': 5})


class StatsTestCase(unittest.TestCase):
    def setUp(self):
        stats_file = support.findfile('pstats.pck')
        self.stats = pstats.Stats(stats_file)
        to_compile = 'import os'
        self.temp_storage = tempfile.mktemp()
        profiled = compile(to_compile, '<string>', 'exec')
        cProfile.run(profiled, filename=self.temp_storage)

    def tearDown(self):
        remove(self.temp_storage)

    def test_add(self):
        stream = StringIO()
        stats = pstats.Stats(stream=stream)
        stats.add(self.stats, self.stats)

    def test_dump_and_load_works_correctly(self):
        self.stats.dump_stats(filename=self.temp_storage)
        tmp_stats = pstats.Stats(self.temp_storage)
        self.assertEqual(self.stats.stats, tmp_stats.stats)

    def test_load_equivalent_to_init(self):
        empty = pstats.Stats()
        empty.load_stats(self.temp_storage)
        created = pstats.Stats(self.temp_storage)
        self.assertEqual(empty.stats, created.stats)

    def test_loading_wrong_types(self):
        empty = pstats.Stats()
        with self.assertRaises(TypeError):
            empty.load_stats(42)

    def test_sort_stats_int(self):
        valid_args = {-1: 'stdname',
                      0: 'calls',
                      1: 'time',
                      2: 'cumulative'}
        for arg_int, arg_str in valid_args.items():
            self.stats.sort_stats(arg_int)
            self.assertEqual(self.stats.sort_type,
                             self.stats.sort_arg_dict_default[arg_str][-1])

    def test_sort_stats_string(self):
        for sort_name in ['calls', 'ncalls', 'cumtime', 'cumulative',
                    'filename', 'line', 'module', 'name', 'nfl', 'pcalls',
                    'stdname', 'time', 'tottime']:
            self.stats.sort_stats(sort_name)
            self.assertEqual(self.stats.sort_type,
                             self.stats.sort_arg_dict_default[sort_name][-1])

    def test_sort_stats_partial(self):
        sortkey = 'filename'
        for sort_name in ['f', 'fi', 'fil', 'file', 'filen', 'filena',
                           'filenam', 'filename']:
            self.stats.sort_stats(sort_name)
            self.assertEqual(self.stats.sort_type,
                             self.stats.sort_arg_dict_default[sortkey][-1])

    def test_sort_stats_enum(self):
        for member in SortKey:
            self.stats.sort_stats(member)
            self.assertEqual(
                    self.stats.sort_type,
                    self.stats.sort_arg_dict_default[member.value][-1])
        class CheckedSortKey(StrEnum):
            CALLS = 'calls', 'ncalls'
            CUMULATIVE = 'cumulative', 'cumtime'
            FILENAME = 'filename', 'module'
            LINE = 'line'
            NAME = 'name'
            NFL = 'nfl'
            PCALLS = 'pcalls'
            STDNAME = 'stdname'
            TIME = 'time', 'tottime'
            def __new__(cls, *values):
                value = values[0]
                obj = str.__new__(cls, value)
                obj._value_ = value
                for other_value in values[1:]:
                    cls._value2member_map_[other_value] = obj
                obj._all_values = values
                return obj
        _test_simple_enum(CheckedSortKey, SortKey)

    def test_sort_starts_mix(self):
        self.assertRaises(TypeError, self.stats.sort_stats,
                          'calls',
                          SortKey.TIME)
        self.assertRaises(TypeError, self.stats.sort_stats,
                          SortKey.TIME,
                          'calls')

    def test_get_stats_profile(self):
        def pass1(): pass
        def pass2(): pass
        def pass3(): pass

        pr = cProfile.Profile()
        pr.enable()
        pass1()
        pass2()
        pass3()
        pr.create_stats()
        ps = pstats.Stats(pr)

        stats_profile = ps.get_stats_profile()
        funcs_called = set(stats_profile.func_profiles.keys())
        self.assertIn('pass1', funcs_called)
        self.assertIn('pass2', funcs_called)
        self.assertIn('pass3', funcs_called)

    def test_SortKey_enum(self):
        self.assertEqual(SortKey.FILENAME, 'filename')
        self.assertNotEqual(SortKey.FILENAME, SortKey.CALLS)


class TupleCompTestCase(unittest.TestCase):

    def test_tuple_comp_compare_is_correct(self):
        comp_list = [(0, 1), (1, -1), (2, 1)]
        tup = pstats.TupleComp(comp_list)
        to_sort = [(1, 3, 4), (2, 3, 1), (5, 4, 3)]
        desired = [(1, 3, 4), (2, 3, 1), (5, 4, 3)]
        to_sort.sort(key=cmp_to_key(tup.compare))
        self.assertEqual(to_sort, desired)


class UtilsTestCase(unittest.TestCase):
    def test_count_calls(self):
        dic = {
            1: 2, 3: 5
        }
        dic_null = {
            1: 0, 2: 0
        }
        self.assertEqual(pstats.count_calls(dic), 7)
        self.assertEqual(pstats.count_calls(dic_null), 0)

    def test_f8(self):
        self.assertEqual(pstats.f8(2.3232), '   2.323')
        self.assertEqual(pstats.f8(0), '   0.000')

    def test_func_name(self):
        func = ('file', 10, 'name')
        self.assertEqual(pstats.func_get_function_name(func), 'name')

    def test_strip_path(self):
        func = (path.join('long', 'path'), 10, 'name')
        desired = ('path', 10, 'name')
        self.assertEqual(pstats.func_strip_path(func), desired)


if __name__ == "__main__":
    unittest.main()
