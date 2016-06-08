'''Test idlelib.macosx.py
'''
from idlelib import macosx
from test.support import requires
import sys
import tkinter as tk
import unittest
import unittest.mock as mock

MAC = sys.platform == 'darwin'
mactypes = {'carbon', 'cocoa', 'xquartz'}
nontypes = {'other'}
alltypes = mactypes | nontypes


class InitTktypeTest(unittest.TestCase):
    "Test _init_tk_type."

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_init_sets_tktype(self):
        "Test that _init_tk_type sets _tk_type according to platform."
        for root in (None, self.root):
            with self.subTest(root=root):
                macosx._tk_type == None
                macosx._init_tk_type(root)
                self.assertIn(macosx._tk_type,
                              mactypes if MAC else nontypes)


class IsTypeTkTest(unittest.TestCase):
    "Test each of the four isTypeTk predecates."
    isfuncs = ((macosx.isAquaTk, ('carbon', 'cocoa')),
               (macosx.isCarbonTk, ('carbon')),
               (macosx.isCocoaTk, ('cocoa')),
               (macosx.isXQuartz, ('xquartz')),
               )

    @mock.patch('idlelib.macosx._init_tk_type')
    def test_is_calls_init(self, mockinit):
        "Test that each isTypeTk calls _init_tk_type when _tk_type is None."
        macosx._tk_type = None
        for func, whentrue in self.isfuncs:
            with self.subTest(func=func):
                func()
                self.assertTrue(mockinit.called)
                mockinit.reset_mock()

    def test_isfuncs(self):
        "Test that each isTypeTk return correct bool."
        for func, whentrue in self.isfuncs:
            for tktype in alltypes:
                with self.subTest(func=func, whentrue=whentrue, tktype=tktype):
                    macosx._tk_type = tktype
                    (self.assertTrue if tktype in whentrue else self.assertFalse)\
                                     (func())


if __name__ == '__main__':
    unittest.main(verbosity=2)
