"""Test debugger, coverage 66%

Try to make tests pass with draft bdbx, which may replace bdb in 3.13+.
"""

from idlelib import debugger
from collections import namedtuple
from textwrap import dedent
from tkinter import Tk

from test.support import requires
import unittest
from unittest import mock
from unittest.mock import Mock, patch

"""A test python script for the debug tests."""
TEST_CODE = dedent("""
    i = 1
    i += 2
    if i == 3:
       print(i)
    """)


class MockFrame:
    "Minimal mock frame."

    def __init__(self, code, lineno):
        self.f_code = code
        self.f_lineno = lineno


class IdbTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.gui = Mock()
        cls.idb = debugger.Idb(cls.gui)

        # Create test and code objects to simulate a debug session.
        code_obj = compile(TEST_CODE, 'idlelib/file.py', mode='exec')
        frame1 = MockFrame(code_obj, 1)
        frame1.f_back = None
        frame2 = MockFrame(code_obj, 2)
        frame2.f_back = frame1
        cls.frame = frame2
        cls.msg = 'file.py:2: <module>()'

    def test_init(self):
        self.assertIs(self.idb.gui, self.gui)
        # Won't test super call since two Bdbs are very different.

    def test_user_line(self):
        # Test that .user_line() creates a string message for a frame.
        self.gui.interaction = Mock()
        self.idb.user_line(self.frame)
        self.gui.interaction.assert_called_once_with(self.msg, self.frame)

    def test_user_exception(self):
        # Test that .user_exception() creates a string message for a frame.
        exc_info = (type(ValueError), ValueError(), None)
        self.gui.interaction = Mock()
        self.idb.user_exception(self.frame, exc_info)
        self.gui.interaction.assert_called_once_with(
                self.msg, self.frame, exc_info)


class FunctionTest(unittest.TestCase):
    # Test module functions together.

    def test_functions(self):
        rpc_obj = compile(TEST_CODE,'rpc.py', mode='exec')
        rpc_frame = MockFrame(rpc_obj, 2)
        rpc_frame.f_back = rpc_frame
        self.assertTrue(debugger._in_rpc_code(rpc_frame))
        self.assertEqual(debugger._frame2message(rpc_frame),
                         'rpc.py:2: <module>()')

        code_obj = compile(TEST_CODE, 'idlelib/debugger.py', mode='exec')
        code_frame = MockFrame(code_obj, 1)
        code_frame.f_back = None
        self.assertFalse(debugger._in_rpc_code(code_frame))
        self.assertEqual(debugger._frame2message(code_frame),
                         'debugger.py:1: <module>()')

        code_frame.f_back = code_frame
        self.assertFalse(debugger._in_rpc_code(code_frame))
        code_frame.f_back = rpc_frame
        self.assertTrue(debugger._in_rpc_code(code_frame))


