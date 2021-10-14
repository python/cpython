"Test , coverage 17%."

from idlelib import iomenu
import unittest
from test.support import requires
from tkinter import Tk
from idlelib.editor import EditorWindow


class IOBindingTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.editwin = EditorWindow(root=cls.root)
        cls.io = iomenu.IOBinding(cls.editwin)

    @classmethod
    def tearDownClass(cls):
        cls.io.close()
        cls.editwin._close()
        del cls.editwin
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        self.assertIs(self.io.editwin, self.editwin)

    def test_fixnewlines_end(self):
        eq = self.assertEqual
        io = self.io
        fix = io.fixnewlines
        text = io.editwin.text
        self.editwin.interp = None
        eq(fix(), '')
        del self.editwin.interp
        text.insert(1.0, 'a')
        eq(fix(), 'a'+io.eol_convention)
        eq(text.get('1.0', 'end-1c'), 'a\n')
        eq(fix(), 'a'+io.eol_convention)

    def check_filetype(self, ext):
        return any(
            ext in filetype_tuple[1] for filetype_tuple in self.io.filetypes
        )

    def test_python_files(self):
        self.assertTrue(self.check_filetype('*.py'))
        self.assertTrue(self.check_filetype('*.pyw'))

    def test_python_stub_files(self):
        self.assertTrue(self.check_filetype('*.pyi'))

    def test_text_files(self):
        self.assertTrue(self.check_filetype('*.txt'))

    def test_all_files(self):
        self.assertTrue(self.check_filetype('*'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
