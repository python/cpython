import unittest
import tkinter
from tkinter import font
from test.support import requires, run_unittest, gc_collect, ALWAYS_EQ
from tkinter.test.support import AbstractTkTest, AbstractDefaultRootTest

requires('gui')

fontname = "TkDefaultFont"

class FontTest(AbstractTkTest, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        AbstractTkTest.setUpClass.__func__(cls)
        try:
            cls.font = font.Font(root=cls.root, name=fontname, exists=True)
        except tkinter.TclError:
            cls.font = font.Font(root=cls.root, name=fontname, exists=False)

    def test_configure(self):
        options = self.font.configure()
        self.assertGreaterEqual(set(options),
            {'family', 'size', 'weight', 'slant', 'underline', 'overstrike'})
        for key in options:
            self.assertEqual(self.font.cget(key), options[key])
            self.assertEqual(self.font[key], options[key])
        for key in 'family', 'weight', 'slant':
            self.assertIsInstance(options[key], str)
            self.assertIsInstance(self.font.cget(key), str)
            self.assertIsInstance(self.font[key], str)
        sizetype = int if self.wantobjects else str
        for key in 'size', 'underline', 'overstrike':
            self.assertIsInstance(options[key], sizetype)
            self.assertIsInstance(self.font.cget(key), sizetype)
            self.assertIsInstance(self.font[key], sizetype)

    def test_unicode_family(self):
        family = 'MS \u30b4\u30b7\u30c3\u30af'
        try:
            f = font.Font(root=self.root, family=family, exists=True)
        except tkinter.TclError:
            f = font.Font(root=self.root, family=family, exists=False)
        self.assertEqual(f.cget('family'), family)
        del f
        gc_collect()

    def test_actual(self):
        options = self.font.actual()
        self.assertGreaterEqual(set(options),
            {'family', 'size', 'weight', 'slant', 'underline', 'overstrike'})
        for key in options:
            self.assertEqual(self.font.actual(key), options[key])
        for key in 'family', 'weight', 'slant':
            self.assertIsInstance(options[key], str)
            self.assertIsInstance(self.font.actual(key), str)
        sizetype = int if self.wantobjects else str
        for key in 'size', 'underline', 'overstrike':
            self.assertIsInstance(options[key], sizetype)
            self.assertIsInstance(self.font.actual(key), sizetype)

    def test_name(self):
        self.assertEqual(self.font.name, fontname)
        self.assertEqual(str(self.font), fontname)

    def test_eq(self):
        font1 = font.Font(root=self.root, name=fontname, exists=True)
        font2 = font.Font(root=self.root, name=fontname, exists=True)
        self.assertIsNot(font1, font2)
        self.assertEqual(font1, font2)
        self.assertNotEqual(font1, font1.copy())
        self.assertNotEqual(font1, 0)
        self.assertEqual(font1, ALWAYS_EQ)

    def test_measure(self):
        self.assertIsInstance(self.font.measure('abc'), int)

    def test_metrics(self):
        metrics = self.font.metrics()
        self.assertGreaterEqual(set(metrics),
            {'ascent', 'descent', 'linespace', 'fixed'})
        for key in metrics:
            self.assertEqual(self.font.metrics(key), metrics[key])
            self.assertIsInstance(metrics[key], int)
            self.assertIsInstance(self.font.metrics(key), int)

    def test_families(self):
        families = font.families(self.root)
        self.assertIsInstance(families, tuple)
        self.assertTrue(families)
        for family in families:
            self.assertIsInstance(family, str)
            self.assertTrue(family)

    def test_names(self):
        names = font.names(self.root)
        self.assertIsInstance(names, tuple)
        self.assertTrue(names)
        for name in names:
            self.assertIsInstance(name, str)
            self.assertTrue(name)
        self.assertIn(fontname, names)

    def test_repr(self):
        self.assertEqual(
            repr(self.font), f'<tkinter.font.Font object {fontname!r}>'
        )


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_families(self):
        self.assertRaises(RuntimeError, font.families)
        root = tkinter.Tk()
        families = font.families()
        self.assertIsInstance(families, tuple)
        self.assertTrue(families)
        for family in families:
            self.assertIsInstance(family, str)
            self.assertTrue(family)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, font.families)

    def test_names(self):
        self.assertRaises(RuntimeError, font.names)
        root = tkinter.Tk()
        names = font.names()
        self.assertIsInstance(names, tuple)
        self.assertTrue(names)
        for name in names:
            self.assertIsInstance(name, str)
            self.assertTrue(name)
        self.assertIn(fontname, names)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, font.names)


tests_gui = (FontTest, DefaultRootTest)

if __name__ == "__main__":
    run_unittest(*tests_gui)
