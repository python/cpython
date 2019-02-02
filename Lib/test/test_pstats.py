import unittest
from test import support
from io import StringIO
import pstats
from pstats import SortKey



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

    def test_sort_starts_mix(self):
        self.assertRaises(TypeError, self.stats.sort_stats,
                          'calls',
                          SortKey.TIME)
        self.assertRaises(TypeError, self.stats.sort_stats,
                          SortKey.TIME,
                          'calls')


if __name__ == "__main__":
    unittest.main()
