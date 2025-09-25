import os
import posix
import subprocess
import sys
import unittest
import warnings
from test import support
from test import mapping_tests
from test.support import os_helper
from test.support import unix_shell


class EnvironTests(mapping_tests.BasicTestMappingProtocol):
    """check that os.environ object conform to mapping protocol"""
    type2test = None

    def setUp(self):
        self.__save = dict(os.environ)
        if os.supports_bytes_environ:
            self.__saveb = dict(os.environb)
        for key, value in self._reference().items():
            os.environ[key] = value

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self.__save)
        if os.supports_bytes_environ:
            os.environb.clear()
            os.environb.update(self.__saveb)

    def _reference(self):
        return {"KEY1":"VALUE1", "KEY2":"VALUE2", "KEY3":"VALUE3"}

    def _empty_mapping(self):
        os.environ.clear()
        return os.environ

    # Bug 1110478
    @unittest.skipUnless(unix_shell and os.path.exists(unix_shell),
                         'requires a shell')
    @unittest.skipUnless(hasattr(os, 'popen'), "needs os.popen()")
    @support.requires_subprocess()
    def test_update2(self):
        os.environ.clear()
        os.environ.update(HELLO="World")
        with os.popen("%s -c 'echo $HELLO'" % unix_shell) as popen:
            value = popen.read().strip()
            self.assertEqual(value, "World")

    @unittest.skipUnless(unix_shell and os.path.exists(unix_shell),
                         'requires a shell')
    @unittest.skipUnless(hasattr(os, 'popen'), "needs os.popen()")
    @support.requires_subprocess()
    def test_os_popen_iter(self):
        with os.popen("%s -c 'echo \"line1\nline2\nline3\"'"
                      % unix_shell) as popen:
            it = iter(popen)
            self.assertEqual(next(it), "line1\n")
            self.assertEqual(next(it), "line2\n")
            self.assertEqual(next(it), "line3\n")
            self.assertRaises(StopIteration, next, it)

    # Verify environ keys and values from the OS are of the
    # correct str type.
    def test_keyvalue_types(self):
        for key, val in os.environ.items():
            self.assertEqual(type(key), str)
            self.assertEqual(type(val), str)

    def test_items(self):
        for key, value in self._reference().items():
            self.assertEqual(os.environ.get(key), value)

    # Issue 7310
    def test___repr__(self):
        """Check that the repr() of os.environ looks like environ({...})."""
        env = os.environ
        formatted_items = ", ".join(
            f"{key!r}: {value!r}"
            for key, value in env.items()
        )
        self.assertEqual(repr(env), f"environ({{{formatted_items}}})")

    def test_get_exec_path(self):
        defpath_list = os.defpath.split(os.pathsep)
        test_path = ['/monty', '/python', '', '/flying/circus']
        test_env = {'PATH': os.pathsep.join(test_path)}

        saved_environ = os.environ
        try:
            os.environ = dict(test_env)
            # Test that defaulting to os.environ works.
            self.assertSequenceEqual(test_path, os.get_exec_path())
            self.assertSequenceEqual(test_path, os.get_exec_path(env=None))
        finally:
            os.environ = saved_environ

        # No PATH environment variable
        self.assertSequenceEqual(defpath_list, os.get_exec_path({}))
        # Empty PATH environment variable
        self.assertSequenceEqual(('',), os.get_exec_path({'PATH':''}))
        # Supplied PATH environment variable
        self.assertSequenceEqual(test_path, os.get_exec_path(test_env))

        if os.supports_bytes_environ:
            # env cannot contain 'PATH' and b'PATH' keys
            try:
                # ignore BytesWarning warning
                with warnings.catch_warnings(record=True):
                    mixed_env = {'PATH': '1', b'PATH': b'2'}
            except BytesWarning:
                # mixed_env cannot be created with python -bb
                pass
            else:
                self.assertRaises(ValueError, os.get_exec_path, mixed_env)

            # bytes key and/or value
            self.assertSequenceEqual(os.get_exec_path({b'PATH': b'abc'}),
                ['abc'])
            self.assertSequenceEqual(os.get_exec_path({b'PATH': 'abc'}),
                ['abc'])
            self.assertSequenceEqual(os.get_exec_path({'PATH': b'abc'}),
                ['abc'])

    @unittest.skipUnless(os.supports_bytes_environ,
                         "os.environb required for this test.")
    def test_environb(self):
        # os.environ -> os.environb
        value = 'euro\u20ac'
        try:
            value_bytes = value.encode(sys.getfilesystemencoding(),
                                       'surrogateescape')
        except UnicodeEncodeError:
            msg = "U+20AC character is not encodable to %s" % (
                sys.getfilesystemencoding(),)
            self.skipTest(msg)
        os.environ['unicode'] = value
        self.assertEqual(os.environ['unicode'], value)
        self.assertEqual(os.environb[b'unicode'], value_bytes)

        # os.environb -> os.environ
        value = b'\xff'
        os.environb[b'bytes'] = value
        self.assertEqual(os.environb[b'bytes'], value)
        value_str = value.decode(sys.getfilesystemencoding(), 'surrogateescape')
        self.assertEqual(os.environ['bytes'], value_str)

    @support.requires_subprocess()
    def test_putenv_unsetenv(self):
        name = "PYTHONTESTVAR"
        value = "testvalue"
        code = f'import os; print(repr(os.environ.get({name!r})))'

        with os_helper.EnvironmentVarGuard() as env:
            env.pop(name, None)

            os.putenv(name, value)
            proc = subprocess.run([sys.executable, '-c', code], check=True,
                                  stdout=subprocess.PIPE, text=True)
            self.assertEqual(proc.stdout.rstrip(), repr(value))

            os.unsetenv(name)
            proc = subprocess.run([sys.executable, '-c', code], check=True,
                                  stdout=subprocess.PIPE, text=True)
            self.assertEqual(proc.stdout.rstrip(), repr(None))

    # On OS X < 10.6, unsetenv() doesn't return a value (bpo-13415).
    @support.requires_mac_ver(10, 6)
    def test_putenv_unsetenv_error(self):
        # Empty variable name is invalid.
        # "=" and null character are not allowed in a variable name.
        for name in ('', '=name', 'na=me', 'name='):
            self.assertRaises((OSError, ValueError), os.putenv, name, "value")
            self.assertRaises((OSError, ValueError), os.unsetenv, name)
        for name in ('name\0', 'na\0me'):
            self.assertRaises(ValueError, os.putenv, name, "value")
            self.assertRaises(ValueError, os.unsetenv, name)

        if sys.platform == "win32":
            # On Windows, an environment variable string ("name=value" string)
            # is limited to 32,767 characters
            longstr = 'x' * 32_768
            self.assertRaises(ValueError, os.putenv, longstr, "1")
            self.assertRaises(ValueError, os.putenv, "X", longstr)
            self.assertRaises(ValueError, os.unsetenv, longstr)

    def test_key_type(self):
        missing = 'missingkey'
        self.assertNotIn(missing, os.environ)

        with self.assertRaises(KeyError) as cm:
            os.environ[missing]
        self.assertIs(cm.exception.args[0], missing)
        self.assertTrue(cm.exception.__suppress_context__)

        with self.assertRaises(KeyError) as cm:
            del os.environ[missing]
        self.assertIs(cm.exception.args[0], missing)
        self.assertTrue(cm.exception.__suppress_context__)

    def _test_environ_iteration(self, collection):
        iterator = iter(collection)
        new_key = "__new_key__"

        next(iterator)  # start iteration over os.environ.items

        # add a new key in os.environ mapping
        os.environ[new_key] = "test_environ_iteration"

        try:
            next(iterator)  # force iteration over modified mapping
            self.assertEqual(os.environ[new_key], "test_environ_iteration")
        finally:
            del os.environ[new_key]

    def test_iter_error_when_changing_os_environ(self):
        self._test_environ_iteration(os.environ)

    def test_iter_error_when_changing_os_environ_items(self):
        self._test_environ_iteration(os.environ.items())

    def test_iter_error_when_changing_os_environ_values(self):
        self._test_environ_iteration(os.environ.values())

    def _test_underlying_process_env(self, var, expected):
        if not (unix_shell and os.path.exists(unix_shell)):
            return
        elif not support.has_subprocess_support:
            return

        with os.popen(f"{unix_shell} -c 'echo ${var}'") as popen:
            value = popen.read().strip()

        self.assertEqual(expected, value)

    def test_or_operator(self):
        overridden_key = '_TEST_VAR_'
        original_value = 'original_value'
        os.environ[overridden_key] = original_value

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(os.environ)
        expected.update(new_vars_dict)

        actual = os.environ | new_vars_dict
        self.assertDictEqual(expected, actual)
        self.assertEqual('3', actual[overridden_key])

        new_vars_items = new_vars_dict.items()
        self.assertIs(NotImplemented, os.environ.__or__(new_vars_items))

        self._test_underlying_process_env('_A_', '')
        self._test_underlying_process_env(overridden_key, original_value)

    def test_ior_operator(self):
        overridden_key = '_TEST_VAR_'
        os.environ[overridden_key] = 'original_value'

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(os.environ)
        expected.update(new_vars_dict)

        os.environ |= new_vars_dict
        self.assertEqual(expected, os.environ)
        self.assertEqual('3', os.environ[overridden_key])

        self._test_underlying_process_env('_A_', '1')
        self._test_underlying_process_env(overridden_key, '3')

    def test_ior_operator_invalid_dicts(self):
        os_environ_copy = os.environ.copy()
        with self.assertRaises(TypeError):
            dict_with_bad_key = {1: '_A_'}
            os.environ |= dict_with_bad_key

        with self.assertRaises(TypeError):
            dict_with_bad_val = {'_A_': 1}
            os.environ |= dict_with_bad_val

        # Check nothing was added.
        self.assertEqual(os_environ_copy, os.environ)

    def test_ior_operator_key_value_iterable(self):
        overridden_key = '_TEST_VAR_'
        os.environ[overridden_key] = 'original_value'

        new_vars_items = (('_A_', '1'), ('_B_', '2'), (overridden_key, '3'))
        expected = dict(os.environ)
        expected.update(new_vars_items)

        os.environ |= new_vars_items
        self.assertEqual(expected, os.environ)
        self.assertEqual('3', os.environ[overridden_key])

        self._test_underlying_process_env('_A_', '1')
        self._test_underlying_process_env(overridden_key, '3')

    def test_ror_operator(self):
        overridden_key = '_TEST_VAR_'
        original_value = 'original_value'
        os.environ[overridden_key] = original_value

        new_vars_dict = {'_A_': '1', '_B_': '2', overridden_key: '3'}
        expected = dict(new_vars_dict)
        expected.update(os.environ)

        actual = new_vars_dict | os.environ
        self.assertDictEqual(expected, actual)
        self.assertEqual(original_value, actual[overridden_key])

        new_vars_items = new_vars_dict.items()
        self.assertIs(NotImplemented, os.environ.__ror__(new_vars_items))

        self._test_underlying_process_env('_A_', '')
        self._test_underlying_process_env(overridden_key, original_value)

    def test_reload_environ(self):
        # Test os.reload_environ()
        has_environb = hasattr(os, 'environb')

        # Test with putenv() which doesn't update os.environ
        os.environ['test_env'] = 'python_value'
        os.putenv("test_env", "new_value")
        self.assertEqual(os.environ['test_env'], 'python_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'python_value')

        os.reload_environ()
        self.assertEqual(os.environ['test_env'], 'new_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'new_value')

        # Test with unsetenv() which doesn't update os.environ
        os.unsetenv('test_env')
        self.assertEqual(os.environ['test_env'], 'new_value')
        if has_environb:
            self.assertEqual(os.environb[b'test_env'], b'new_value')

        os.reload_environ()
        self.assertNotIn('test_env', os.environ)
        if has_environb:
            self.assertNotIn(b'test_env', os.environb)

        if has_environb:
            # test reload_environ() on os.environb with putenv()
            os.environb[b'test_env'] = b'python_value2'
            os.putenv("test_env", "new_value2")
            self.assertEqual(os.environb[b'test_env'], b'python_value2')
            self.assertEqual(os.environ['test_env'], 'python_value2')

            os.reload_environ()
            self.assertEqual(os.environb[b'test_env'], b'new_value2')
            self.assertEqual(os.environ['test_env'], 'new_value2')

            # test reload_environ() on os.environb with unsetenv()
            os.unsetenv('test_env')
            self.assertEqual(os.environb[b'test_env'], b'new_value2')
            self.assertEqual(os.environ['test_env'], 'new_value2')

            os.reload_environ()
            self.assertNotIn(b'test_env', os.environb)
            self.assertNotIn('test_env', os.environ)


class PosixTester(unittest.TestCase):

    def test_environ(self):
        if os.name == "nt":
            item_type = str
        else:
            item_type = bytes
        for k, v in posix.environ.items():
            self.assertEqual(type(k), item_type)
            self.assertEqual(type(v), item_type)

    def test_putenv(self):
        with self.assertRaises(ValueError):
            os.putenv('FRUIT\0VEGETABLE', 'cabbage')
        with self.assertRaises(ValueError):
            os.putenv('FRUIT', 'orange\0VEGETABLE=cabbage')
        with self.assertRaises(ValueError):
            os.putenv('FRUIT=ORANGE', 'lemon')
        if os.name == 'posix':
            with self.assertRaises(ValueError):
                os.putenv(b'FRUIT\0VEGETABLE', b'cabbage')
            with self.assertRaises(ValueError):
                os.putenv(b'FRUIT', b'orange\0VEGETABLE=cabbage')
            with self.assertRaises(ValueError):
                os.putenv(b'FRUIT=ORANGE', b'lemon')


if __name__ == "__main__":
    unittest.main()
