import unittest

from test import support
from test.support.import_helper import ensure_lazy_imports
from io import StringIO
from pstats import SortKey
from enum import StrEnum, _test_simple_enum

import os
import pstats
import tempfile
import cProfile

class LazyImportTest(unittest.TestCase):
    @support.cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("pstats", {"typing"})


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

    def test_add(self):
        stream = StringIO()
        stats = pstats.Stats(stream=stream)
        stats.add(self.stats, self.stats)

    def test_dump_and_load_works_correctly(self):
        temp_storage_new = tempfile.NamedTemporaryFile(delete=False)
        try:
            self.stats.dump_stats(filename=temp_storage_new.name)
            tmp_stats = pstats.Stats(temp_storage_new.name)
            self.assertEqual(self.stats.stats, tmp_stats.stats)
        finally:
            temp_storage_new.close()
            os.remove(temp_storage_new.name)

    def test_load_equivalent_to_init(self):
        stats = pstats.Stats()
        self.temp_storage = tempfile.NamedTemporaryFile(delete=False)
        try:
            cProfile.run('import os', filename=self.temp_storage.name)
            stats.load_stats(self.temp_storage.name)
            created = pstats.Stats(self.temp_storage.name)
            self.assertEqual(stats.stats, created.stats)
        finally:
            self.temp_storage.close()
            os.remove(self.temp_storage.name)

    def test_loading_wrong_types(self):
        stats = pstats.Stats()
        with self.assertRaises(TypeError):
            stats.load_stats(42)

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

if __name__ == "__main__":
    unittest.main()
