import json
import os
import sys
import sysconfig
import string
import unittest

from test.support import is_android, is_apple_mobile, is_wasm32


class FormatTestsBase:
    @property
    def contents(self):
        """Install details file contents. Should be overriden by subclasses."""
        raise NotImplementedError

    @property
    def data(self):
        """Parsed install details file data, as a Python object."""
        return json.loads(self.contents)

    def key(self, name):
        """Helper to fetch subsection entries.

        It takes the entry name, allowing the usage of a dot to separate the
        different subsection names (eg. specifying 'a.b.c' as the key will
        return the value of self.data['a']['b']['c']).
        """
        value = self.data
        for part in name.split('.'):
            value = value[part]
        return value

    def test_parse(self):
        self.data

    def test_top_level_container(self):
        self.assertIsInstance(self.data, dict)
        for key, value in self.data.items():
            with self.subTest(key=key):
                if key in ('schema_version', 'base_prefix', 'base_interpreter', 'platform'):
                    self.assertIsInstance(value, str)
                elif key in ('language', 'implementation', 'abi', 'suffixes', 'libpython', 'c_api', 'arbitrary_data'):
                    self.assertIsInstance(value, dict)

    def test_base_prefix(self):
        self.assertIsInstance(self.key('base_prefix'), str)

    def test_base_interpreter(self):
        """Test the base_interpreter entry.

        The generic test wants the key to be missing. If your implementation
        provides a value for it, you should override this test.
        """
        with self.assertRaises(KeyError):
            self.key('base_interpreter')

    def test_platform(self):
        self.assertEqual(self.key('platform'), sysconfig.get_platform())

    def test_language_version(self):
        allowed_characters = string.digits + string.ascii_letters + '.'
        value = self.key('language.version')

        self.assertLessEqual(set(value), set(allowed_characters))
        self.assertTrue(sys.version.startswith(value))

    def test_language_version_info(self):
        value = self.key('language.version_info')

        self.assertEqual(len(value), sys.version_info.n_fields)
        for part_name, part_value in value.items():
            with self.subTest(part=part_name):
                self.assertEqual(part_value, getattr(sys.version_info, part_name))

    def test_implementation(self):
        for key, value in self.key('implementation').items():
            with self.subTest(part=key):
                if key == 'version':
                    self.assertEqual(len(value), len(sys.implementation.version))
                    for part_name, part_value in value.items():
                        self.assertEqual(getattr(sys.implementation.version, part_name), part_value)
                else:
                    self.assertEqual(getattr(sys.implementation, key), value)


needs_installed_python = unittest.skipIf(
    sysconfig.is_python_build(),
    'This test can only run in an installed Python',
)


@unittest.skipIf(os.name != 'posix', 'Feature only implemented on POSIX right now')
@unittest.skipIf(is_wasm32, 'Feature not available on WebAssembly builds')
class CPythonBuildDetailsTests(unittest.TestCase, FormatTestsBase):
    """Test CPython's install details file implementation."""

    @property
    def location(self):
        if sysconfig.is_python_build():
            projectdir = sysconfig.get_config_var('projectbase')
            with open(os.path.join(projectdir, 'pybuilddir.txt')) as f:
                dirname = os.path.join(projectdir, f.read())
        else:
            dirname = sysconfig.get_path('stdlib')
        return os.path.join(dirname, 'build-details.json')

    @property
    def contents(self):
        with open(self.location, 'r') as f:
            return f.read()

    @needs_installed_python
    def test_location(self):
        self.assertTrue(os.path.isfile(self.location))

    # Override generic format tests with tests for our specific implemenation.

    @needs_installed_python
    @unittest.skipIf(
        is_android or is_apple_mobile,
        'Android and iOS run tests via a custom testbed method that changes sys.executable'
    )
    def test_base_interpreter(self):
        value = self.key('base_interpreter')

        # Skip check if installation is relocated
        if sysconfig._installation_is_relocated():
            self.skipTest("Installation is relocated")

        self.assertEqual(os.path.realpath(value), os.path.realpath(sys.executable))

    @needs_installed_python
    @unittest.skipIf(
        is_android or is_apple_mobile,
        "Android and iOS run tests via a custom testbed method that doesn't ship headers"
    )
    def test_c_api(self):
        value = self.key('c_api')

        # Skip check if installation is relocated
        if sysconfig._installation_is_relocated():
            self.skipTest("Installation is relocated")

        self.assertTrue(os.path.exists(os.path.join(value['headers'], 'Python.h')))
        version = sysconfig.get_config_var('VERSION')
        self.assertTrue(os.path.exists(os.path.join(value['pkgconfig_path'], f'python-{version}.pc')))


if __name__ == '__main__':
    unittest.main()
