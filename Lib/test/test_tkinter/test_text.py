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

        # Check if we are getting the indices as strings -- you are likely
        # to get Tcl_Obj under Tk 8.5 if Tkinter doesn't convert it.
        text.insert('1.0', 'hi-test')
        self.assertEqual(text.search('-test', '1.0', 'end'), '1.2')
        self.assertEqual(text.search('test', '1.0', 'end'), '1.3')

        text.delete('1.0', 'end')
        text.insert('1.0',
            'This is a test. This is only a test.\n'
            'Another line.\n'
            'Yet another line.')

        result = text.search('line', '1.0', 'end', nolinestop=True, regexp=True)
        self.assertEqual(result, '2.8')

        strict_res = text.search('test', '1.0', '1.20', strictlimits=True)
        self.assertEqual(strict_res, '1.10')

    def test_search_all(self):
        text = self.text
        text.insert('1.0',
            'ababa ababa\n'
            'ababababa\n'
            'aba aba')

        all_res = text.search_all('aba', '1.0', 'end')
        all_res_strs = [str(i) for i in all_res]
        self.assertIsInstance(all_res, tuple)
        self.assertGreaterEqual(len(all_res), 3)
        self.assertEqual(str(all_res[0]), '1.0')
        self.assertEqual(str(all_res[1]), '1.6')

        overlap_res = text.search_all('aba', '1.0', 'end', overlap=True)
        overlap_res_strs = [str(i) for i in overlap_res]
        self.assertIsInstance(overlap_res, tuple)
        self.assertGreater(len(overlap_res), len(all_res))

        # Check that overlap actually finds overlapping matches
        self.assertIn('2.0', overlap_res_strs)
        self.assertIn('2.2', overlap_res_strs)
        self.assertIn('2.4', overlap_res_strs)
        self.assertNotIn('2.2', all_res_strs)

        # Ensure all results are valid text indices
        for i in overlap_res:
            self.assertRegex(str(i), r'^\d+\.\d+$')

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
