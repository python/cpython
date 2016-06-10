''' Test idlelib.config_key.

Coverage: 56%
'''
from idlelib import config_key
from test.support import requires
requires('gui')
import unittest
from tkinter import Tk, Text


class GetKeysTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root


    def test_init(self):
        dia = config_key.GetKeysDialog(
            self.root, 'test', '<<Test>>', ['<Key-F12>'], _utest=True)
        dia.Cancel()


if __name__ == '__main__':
    unittest.main(verbosity=2)
