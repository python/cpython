import unittest

from test import support
from test.support.import_helper import ensure_lazy_imports
from io import StringIO
from pstats import SortKey
from enum import StrEnum, _test_simple_enum

import os
import pstats
import re
import subprocess
import tempfile
import cProfile
from test.support import os_helper, script_helper

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


def _profiled_callee():
    return 1


def _profiled_caller():
    for _ in range(3):
        _profiled_callee()


@support.requires_subprocess()
class CommandLineTests(unittest.TestCase):
    """Tests for the interactive profile browser (``python -m pstats``)."""

    @classmethod
    def setUpClass(cls):
        cls.tmpdir = cls.enterClassContext(os_helper.temp_dir())
        cls.profile = os.path.join(cls.tmpdir, 'test.prof')
        cProfile.runctx('_profiled_caller()', globals(), {},
                        filename=cls.profile)

    def run_browser(self, *args, commands=()):
        """Run ``python -m pstats`` with *args*, feeding *commands* on stdin.

        Return what the browser wrote to stdout.
        """
        proc = script_helper.spawn_python('-m', 'pstats', *args,
                                          stderr=subprocess.PIPE)
        stdin = ''.join(f'{command}\n' for command in commands)
        stdout, stderr = proc.communicate(stdin.encode())
        stdout = stdout.decode('utf-8', 'backslashreplace')
        stderr = stderr.decode('utf-8', 'backslashreplace')
        self.assertEqual(proc.returncode, 0, stderr)
        self.assertEqual(stderr, '')
        return stdout.replace('\r\n', '\n')

    def test_start_and_quit(self):
        stdout = self.run_browser(commands=['quit'])
        self.assertEqual(stdout,
                         'Welcome to the profile statistics browser.\n'
                         '% Goodbye.\n')

    def test_eof(self):
        # Closing stdin leaves the browser, like the "quit" command.
        stdout = self.run_browser(commands=[])
        self.assertEqual(stdout,
                         'Welcome to the profile statistics browser.\n'
                         '% \nGoodbye.\n')

    def test_profile_argument(self):
        stdout = self.run_browser(self.profile, commands=['stats', 'quit'])
        # The prompt shows the name of the loaded file.
        self.assertIn(f'\n{self.profile}% ', stdout)
        # So does the header of the report.
        self.assertIn(f'    {self.profile}\n', stdout)
        self.assertRegex(stdout, r'\n +\d+ function calls in ')
        self.assertRegex(stdout, r'\n +3 +.*\(_profiled_callee\)\n')
        self.assertRegex(stdout, r'\n +1 +.*\(_profiled_caller\)\n')

    def test_multiple_profile_arguments(self):
        # The statistics of all the files given on the command line
        # are added together.
        stdout = self.run_browser(self.profile, self.profile,
                                  commands=['stats', 'quit'])
        self.assertEqual(stdout.count(f'    {self.profile}\n'), 2)
        self.assertRegex(stdout, r'\n +6 +.*\(_profiled_callee\)\n')

    def test_missing_profile_argument(self):
        missing = os.path.join(self.tmpdir, 'missing.prof')
        stdout = self.run_browser(missing, commands=['stats', 'quit'])
        self.assertIn('No such file or directory\n', stdout)
        self.assertIn('\n% No statistics object is loaded.\n', stdout)

    def test_commands_without_statistics(self):
        for command in ('stats', 'callers', 'callees', 'sort calls',
                        'strip', 'reverse', f'add {self.profile}'):
            with self.subTest(command=command):
                stdout = self.run_browser(commands=[command, 'quit'])
                self.assertIn('\n% No statistics object is loaded.\n',
                              stdout)
        stdout = self.run_browser(commands=['read', 'quit'])
        self.assertIn(
            '\n% No statistics object is current -- cannot reload.\n', stdout)

    def test_read(self):
        stdout = self.run_browser(commands=[f'read {self.profile}',
                                            'stats', 'quit'])
        self.assertIn(f'\n{self.profile}% ', stdout)
        self.assertRegex(stdout, r'\n +3 +.*\(_profiled_callee\)\n')

    def test_read_errors(self):
        missing = os.path.join(self.tmpdir, 'missing.prof')
        stdout = self.run_browser(commands=[f'read {missing}', 'quit'])
        # The error is reported and no file is loaded.
        self.assertEqual(stdout,
                         'Welcome to the profile statistics browser.\n'
                         '% No such file or directory\n'
                         '% Goodbye.\n')

        empty = os.path.join(self.tmpdir, 'empty.prof')
        os_helper.create_empty_file(empty)
        stdout = self.run_browser(commands=[f'read {empty}', 'quit'])
        self.assertIn('\n% EOFError: ', stdout)
        self.assertIn('\n% Goodbye.\n', stdout)

    def test_add(self):
        stdout = self.run_browser(self.profile,
                                  commands=[f'add {self.profile}',
                                            'stats', 'quit'])
        self.assertEqual(stdout.count(f'    {self.profile}\n'), 2)
        self.assertRegex(stdout, r'\n +6 +.*\(_profiled_callee\)\n')

    def test_add_errors(self):
        missing = os.path.join(self.tmpdir, 'missing.prof')
        stdout = self.run_browser(self.profile,
                                  commands=[f'add {missing}', 'quit'])
        self.assertIn(f'% Failed to load statistics for {missing}: ', stdout)

    def test_stats_restrictions(self):
        stdout = self.run_browser(self.profile,
                                  commands=['stats 2', 'stats 0.5',
                                            'stats _profiled_callee', 'quit'])
        self.assertRegex(stdout,
                         r'List reduced from \d+ to 2 due to restriction <2>')
        self.assertRegex(stdout, r'List reduced from \d+ to \d+ '
                                 r'due to restriction <0\.5>')
        self.assertRegex(stdout,
                         r"List reduced from \d+ to 1 due to restriction "
                         r"<'_profiled_callee'>")
        by_name = stdout.split("<'_profiled_callee'>")[1]
        self.assertIn('(_profiled_callee)', by_name)
        self.assertNotIn('(_profiled_caller)', by_name)

    def test_stats_invalid_fraction(self):
        stdout = self.run_browser(self.profile,
                                  commands=['stats 1.5', 'quit'])
        self.assertIn('% Fraction argument must be in [0, 1]\n', stdout)
        # The invalid argument is ignored and the full report is printed.
        self.assertIn('(_profiled_caller)', stdout)
        self.assertIn('(_profiled_callee)', stdout)

    def test_sort(self):
        # Unique prefixes of the sort keys are accepted.
        stdout = self.run_browser(self.profile,
                                  commands=['sort calls', 'stats',
                                            'sort cumu', 'stats', 'quit'])
        self.assertIn('Ordered by: call count', stdout)
        self.assertIn('Ordered by: cumulative time', stdout)
        by_calls = stdout.split('Ordered by: ')[1]
        self.assertLess(by_calls.index('(_profiled_callee)'),
                        by_calls.index('(_profiled_caller)'))

    def test_sort_without_valid_keys(self):
        for command in ('sort', 'sort bogus'):
            with self.subTest(command=command):
                stdout = self.run_browser(self.profile,
                                          commands=[command, 'quit'])
                self.assertIn(
                    'Valid sort keys (unique prefixes are accepted):', stdout)
                self.assertIn('\ncalls -- call count\n', stdout)
                self.assertIn('\ncumulative -- cumulative time\n', stdout)

    def test_reverse(self):
        stdout = self.run_browser(self.profile,
                                  commands=['sort calls', 'reverse',
                                            'stats', 'quit'])
        self.assertIn('Ordered by: call count', stdout)
        self.assertLess(stdout.index('(_profiled_caller)'),
                        stdout.index('(_profiled_callee)'))

    def test_strip(self):
        filename = _profiled_callee.__code__.co_filename
        basename = os.path.basename(filename)
        stdout = self.run_browser(self.profile,
                                  commands=['stats _profiled_callee', 'strip',
                                            'stats _profiled_callee', 'quit'])
        before, after = stdout.split('due to restriction')[1:]
        self.assertIn(f'{filename}:', before)
        # Only the base name is left after "strip".
        self.assertRegex(
            after, r' ' + re.escape(basename) + r':\d+\(_profiled_callee\)')

    def test_read_reloads_current_file(self):
        # "read" without an argument reloads the current file, which
        # undoes "strip".
        filename = _profiled_callee.__code__.co_filename
        stdout = self.run_browser(self.profile,
                                  commands=['strip', 'read',
                                            'stats _profiled_callee', 'quit'])
        self.assertIn(f'{filename}:', stdout.split('due to restriction')[1])

    def test_callers_and_callees(self):
        stdout = self.run_browser(self.profile,
                                  commands=['callers _profiled_callee',
                                            'callees _profiled_caller',
                                            'quit'])
        self.assertIn('was called by...', stdout)
        self.assertRegex(
            stdout, r'\(_profiled_callee\) +<- +3 .*\(_profiled_caller\)\n')
        self.assertIn('called...', stdout)
        self.assertRegex(
            stdout, r'\(_profiled_caller\) +-> +3 .*\(_profiled_callee\)\n')

    def test_help(self):
        stdout = self.run_browser(commands=['help', 'help stats', 'quit'])
        lines = stdout.splitlines()
        header = 'Documented commands (type help <topic>):'
        self.assertIn(header, lines)
        commands = lines[lines.index(header) + 2]
        self.assertEqual(commands.split(),
                         ['EOF', 'add', 'callees', 'callers', 'help', 'quit',
                          'read', 'reverse', 'sort', 'stats', 'strip'])
        self.assertIn('\n% Print statistics from the current stat object.\n',
                      stdout)
        self.assertIn('\nArguments may be:\n', stdout)


if __name__ == "__main__":
    unittest.main()
