"Test help, coverage 87%."

from idlelib import help
import unittest
from test.support import requires
requires('gui')
from os.path import abspath, dirname, join
from tkinter import Tk
from tkinter import font as tkfont
from idlelib import config

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


class HelpTestTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = root = Tk()
        root.withdraw()
        helpfile = join(dirname(dirname(abspath(__file__))), 'help.html')
        cls.text = help.HelpText(root, helpfile)
        help.idleConf.userCfg = testcfg

    @classmethod
    def tearDownClass(cls):
        help.idleConf.userCfg = usercfg
        del cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_scale_fontsize(self):
        text = self.text
        eq = self.assertEqual
        tags = ('normal', 'h3', 'h2', 'h1', 'em', 'pre', 'preblock')
        save_fonts = text.fonts
        text.fonts = {tag: tkfont.Font(text, family='courier') for tag in tags}

        text.scale_fontsize()
        sizes = [text.fonts[tag]['size'] for tag in tags]
        eq(sizes, [12, 16, 19, 24, 12, 12, 10])

        text.scale_fontsize(21)
        sizes = [text.fonts[tag]['size'] for tag in tags]
        eq(sizes, [21, 29, 33, 42, 21, 21, 18])

        text.fonts = save_fonts

    def test_zoom(self):
        text = self.text
        eq = self.assertEqual

        tags = ('normal', 'h3', 'h2', 'h1', 'em', 'pre', 'preblock')
        base = [12, 16, 19, 24, 12, 12, 10]
        zoomout = [13, 18, 20, 26, 13, 13, 11]
        zoomin = [11, 15, 17, 22, 11, 11, 9]

        tests = (('<<zoom-text-out>>', 0, zoomout),
                 ('<<zoom-text-in>>', 0, base),
                 ('<<zoom-text-in>>', 0, zoomin),
                 ('<Control-MouseWheel>', 0, base),
                 ('<Control-MouseWheel>', -120, zoomin),
                 ('<Control-MouseWheel>', 120, base),
                 ('<Control-Button-4>', 0, zoomout),
                 ('<Control-Button-5>', 0, base))

        # Base size starts at 12.
        sizes = [text.fonts[tag]['size'] for tag in tags]
        eq(text.base_size, base[0])
        eq(sizes, base)

        for event, delta, result in tests:
            with self.subTest(event=event):
                if event == '<Control-MouseWheel>':
                    text.event_generate(event, delta=delta)
                else:
                    text.event_generate(event)
                sizes = [text.fonts[tag]['size'] for tag in tags]
                eq(text.base_size, result[0])
                eq(sizes, result)


if __name__ == '__main__':
    unittest.main(verbosity=2)
