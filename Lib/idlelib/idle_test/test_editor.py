import unittest
from tkinter import Tk, Text
from idlelib.EditorWindow import EditorWindow
from test.support import requires

class Editor_func_test(unittest.TestCase):
    def test_filename_to_unicode(self):
        func = EditorWindow._filename_to_unicode
        class dummy(): filesystemencoding = 'utf-8'
        pairs = (('abc', 'abc'), ('a\U00011111c', 'a\ufffdc'),
                 (b'abc', 'abc'), (b'a\xf0\x91\x84\x91c', 'a\ufffdc'))
        for inp, out in pairs:
            self.assertEqual(func(dummy, inp), out)

if __name__ == '__main__':
    unittest.main(verbosity=2)
