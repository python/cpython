"Test debugger, coverage 19%"

from idlelib import debugger
import unittest
from unittest import mock
from test.support import requires
requires('gui')
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


if __name__ == '__main__':
    unittest.main(verbosity=2)
