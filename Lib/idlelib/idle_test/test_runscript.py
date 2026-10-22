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


if __name__ == '__main__':
    unittest.main(verbosity=2)
