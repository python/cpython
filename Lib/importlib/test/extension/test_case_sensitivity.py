import sys
from test import support as test_support
import unittest
import importlib
from . import test_path_hook


class ExtensionModuleCaseSensitivityTest(unittest.TestCase):

    def find_module(self):
        good_name = test_path_hook.NAME
        bad_name = good_name.upper()
        assert good_name != bad_name
        finder = importlib.ExtensionFileImporter(test_path_hook.PATH)
        return finder.find_module(bad_name)

    def test_case_sensitive(self):
        with test_support.EnvironmentVarGuard() as env:
            env.unset('PYTHONCASEOK')
            loader = self.find_module()
            self.assert_(loader is None)

    def test_case_insensitivity(self):
        with test_support.EnvironmentVarGuard() as env:
            env.set('PYTHONCASEOK', '1')
            loader = self.find_module()
            self.assert_(hasattr(loader, 'load_module'))




def test_main():
    if sys.platform not in ('win32', 'darwin', 'cygwin'):
        return
    test_support.run_unittest(ExtensionModuleCaseSensitivityTest)


if __name__ == '__main__':
    test_main()
