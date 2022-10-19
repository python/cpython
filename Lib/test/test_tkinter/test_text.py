import unittest
import tkinter
from test.support import requires
from test.test_tkinter.support import AbstractTkTest

requires('gui')

class TextTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.text = tkinter.Text(self.root)

    def test_debug(self):
        text = self.text
        olddebug = text.debug()
        try:
            text.debug(0)
            self.assertEqual(text.debug(), 0)
            text.debug(1)
            self.assertEqual(text.debug(), 1)
        finally:
            text.debug(olddebug)
            self.assertEqual(text.debug(), olddebug)

    def test_search(self):
        text = self.text

        # pattern and index are obligatory arguments.
        self.assertRaises(tkinter.TclError, text.search, None, '1.0')
        self.assertRaises(tkinter.TclError, text.search, 'a', None)
        self.assertRaises(tkinter.TclError, text.search, None, None)

        # Invalid text index.
        self.assertRaises(tkinter.TclError, text.search, '', 0)

        # Check if we are getting the indices as strings -- you are likely
        # to get Tcl_Obj under Tk 8.5 if Tkinter doesn't convert it.
        text.insert('1.0', 'hi-test')
        self.assertEqual(text.search('-test', '1.0', 'end'), '1.2')
        self.assertEqual(text.search('test', '1.0', 'end'), '1.3')

    def test_count(self):
        # XXX Some assertions do not check against the intended result,
        # but instead check the current result to prevent regression.
        text = self.text
        text.insert('1.0',
            'Lorem ipsum dolor sit amet,\n'
            'consectetur adipiscing elit,\n'
            'sed do eiusmod tempor incididunt\n'
            'ut labore et dolore magna aliqua.')

        options = ('chars', 'indices', 'lines',
                   'displaychars', 'displayindices', 'displaylines',
                   'xpixels', 'ypixels')
        if self.wantobjects:
            self.assertEqual(len(text.count('1.0', 'end', *options)), 8)
        else:
            text.count('1.0', 'end', *options)
        self.assertEqual(text.count('1.0', 'end', 'chars', 'lines'), (124, 4)
                         if self.wantobjects else '124 4')
        self.assertEqual(text.count('1.3', '4.5', 'chars', 'lines'), (92, 3)
                         if self.wantobjects else '92 3')
        self.assertEqual(text.count('4.5', '1.3', 'chars', 'lines'), (-92, -3)
                         if self.wantobjects else '-92 -3')
        self.assertEqual(text.count('1.3', '1.3', 'chars', 'lines'), (0, 0)
                         if self.wantobjects else '0 0')
        self.assertEqual(text.count('1.0', 'end', 'lines'), (4,)
                         if self.wantobjects else ('4',))
        self.assertEqual(text.count('end', '1.0', 'lines'), (-4,)
                         if self.wantobjects else ('-4',))
        self.assertEqual(text.count('1.3', '1.5', 'lines'), None
                         if self.wantobjects else ('0',))
        self.assertEqual(text.count('1.3', '1.3', 'lines'), None
                         if self.wantobjects else ('0',))
        self.assertEqual(text.count('1.0', 'end'), (124,)  # 'indices' by default
                         if self.wantobjects else ('124',))
        self.assertRaises(tkinter.TclError, text.count, '1.0', 'end', 'spam')
        # '-lines' is ignored, 'indices' is used by default
        self.assertEqual(text.count('1.0', 'end', '-lines'), (124,)
                         if self.wantobjects else ('124',))

        self.assertIsInstance(text.count('1.3', '1.5', 'ypixels'), tuple)
        self.assertIsInstance(text.count('1.3', '1.5', 'update', 'ypixels'), int
                              if self.wantobjects else str)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'ypixels'), None
                         if self.wantobjects else '0')
        self.assertEqual(text.count('1.3', '1.5', 'update', 'indices'), 2
                         if self.wantobjects else '2')
        self.assertEqual(text.count('1.3', '1.3', 'update', 'indices'), None
                         if self.wantobjects else '0')
        self.assertEqual(text.count('1.3', '1.5', 'update'), (2,)
                         if self.wantobjects else ('2',))
        self.assertEqual(text.count('1.3', '1.3', 'update'), None
                         if self.wantobjects else ('0',))


if __name__ == "__main__":
    unittest.main()
