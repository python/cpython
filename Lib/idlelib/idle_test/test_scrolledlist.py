"Test scrolledlist, coverage 38%."

from idlelib.scrolledlist import ScrolledList
from idlelib.config import idleConf
import unittest
from test.support import requires
requires('gui')
from tkinter import Tk


class ScrolledListTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root


    def test_init(self):
        sl = ScrolledList(self.root)
        # gh-86997: both colors are set, from the IDLE theme.
        colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'normal')
        self.assertEqual(sl.listbox['foreground'], colors['foreground'])
        self.assertEqual(sl.listbox['background'], colors['background'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
