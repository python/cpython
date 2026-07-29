import unittest
import tkinter
from tkinter import font
from test.support import requires, gc_collect, ALWAYS_EQ
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest, AbstractDefaultRootTest

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
        self.assertEqual(self.font.config, self.font.configure)
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
        self.assertRaisesRegex(tkinter.TclError, 'bad option "-spam"',
                               self.font.cget, 'spam')
        self.assertRaisesRegex(tkinter.TclError, 'bad option "-spam"',
                               self.font.configure, spam='x')
        self.assertRaises(TypeError, self.font.cget)
        self.assertRaises(TypeError, self.font.cget, 'size', 'weight')

    def test_create_from_named_font(self):
        # gh-143990: a font created from a named font copies its configured
        # options, preserving a size specified in pixels (a negative size).
        sizetype = int if self.wantobjects else str
        named = font.Font(root=self.root, name='my named font',  # name with spaces
                          family='Times', size=-20, weight='bold')
        # The source is the name of a named font or a Font representing one.
        for source in ['my named font', named]:
            with self.subTest(source=source):
                f = font.Font(root=self.root, font=source)
                self.assertEqual(f.cget('size'), sizetype(-20))
                self.assertEqual(f.actual('family'), named.actual('family'))
                self.assertEqual(f.actual('weight'), 'bold')

    def test_create_from_description(self):
        # gh-143990: a font created from a font description is resolved via
        # "font actual", so a size in pixels (negative) becomes a size in points.
        descriptions = [
            ('Times', -20),                     # tuple
            ('Times', -20, 'bold'),             # tuple with a style
            'Times -20',                        # string
            'Times -20 bold',                   # string with a style
            '{Times New Roman} -20',            # string, family with spaces
        ]
        for desc in descriptions:
            with self.subTest(font=desc):
                f = font.Font(root=self.root, font=desc)
                self.assertGreater(int(f.cget('size')), 0)  # pixels -> points

    def test_copy(self):
        # size=-20 (pixels): copy() copies the configured options, so the
        # size is preserved rather than resolved (gh-143990).
        f = font.Font(root=self.root, family='Times', size=-20, weight='bold')
        copied = f.copy()
        self.assertIsInstance(copied, font.Font)
        self.assertIsNot(copied, f)
        self.assertNotEqual(copied.name, f.name)
        self.assertEqual(copied.actual(), f.actual())
        sizetype = int if self.wantobjects else str
        self.assertEqual(copied.cget('size'), sizetype(-20))
        # The copy is independent of the original.
        copied.configure(size=20)
        self.assertEqual(f.cget('size'), sizetype(-20))
        self.assertEqual(copied.cget('size'), sizetype(20))
        self.assertRaises(TypeError, f.copy, 'x')

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
        self.assertRaisesRegex(tkinter.TclError, 'bad option "-spam"',
                               self.font.actual, 'spam')
        self.assertRaises(TypeError, self.font.actual, 'size', 'weight', 'slant')

    def test_name(self):
        self.assertEqual(self.font.name, fontname)
        self.assertEqual(str(self.font), fontname)

    def test_equality(self):
        font1 = font.Font(root=self.root, name=fontname, exists=True)
        font2 = font.Font(root=self.root, name=fontname, exists=True)
        self.assertIsNot(font1, font2)
        self.assertEqual(font1, font2)
        self.assertNotEqual(font1, font1.copy())

        self.assertNotEqual(font1, 0)
        self.assertEqual(font1, ALWAYS_EQ)

        root2 = tkinter.Tk()
        self.addCleanup(root2.destroy)
        font3 = font.Font(root=root2, name=fontname, exists=True)
        self.assertEqual(str(font1), str(font3))
        self.assertNotEqual(font1, font3)

    def test_measure(self):
        self.assertIsInstance(self.font.measure('abc'), int)
        self.assertEqual(self.font.measure(''), 0)
        self.assertIsInstance(
            self.font.measure('abc', displayof=self.root), int)
        self.assertRaises(TypeError, self.font.measure)
        self.assertRaises(TypeError, self.font.measure, 'a', 'b', 'c')

    def test_metrics(self):
        metrics = self.font.metrics()
        self.assertGreaterEqual(set(metrics),
            {'ascent', 'descent', 'linespace', 'fixed'})
        for key in metrics:
            self.assertEqual(self.font.metrics(key), metrics[key])
            self.assertEqual(self.font.metrics(key, displayof=self.root),
                             metrics[key])
            self.assertIsInstance(metrics[key], int)
            self.assertIsInstance(self.font.metrics(key), int)
        self.assertRaisesRegex(tkinter.TclError, 'bad metric "-spam"',
                               self.font.metrics, 'spam')

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

    def test_nametofont(self):
        testfont = font.nametofont(fontname, root=self.root)
        self.assertIsInstance(testfont, font.Font)
        self.assertEqual(testfont.name, fontname)

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

    def test_nametofont(self):
        self.assertRaises(RuntimeError, font.nametofont, fontname)
        root = tkinter.Tk()
        testfont = font.nametofont(fontname)
        self.assertIsInstance(testfont, font.Font)
        self.assertEqual(testfont.name, fontname)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, font.nametofont, fontname)


if __name__ == "__main__":
    unittest.main()