class DebuggerTest(unittest.TestCase):
    "Tests for Debugger that do not need a real root."

    @classmethod
    def setUpClass(cls):
        cls.pyshell = Mock()
        cls.pyshell.root = Mock()
        cls.idb = Mock()
        with patch.object(debugger.Debugger, 'make_gui'):
            cls.debugger = debugger.Debugger(cls.pyshell, cls.idb)
        cls.debugger.root = Mock()

    def test_cont(self):
        self.debugger.cont()
        self.idb.set_continue.assert_called_once()

    def test_step(self):
        self.debugger.step()
        self.idb.set_step.assert_called_once()

    def test_quit(self):
        self.debugger.quit()
        self.idb.set_quit.assert_called_once()

    def test_next(self):
        with patch.object(self.debugger, 'frame') as frame:
            self.debugger.next()
            self.idb.set_next.assert_called_once_with(frame)

    def test_ret(self):
        with patch.object(self.debugger, 'frame') as frame:
            self.debugger.ret()
            self.idb.set_return.assert_called_once_with(frame)

    def test_clear_breakpoint(self):
        self.debugger.clear_breakpoint('test.py', 4)
        self.idb.clear_break.assert_called_once_with('test.py', 4)

    def test_clear_file_breaks(self):
        self.debugger.clear_file_breaks('test.py')
        self.idb.clear_all_file_breaks.assert_called_once_with('test.py')

    def test_set_load_breakpoints(self):
        # Test the .load_breakpoints() method calls idb.
        FileIO = namedtuple('FileIO', 'filename')

        class MockEditWindow(object):
            def __init__(self, fn, breakpoints):
                self.io = FileIO(fn)
                self.breakpoints = breakpoints

        self.pyshell.flist = Mock()
        self.pyshell.flist.inversedict = (
            MockEditWindow('test1.py', [4, 4]),
            MockEditWindow('test2.py', [13, 44, 45]),
        )
        self.debugger.set_breakpoint('test0.py', 1)
        self.idb.set_break.assert_called_once_with('test0.py', 1)
        self.debugger.load_breakpoints()  # Call set_breakpoint 5 times.
        self.idb.set_break.assert_has_calls(
            [mock.call('test0.py', 1),
             mock.call('test1.py', 4),
             mock.call('test1.py', 4),
             mock.call('test2.py', 13),
             mock.call('test2.py', 44),
             mock.call('test2.py', 45)])

    def test_sync_source_line(self):
        # Test that .sync_source_line() will set the flist.gotofileline with fixed frame.
        test_code = compile(TEST_CODE, 'test_sync.py', 'exec')
        test_frame = MockFrame(test_code, 1)
        self.debugger.frame = test_frame

        self.debugger.flist = Mock()
        with patch('idlelib.debugger.os.path.exists', return_value=True):
            self.debugger.sync_source_line()
        self.debugger.flist.gotofileline.assert_called_once_with('test_sync.py', 1)


class DebuggerGuiTest(unittest.TestCase):
    """Tests for debugger.Debugger that need tk root.

    close needs debugger.top set in make_gui.
    """

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = root = Tk()
        root.withdraw()
        cls.pyshell = Mock()
        cls.pyshell.root = root
        cls.idb = Mock()
# stack tests fail with debugger here.
##        cls.debugger = debugger.Debugger(cls.pyshell, cls.idb)
##        cls.debugger.root = root
##        # real root needed for real make_gui
##        # run, interacting, abort_loop

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.debugger = debugger.Debugger(self.pyshell, self.idb)
        self.debugger.root = self.root
        # real root needed for real make_gui
        # run, interacting, abort_loop

    def test_run_debugger(self):
        self.debugger.run(1, 'two')
        self.idb.run.assert_called_once_with(1, 'two')
        self.assertEqual(self.debugger.interacting, 0)

    def test_close(self):
        # Test closing the window in an idle state.
        self.debugger.close()
        self.pyshell.close_debugger.assert_called_once()

    def test_show_stack(self):
        self.debugger.show_stack()
        self.assertEqual(self.debugger.stackviewer.gui, self.debugger)

    def test_show_stack_with_frame(self):
        test_frame = MockFrame(None, None)
        self.debugger.frame = test_frame

        # Reset the stackviewer to force it to be recreated.
        self.debugger.stackviewer = None
        self.idb.get_stack.return_value = ([], 0)
        self.debugger.show_stack()

        # Check that the newly created stackviewer has the test gui as a field.
        self.assertEqual(self.debugger.stackviewer.gui, self.debugger)
        self.idb.get_stack.assert_called_once_with(test_frame, None)


class StackViewerTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.code = compile(TEST_CODE, 'test_stackviewer.py', 'exec')
        self.stack = [
            (MockFrame(self.code, 1), 1),
            (MockFrame(self.code, 2), 2)
        ]
        # Create a stackviewer and load the test stack.
        self.sv = debugger.StackViewer(self.root, None, None)
        self.addCleanup(self.sv.close)
        self.sv.load_stack(self.stack)

    def test_init(self):
        # Test creation of StackViewer.
        gui = None
        flist = None
        master_window = self.root
        sv = debugger.StackViewer(master_window, flist, gui)
        self.assertHasAttr(sv, 'stack')

    def test_load_stack(self):
        # Test the .load_stack() method against a fixed test stack.
        # A row holds the module, the function, the line and its source.
        self.assertEqual(self.sv.stack, self.stack)
        self.assertEqual(self.sv.get(0), ('?', '<module>', '1', ''))
        self.assertEqual(self.sv.get(1), ('?', '<module>', '2', ''))

    def test_load_stack_marks_the_current_frame(self):
        self.sv.load_stack(self.stack, 1)
        tree = self.sv.tree
        rows = tree.get_children()
        self.assertEqual(tree.item(rows[0], 'tags'), (self.sv.PLAIN,))
        self.assertEqual(tree.item(rows[1], 'tags'), (self.sv.CURRENT,))
        # The marker is a tag now, not a prefix on the module name.
        self.assertFalse(self.sv.get(1)[0].startswith('> '))
        # Both tags carry an image, so that the texts line up.
        self.assertTrue(tree.tag_configure(self.sv.PLAIN, 'image'))
        self.assertTrue(tree.tag_configure(self.sv.CURRENT, 'image'))
        self.assertEqual(self.sv.index(), 1)

    def test_load_empty_stack(self):
        self.sv.load_stack([])
        self.assertEqual(self.sv.get(0), (self.sv.default, '', '', ''))
        self.assertIsNone(self.sv.index())

    def test_load_stack_does_not_show_the_frame(self):
        # Only the user selecting a row tells the debugger to show a frame.
        self.sv.gui = Mock()
        self.sv.load_stack(self.stack, 1)
        self.sv.gui.show_frame.assert_not_called()

    def test_select_shows_the_frame(self):
        self.sv.gui = Mock()
        self.sv.tree.selection_set(self.sv.tree.get_children()[1])
        # Tk queues the event that selecting a row sends; deliver it now.
        self.sv.tree.event_generate('<<TreeviewSelect>>', when='now')
        self.sv.gui.show_frame.assert_called_once_with(self.stack[1])

    def test_show_stack_frame(self):
        self.sv.gui = Mock()
        self.sv.select(1)
        self.sv.show_stack_frame()
        self.sv.gui.show_frame.assert_called_once_with(self.stack[1])

    def test_show_source(self):
        # Test the .show_source() method against a fixed test stack.
        # Patch out the file list to monitor it
        self.sv.flist = Mock()
        # Patch out isfile to pretend file exists.
        with patch('idlelib.debugger.os.path.isfile', return_value=True) as isfile:
            self.sv.show_source(1)
            isfile.assert_called_once_with('test_stackviewer.py')
            self.sv.flist.open.assert_called_once_with('test_stackviewer.py')


class NameSpaceTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.nv = debugger.NamespaceViewer(self.root, 'Test')
        self.addCleanup(self.nv.close)

    def rows(self):
        "Return the (name, value) pairs shown."
        tree = self.nv.treeview.tree
        return [tree.item(iid, 'values') for iid in tree.get_children()]

    def test_init(self):
        self.assertEqual(self.rows(), [('None', '')])

    def test_load_dict(self):
        self.nv.load_dict({'b': 2, 'a': 'spam'})  # Names are sorted.
        self.assertEqual(self.rows(), [('a', "'spam'"), ('b', '2')])

    def test_load_no_dict(self):
        self.nv.load_dict({'a': 1})
        self.nv.load_dict(None)
        self.assertEqual(self.rows(), [('None', '')])

    def test_load_same_dict(self):
        odict = {'a': 1}
        self.nv.load_dict(odict)
        odict['a'] = 2
        self.nv.load_dict(odict)  # The same dict is not read again.
        self.assertEqual(self.rows(), [('a', '1')])
        self.nv.load_dict(odict, force=1)
        self.assertEqual(self.rows(), [('a', '2')])

    def test_load_dict_from_the_subprocess(self):
        # The values arrive as reprs; the quotes added by repr'ing them
        # again are stripped.
        self.nv.load_dict({'a': "'spam'"}, rpc_client=Mock())
        self.assertEqual(self.rows(), [('a', "'spam'")])

    def test_height_is_limited(self):
        self.nv.load_dict({'a': 1, 'b': 2})
        self.assertEqual(int(self.nv.treeview.tree['height']), 2)
        self.nv.load_dict({'v%d' % i: i for i in range(40)})
        self.assertEqual(int(self.nv.treeview.tree['height']),
                         self.nv.maxrows)


if __name__ == '__main__':
    unittest.main(verbosity=2)
