import json
import os
import sys
import sysconfig
import string
import unittest


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
            with self.subTest(section=key):
                self.assertIsInstance(value, dict)

    def test_python_version(self):
        allowed_characters = string.digits + string.ascii_letters + '.'
        value = self.key('python.version')

        self.assertLessEqual(set(value), set(allowed_characters))
        self.assertTrue(sys.version.startswith(value))

    def test_python_version_parts(self):
        value = self.key('python.version_parts')

        self.assertEqual(len(value), sys.version_info.n_fields)
        for part_name, part_value in value.items():
            with self.subTest(part=part_name):
                self.assertEqual(part_value, getattr(sys.version_info, part_name))

    def test_python_executable(self):
        """Test the python.executable entry.

        The generic test wants the key to be missing. If your implementation
        provides a value for it, you should override this test.
        """
        with self.assertRaises(KeyError):
            self.key('python.executable')

    def test_python_stdlib(self):
        """Test the python.stdlib entry.

        The generic test wants the key to be missing. If your implementation
        provides a value for it, you should override this test.
        """
        with self.assertRaises(KeyError):
            self.key('python.stdlib')


needs_installed_python = unittest.skipIf(
    sysconfig.is_python_build(),
    'This test can only run in an installed Python',
)


@unittest.skipIf(os.name == 'nt', 'Feature only implemented on POSIX right now')
class CPythonInstallDetailsFileTests(unittest.TestCase, FormatTestsBase):
    """Test CPython's install details file implementation."""

    @property
    def location(self):
        if sysconfig.is_python_build():
            dirname = sysconfig.get_config_var('projectbase')
        else:
            dirname = sysconfig.get_path('stdlib')
        return os.path.join(dirname, 'install-details.json')

    @property
    def contents(self):
        with open(self.location, 'r') as f:
            return f.read()

    @needs_installed_python
    def test_location(self):
        self.assertTrue(os.path.isfile(self.location))

    # Override generic format tests with tests for our specific implemenation.

    @needs_installed_python
    def test_python_executable(self):
        value = self.key('python.executable')

        self.assertEqual(os.path.realpath(value), os.path.realpath(sys.executable))

    @needs_installed_python
    def test_python_stdlib(self):
        value = self.key('python.stdlib')

        try:
            stdlib = os.path.dirname(unittest.__path__)
        except AttributeError as exc:
            self.skipTest(str(exc))

        self.assertEqual(os.path.realpath(value), os.path.realpath(stdlib))


if __name__ == "__main__":
    unittest.main()
