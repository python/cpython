"""Test idlelib.codecontext.

Coverage: 100%
"""

import re

import unittest
from unittest import mock
from test.support import requires
from tkinter import Tk, Frame, Text, TclError

import idlelib.codecontext as codecontext
from idlelib import config


usercfg = codecontext.idleConf.userCfg
testcfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}
code_sample = """\

class C1():
    # Class comment.
    def __init__(self, a, b):
        self.a = a
        self.b = b
    def compare(self):
        if a > b:
            return a
        elif a < b:
            return b
        else:
            return None
"""


class DummyEditwin:
    def __init__(self, root, frame, text):
        self.root = root
        self.top = root
        self.text_frame = frame
        self.text = text


class CodeContextTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = Tk()
        root.withdraw()
        frame = cls.frame = Frame(root)
        text = cls.text = Text(frame)
        text.insert('1.0', code_sample)
        # Need to pack for creation of code context label widget.
        frame.pack(side='left', fill='both', expand=1)
        text.pack(side='top', fill='both', expand=1)
        cls.editor = DummyEditwin(root, frame, text)
        codecontext.idleConf.userCfg = testcfg

    @classmethod
    def tearDownClass(cls):
        codecontext.idleConf.userCfg = usercfg
        cls.editor.text.delete('1.0', 'end')
        del cls.editor, cls.frame, cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.cc = codecontext.CodeContext(self.editor)

    def tearDown(self):
        if self.cc.label:
            self.cc.label.destroy()
        # Explicitly call __del__ to remove scheduled scripts.
        self.cc.__del__()
        del self.cc.label, self.cc

    def test_init(self):
        eq = self.assertEqual
        ed = self.editor
        cc = self.cc

        eq(cc.editwin, ed)
        eq(cc.text, ed.text)
        eq(cc.textfont, ed.text['font'])
        self.assertIsNone(cc.label)
        eq(cc.info, [(0, -1, '', False)])
        eq(cc.topvisible, 1)
        eq(self.root.tk.call('after', 'info', self.cc.t1)[1], 'timer')
        eq(self.root.tk.call('after', 'info', self.cc.t2)[1], 'timer')

    def test_del(self):
        self.cc.__del__()
        with self.assertRaises(TclError) as msg:
            self.root.tk.call('after', 'info', self.cc.t1)
            self.assertIn("doesn't exist", msg)
        with self.assertRaises(TclError) as msg:
            self.root.tk.call('after', 'info', self.cc.t2)
            self.assertIn("doesn't exist", msg)
        # For coverage on the except.  Have to delete because the
        # above Tcl error is caught by after_cancel.
        del self.cc.t1, self.cc.t2
        self.cc.__del__()

    def test_reload(self):
        codecontext.CodeContext.reload()
        self.assertEqual(self.cc.context_depth, 3)

    def test_toggle_code_context_event(self):
        eq = self.assertEqual
        cc = self.cc
        toggle = cc.toggle_code_context_event

        # Make sure code context is off.
        if cc.label:
            toggle()

        # Toggle on.
        eq(toggle(), 'break')
        self.assertIsNotNone(cc.label)
        eq(cc.label['font'], cc.textfont)
        eq(cc.label['fg'], cc.fgcolor)
        eq(cc.label['bg'], cc.bgcolor)
        eq(cc.label['text'], '\n' * 2)

        # Toggle off.
        eq(toggle(), 'break')
        self.assertIsNone(cc.label)

    def test_get_context(self):
        eq = self.assertEqual
        gc = self.cc.get_context

        # stopline must be greater than 0.
        with self.assertRaises(AssertionError):
            gc(1, stopline=0)

        eq(gc(3), ([(2, 0, 'class C1():', 'class')], 0))

        # Don't return comment.
        eq(gc(4), ([(2, 0, 'class C1():', 'class')], 0))

        # Two indentation levels and no comment.
        eq(gc(5), ([(2, 0, 'class C1():', 'class'),
                    (4, 4, '    def __init__(self, a, b):', 'def')], 0))

        # Only one 'def' is returned, not both at the same indent level.
        eq(gc(10), ([(2, 0, 'class C1():', 'class'),
                     (7, 4, '    def compare(self):', 'def'),
                     (8, 8, '        if a > b:', 'if')], 0))

        # With 'elif', also show the 'if' even though it's at the same level.
        eq(gc(11), ([(2, 0, 'class C1():', 'class'),
                     (7, 4, '    def compare(self):', 'def'),
                     (8, 8, '        if a > b:', 'if'),
                     (10, 8, '        elif a < b:', 'elif')], 0))

        # Set stop_line to not go back to first line in source code.
        # Return includes stop_line.
        eq(gc(11, stopline=2), ([(2, 0, 'class C1():', 'class'),
                                 (7, 4, '    def compare(self):', 'def'),
                                 (8, 8, '        if a > b:', 'if'),
                                 (10, 8, '        elif a < b:', 'elif')], 0))
        eq(gc(11, stopline=3), ([(7, 4, '    def compare(self):', 'def'),
                                 (8, 8, '        if a > b:', 'if'),
                                 (10, 8, '        elif a < b:', 'elif')], 4))
        eq(gc(11, stopline=8), ([(8, 8, '        if a > b:', 'if'),
                                 (10, 8, '        elif a < b:', 'elif')], 8))

        # Set stop_indent to test indent level to stop at.
        eq(gc(11, stopindent=4), ([(7, 4, '    def compare(self):', 'def'),
                                   (8, 8, '        if a > b:', 'if'),
                                   (10, 8, '        elif a < b:', 'elif')], 4))
        # Check that the 'if' is included.
        eq(gc(11, stopindent=8), ([(8, 8, '        if a > b:', 'if'),
                                   (10, 8, '        elif a < b:', 'elif')], 8))

    def test_update_code_context(self):
        eq = self.assertEqual
        cc = self.cc
        # Ensure code context is active.
        if not cc.label:
            cc.toggle_code_context_event()

        # Invoke update_code_context without scrolling - nothing happens.
        self.assertIsNone(cc.update_code_context())
        eq(cc.info, [(0, -1, '', False)])
        eq(cc.topvisible, 1)

        # Scroll down to line 2.
        cc.text.yview(2)
        cc.update_code_context()
        eq(cc.info, [(0, -1, '', False), (2, 0, 'class C1():', 'class')])
        eq(cc.topvisible, 3)
        # context_depth is 3 so it pads with blank lines.
        eq(cc.label['text'], '\n'
                             '\n'
                             'class C1():')

        # Scroll down to line 3.  Since it's a comment, nothing changes.
        cc.text.yview(3)
        cc.update_code_context()
        eq(cc.info, [(0, -1, '', False), (2, 0, 'class C1():', 'class')])
        eq(cc.topvisible, 4)
        eq(cc.label['text'], '\n'
                             '\n'
                             'class C1():')

        # Scroll down to line 4.
        cc.text.yview(4)
        cc.update_code_context()
        eq(cc.info, [(0, -1, '', False),
                     (2, 0, 'class C1():', 'class'),
                     (4, 4, '    def __init__(self, a, b):', 'def')])
        eq(cc.topvisible, 5)
        eq(cc.label['text'], '\n'
                             'class C1():\n'
                             '    def __init__(self, a, b):')

        # Scroll down to line 11.  Last 'def' is removed.
        cc.text.yview(11)
        cc.update_code_context()
        eq(cc.info, [(0, -1, '', False),
                     (2, 0, 'class C1():', 'class'),
                     (7, 4, '    def compare(self):', 'def'),
                     (8, 8, '        if a > b:', 'if'),
                     (10, 8, '        elif a < b:', 'elif')])
        eq(cc.topvisible, 12)
        eq(cc.label['text'], '    def compare(self):\n'
                             '        if a > b:\n'
                             '        elif a < b:')

        # No scroll.  No update, even though context_depth changed.
        cc.update_code_context()
        cc.context_depth = 1
        eq(cc.info, [(0, -1, '', False),
                     (2, 0, 'class C1():', 'class'),
                     (7, 4, '    def compare(self):', 'def'),
                     (8, 8, '        if a > b:', 'if'),
                     (10, 8, '        elif a < b:', 'elif')])
        eq(cc.topvisible, 12)
        eq(cc.label['text'], '    def compare(self):\n'
                             '        if a > b:\n'
                             '        elif a < b:')

        # Scroll up.
        cc.text.yview(5)
        cc.update_code_context()
        eq(cc.info, [(0, -1, '', False),
                     (2, 0, 'class C1():', 'class'),
                     (4, 4, '    def __init__(self, a, b):', 'def')])
        eq(cc.topvisible, 6)
        # context_depth is 1.
        eq(cc.label['text'], '    def __init__(self, a, b):')

    @mock.patch.object(codecontext.CodeContext, 'update_code_context')
    def test_timer_event(self, mock_update):
        # Ensure code context is not active.
        if self.cc.label:
            self.cc.toggle_code_context_event()
        self.cc.timer_event()
        mock_update.assert_not_called()

        # Activate code context.
        self.cc.toggle_code_context_event()
        self.cc.timer_event()
        mock_update.assert_called()

    def test_font_timer_event(self):
        eq = self.assertEqual
        cc = self.cc
        save_font = cc.text['font']
        test_font = 'FakeFont'

        # Ensure code context is not active.
        if cc.label:
            cc.toggle_code_context_event()

        # Nothing updates on inactive code context.
        cc.text['font'] = test_font
        cc.font_timer_event()
        eq(cc.textfont, save_font)

        # Activate code context, but no change to font.
        cc.toggle_code_context_event()
        cc.text['font'] = save_font
        cc.font_timer_event()
        eq(cc.textfont, save_font)
        eq(cc.label['font'], save_font)

        # Active code context, change font.
        cc.text['font'] = test_font
        cc.font_timer_event()
        eq(cc.textfont, test_font)
        eq(cc.label['font'], test_font)

        cc.text['font'] = save_font
        cc.font_timer_event()


