"Test runscript, coverage 16%."

from idlelib import runscript
import unittest
from unittest import mock
from test.support import requires
from tkinter import Tk
from idlelib.editor import EditorWindow


class ScriptBindingTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        for id in cls.root.after_info():
            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        ew = EditorWindow(root=self.root)
        sb = runscript.ScriptBinding(ew)
        ew._close()

    def test_checksyntax_compile_error(self):
        # gh-69919: any error raised by compile() is reported.
        ew = EditorWindow(root=self.root)
        sb = runscript.ScriptBinding(ew)
        sb.flist = mock.Mock()
        sb.errorbox = mock.Mock()
        with (mock.patch('idlelib.runscript.compile', create=True,
                         side_effect=MemoryError()),
              mock.patch('idlelib.runscript.open', mock.mock_open(read_data=b'x\n'))):
            self.assertFalse(sb.checksyntax('test.py'))
        sb.errorbox.assert_called_once_with('MemoryError', '<no detail available>')
        ew._close()

    def test_run_module_event_shell_busy_no_restart(self):
        # gh-82183: running without restarting the busy shell aborts.
        ew = EditorWindow(root=self.root)
        sb = runscript.ScriptBinding(ew)
        sb.getfilename = mock.Mock(return_value='test.py')
        sb.checksyntax = mock.Mock(return_value='code')
        sb.tabnanny = mock.Mock(return_value=True)
        sb.shell = shell = mock.Mock()
        shell.executing = True
        interp = shell.interp
        with mock.patch.object(runscript, 'CustomRun') as customrun:
            # Restart shell unchecked.
            customrun.return_value.result = (['arg'], False)
            result = sb.run_module_event(None, customize=True)
        self.assertEqual(result, 'break')
        interp.display_executing_dialog.assert_called_once()
        interp.restart_subprocess.assert_not_called()
        interp.runcode.assert_not_called()
        ew._close()

    def test_checksyntax_reports_error_in_shell(self):
        ew = EditorWindow(root=self.root)
        self.addCleanup(ew._close)

        sb = runscript.ScriptBinding(ew)
        shell = mock.Mock()
        sb.flist = mock.Mock()
        sb.flist.open_shell.return_value = shell

        source = b"def hello()\n    print('hello')\n"

        with (
            mock.patch("builtins.open", mock.mock_open(read_data=source)),
            mock.patch.object(sb, "errorbox") as errorbox,
            mock.patch.object(ew, "colorize_syntax_error") as colorize,
        ):
            result = sb.checksyntax("example.py")
        self.assertFalse(result)

        # The Shell receives the complete syntax diagnostic.
        output = ''.join(
            call.args[0] for call in shell.stderr.write.call_args_list
        )
        self.assertIn("example.py", output)
        self.assertIn("line 1", output)
        self.assertIn("def hello()", output)
        self.assertIn("^", output)
        self.assertIn("SyntaxError: expected ':'", output)

        # No modal error dialog.
        errorbox.assert_not_called()

        # Preserve error highlighting in the editor.
        colorize.assert_called_once()
        # Make the Shell visible and focus it.
        shell.top.lift.assert_called_once()
        shell.text.focus_set.assert_called_once()


if __name__ == '__main__':
    unittest.main(verbosity=2)
