'''Test idlelib.parenclose.
'''
from idlelib.ParenClose import ParenClose
import unittest
from unittest.mock import Mock
from tkinter import Text  # mocktk doesn't do text position addition correctly


class MockEvent(object):
    def __init__(self, char):
        self.char = char


class ParenCloseTest(unittest.TestCase):
    def test_parenClose(self):
        self.p_open_event = ParenClose.p_open_event
        self.p_close_event = ParenClose.p_close_event
        self.t_open_event = ParenClose.t_open_event
        self.text = Text()
        p_open = self.p_open_event
        p_close = self.p_close_event
        t_open = self.t_open_event
        for paren_close_set, tick_close_set, skip_closures_set, \
            insert, func, pos, char, result, posend in [
              (1, 1, 1, 'def abuse', p_open, '1.9', '(',
               'def abuse()', '1.10'),
              (1, 1, 1, 'spam = ', p_open, '1.7', '[',
               'spam = []', '1.8'),
              (1, 1, 1, 'eggs = ', p_open, '1.7', '{',
               'eggs = {}', '1.8'),
              (1, 1, 1, 'def abuse2()', p_close, '1.11', ')',
               'def abuse2()', '1.12'),
              (1, 1, 1, 'spam2 = []', p_close, '1.9', ']',
               'spam2 = []', '1.10'),
              (1, 1, 1, 'eggs2 = {}', p_close, '1.9', '}',
               'eggs2 = {}', '1.10'),
              (1, 1, 1, "color = ", t_open, '1.8', "'",
               "color = ''", '1.9'),
              (1, 1, 1, "velocity = ", t_open, '1.11', '"',
               'velocity = ""', '1.12'),
              (1, 1, 1, "more_spam = ''", t_open, '1.14', "'",
               "more_spam = ''''''", '1.15'),
              (1, 1, 1, 'more_eggs = ""', t_open, '1.14', '"',
               'more_eggs = """"""', '1.15'),
              (1, 1, 1, "more_spam2 = ''''''", t_open, '1.16', "'",
               "more_spam2 = ''''''", '1.17'),
              (1, 1, 1, 'more_eggs2 = """"""', t_open, '1.16', '"',
               'more_eggs2 = """"""', '1.17'),
              (0, 1, 1, 'no_spam = ', p_open, '1.10', '(',
               'no_spam = (', '1.11'),
              (1, 0, 1, 'no_velocity = ', t_open, '1.14', "'",
               "no_velocity = '", '1.15'),
              (1, 0, 1, 'no_more_eggs = ""', t_open, '1.17', '"',
               'no_more_eggs = """', '1.18'),
              (1, 1, 0, 'tinny = []', p_close, '1.9', ']',
               'tinny = []]', '1.10'),
              (1, 1, 0, 'gone = ""', t_open, '1.8', '"',
               'gone = """"', '1.9')]:
                    # reset text and self
                    self.paren_close = paren_close_set
                    self.tick_close = tick_close_set
                    self.skip_closures = skip_closures_set
                    self.text.delete('1.0', 'end')
                    # write text, move to current position, write character
                    self.text.insert('1.0', insert)
                    self.text.mark_set('insert', pos)
                    event = MockEvent(char)
                    func(self, event)
                    p = self.text.index('insert')
                    self.text.insert(p, char)
                    # checking position and text at the same time
                    # makes it easier to spot where errors occur
                    p = self.text.index('insert')
                    actual = self.text.get('1.0', 'end')
                    self.assertTupleEqual((actual, p), (result+'\n', posend))

if __name__ == '__main__':
    unittest.main(verbosity=2)
