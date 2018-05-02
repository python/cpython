"Test help, coverage 87%."

import sys

from idlelib import help
import unittest
from test.support import requires
requires('gui')
from os.path import abspath, dirname, join
from tkinter import Tk, Text, TclError
from tkinter import font as tkfont
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


class FontSizerTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = root = Tk()
        root.withdraw()
        cls.text = Text(root)

    @classmethod
    def tearDownClass(cls):
        del cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        text = self.text
        self.font = font = tkfont.Font(text, ('courier', 30))
        text['font'] = font
        text.insert('end', 'Test Text')
        self.sizer = help.FontSizer(text)

    def tearDown(self):
        del self.sizer, self.font

    def test_zoom(self):
        text = self.text
        font = tkfont.Font(name=text['font'], exists=True, root=text)
        eq = self.assertEqual
        text.focus_set()

        wheel = '<Control-MouseWheel>'
        button4 = '<Control-Button-4>'
        button5 = '<Control-Button-5>'

        tests = ((wheel, {}, 31, None),
                 (wheel, {'delta': 1 if darwin else -120}, 30, None),
                 (button5, {}, 29, '<ButtonRelease-5>'),
                 (wheel, {'delta': -1 if darwin else 120}, 30, None),
                 (button4, {}, 31, '<ButtonRelease-4>'))

        eq(font['size'], 30)

        for event, kw, result, after in tests:
            with self.subTest(event=event):
                text.event_generate(event, **kw)
                eq(font['size'], result)
                if after:
                    text.event_generate(after)

class HelpTextTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        help.idleConf.userCfg = testcfg
        testcfg['main'].SetOption('EditorWindow', 'font-size', '12')
        cls.root = root = Tk()
        root.withdraw()
        helpfile = join(dirname(dirname(abspath(__file__))), 'help.html')
        cls.text = help.HelpText(root, helpfile)
        cls.tags = ('h3', 'h2', 'h1', 'em', 'pre', 'preblock')

    @classmethod
    def tearDownClass(cls):
        help.idleConf.userCfg = usercfg
        del cls.text, cls.tags
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def get_sizes(self):
        return [self.text.fonts[tag]['size'] for tag in self.tags]

    def test_scale_tagfonts(self):
        text = self.text
        eq = self.assertEqual

        save_fonts = text.fonts
        text.fonts = {tag: tkfont.Font(text, family='courier') for tag in self.tags}

        text.scale_tagfonts(12)
        eq(self.get_sizes(), [14, 16, 19, 12, 12, 10])

        text.scale_tagfonts(21)
        eq(self.get_sizes(), [25, 29, 33, 21, 21, 18])

        text.fonts = save_fonts

    def test_font_sizing(self):
        text = self.text
        eq = self.assertEqual

        base = [14, 16, 19, 12, 12, 10]
        zoomout = [15, 18, 20, 13, 13, 11]
        zoomin = [13, 15, 17, 11, 11, 9]
        wheel = '<Control-MouseWheel>'
        button4 = '<Control-Button-4>'
        button5 = '<Control-Button-5>'

        tests = ((wheel, {}, zoomout, None),
                 (wheel, {'delta': 1 if darwin else -120}, base, None),
                 (button5, {}, zoomin, '<ButtonRelease-5>'),
                 (wheel, {'delta': -1 if darwin else 120}, base, None),
                 (button4, {}, zoomout, '<ButtonRelease-4>'))

        eq(self.get_sizes(), base)

        for event, kw, result, after in tests:
            with self.subTest(event=event):
                text.event_generate(event, **kw)
                eq(self.get_sizes(), result)
                if after:
                    text.event_generate(after)


if __name__ == '__main__':
    unittest.main(verbosity=2)
