import unittest
import tkinter
from tkinter import colorchooser
from test.support import requires, run_unittest
from tkinter.test.support import AbstractTkTest

requires('gui')

class ChooserTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        cls.cc = colorchooser.Chooser(initialcolor='dark blue slate')

    def test_fixoptions(self):
        cc = self.cc
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], 'dark blue slate')

        cc.options['initialcolor'] = '#D2D269691E1E'
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], '#D2D269691E1E')

        cc.options['initialcolor'] = (210, 105, 30)
        cc._fixoptions()
        self.assertEqual(cc.options['initialcolor'], '#d2691e')

    def test_fixresult(self):
        cc = self.cc
        self.assertEqual(cc._fixresult(self.root, ()), (None, None))
        self.assertEqual(cc._fixresult(self.root, ''), (None, None))
        self.assertEqual(cc._fixresult(self.root, 'chocolate'),
                         ((210, 105, 30), 'chocolate'))
        self.assertEqual(cc._fixresult(self.root, '#1e90ff'),
                         ((30, 144, 255), '#1e90ff'))


tests_gui = (ChooserTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
