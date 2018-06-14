"Test calltip_w, coverage 18%."

import unittest
from test.support import requires
from tkinter import Tk, Text

from idlelib import calltip_w

class CallTipTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.text = Text(cls.root)
        cls.calltip = calltip_w.CallTip(cls.text)

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.text, cls.root

    def test_init(self):
        self.assertEqual(self.calltip.widget, self.text)

if __name__ == '__main__':
    unittest.main(verbosity=2)
