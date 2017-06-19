'''Test idlelib.parenmatch.

This must currently be a gui test because ParenMatch methods use
several text methods not defined on idlelib.idle_test.mock_tk.Text.
'''
from test.support import requires
requires('gui')

import unittest
from unittest.mock import Mock
from tkinter import Tk, Text
from idlelib.parenmatch import ParenMatch

class DummyEditwin:
    def __init__(self, text):
        self.text = text
        self.indentwidth = 8
        self.tabwidth = 8
        self.context_use_ps1 = True


class ParenMatchTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.editwin = DummyEditwin(cls.text)
        cls.editwin.text_frame = Mock()

    @classmethod
    def tearDownClass(cls):
        del cls.text, cls.editwin
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def get_parenmatch(self):
        pm = ParenMatch(self.editwin)
        pm.bell = lambda: None
        return pm

    def test_paren_expression(self):
        """
        Test ParenMatch with 'expression' style.
        """
        text = self.text
        pm = self.get_parenmatch()
        pm.set_style('expression')

        text.insert('insert', 'def foobar(a, b')
        pm.flash_paren_event('event')
        self.assertIn('<<parenmatch-check-restore>>', text.event_info())
        self.assertTupleEqual(text.tag_prevrange('paren', 'end'),
                             ('1.10', '1.15'))
        text.insert('insert', ')')
        pm.restore_event()
        self.assertNotIn('<<parenmatch-check-restore>>', text.event_info())
        self.assertEqual(text.tag_prevrange('paren', 'end'), ())

        # paren_closed_event can only be tested as below
        pm.paren_closed_event('event')
        self.assertTupleEqual(text.tag_prevrange('paren', 'end'),
                                                ('1.10', '1.16'))

    def test_paren_default(self):
        """
        Test ParenMatch with 'default' style.
        """
        text = self.text
        pm = self.get_parenmatch()
        pm.set_style('default')

        text.insert('insert', 'def foobar(a, b')
        pm.flash_paren_event('event')
        self.assertIn('<<parenmatch-check-restore>>', text.event_info())
        self.assertTupleEqual(text.tag_prevrange('paren', 'end'),
                             ('1.10', '1.11'))
        text.insert('insert', ')')
        pm.restore_event()
        self.assertNotIn('<<parenmatch-check-restore>>', text.event_info())
        self.assertEqual(text.tag_prevrange('paren', 'end'), ())

    def test_paren_corner(self):
        """
        Test corner cases in flash_paren_event and paren_closed_event.

        These cases force conditional expression and alternate paths.
        """
        text = self.text
        pm = self.get_parenmatch()

        text.insert('insert', '# this is a commen)')
        self.assertIsNone(pm.paren_closed_event('event'))

        text.insert('insert', '\ndef')
        self.assertIsNone(pm.flash_paren_event('event'))
        self.assertIsNone(pm.paren_closed_event('event'))

        text.insert('insert', ' a, *arg)')
        self.assertIsNone(pm.paren_closed_event('event'))

    def test_handle_restore_timer(self):
        pm = self.get_parenmatch()
        pm.restore_event = Mock()
        pm.handle_restore_timer(0)
        self.assertTrue(pm.restore_event.called)
        pm.restore_event.reset_mock()
        pm.handle_restore_timer(1)
        self.assertFalse(pm.restore_event.called)


if __name__ == '__main__':
    unittest.main(verbosity=2)
