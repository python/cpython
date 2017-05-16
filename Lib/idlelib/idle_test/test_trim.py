import unittest
import idlelib.trim as trim
from idlelib.editor import EditorWindow as Editor
from tkinter import Tk


class trimTest(unittest.TestCase):

    def test_trim_line(self):
        editor = Editor(root=Tk())
        text = editor.text
        do_trim = trim.TrimExtension(editor).do_trim

        do_trim()
        self.assertEqual(text.get('1.0', 'insert'), '')
        text.insert('1.0', '     ')
        do_trim()
        self.assertEqual(text.get('1.0', 'insert'), '')
        text.insert('1.0', '     \n')
        do_trim()
        self.assertEqual(text.get('1.0', 'insert'), '')

    def test_trim_multiple(self):
        editor = Editor(root=Tk())
        text = editor.text
        do_trim = trim.TrimExtension(editor).do_trim

        original = (
            "Line with an ending tab\t\n"
            "Line ending in 5 spaces     \n"
            "Linewithnospaces\n"
            "    indented line\n"
            "    indented line with trailing space \n"
            "    \n"
            "\n"
            "       \n"
            "\n")
        stripped = (
            "Line with an ending tab\n"
            "Line ending in 5 spaces\n"
            "Linewithnospaces\n"
            "    indented line\n"
            "    indented line with trailing space\n")

        text.insert('1.0', original)
        do_trim()
        self.assertEqual(text.get('1.0', 'insert'), stripped)


if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
