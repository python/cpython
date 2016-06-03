"""Unit tests for idlelib.AutoExpand"""
import unittest
from test.test_support import requires
from Tkinter import Text, Tk
#from idlelib.idle_test.mock_tk import Text
from idlelib.AutoExpand import AutoExpand


class Dummy_Editwin:
    # AutoExpand.__init__ only needs .text
    def __init__(self, text):
        self.text = text

class AutoExpandTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if 'Tkinter' in str(Text):
            requires('gui')
            cls.tk = Tk()
            cls.text = Text(cls.tk)
        else:
            cls.text = Text()
        cls.auto_expand = AutoExpand(Dummy_Editwin(cls.text))

    @classmethod
    def tearDownClass(cls):
        del cls.text, cls.auto_expand
        if hasattr(cls, 'tk'):
            cls.tk.destroy()
            del cls.tk

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def test_get_prevword(self):
        text = self.text
        previous = self.auto_expand.getprevword
        equal = self.assertEqual

        equal(previous(), '')

        text.insert('insert', 't')
        equal(previous(), 't')

        text.insert('insert', 'his')
        equal(previous(), 'this')

        text.insert('insert', ' ')
        equal(previous(), '')

        text.insert('insert', 'is')
        equal(previous(), 'is')

        text.insert('insert', '\nsample\nstring')
        equal(previous(), 'string')

        text.delete('3.0', 'insert')
        equal(previous(), '')

        text.delete('1.0', 'end')
        equal(previous(), '')

    def test_before_only(self):
        previous = self.auto_expand.getprevword
        expand = self.auto_expand.expand_word_event
        equal = self.assertEqual

        self.text.insert('insert', 'ab ac bx ad ab a')
        equal(self.auto_expand.getwords(), ['ab', 'ad', 'ac', 'a'])
        expand('event')
        equal(previous(), 'ab')
        expand('event')
        equal(previous(), 'ad')
        expand('event')
        equal(previous(), 'ac')
        expand('event')
        equal(previous(), 'a')

    def test_after_only(self):
        # Also add punctuation 'noise' that shoud be ignored.
        text = self.text
        previous = self.auto_expand.getprevword
        expand = self.auto_expand.expand_word_event
        equal = self.assertEqual

        text.insert('insert', 'a, [ab] ac: () bx"" cd ac= ad ya')
        text.mark_set('insert', '1.1')
        equal(self.auto_expand.getwords(), ['ab', 'ac', 'ad', 'a'])
        expand('event')
        equal(previous(), 'ab')
        expand('event')
        equal(previous(), 'ac')
        expand('event')
        equal(previous(), 'ad')
        expand('event')
        equal(previous(), 'a')

    def test_both_before_after(self):
        text = self.text
        previous = self.auto_expand.getprevword
        expand = self.auto_expand.expand_word_event
        equal = self.assertEqual

        text.insert('insert', 'ab xy yz\n')
        text.insert('insert', 'a ac by ac')

        text.mark_set('insert', '2.1')
        equal(self.auto_expand.getwords(), ['ab', 'ac', 'a'])
        expand('event')
        equal(previous(), 'ab')
        expand('event')
        equal(previous(), 'ac')
        expand('event')
        equal(previous(), 'a')

    def test_other_expand_cases(self):
        text = self.text
        expand = self.auto_expand.expand_word_event
        equal = self.assertEqual

        # no expansion candidate found
        equal(self.auto_expand.getwords(), [])
        equal(expand('event'), 'break')

        text.insert('insert', 'bx cy dz a')
        equal(self.auto_expand.getwords(), [])

        # reset state by successfully expanding once
        # move cursor to another position and expand again
        text.insert('insert', 'ac xy a ac ad a')
        text.mark_set('insert', '1.7')
        expand('event')
        initial_state = self.auto_expand.state
        text.mark_set('insert', '1.end')
        expand('event')
        new_state = self.auto_expand.state
        self.assertNotEqual(initial_state, new_state)

if __name__ == '__main__':
    unittest.main(verbosity=2)
