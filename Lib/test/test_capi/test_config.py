"""
Tests PyConfig_Get() and PyConfig_Set() C API (PEP 741).
"""
import os
import sys
import sysconfig
import types
import unittest
from test import support
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')


# Is the Py_STATS macro defined?
Py_STATS = hasattr(sys, '_stats_on')


class CAPITests(unittest.TestCase):
    def test_config_get(self):
        # Test PyConfig_Get()
        config_get = _testcapi.config_get
        config_names = _testcapi.config_names

        TEST_VALUE = {
            str: "TEST_MARKER_STR",
            str | None: "TEST_MARKER_OPT_STR",
            list[str]: ("TEST_MARKER_STR_TUPLE",),
            dict[str, str | bool]: {"x": "value", "y": True},
        }

        # read config options and check their type
        options = [
            ("allocator", int, None),
            ("argv", list[str], "argv"),
            ("base_exec_prefix", str | None, "base_exec_prefix"),
            ("base_executable", str | None, "_base_executable"),
            ("base_prefix", str | None, "base_prefix"),
            ("buffered_stdio", bool, None),
            ("bytes_warning", int, None),
            ("check_hash_pycs_mode", str, None),
            ("code_debug_ranges", bool, None),
            ("configure_c_stdio", bool, None),
            ("coerce_c_locale", bool, None),
            ("coerce_c_locale_warn", bool, None),
            ("configure_locale", bool, None),
            ("cpu_count", int, None),
            ("dev_mode", bool, None),
            ("dump_refs", bool, None),
            ("dump_refs_file", str | None, None),
            ("exec_prefix", str | None, "exec_prefix"),
            ("executable", str | None, "executable"),
            ("faulthandler", bool, None),
            ("filesystem_encoding", str, None),
            ("filesystem_errors", str, None),
            ("hash_seed", int, None),
            ("home", str | None, None),
            ("thread_inherit_context", int, None),
            ("context_aware_warnings", int, None),
            ("import_time", int, None),
            ("inspect", bool, None),
            ("install_signal_handlers", bool, None),
            ("int_max_str_digits", int, None),
            ("interactive", bool, None),
            ("isolated", bool, None),
            ("malloc_stats", bool, None),
            ("module_search_paths", list[str], "path"),
            ("optimization_level", int, None),
            ("orig_argv", list[str], "orig_argv"),
            ("parser_debug", bool, None),
            ("parse_argv", bool, None),
            ("pathconfig_warnings", bool, None),
            ("perf_profiling", int, None),
            ("platlibdir", str, "platlibdir"),
            ("prefix", str | None, "prefix"),
            ("program_name", str, None),
            ("pycache_prefix", str | None, "pycache_prefix"),
            ("quiet", bool, None),
            ("remote_debug", int, None),
            ("run_command", str | None, None),
            ("run_filename", str | None, None),
            ("run_module", str | None, None),
            ("safe_path", bool, None),
            ("show_ref_count", bool, None),
            ("site_import", bool, None),
            ("skip_source_first_line", bool, None),
            ("stdio_encoding", str, None),
            ("stdio_errors", str, None),
            ("stdlib_dir", str | None, "_stdlib_dir"),
            ("tracemalloc", int, None),
            ("use_environment", bool, None),
            ("use_frozen_modules", bool, None),
            ("use_hash_seed", bool, None),
            ("user_site_directory", bool, None),
            ("utf8_mode", bool, None),
            ("verbose", int, None),
            ("warn_default_encoding", bool, None),
            ("warnoptions", list[str], "warnoptions"),
            ("write_bytecode", bool, None),
            ("xoptions", dict[str, str | bool], "_xoptions"),
        ]
        if support.Py_DEBUG:
            options.append(("run_presite", str | None, None))
        if support.Py_GIL_DISABLED:
            options.append(("enable_gil", int, None))
            options.append(("tlbc_enabled", int, None))
        if support.MS_WINDOWS:
            options.extend((
                ("legacy_windows_stdio", bool, None),
                ("legacy_windows_fs_encoding", bool, None),
            ))
        if Py_STATS:
            options.extend((
                ("_pystats", bool, None),
            ))
        if support.is_apple:
            options.extend((
                ("use_system_logger", bool, None),
            ))

        for name, option_type, sys_attr in options:
            with self.subTest(name=name, option_type=option_type,
                              sys_attr=sys_attr):
                value = config_get(name)
                if isinstance(option_type, types.GenericAlias):
                    self.assertIsInstance(value, option_type.__origin__)
                    if option_type.__origin__ == dict:
                        key_type = option_type.__args__[0]
                        value_type = option_type.__args__[1]
                        for item in value.items():
                            self.assertIsInstance(item[0], key_type)
                            self.assertIsInstance(item[1], value_type)
                    else:
                        item_type = option_type.__args__[0]
                        for item in value:
                            self.assertIsInstance(item, item_type)
                else:
                    self.assertIsInstance(value, option_type)

                if sys_attr is not None:
                    expected = getattr(sys, sys_attr)
                    self.assertEqual(expected, value)

                    override = TEST_VALUE[option_type]
                    with support.swap_attr(sys, sys_attr, override):
                        self.assertEqual(config_get(name), override)

        # check that the test checks all options
        self.assertEqual(sorted(name for name, option_type, sys_attr in options),
                         sorted(config_names()))

    def test_config_get_sys_flags(self):
        # Test PyConfig_Get()
        config_get = _testcapi.config_get

        # compare config options with sys.flags
        for flag, name, negate in (
            ("debug", "parser_debug", False),
            ("inspect", "inspect", False),
            ("interactive", "interactive", False),
            ("optimize", "optimization_level", False),
            ("dont_write_bytecode", "write_bytecode", True),
            ("no_user_site", "user_site_directory", True),
            ("no_site", "site_import", True),
            ("ignore_environment", "use_environment", True),
            ("verbose", "verbose", False),
            ("bytes_warning", "bytes_warning", False),
            ("quiet", "quiet", False),
            # "hash_randomization" is tested below
            ("isolated", "isolated", False),
            ("dev_mode", "dev_mode", False),
            ("utf8_mode", "utf8_mode", False),
            ("warn_default_encoding", "warn_default_encoding", False),
            ("safe_path", "safe_path", False),
            ("int_max_str_digits", "int_max_str_digits", False),
            # "gil", "thread_inherit_context" and "context_aware_warnings" are tested below
        ):
            with self.subTest(flag=flag, name=name, negate=negate):
                value = config_get(name)
                if negate:
                    value = not value
                self.assertEqual(getattr(sys.flags, flag), value)

        self.assertEqual(sys.flags.hash_randomization,
                         config_get('use_hash_seed') == 0
                         or config_get('hash_seed') != 0)

        if support.Py_GIL_DISABLED:
            value = config_get('enable_gil')
            expected = (value if value != -1 else None)
            self.assertEqual(sys.flags.gil, expected)

        expected_inherit_context = 1 if support.Py_GIL_DISABLED else 0
        self.assertEqual(sys.flags.thread_inherit_context, expected_inherit_context)

        expected_safe_warnings = 1 if support.Py_GIL_DISABLED else 0
        self.assertEqual(sys.flags.context_aware_warnings, expected_safe_warnings)

    def test_config_get_non_existent(self):
        # Test PyConfig_Get() on non-existent option name
        config_get = _testcapi.config_get
        nonexistent_key = 'NONEXISTENT_KEY'
        err_msg = f'unknown config option name: {nonexistent_key}'
        with self.assertRaisesRegex(ValueError, err_msg):
            config_get(nonexistent_key)

    def test_config_get_write_bytecode(self):
        # PyConfig_Get("write_bytecode") gets sys.dont_write_bytecode
        # as an integer
        config_get = _testcapi.config_get
        with support.swap_attr(sys, "dont_write_bytecode", 0):
            self.assertEqual(config_get('write_bytecode'), 1)
        with support.swap_attr(sys, "dont_write_bytecode", "yes"):
            self.assertEqual(config_get('write_bytecode'), 0)
        with support.swap_attr(sys, "dont_write_bytecode", []):
            self.assertEqual(config_get('write_bytecode'), 1)

    def test_config_getint(self):
        # Test PyConfig_GetInt()
        config_getint = _testcapi.config_getint

        # PyConfig_MEMBER_INT type
        self.assertEqual(config_getint('verbose'), sys.flags.verbose)

        # PyConfig_MEMBER_UINT type
        self.assertEqual(config_getint('isolated'), sys.flags.isolated)

        # PyConfig_MEMBER_ULONG type
        self.assertIsInstance(config_getint('hash_seed'), int)

        # PyPreConfig member
        self.assertIsInstance(config_getint('allocator'), int)

        # platlibdir type is str
        with self.assertRaises(TypeError):
            config_getint('platlibdir')

    def test_get_config_names(self):
        names = _testcapi.config_names()
        self.assertIsInstance(names, frozenset)
        for name in names:
            self.assertIsInstance(name, str)

    def test_config_set_sys_attr(self):
        # Test PyConfig_Set() with sys attributes
        config_get = _testcapi.config_get
        config_set = _testcapi.config_set

        # mutable configuration option mapped to sys attributes
        for name, sys_attr, option_type in (
            ('argv', 'argv', list[str]),
            ('base_exec_prefix', 'base_exec_prefix', str | None),
            ('base_executable', '_base_executable', str | None),
            ('base_prefix', 'base_prefix', str | None),
            ('exec_prefix', 'exec_prefix', str | None),
            ('executable', 'executable', str | None),
            ('module_search_paths', 'path', list[str]),
            ('platlibdir', 'platlibdir', str),
            ('prefix', 'prefix', str | None),
            ('pycache_prefix', 'pycache_prefix', str | None),
            ('stdlib_dir', '_stdlib_dir', str | None),
            ('warnoptions', 'warnoptions', list[str]),
            ('xoptions', '_xoptions', dict[str, str | bool]),
        ):
            with self.subTest(name=name):
                if option_type == str:
                    test_values = ('TEST_REPLACE',)
                    invalid_types = (1, None)
                elif option_type == str | None:
                    test_values = ('TEST_REPLACE', None)
                    invalid_types = (123,)
                elif option_type == list[str]:
                    test_values = (['TEST_REPLACE'], [])
                    invalid_types = ('text', 123, [123])
                else:  # option_type == dict[str, str | bool]:
                    test_values = ({"x": "value", "y": True},)
                    invalid_types = ('text', 123, ['option'],
                                     {123: 'value'},
                                     {'key': b'bytes'})

                old_opt_value = config_get(name)
                old_sys_value = getattr(sys, sys_attr)
                try:
                    for value in test_values:
                        config_set(name, value)
                        self.assertEqual(config_get(name), value)
                        self.assertEqual(getattr(sys, sys_attr), value)

                    for value in invalid_types:
                        with self.assertRaises(TypeError):
                            config_set(name, value)
                finally:
                    setattr(sys, sys_attr, old_sys_value)
                    config_set(name, old_opt_value)

    def test_config_set_sys_flag(self):
        # Test PyConfig_Set() with sys.flags
        config_get = _testcapi.config_get
        config_set = _testcapi.config_set

        # mutable configuration option mapped to sys.flags
        class unsigned_int(int):
            pass

        def expect_int(value):
            value = int(value)
            return (value, value)

        def expect_bool(value):
            value = int(bool(value))
            return (value, value)

        def expect_bool_not(value):
            value = bool(value)
            return (int(value), int(not value))

        for name, sys_flag, option_type, expect_func in (
            # (some flags cannot be set, see comments below.)
            ('parser_debug', 'debug', bool, expect_bool),
            ('inspect', 'inspect', bool, expect_bool),
            ('interactive', 'interactive', bool, expect_bool),
            ('optimization_level', 'optimize', unsigned_int, expect_int),
            ('write_bytecode', 'dont_write_bytecode', bool, expect_bool_not),
            # user_site_directory
            # site_import
            ('use_environment', 'ignore_environment', bool, expect_bool_not),
            ('verbose', 'verbose', unsigned_int, expect_int),
            ('bytes_warning', 'bytes_warning', unsigned_int, expect_int),
            ('quiet', 'quiet', bool, expect_bool),
            # hash_randomization
            # isolated
            # dev_mode
            # utf8_mode
            # warn_default_encoding
            # safe_path
            ('int_max_str_digits', 'int_max_str_digits', unsigned_int, expect_int),
            # gil
        ):
            if name == "int_max_str_digits":
                new_values = (0, 5_000, 999_999)
                invalid_values = (-1, 40)  # value must 0 or >= 4300
                invalid_types = (1.0, "abc")
            elif option_type == int:
                new_values = (False, True, 0, 1, 5, -5)
                invalid_values = ()
                invalid_types = (1.0, "abc")
            else:
                new_values = (False, True, 0, 1, 5)
                invalid_values = (-5,)
                invalid_types = (1.0, "abc")

            with self.subTest(name=name):
                old_value = config_get(name)
                try:
                    for value in new_values:
                        expected, expect_flag = expect_func(value)

                        config_set(name, value)
                        self.assertEqual(config_get(name), expected)
                        self.assertEqual(getattr(sys.flags, sys_flag), expect_flag)
                        if name == "write_bytecode":
                            self.assertEqual(getattr(sys, "dont_write_bytecode"),
                                             expect_flag)
                        if name == "int_max_str_digits":
                            self.assertEqual(sys.get_int_max_str_digits(),
                                             expect_flag)

                    for value in invalid_values:
                        with self.assertRaises(ValueError):
                            config_set(name, value)

                    for value in invalid_types:
                        with self.assertRaises(TypeError):
                            config_set(name, value)
                finally:
                    config_set(name, old_value)

    def test_config_set_cpu_count(self):
        config_get = _testcapi.config_get
        config_set = _testcapi.config_set

        old_value = config_get('cpu_count')
        try:
            config_set('cpu_count', 123)
            self.assertEqual(os.cpu_count(), 123)
        finally:
            config_set('cpu_count', old_value)

    def test_config_set_read_only(self):
        # Test PyConfig_Set() on read-only options
        config_set = _testcapi.config_set
        for name, value in (
            ("allocator", 0),  # PyPreConfig member
            ("perf_profiling", 8),
            ("dev_mode", True),
            ("filesystem_encoding", "utf-8"),
        ):
            with self.subTest(name=name, value=value):
                with self.assertRaisesRegex(ValueError, r"read-only"):
                    config_set(name, value)


if __name__ == "__main__":
    unittest.main()
