"Test help, coverage 90%."

import sys

from idlelib import help
import unittest
from test.support import requires
requires('gui')
from os.path import abspath, dirname, join
from tkinter import Tk
from idlelib import config

darwin = sys.platform == 'darwin'
usercfg = help.idleConf.userCfg
testcfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}


class HelpFrameTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        "By itself, this tests that file parsed without exception."
        cls.root = root = Tk()
        root.withdraw()
        helpfile = join(dirname(dirname(abspath(__file__))), 'help.html')
        cls.frame = help.HelpFrame(root, helpfile)

    @classmethod
    def tearDownClass(cls):
        del cls.frame
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_line1(self):
        text = self.frame.text
        self.assertEqual(text.get('1.0', '1.end'), ' IDLE ')


class HelpTextTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        help.idleConf.userCfg = testcfg
        testcfg['main'].SetOption('EditorWindow', 'font-size', '12')
        cls.root = root = Tk()
        root.withdraw()

    @classmethod
    def tearDownClass(cls):
        help.idleConf.userCfg = usercfg
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        helpfile = join(dirname(dirname(abspath(__file__))), 'help.html')
        self.text = help.HelpText(self.root, helpfile)
        self.tags = ('h3', 'h2', 'h1', 'em', 'pre', 'preblock')

    def tearDown(self):
        del self.text, self.tags

    def get_sizes(self):
        return [self.text.fonts[tag]['size'] for tag in self.tags]

    def test_scale_tagfonts(self):
        text = self.text
        eq = self.assertEqual

        text.scale_tagfonts(12)
        eq(self.get_sizes(), [14, 16, 19, 12, 12, 10])

        text.scale_tagfonts(21)
        eq(self.get_sizes(), [25, 29, 33, 21, 21, 18])

    def test_resizing_callback(self):
        text = self.text
        eq = self.assertEqual

        base = [14, 16, 19, 12, 12, 10]
        larger = [15, 17, 20, 13, 13, 11]
        smaller = [13, 15, 18, 11, 11, 9]

        tests = (('<<increase_font_size>>', larger),
                 ('<<decrease_font_size>>', base),
                 ('<<decrease_font_size>>', smaller),
                 ('<<increase_font_size>>', base))

        eq(self.get_sizes(), base)

        for event, result in tests:
            with self.subTest(event=event):
                text.event_generate(event)
                eq(self.get_sizes(), result)


if __name__ == '__main__':
    unittest.main(verbosity=2)