class HelperFunctionText(unittest.TestCase):

    def test_get_spaces_firstword(self):
        get = codecontext.get_spaces_firstword
        test_lines = (
            ('    first word', ('    ', 'first')),
            ('\tfirst word', ('\t', 'first')),
            ('  \u19D4\u19D2: ', ('  ', '\u19D4\u19D2')),
            ('no spaces', ('', 'no')),
            ('', ('', '')),
            ('# TEST COMMENT', ('', '')),
            ('    (continuation)', ('    ', ''))
            )
        for line, expected_output in test_lines:
            self.assertEqual(get(line), expected_output)

        # Send the pattern in the call.
        self.assertEqual(get('    (continuation)',
                             c=re.compile(r'^(\s*)([^\s]*)')),
                         ('    ', '(continuation)'))

    def test_get_line_info(self):
        eq = self.assertEqual
        gli = codecontext.get_line_info
        lines = code_sample.splitlines()

        # Line 1 is not a BLOCKOPENER.
        eq(gli(lines[0]), (codecontext.INFINITY, '', False))
        # Line 2 is a BLOCKOPENER without an indent.
        eq(gli(lines[1]), (0, 'class C1():', 'class'))
        # Line 3 is not a BLOCKOPENER and does not return the indent level.
        eq(gli(lines[2]), (codecontext.INFINITY, '    # Class comment.', False))
        # Line 4 is a BLOCKOPENER and is indented.
        eq(gli(lines[3]), (4, '    def __init__(self, a, b):', 'def'))
        # Line 8 is a different BLOCKOPENER and is indented.
        eq(gli(lines[7]), (8, '        if a > b:', 'if'))
        # Test tab.
        eq(gli('\tif a == b:'), (1, '\tif a == b:', 'if'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
