"Test debugger, coverage 19%"

from idlelib import debugger
import unittest
from unittest import mock
from test.support import requires
# requires('gui')
from tkinter import Tk
from textwrap import dedent


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
        test_code = dedent("""
        i = 1
        i += 2
        if i == 3:
           print(i)
        """)
        code_obj = compile(test_code,
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

        assert not idb.in_rpc_code(test_frame2)
        gui.interaction.assert_called_once()
        gui.interaction.assert_called_with('test_user_line_basic_frame.py:2: <module>()', test_frame2)

    def test_user_exception(self):
        """Test that .user_exception() creates a string message for a frame."""

        # Create a test code object to simulate a debug session.
        test_code = dedent("""
        i = 1
        i += 2
        if i == 3:
           print(i)
        """)
        code_obj = compile(test_code,
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

        assert not idb.in_rpc_code(test_frame1)
        gui.interaction.assert_called_once()
        gui.interaction.assert_called_with('test_user_exception.py:1: <module>()', test_frame1, test_exc_info)

    def test_in_rpc_code_rpc_py(self):
        """Test that .in_rpc_code detects position of rpc.py."""

        # Create a test code object to simulate a debug session.
        test_code = dedent("""
            i = 1
            i += 2
            if i == 3:
               print(i)
            """)
        code_obj = compile(test_code,
                           'rpc.py',
                           mode='exec')

        # Create 1 test frame
        test_frame = MockFrameType()
        test_frame.f_code = code_obj
        test_frame.f_lineno = 1

        gui = mock.Mock()
        gui.interaction = mock.Mock()

        idb = debugger.Idb(gui)

        assert idb.in_rpc_code(test_frame)

    def test_in_rpc_code_debugger_star_dot_py(self):
        """Test that .in_rpc_code detects position of idlelib/debugger*.py."""

        # Create a test code object to simulate a debug session.
        for filename in ('idlelib/debugger.py', 'idlelib/debugger_r.py'):
            test_code = dedent("""
                    i = 1
                    i += 2
                    if i == 3:
                       print(i)
                    """)
            code_obj = compile(test_code,
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

            assert not idb.in_rpc_code(test_frame2)


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

        assert test_debugger.pyshell == pyshell
        assert test_debugger.frame is None

    def test_run_debugger_with_idb(self):
        """Test Debugger.run() with an Idb instance."""
        mock_idb = mock.Mock()  # Mocked debugger.Idb
        test_debugger = debugger.Debugger(make_pyshell_mock(), idb=mock_idb)
        test_debugger.run(1, 'two')
        mock_idb.run.assert_called_once()
        mock_idb.run.called_with(1, 'two')
        assert test_debugger.interacting == 0

    def test_run_debugger_no_idb(self):
        """Test Debugger.run() with no Idb instance."""
        test_debugger = debugger.Debugger(make_pyshell_mock(), idb=None)
        assert test_debugger.idb is not None
        test_debugger.idb.run = mock.Mock()
        test_debugger.run(1, 'two')
        test_debugger.idb.run.assert_called_once()
        test_debugger.idb.run.called_with(1, 'two')
        assert test_debugger.interacting == 0

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
        test_message = "testing 1234.."
        test_frame = MockFrameType()

        pyshell = make_pyshell_mock()
        test_debugger = debugger.Debugger(pyshell)

        # Patch out the status label so we can check messages
        test_debugger.status = mock.Mock()
        test_debugger.interaction(test_message, test_frame)


if __name__ == '__main__':
    unittest.main(verbosity=2)
