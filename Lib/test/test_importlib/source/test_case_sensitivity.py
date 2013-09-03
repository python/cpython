"""Test case-sensitivity (PEP 235)."""
from .. import util
from . import util as source_util

from importlib import _bootstrap
from importlib import machinery
import os
import sys
from test import support as test_support
import unittest
import subprocess


@util.case_insensitive_tests
class CaseSensitivityTest(unittest.TestCase):

    """PEP 235 dictates that on case-preserving, case-insensitive file systems
    that imports are case-sensitive unless the PYTHONCASEOK environment
    variable is set."""

    name = 'MoDuLe'
    assert name != name.lower()

    def find(self, path):
        finder = machinery.FileFinder(path,
                                      (machinery.SourceFileLoader,
                                            machinery.SOURCE_SUFFIXES),
                                        (machinery.SourcelessFileLoader,
                                            machinery.BYTECODE_SUFFIXES))
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
        sensitive_pkg = 'sensitive.{0}'.format(self.name)
        insensitive_pkg = 'insensitive.{0}'.format(self.name.lower())
        context = source_util.create_modules(insensitive_pkg, sensitive_pkg)
        with context as mapping:
            sensitive_path = os.path.join(mapping['.root'], 'sensitive')
            insensitive_path = os.path.join(mapping['.root'], 'insensitive')
            find_snippet = """if True:
                import sys
                from importlib import machinery

                def find(path):
                    f = machinery.FileFinder(path,
                                             (machinery.SourceFileLoader,
                                              machinery.SOURCE_SUFFIXES),
                                             (machinery.SourcelessFileLoader,
                                              machinery.BYTECODE_SUFFIXES))
                    return f.find_module('{name}')

                sensitive = find('{sensitive_path}')
                insensitive = find('{insensitive_path}')
                print(str(hasattr(sensitive, 'load_module')))
                if hasattr(sensitive, 'load_module'):
                    print(sensitive.get_filename('{name}'))
                else:
                    print('None')
                print(str(hasattr(insensitive, 'load_module')))
                if hasattr(insensitive, 'load_module'):
                    print(insensitive.get_filename('{name}'))
                else:
                    print('None')
                """.format(sensitive_path=sensitive_path,
                           insensitive_path=insensitive_path,
                           name=self.name)

            newenv = os.environ.copy()
            newenv["PYTHONCASEOK"] = "1"

            def check_output(expected, extra_arg=None):
                args = [sys.executable]
                if extra_arg:
                    args.append(extra_arg)
                args.extend(["-c", find_snippet])
                p = subprocess.Popen(args, stdout=subprocess.PIPE,
                                         env=newenv)
                actual = p.communicate()[0].decode().split()
                self.assertEqual(expected[0], actual[0])
                self.assertIn(expected[1], actual[1])
                self.assertEqual(expected[2], actual[2])
                self.assertIn(expected[3], actual[3])
                self.assertEqual(p.wait(), 0)

            # Test with PYTHONCASEOK=1.
            check_output(["True", self.name, "True", self.name])

            # Test with PYTHONCASEOK=1 ignored because of -E.
            check_output(["True", self.name, "False", "None"], "-E")


def test_main():
    test_support.run_unittest(CaseSensitivityTest)


if __name__ == '__main__':
    test_main()
