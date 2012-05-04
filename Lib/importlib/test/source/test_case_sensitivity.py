"""Test case-sensitivity (PEP 235)."""
from importlib import _bootstrap
from .. import util
from . import util as source_util
import imp
import os
import sys
from test import support as test_support
import unittest


@util.case_insensitive_tests
class CaseSensitivityTest(unittest.TestCase):

    """PEP 235 dictates that on case-preserving, case-insensitive file systems
    that imports are case-sensitive unless the PYTHONCASEOK environment
    variable is set."""

    name = 'MoDuLe'
    assert name != name.lower()

    def find(self, path):
        finder = _bootstrap.FileFinder(path,
                                        (_bootstrap.SourceFileLoader,
                                            _bootstrap._SOURCE_SUFFIXES,
                                            True),
                                        (_bootstrap.SourcelessFileLoader,
                                            [_bootstrap._BYTECODE_SUFFIX],
                                            True))
        return finder.find_module(self.name)

    def sensitivity_test(self):
        """Look for a module with matching and non-matching sensitivity."""
        sensitive_pkg = 'sensitive.{0}'.format(self.name)
        insensitive_pkg = 'insensitive.{0}'.format(self.name.lower())
        context = source_util.create_modules(insensitive_pkg, sensitive_pkg)
        with context as mapping:
            sensitive_path = os.path.join(mapping['.root'], 'sensitive')
            insensitive_path = os.path.join(mapping['.root'], 'insensitive')
            return self.find(sensitive_path), self.find(insensitive_path)

    def test_sensitive(self):
        with test_support.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            if b'PYTHONCASEOK' in _bootstrap._os.environ:
                self.skipTest('os.environ changes not reflected in '
                              '_os.environ')
            sensitive, insensitive = self.sensitivity_test()
            self.assertTrue(hasattr(sensitive, 'load_module'))
            self.assertIn(self.name, sensitive.get_filename(self.name))
            self.assertIsNone(insensitive)

    def test_insensitive(self):
        with test_support.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            if b'PYTHONCASEOK' not in _bootstrap._os.environ:
                self.skipTest('os.environ changes not reflected in '
                              '_os.environ')
            sensitive, insensitive = self.sensitivity_test()
            self.assertTrue(hasattr(sensitive, 'load_module'))
            self.assertIn(self.name, sensitive.get_filename(self.name))
            self.assertTrue(hasattr(insensitive, 'load_module'))
            self.assertIn(self.name, insensitive.get_filename(self.name))


def test_main():
    test_support.run_unittest(CaseSensitivityTest)


if __name__ == '__main__':
    test_main()
