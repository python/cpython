import unittest
import tkinter
from test.support import requires
from test.test_tkinter.support import AbstractTkTest

requires('gui')

class TextTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.text = tkinter.Text(self.root)
        self.text.pack()

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
        self.assertRaises(tkinter.TclError, text.search, '', '')
        self.assertRaises(tkinter.TclError, text.search, '', 'invalid')
        self.assertRaises(tkinter.TclError, text.search, '', '1.0', 'invalid')

        text.insert('1.0',
            'This is a test. This is only a test.\n'
            'Another line.\n'
            'Yet another line.\n'
            '64-bit')

        self.assertEqual(text.search('test', '1.0'), '1.10')
        self.assertEqual(text.search('test', '1.0', 'end'), '1.10')
        self.assertEqual(text.search('test', '1.0', '1.10'), '')
        self.assertEqual(text.search('test', '1.11'), '1.31')
        self.assertEqual(text.search('test', '1.32', 'end'), '')
        self.assertEqual(text.search('test', '1.32'), '1.10')

        self.assertEqual(text.search('', '1.0'), '1.0')  # empty pattern
        self.assertEqual(text.search('nonexistent', '1.0'), '')
        self.assertEqual(text.search('-bit', '1.0'), '4.2')  # starts with a hyphen

        self.assertEqual(text.search('line', '3.0'), '3.12')
        self.assertEqual(text.search('line', '3.0', forwards=True), '3.12')
        self.assertEqual(text.search('line', '3.0', backwards=True), '2.8')
        self.assertEqual(text.search('line', '3.0', forwards=True, backwards=True), '2.8')

        self.assertEqual(text.search('t.', '1.0'), '1.13')
        self.assertEqual(text.search('t.', '1.0', exact=True), '1.13')
        self.assertEqual(text.search('t.', '1.0', regexp=True), '1.10')
        self.assertEqual(text.search('t.', '1.0', exact=True, regexp=True), '1.10')

        self.assertEqual(text.search('TEST', '1.0'), '')
        self.assertEqual(text.search('TEST', '1.0', nocase=True), '1.10')

        var = tkinter.Variable(self.root)
        self.assertEqual(text.search('test', '1.0', count=var), '1.10')
        self.assertEqual(var.get(), 4 if self.wantobjects else '4')

        # TODO: Add test for elide=True

    def test_count(self):
        text = self.text
        text.insert('1.0',
            'Lorem ipsum dolor sit amet,\n'
            'consectetur adipiscing elit,\n'
            'sed do eiusmod tempor incididunt\n'
            'ut labore et dolore magna aliqua.')

        options = ('chars', 'indices', 'lines',
                   'displaychars', 'displayindices', 'displaylines',
                   'xpixels', 'ypixels')
        self.assertEqual(len(text.count('1.0', 'end', *options, return_ints=True)), 8)
        self.assertEqual(len(text.count('1.0', 'end', *options)), 8)
        self.assertEqual(text.count('1.0', 'end', 'chars', 'lines', return_ints=True),
                         (124, 4))
        self.assertEqual(text.count('1.3', '4.5', 'chars', 'lines'), (92, 3))
        self.assertEqual(text.count('4.5', '1.3', 'chars', 'lines', return_ints=True),
                         (-92, -3))
        self.assertEqual(text.count('4.5', '1.3', 'chars', 'lines'), (-92, -3))
        self.assertEqual(text.count('1.3', '1.3', 'chars', 'lines', return_ints=True),
                         (0, 0))
        self.assertEqual(text.count('1.3', '1.3', 'chars', 'lines'), (0, 0))
        self.assertEqual(text.count('1.0', 'end', 'lines', return_ints=True), 4)
        self.assertEqual(text.count('1.0', 'end', 'lines'), (4,))
        self.assertEqual(text.count('end', '1.0', 'lines', return_ints=True), -4)
        self.assertEqual(text.count('end', '1.0', 'lines'), (-4,))
        self.assertEqual(text.count('1.3', '1.5', 'lines', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.5', 'lines'), None)
        self.assertEqual(text.count('1.3', '1.3', 'lines', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'lines'), None)
        # Count 'indices' by default.
        self.assertEqual(text.count('1.0', 'end', return_ints=True), 124)
        self.assertEqual(text.count('1.0', 'end'), (124,))
        self.assertEqual(text.count('1.0', 'end', 'indices', return_ints=True), 124)
        self.assertEqual(text.count('1.0', 'end', 'indices'), (124,))
        self.assertRaises(tkinter.TclError, text.count, '1.0', 'end', 'spam')
        self.assertRaises(tkinter.TclError, text.count, '1.0', 'end', '-lines')

        self.assertIsInstance(text.count('1.3', '1.5', 'ypixels', return_ints=True), int)
        self.assertIsInstance(text.count('1.3', '1.5', 'ypixels'), tuple)
        self.assertIsInstance(text.count('1.3', '1.5', 'update', 'ypixels', return_ints=True), int)
        self.assertIsInstance(text.count('1.3', '1.5', 'update', 'ypixels'), int)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'ypixels', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'ypixels'), None)
        self.assertEqual(text.count('1.3', '1.5', 'update', 'indices', return_ints=True), 2)
        self.assertEqual(text.count('1.3', '1.5', 'update', 'indices'), 2)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'indices', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update', 'indices'), None)
        self.assertEqual(text.count('1.3', '1.5', 'update', return_ints=True), 2)
        self.assertEqual(text.count('1.3', '1.5', 'update'), (2,))
        self.assertEqual(text.count('1.3', '1.3', 'update', return_ints=True), 0)
        self.assertEqual(text.count('1.3', '1.3', 'update'), None)


if __name__ == "__main__":
    unittest.main()
