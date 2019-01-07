"Test debugger, coverage 19%"

from idlelib import debugger
import unittest
from unittest import mock
from test.support import requires
# requires('gui')
from tkinter import Tk
from textwrap import dedent

"""A test python script for the debug tests."""
TEST_CODE = dedent("""
    i = 1
    i += 2
    if i == 3:
       print(i)
    """)


class NameSpaceTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_init(self):
        debugger.NamespaceViewer(self.root, 'Test')


class MockFrameType(object):
    """Reflection of the types.FrameType."""
    f_back = None
    f_builtins = None
    f_code = None
    f_globals = None
    f_lasti = None
    f_lineno = None
    f_locals = None
    f_restricted = 0
    f_trace = None

    def __init__(self):
        pass


class IdbTest(unittest.TestCase):

    def test_init_runs_bdb_init(self):
        """Test that Idb calls the base Bdb __init__."""
        idb = debugger.Idb(None)
        assert idb.gui is None
        assert hasattr(idb, 'breaks')

    def test_user_line_basic_frame(self):
        """Test that .user_line() creates a string message for a frame."""

        # Create a test code object to simulate a debug session.
        code_obj = compile(TEST_CODE,
                           'test_user_line_basic_frame.py',
                           mode='exec')

        # Create 2 test frames for lines 1 and 2 of the test code.
        test_frame1 = MockFrameType()
        test_frame1.f_code = code_obj
        test_frame1.f_lineno = 1

        test_frame2 = MockFrameType()
        test_frame2.f_code = code_obj
        test_frame2.f_lineno = 2
        test_frame2.f_back = test_frame1

        gui = mock.Mock()
        gui.interaction = mock.Mock()

        idb = debugger.Idb(gui)
        idb.user_line(test_frame2)

        self.assertFalse(idb.in_rpc_code(test_frame2))
        gui.interaction.assert_called_once()
        gui.interaction.assert_called_with('test_user_line_basic_frame.py:2: <module>()', test_frame2)

    def test_user_exception(self):
        """Test that .user_exception() creates a string message for a frame."""

        # Create a test code object to simulate a debug session.
        code_obj = compile(TEST_CODE,
                           'test_user_exception.py',
                           mode='exec')

        # Create 1 test frame
        test_frame1 = MockFrameType()
        test_frame1.f_code = code_obj
        test_frame1.f_lineno = 1

        # Example from sys.exc_info()
        test_exc_info = (type(ValueError), ValueError(), None)

        gui = mock.Mock()
        gui.interaction = mock.Mock()

        idb = debugger.Idb(gui)
        idb.user_exception(test_frame1, test_exc_info)

        self.assertFalse(idb.in_rpc_code(test_frame1))
        gui.interaction.assert_called_once()
        gui.interaction.assert_called_with('test_user_exception.py:1: <module>()', test_frame1, test_exc_info)

    def test_in_rpc_code_rpc_py(self):
        """Test that .in_rpc_code detects position of rpc.py."""

        # Create a test code object to simulate a debug session.
        code_obj = compile(TEST_CODE,
                           'rpc.py',
                           mode='exec')

        # Create 1 test frame
        test_frame = MockFrameType()
        test_frame.f_code = code_obj
        test_frame.f_lineno = 1

        gui = mock.Mock()
        gui.interaction = mock.Mock()

        idb = debugger.Idb(gui)

        self.assertTrue(idb.in_rpc_code(test_frame))

    def test_in_rpc_code_debugger_star_dot_py(self):
        """Test that .in_rpc_code detects position of idlelib/debugger*.py."""

        # Create a test code object to simulate a debug session.
        for filename in ('idlelib/debugger.py', 'idlelib/debugger_r.py'):

            code_obj = compile(TEST_CODE,
                               filename,
                               mode='exec')

            # Create 2 test frames
            test_frame = MockFrameType()
            test_frame.f_code = code_obj
            test_frame.f_lineno = 1

            test_frame2 = MockFrameType()
            test_frame2.f_code = code_obj
            test_frame2.f_lineno = 2
            test_frame2.f_back = test_frame

            gui = mock.Mock()
            gui.interaction = mock.Mock()

            idb = debugger.Idb(gui)

            self.assertFalse(idb.in_rpc_code(test_frame2))


def make_pyshell_mock():
    """Factory for generating test fixtures of PyShell."""
    pyshell = mock.Mock()
    pyshell.root = None
    return pyshell


