'''Test idlelib.parenclose.
'''
from idlelib.ParenClose import ParenClose
import unittest
# Can't use mock text; it doesn't handle positioning correctly.
from tkinter import Text
from test.support import requires
requires('gui')

class MockEvent(object):
    def __init__(self, char):
        self.char = char


class ParenCloseTest(unittest.TestCase):
    def test_parenClose(self):
        p = ParenClose()
        p.text = Text()
        p_open = p.p_open_event
        p_close = p.p_close_event
        t_open = p.t_open_event
        back = p.back_event
        delete = p.delete_event
        for paren_close_set, tick_close_set, skip_closures_set, mutual_b_set, \
            mutual_d_set, insert, func, pos, char, result, posend in [
              (1, 1, 1, 1, 1, 'def abuse', p_open, '1.9', '(',
               'def abuse()', '1.10'),
              (1, 1, 1, 1, 1, 'spam = ', p_open, '1.7', '[',
               'spam = []', '1.8'),
              (1, 1, 1, 1, 1, 'eggs = ', p_open, '1.7', '{',
               'eggs = {}', '1.8'),
              (1, 1, 1, 1, 1, 'def abuse2()', p_close, '1.11', ')',
               'def abuse2()', '1.12'),
              (1, 1, 1, 1, 1, 'spam2 = []', p_close, '1.9', ']',
               'spam2 = []', '1.10'),
              (1, 1, 1, 1, 1, 'eggs2 = {}', p_close, '1.9', '}',
               'eggs2 = {}', '1.10'),
              (1, 1, 1, 1, 1, "color = ", t_open, '1.8', "'",
               "color = ''", '1.9'),
              (1, 1, 1, 1, 1, "velocity = ", t_open, '1.11', '"',
               'velocity = ""', '1.12'),
              (1, 1, 1, 1, 1, "more_spam = ''", t_open, '1.14', "'",
               "more_spam = ''''''", '1.15'),
              (1, 1, 1, 1, 1, 'more_eggs = ""', t_open, '1.14', '"',
               'more_eggs = """"""', '1.15'),
              (1, 1, 1, 1, 1, "more_spam2 = ''''''", t_open, '1.16', "'",
               "more_spam2 = ''''''", '1.17'),
              (1, 1, 1, 1, 1, 'more_eggs2 = """"""', t_open, '1.16', '"',
               'more_eggs2 = """"""', '1.17'),
              (0, 1, 1, 1, 1,  'no_spam = ', p_open, '1.10', '(',
               'no_spam = (', '1.11'),
              (1, 0, 1, 1, 1, 'no_velocity = ', t_open, '1.14', "'",
               "no_velocity = '", '1.15'),
              (1, 0, 1, 1, 1, 'no_more_eggs = ""', t_open, '1.17', '"',
               'no_more_eggs = """', '1.18'),
              (1, 1, 0, 1, 1, 'tinny = []', p_close, '1.9', ']',
               'tinny = []]', '1.10'),
              (1, 1, 0, 1, 1, 'gone = ""', t_open, '1.8', '"',
               'gone = """"', '1.9'),
              (0, 0, 0, 0, 1, "honk = []", back, '1.8', '',
               "honk = []", '1.8'),
              (0, 0, 0, 1, 0, "henk = ()", delete, '1.8', '',
               "henk = ()", '1.8'),
              (0, 0, 0, 1, 1, 'nurb = ""', back, '1.8', '',
               'nurb = "', '1.8'),
              (0, 0, 0, 1, 1, 'chiz = []', delete, '1.8', '',
               'chiz = ]', '1.7'),
              (0, 0, 0, 1, 1, 'pink = {}', back, '1.8', '',
               'pink = {', '1.8')
              ]:
            # The actual delete and backspace events do not occur in
            # this test. We are checking to make sure everything is
            # correct before they would delete or backspace.

            # Reset text and self.
            p.paren_close = paren_close_set
            p.tick_close = tick_close_set
            p.skip_closures = skip_closures_set
            p.mutual_delete = mutual_d_set
            p.mutual_backspace = mutual_b_set
            p.text.delete('1.0', 'end')
            # Write text, move to current position, write character.
            p.text.insert('1.0', insert)
            p.text.mark_set('insert', pos)
            event = MockEvent(char)
            func(event)
            pos = p.text.index('insert')
            p.text.insert(pos, char)
            # Checking position and text at the same time
            # makes it easier to spot where errors occur.
            pos = p.text.index('insert')
            actual = p.text.get('1.0', 'end')
            self.assertTupleEqual((actual, pos), (result+'\n', posend))


if __name__ == '__main__':
    unittest.main(verbosity=2)
