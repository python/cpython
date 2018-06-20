"Test rstrip, coverage 100%."

from idlelib import rstrip
import unittest
from idlelib.idle_test.mock_idle import Editor

class rstripTest(unittest.TestCase):

    def test_rstrip_line(self):
        editor = Editor()
        text = editor.text
        do_rstrip = rstrip.Rstrip(editor).do_rstrip

        do_rstrip()
        self.assertEqual(text.get('1.0', 'insert'), '')
        text.insert('1.0', '     ')
        do_rstrip()
        self.assertEqual(text.get('1.0', 'insert'), '')
        text.insert('1.0', '     \n')
        do_rstrip()
        self.assertEqual(text.get('1.0', 'insert'), '\n')

    def test_rstrip_multiple(self):
        editor = Editor()
        #  Comment above, uncomment 3 below to test with real Editor & Text.
        #from idlelib.editor import EditorWindow as Editor
        #from tkinter import Tk
        #editor = Editor(root=Tk())
        text = editor.text
        do_rstrip = rstrip.Rstrip(editor).do_rstrip

        original = (
            "Line with an ending tab    \n"
            "Line ending in 5 spaces     \n"
            "Linewithnospaces\n"
            "    indented line\n"
            "    indented line with trailing space \n"
            "    ")
        stripped = (
            "Line with an ending tab\n"
            "Line ending in 5 spaces\n"
            "Linewithnospaces\n"
            "    indented line\n"
            "    indented line with trailing space\n")

        text.insert('1.0', original)
        do_rstrip()
        self.assertEqual(text.get('1.0', 'insert'), stripped)



if __name__ == '__main__':
    unittest.main(verbosity=2)
