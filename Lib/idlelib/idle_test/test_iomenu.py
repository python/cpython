"Test , coverage 17%."

from idlelib import iomenu
import unittest
from unittest.mock import patch, mock_open
from test.support import requires
from tkinter import Tk
from idlelib.editor import EditorWindow
from idlelib import util
from idlelib.idle_test.mock_idle import Func

# Fail if either tokenize.open and t.detect_encoding does not exist.
# These are used in loadfile and encode.
# Also used in pyshell.MI.execfile and runscript.tabnanny.
from tokenize import open, detect_encoding
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

    def test_init(self):
        self.assertIs(self.io.editwin, self.editwin)

    def test_fixnewlines_end(self):
        eq = self.assertEqual
        io = self.io
        fix = io.fixnewlines
        text = io.editwin.text

        # Make the editor temporarily look like Shell.
        self.editwin.interp = None
        shelltext = '>>> if 1'
        self.editwin.get_prompt_text = Func(result=shelltext)
        eq(fix(), shelltext)  # Get... call and '\n' not added.
        del self.editwin.interp, self.editwin.get_prompt_text

        text.insert(1.0, 'a')
        eq(fix(), 'a'+io.eol_convention)
        eq(text.get('1.0', 'end-1c'), 'a\n')
        eq(fix(), 'a'+io.eol_convention)

    def test_reload_no_file(self):
        # Test reload when no file is associated
        io = self.io
        io.filename = None

        with patch.object(iomenu.messagebox, 'showinfo') as mock_showinfo:
            result = io.reload(None)
            self.assertEqual(result, "break")
            mock_showinfo.assert_called_once()
            args, kwargs = mock_showinfo.call_args
            self.assertIn("File Not Found", args[0])

    @patch('idlelib.iomenu.messagebox.showerror')
    def test_reload_with_file(self, mock_showerror):
        # Test reload with an actual file
        io = self.io
        text = io.editwin.text
        io.filename = "/dummy/path/test.py"

        original_content = "# Original content\n"
        modified_content = "# Modified content\n"

        m = mock_open()
        m.side_effect = [
            mock_open(read_data=original_content).return_value,
            mock_open(read_data=modified_content).return_value,
        ]

        with patch('builtins.open', m):
            io.loadfile(io.filename)
            self.assertEqual(text.get('1.0', 'end-1c'), original_content)
            result = io.reload(None)

        mock_showerror.assert_not_called()
        self.assertEqual(result, "break")
        self.assertEqual(text.get('1.0', 'end-1c'), modified_content)

    def test_reload_with_unsaved_changes_cancel(self):
        # Test reload with unsaved changes and user cancels
        io = self.io
        text = io.editwin.text
        io.filename = "/dummy/path/test.py"
        original_content = "# Original content\n"
        unsaved_content = original_content + "\n# Unsaved change"

        # Mock the initial file load.
        with patch('builtins.open', mock_open(read_data=original_content)):
            io.loadfile(io.filename)

        text.insert('end', "\n# Unsaved change")
        io.set_saved(False)

        with patch('idlelib.iomenu.messagebox.askokcancel', return_value=False) as mock_ask:
            result = io.reload(None)
            self.assertEqual(result, "break")
            # Content should not change.
            self.assertEqual(text.get('1.0', 'end-1c'), unsaved_content)
            mock_ask.assert_called_once()

    def test_reload_with_unsaved_changes_confirm(self):
        # Test reload with unsaved changes and user confirms
        io = self.io
        text = io.editwin.text
        io.filename = "/dummy/path/test.py"
        original_content = "# Original content\n"

        with patch('builtins.open', mock_open(read_data=original_content)):
            io.loadfile(io.filename)
            text.insert('end', "\n# Unsaved change")
            io.set_saved(False)

            with patch('idlelib.iomenu.messagebox.askokcancel', return_value=True) as mock_ask:
                result = io.reload(None)

        self.assertEqual(result, "break")
        # Content should be reverted to original.
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
