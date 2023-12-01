"""
Tests on PyConfig API (PEP 587).
"""
import os
import sys
import unittest
from test import support
try:
    import _testcapi
except ImportError:
    _testcapi = None


@unittest.skipIf(_testcapi is None, 'need _testcapi')
class CAPITests(unittest.TestCase):
    def check_config_get(self, get_func):
        # write_bytecode is read from sys.dont_write_bytecode as int
        with support.swap_attr(sys, "dont_write_bytecode", 0):
            self.assertEqual(get_func('write_bytecode'), 1)
        with support.swap_attr(sys, "dont_write_bytecode", "yes"):
            self.assertEqual(get_func('write_bytecode'), 0)
        with support.swap_attr(sys, "dont_write_bytecode", []):
            self.assertEqual(get_func('write_bytecode'), 1)

        # non-existent config option name
        NONEXISTENT_KEY = 'NONEXISTENT_KEY'
        err_msg = f'unknown config option name: {NONEXISTENT_KEY}'
        with self.assertRaisesRegex(ValueError, err_msg):
            get_func('NONEXISTENT_KEY')

    def test_config_get(self):
        config_get = _testcapi.config_get

        self.check_config_get(config_get)

        for name, config_type, expected in (
            ('verbose', int, sys.flags.verbose),   # PyConfig_MEMBER_INT
            ('isolated', int, sys.flags.isolated), # PyConfig_MEMBER_UINT
            ('platlibdir', str, sys.platlibdir),   # PyConfig_MEMBER_WSTR
            ('argv', list, sys.argv),              # PyConfig_MEMBER_WSTR_LIST
            ('xoptions', dict, sys._xoptions),     # xoptions dict
        ):
            with self.subTest(name=name):
                value = config_get(name)
                self.assertEqual(type(value), config_type)
                self.assertEqual(value, expected)

        # PyConfig_MEMBER_ULONG type
        hash_seed = config_get('hash_seed')
        self.assertIsInstance(hash_seed, int)
        self.assertGreaterEqual(hash_seed, 0)

        # PyConfig_MEMBER_WSTR_OPT type
        if 'PYTHONDUMPREFSFILE' not in os.environ:
            self.assertIsNone(config_get('dump_refs_file'))

        # attributes read from sys
        value_str = "TEST_MARKER_STR"
        value_list = ["TEST_MARKER_STRLIST"]
        value_dict = {"x": "value", "y": True}
        for name, sys_name, value in (
            ("base_exec_prefix", None, value_str),
            ("base_prefix", None, value_str),
            ("exec_prefix", None, value_str),
            ("executable", None, value_str),
            ("platlibdir", None, value_str),
            ("prefix", None, value_str),
            ("pycache_prefix", None, value_str),
            ("base_executable", "_base_executable", value_str),
            ("stdlib_dir", "_stdlib_dir", value_str),
            ("argv", None, value_list),
            ("orig_argv", None, value_list),
            ("warnoptions", None, value_list),
            ("module_search_paths", "path", value_list),
            ("xoptions", "_xoptions", value_dict),
        ):
            with self.subTest(name=name):
                if sys_name is None:
                    sys_name = name
                with support.swap_attr(sys, sys_name, value):
                    self.assertEqual(config_get(name), value)

    def test_config_getint(self):
        config_getint = _testcapi.config_getint

        self.check_config_get(config_getint)

        # PyConfig_MEMBER_INT type
        self.assertEqual(config_getint('verbose'), sys.flags.verbose)

        # PyConfig_MEMBER_UINT type
        self.assertEqual(config_getint('isolated'), sys.flags.isolated)

        # PyConfig_MEMBER_ULONG type
        hash_seed = config_getint('hash_seed')
        self.assertIsInstance(hash_seed, int)
        self.assertGreaterEqual(hash_seed, 0)

        # platlibdir is a str
        with self.assertRaises(TypeError):
            config_getint('platlibdir')


if __name__ == "__main__":
    unittest.main()
