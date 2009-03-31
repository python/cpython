# Copyright (C) 2003 Python Software Foundation

import unittest
from test import test_support

# Skip this test if aetools does not exist.
test_support.import_module('aetools')

class TestScriptpackages(unittest.TestCase):

    def _test_scriptpackage(self, package, testobject=1):
        # Check that we can import the package
        mod = __import__(package)
        # Test that we can get the main event class
        klass = getattr(mod, package)
        # Test that we can instantiate that class
        talker = klass()
        if testobject:
            # Test that we can get an application object
            obj = mod.application(0)

    def test__builtinSuites(self):
        self._test_scriptpackage('_builtinSuites', testobject=0)

    def test_StdSuites(self):
        self._test_scriptpackage('StdSuites')

    def test_SystemEvents(self):
        self._test_scriptpackage('SystemEvents')

    def test_Finder(self):
        self._test_scriptpackage('Finder')

    def test_Terminal(self):
        self._test_scriptpackage('Terminal')

    def test_Netscape(self):
        self._test_scriptpackage('Netscape')

    def test_Explorer(self):
        self._test_scriptpackage('Explorer')

    def test_CodeWarrior(self):
        self._test_scriptpackage('CodeWarrior')

def test_main():
    test_support.run_unittest(TestScriptpackages)


if __name__ == '__main__':
    test_main()
