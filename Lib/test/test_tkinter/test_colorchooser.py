import unittest
import tkinter
from test.support import requires, swap_attr
from tkinter.test.support import AbstractDefaultRootTest, AbstractTkTest
from tkinter import colorchooser
from tkinter.colorchooser import askcolor
from tkinter.commondialog import Dialog

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
        self.assertEqual(cc._fixresult(self.root, '#4a3c8c'),
                         ((74, 60, 140), '#4a3c8c'))


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_askcolor(self):
        def test_callback(dialog, master):
            nonlocal ismapped
            master.update()
            ismapped = master.winfo_ismapped()
            raise ZeroDivisionError

        with swap_attr(Dialog, '_test_callback', test_callback):
            ismapped = None
            self.assertRaises(ZeroDivisionError, askcolor)
            #askcolor()
            self.assertEqual(ismapped, False)

            root = tkinter.Tk()
            ismapped = None
            self.assertRaises(ZeroDivisionError, askcolor)
            self.assertEqual(ismapped, True)
            root.destroy()

            tkinter.NoDefaultRoot()
            self.assertRaises(RuntimeError, askcolor)


if __name__ == "__main__":
    unittest.main()
