import unittest
import tkinter
from tkinter import colorchooser
from test.support import requires, run_unittest, gc_collect
from tkinter.test.support import AbstractTkTest

requires('gui')

class ChooserTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        cls.cc = colorchooser.Chooser(initialcolor='red')

    def test_fixoptions(self):
        cc = self.cc
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], 'red')

        cc.options['initialcolor'] = '#ffff00000000'
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], '#ffff00000000')

        cc.options['initialcolor'] = (255, 0, 0)
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], '#ff0000')

    def test_fixresult(self):
        cc = self.cc
        self.assertEqual(cc._fixresult(self.root, ()), (None, None))
        self.assertEqual(cc._fixresult(self.root, ''), (None, None))
        self.assertEqual(cc._fixresult(self.root, 'red'),
                         ((255, 0, 0), 'red'))
        self.assertEqual(cc._fixresult(self.root, '#ff0000'),
                         ((255, 0, 0), '#ff0000'))


tests_gui = (ChooserTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
