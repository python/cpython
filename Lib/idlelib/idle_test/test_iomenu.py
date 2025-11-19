import builtins
import os
import tempfile
import unittest
from unittest.mock import patch

from test.support import requires
from tkinter import Tk

from idlelib import iomenu, util
from idlelib.editor import EditorWindow
from idlelib.idle_test.mock_idle import Func

# Fail if either tokenize.open and t.detect_encoding does not exist.
# These are used in loadfile and encode.
# Also used in pyshell.MI.execfile and runscript.tabnanny.
from tokenize import open as tokenize_open, detect_encoding
# Remove when we have proper tests that use both.


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

    def _create_tempfile(self, content: str) -> str:
        fd, filename = tempfile.mkstemp(suffix='.py')
        os.close(fd)
        self.addCleanup(os.unlink, filename)
        with builtins.open(filename, 'w', encoding='utf-8') as f:
            f.write(content)
        return filename

    def test_init(self):
        self.assertIs(self.io.editwin, self.editwin)

    def test_fixnewlines_end(self):
        eq = self.assertEqual
        io = self.io
        fix = io.fixnewlines
        text = io.editwin.text

        self.editwin.interp = None
        shelltext = '>>> if 1'
        self.editwin.get_prompt_text = Func(result=shelltext)
        eq(fix(), shelltext)
        del self.editwin.interp, self.editwin.get_prompt_text

        text.insert(1.0, 'a')
        eq(fix(), 'a' + io.eol_convention)
        eq(text.get('1.0', 'end-1c'), 'a\n')
        eq(fix(), 'a' + io.eol_convention)

    def test_reload_no_file(self):
        io = self.io
        io.filename = None

        with patch('idlelib.iomenu.messagebox.showinfo') as mock_showinfo:
            result = io.reload(None)
            self.assertEqual(result, "break")
            mock_showinfo.assert_called_once()
            args, kwargs = mock_showinfo.call_args
            self.assertIn("File Not Found", args[0])

    def test_reload_with_file(self):
        io = self.io
        text = io.editwin.text
        original_content = "# Original content\n"
        modified_content = "# Modified content\n"

        filename = self._create_tempfile(original_content)
        io.filename = filename

        with patch('idlelib.iomenu.messagebox.showerror') as mock_showerror:
            io.loadfile(io.filename)
            self.assertEqual(text.get('1.0', 'end-1c'), original_content)

            with builtins.open(filename, 'w', encoding='utf-8') as f:
                f.write(modified_content)

            result = io.reload(None)

        mock_showerror.assert_not_called()
        self.assertEqual(result, "break")
        self.assertEqual(text.get('1.0', 'end-1c'), modified_content)

    def test_reload_with_unsaved_changes_cancel(self):
        io = self.io
        text = io.editwin.text
        original_content = "# Original content\n"
        unsaved_content = original_content + "\n# Unsaved change"

        filename = self._create_tempfile(original_content)
        io.filename = filename
        io.loadfile(io.filename)

        text.insert('end', "\n# Unsaved change")
        io.set_saved(False)

        with patch('idlelib.iomenu.messagebox.askokcancel', return_value=False) as mock_ask:
            result = io.reload(None)

        self.assertEqual(result, "break")
        self.assertEqual(text.get('1.0', 'end-1c'), unsaved_content)
        mock_ask.assert_called_once()

    def test_reload_with_unsaved_changes_confirm(self):
        io = self.io
        text = io.editwin.text
        original_content = "# Original content\n"

        filename = self._create_tempfile(original_content)
        io.filename = filename
        io.loadfile(io.filename)

        text.insert('end', "\n# Unsaved change")
        io.set_saved(False)

        with patch('idlelib.iomenu.messagebox.askokcancel', return_value=True) as mock_ask:
            result = io.reload(None)

        self.assertEqual(result, "break")
        self.assertEqual(text.get('1.0', 'end-1c'), original_content)
        mock_ask.assert_called_once()


def _extension_in_filetypes(extension):
    return any(
        f'*{extension}' in filetype_tuple[1]
        for filetype_tuple in iomenu.IOBinding.filetypes
    )


class FiletypesTest(unittest.TestCase):
    def test_python_source_files(self):
        for extension in util.py_extensions:
            with self.subTest(extension=extension):
                self.assertTrue(
                    _extension_in_filetypes(extension)
                )

    def test_text_files(self):
        self.assertTrue(_extension_in_filetypes('.txt'))

    def test_all_files(self):
        self.assertTrue(_extension_in_filetypes(''))


if __name__ == '__main__':
    unittest.main(verbosity=2)
