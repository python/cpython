"Test , coverage 17%."

from idlelib import iomenu
import unittest
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
        import tempfile
        import os
        from unittest.mock import Mock

        io = self.io
        # Ensure no filename is set
        io.filename = None

        # Mock the messagebox.showinfo
        orig_showinfo = iomenu.messagebox.showinfo
        showinfo_called = []
        def mock_showinfo(*args, **kwargs):
            showinfo_called.append((args, kwargs))
        iomenu.messagebox.showinfo = mock_showinfo

        try:
            result = io.reload(None)
            self.assertEqual(result, "break")
            self.assertEqual(len(showinfo_called), 1)
            self.assertIn("No File", showinfo_called[0][0])
        finally:
            iomenu.messagebox.showinfo = orig_showinfo

    def test_reload_with_file(self):
        # Test reload with an actual file
        import tempfile
        import os

        io = self.io
        text = io.editwin.text

        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.py') as f:
            f.write("# Original content\n")
            temp_filename = f.name

        try:
            # Load the file
            io.loadfile(temp_filename)
            self.assertEqual(text.get('1.0', 'end-1c'), "# Original content\n")

            # Modify the file content externally
            with open(temp_filename, 'w') as f:
                f.write("# Modified content\n")

            # Reload should update the content
            result = io.reload(None)
            self.assertEqual(result, "break")
            self.assertEqual(text.get('1.0', 'end-1c'), "# Modified content\n")
        finally:
            os.unlink(temp_filename)

    def test_reload_with_unsaved_changes_cancel(self):
        # Test reload with unsaved changes and user cancels
        import tempfile
        import os

        io = self.io
        text = io.editwin.text

        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.py') as f:
            f.write("# Original content\n")
            temp_filename = f.name

        try:
            # Load the file
            io.loadfile(temp_filename)

            # Make unsaved changes
            text.insert('end-1c', "\n# Unsaved change")
            io.set_saved(False)

            # Mock askokcancel to return False (cancel)
            orig_askokcancel = iomenu.messagebox.askokcancel
            iomenu.messagebox.askokcancel = lambda *args, **kwargs: False

            try:
                result = io.reload(None)
                self.assertEqual(result, "break")
                # Content should not change
                self.assertIn("# Unsaved change", text.get('1.0', 'end-1c'))
            finally:
                iomenu.messagebox.askokcancel = orig_askokcancel
        finally:
            os.unlink(temp_filename)

    def test_reload_with_unsaved_changes_confirm(self):
        # Test reload with unsaved changes and user confirms
        import tempfile
        import os

        io = self.io
        text = io.editwin.text

        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.py') as f:
            f.write("# Original content\n")
            temp_filename = f.name

        try:
            # Load the file
            io.loadfile(temp_filename)

            # Make unsaved changes
            text.insert('end-1c', "\n# Unsaved change")
            io.set_saved(False)

            # Mock askokcancel to return True (confirm)
            orig_askokcancel = iomenu.messagebox.askokcancel
            iomenu.messagebox.askokcancel = lambda *args, **kwargs: True

            try:
                result = io.reload(None)
                self.assertEqual(result, "break")
                # Content should be reverted to original
                self.assertEqual(text.get('1.0', 'end-1c'), "# Original content\n")
            finally:
                iomenu.messagebox.askokcancel = orig_askokcancel
        finally:
            os.unlink(temp_filename)


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
