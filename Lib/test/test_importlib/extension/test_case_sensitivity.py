import sys
from test import support
import unittest

from importlib import _bootstrap
from importlib import machinery
from .. import util
from . import util as ext_util
import os
import subprocess

@util.case_insensitive_tests
class ExtensionModuleCaseSensitivityTest(unittest.TestCase):

    def find_module(self):
        good_name = ext_util.NAME
        bad_name = good_name.upper()
        assert good_name != bad_name
        finder = machinery.FileFinder(ext_util.PATH,
                                        (machinery.ExtensionFileLoader,
                                         machinery.EXTENSION_SUFFIXES))
        return finder.find_module(bad_name)

    def test_case_sensitive(self):
        with support.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            if b'PYTHONCASEOK' in _bootstrap._os.environ:
                self.skipTest('os.environ changes not reflected in '
                              '_os.environ')
            loader = self.find_module()
            self.assertIsNone(loader)

    def test_case_insensitivity(self):
        find_snippet = """if True:
            from importlib import _bootstrap
            import sys
            finder = _bootstrap.FileFinder('{path}',
                                           (_bootstrap.ExtensionFileLoader,
                                            _bootstrap.EXTENSION_SUFFIXES))
            loader = finder.find_module('{bad_name}')
            print(str(hasattr(loader, 'load_module')))
            """.format(bad_name=ext_util.NAME.upper(), path=ext_util.PATH)

        newenv = os.environ.copy()
        newenv["PYTHONCASEOK"] = "1"

        def check_output(expected, extra_arg=None):
            args = [sys.executable]
            if extra_arg:
                args.append(extra_arg)
            args.extend(["-c", find_snippet])
            p = subprocess.Popen(args, stdout=subprocess.PIPE, env=newenv)
            actual = p.communicate()[0].decode().strip()
            self.assertEqual(expected, actual)
            self.assertEqual(p.wait(), 0)

        # Test with PYTHONCASEOK=1.
        check_output("True")

        # Test with PYTHONCASEOK=1 ignored because of -E.
        check_output("False", "-E")



def test_main():
    if ext_util.FILENAME is None:
        return
    support.run_unittest(ExtensionModuleCaseSensitivityTest)


if __name__ == '__main__':
    test_main()