class DebuggerTest(unittest.TestCase):
    """Tests for the idlelib.debugger.Debugger class."""

    def test_setup_debugger(self):
        """Test that Debugger can be instantiated with a mock PyShell."""
        pyshell = make_pyshell_mock()
        test_debugger = debugger.Debugger(pyshell)

        self.assertEquals(test_debugger.pyshell, pyshell)
        self.assertIsNone(test_debugger.frame)

    def test_run_debugger_with_idb(self):
        """Test Debugger.run() with an Idb instance."""
        mock_idb = mock.Mock()  # Mocked debugger.Idb
        test_debugger = debugger.Debugger(make_pyshell_mock(), idb=mock_idb)
        test_debugger.run(1, 'two')
        mock_idb.run.assert_called_once()
        mock_idb.run.called_with(1, 'two')
        self.assertEquals(test_debugger.interacting, 0)

    def test_run_debugger_no_idb(self):
        """Test Debugger.run() with no Idb instance."""
        test_debugger = debugger.Debugger(make_pyshell_mock(), idb=None)
        self.assertIsNotNone(test_debugger.idb)
        test_debugger.idb.run = mock.Mock()
        test_debugger.run(1, 'two')
        test_debugger.idb.run.assert_called_once()
        test_debugger.idb.run.called_with(1, 'two')
        self.assertEquals(test_debugger.interacting, 0)

    def test_close(self):
        """Test closing the window in an idle state."""
        pyshell = make_pyshell_mock()
        test_debugger = debugger.Debugger(pyshell)
        test_debugger.close()
        pyshell.close_debugger.assert_called_once()

    def test_close_whilst_interacting(self):
        """Test closing the window in an interactive state."""
        pyshell = make_pyshell_mock()
        test_debugger = debugger.Debugger(pyshell)
        test_debugger.interacting = 1
        test_debugger.close()
        pyshell.close_debugger.assert_not_called()

    def test_interaction_with_message_and_frame(self):
        """Test the interaction sets the window message."""
        test_message = "testing 1234.."
        test_frame = MockFrameType()
        test_code = compile(TEST_CODE, 'test_interaction.py', 'exec')

        test_frame.f_code = test_code
        test_frame.f_lineno = 1

        pyshell = make_pyshell_mock()

        test_debugger = debugger.Debugger(pyshell)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()

        # Patch out the status label so we can check messages
        test_debugger.status = mock.Mock()
        test_debugger.interaction(test_message, test_frame)

        # Check the test message was displayed and cleared
        test_debugger.status.configure.assert_has_calls([mock.call(text='testing 1234..'), mock.call(text='')])

    def test_interaction_with_message_and_frame_and_exc_info(self):
        """Test the interaction sets the window message with exception info."""
        test_message = "testing 1234.."
        test_frame = MockFrameType()
        test_exc_info = (type(ValueError), ValueError(), None)
        test_code = compile(TEST_CODE, 'test_interaction.py', 'exec')

        test_frame.f_code = test_code
        test_frame.f_lineno = 1

        pyshell = make_pyshell_mock()

        test_debugger = debugger.Debugger(pyshell)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()

        # Patch out the status label so we can check messages
        test_debugger.status = mock.Mock()
        test_debugger.interaction(test_message, test_frame, test_exc_info)

        # Check the test message was displayed and cleared
        test_debugger.status.configure.assert_has_calls([mock.call(text='testing 1234..'), mock.call(text='')])

    def test_sync_source_line(self):
        """Test that .sync_source_line() will set the flist.gotofileline with fixed frame."""
        pyshell = make_pyshell_mock()

        test_debugger = debugger.Debugger(pyshell)

        test_frame = MockFrameType()
        test_code = compile(TEST_CODE, 'test_sync.py', 'exec')

        test_frame.f_code = test_code
        test_frame.f_lineno = 1

        test_debugger.frame = test_frame

        # Patch out the file list
        test_debugger.flist = mock.Mock()

        # Pretend file exists
        with mock.patch('idlelib.debugger.os.path.exists', return_value=True):
            test_debugger.sync_source_line()

        test_debugger.flist.gotofileline.assert_called_once_with('test_sync.py', 1)

    def test_cont(self):
        """Test the .cont() method calls idb.set_continue()."""
        pyshell = make_pyshell_mock()
        idb = mock.Mock()
        test_debugger = debugger.Debugger(pyshell, idb)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()

        test_debugger.cont()

        # Check set_continue was called on the idb instance.
        idb.set_continue.assert_called_once()

    def test_step(self):
        """Test the .step() method calls idb.set_step()."""
        pyshell = make_pyshell_mock()
        idb = mock.Mock()
        test_debugger = debugger.Debugger(pyshell, idb)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()

        test_debugger.step()

        # Check set_step was called on the idb instance.
        idb.set_step.assert_called_once()

    def test_next(self):
        """Test the .next() method calls idb.set_next()."""
        pyshell = make_pyshell_mock()
        idb = mock.Mock()
        test_debugger = debugger.Debugger(pyshell, idb)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()
        test_frame = MockFrameType()

        test_debugger.frame = test_frame
        test_debugger.next()

        # Check set_next was called on the idb instance.
        idb.set_next.assert_called_once_with(test_frame)

    def test_ret(self):
        """Test the .ret() method calls idb.set_return()."""
        pyshell = make_pyshell_mock()
        idb = mock.Mock()
        test_debugger = debugger.Debugger(pyshell, idb)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()
        test_frame = MockFrameType()

        test_debugger.frame = test_frame
        test_debugger.ret()

        # Check set_return was called on the idb instance.
        idb.set_return.assert_called_once_with(test_frame)

    def test_quit(self):
        """Test the .quit() method calls idb.set_quit()."""
        pyshell = make_pyshell_mock()
        idb = mock.Mock()
        test_debugger = debugger.Debugger(pyshell, idb)

        # Patch out the root window and tk session so we can test them
        test_debugger.root = mock.Mock()
        test_debugger.root.tk = mock.Mock()

        test_debugger.quit()

        # Check set_quit was called on the idb instance.
        idb.set_quit.assert_called_once_with()


if __name__ == '__main__':
    unittest.main(verbosity=2)
