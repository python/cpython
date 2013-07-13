import unittest
import idlelib.RstripExtension as rs
from idlelib.idle_test.mock_idle import Editor

class rstripTest(unittest.TestCase):

    def test_rstrip_line(self):
        editor = Editor()
        text = editor.text
        do_rstrip = rs.RstripExtension(editor).do_rstrip

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
        #  Uncomment following to verify that test passes with real widgets.
##        from idlelib.EditorWindow import EditorWindow as Editor
##        from tkinter import Tk
##        editor = Editor(root=Tk())
        text = editor.text
        do_rstrip = rs.RstripExtension(editor).do_rstrip

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
    unittest.main(verbosity=2, exit=False)
