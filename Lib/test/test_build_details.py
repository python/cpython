import importlib
import json
import os
import os.path
import sys
import sysconfig
import string
import unittest
from pathlib import Path

from test.support import is_android, is_apple_mobile, is_wasm32

BASE_PATH = Path(
    __file__,  # Lib/test/test_build_details.py
    '..',  # Lib/test
    '..',  # Lib
    '..',  # <src/install dir>
).resolve()
MODULE_PATH = BASE_PATH / 'Tools' / 'build' / 'generate-build-details.py'

try:
    # Import "generate-build-details.py" as "generate_build_details"
    spec = importlib.util.spec_from_file_location(
        "generate_build_details", MODULE_PATH
    )
    generate_build_details = importlib.util.module_from_spec(spec)
    sys.modules["generate_build_details"] = generate_build_details
    spec.loader.exec_module(generate_build_details)
except (FileNotFoundError, ImportError):
    generate_build_details = None


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

    def test_top_level_container(self):
        self.assertIsInstance(self.data, dict)
        for key, value in self.data.items():
            with self.subTest(key=key):
                if key in ('schema_version', 'base_prefix', 'base_interpreter',
                           'platform'):
                    self.assertIsInstance(value, str)
                elif key in ('language', 'implementation', 'abi', 'suffixes',
                             'libpython', 'c_api', 'arbitrary_data'):
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
                sys_version_value = getattr(sys.version_info, part_name)
                self.assertEqual(part_value, sys_version_value)

    def test_implementation(self):
        impl_ver = sys.implementation.version
        for key, value in self.key('implementation').items():
            with self.subTest(part=key):
                if key == 'version':
                    self.assertEqual(len(value), len(impl_ver))
                    for part_name, part_value in value.items():
                        self.assertFalse(isinstance(sys.implementation.version, dict))
                        getattr(sys.implementation.version, part_name)
                        sys_implementation_value = getattr(impl_ver, part_name)
                        self.assertEqual(sys_implementation_value, part_value)
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
            pybuilddir = os.path.join(projectdir, 'pybuilddir.txt')
            with open(pybuilddir, encoding='utf-8') as f:
                dirname = os.path.join(projectdir, f.read())
        else:
            dirname = sysconfig.get_path('stdlib')
        return os.path.join(dirname, 'build-details.json')

    @property
    def contents(self):
        with open(self.location, 'r', encoding='utf-8') as f:
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


@unittest.skipIf(
    generate_build_details is None,
    "Failed to import generate-build-details"
)
@unittest.skipIf(os.name != 'posix', 'Feature only implemented on POSIX right now')
@unittest.skipIf(is_wasm32, 'Feature not available on WebAssembly builds')
class BuildDetailsRelativePathsTests(unittest.TestCase):
    @property
    def build_details_absolute_paths(self):
        data = generate_build_details.generate_data(schema_version='1.0')
        return json.loads(json.dumps(data))

    @property
    def build_details_relative_paths(self):
        data = self.build_details_absolute_paths
        generate_build_details.make_paths_relative(data, config_path=None)
        return data

    def test_round_trip(self):
        data_abs_path = self.build_details_absolute_paths
        data_rel_path = self.build_details_relative_paths

        self.assertEqual(data_abs_path['base_prefix'],
                         data_rel_path['base_prefix'])

        base_prefix = data_abs_path['base_prefix']

        top_level_keys = ('base_interpreter',)
        for key in top_level_keys:
            self.assertEqual(key in data_abs_path, key in data_rel_path)
            if key not in data_abs_path:
                continue

            abs_rel_path = os.path.join(base_prefix, data_rel_path[key])
            abs_rel_path = os.path.normpath(abs_rel_path)
            self.assertEqual(data_abs_path[key], abs_rel_path)

        second_level_keys = (
            ('libpython', 'dynamic'),
            ('libpython', 'dynamic_stableabi'),
            ('libpython', 'static'),
            ('c_api', 'headers'),
            ('c_api', 'pkgconfig_path'),

        )
        for part, key in second_level_keys:
            self.assertEqual(part in data_abs_path, part in data_rel_path)
            if part not in data_abs_path:
                continue
            self.assertEqual(key in data_abs_path[part],
                             key in data_rel_path[part])
            if key not in data_abs_path[part]:
                continue

            abs_rel_path = os.path.join(base_prefix, data_rel_path[part][key])
            abs_rel_path = os.path.normpath(abs_rel_path)
            self.assertEqual(data_abs_path[part][key], abs_rel_path)


if __name__ == '__main__':
    unittest.main()
