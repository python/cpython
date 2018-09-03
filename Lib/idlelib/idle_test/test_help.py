"Test help, coverage 90%."

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

    def test_zoom_in(self):
        text = self.text
        font = self.font
        eq = self.assertEqual
        text.focus_set()

        eq(font['size'], 30)
        text.event_generate('<<zoom-text-in>>')
        eq(font['size'], 31)
        text.event_generate('<<zoom-text-in>>')
        eq(font['size'], 32)

    def test_zoom_out(self):
        text = self.text
        font = self.font
        eq = self.assertEqual
        text.focus_set()

        eq(font['size'], 30)
        text.event_generate('<<zoom-text-out>>')
        eq(font['size'], 29)
        text.event_generate('<<zoom-text-out>>')
        eq(font['size'], 28)

    def test_mousewheel(self):
        text = self.text
        font = self.font
        eq = self.assertEqual
        text.focus_set()

        wheel = '<Control-MouseWheel>'
        button4 = '<Control-Button-4>'
        button5 = '<Control-Button-5>'

        tests = ((wheel, {}, 29, None),
                 (wheel, {'delta': 10}, 30, None),
                 (button5, {}, 29, '<ButtonRelease-5>'),
                 (wheel, {'delta': -10}, 28, None),
                 (button4, {}, 29, '<ButtonRelease-4>'))

        eq(font['size'], 30)
        for event, kw, result, after in tests:
            with self.subTest(event=event):
                text.event_generate(event, **kw)
                eq(font['size'], result)
                if after:
                    text.event_generate(after)

    def test_zoom(self):
        text = self.text
        font = self.font
        eq = self.assertEqual
        text.focus_set()

        tests = (('zoom out', False, 29),
                 ('zoom in', True, 30),
                 ('zoom out', False, 29),
                 ('zoom out', False, 28),
                 ('zoom in', True, 29))

        eq(font['size'], 30)
        for event, increase, result in tests:
            with self.subTest(event=event):
                self.sizer.zoom(increase)
                eq(font['size'], result)

    def test_zoom_lower_boundary(self):
        text = self.text
        font = self.font
        font['size'] = 7
        eq = self.assertEqual
        text.focus_set()

        tests = (('zoom out', False, 6),
                 ('zoom out at limit (no change)', False, 6),
                 ('zoom in', True, 7),
                 ('zoom in', True, 8),
                 ('zoom out', False, 7))

        eq(font['size'], 7)
        for event, increase, result in tests:
            with self.subTest(event=event):
                self.sizer.zoom(increase)
                eq(font['size'], result)

    def test_zoom_upper_boundary(self):
        text = self.text
        font = self.font
        font['size'] = 99
        eq = self.assertEqual
        text.focus_set()

        tests = (('zoom in', True, 100),
                 ('zoom in at limit (no change)', True, 100),
                 ('zoom out', False, 99),
                 ('zoom out', False, 98),
                 ('zoom in', True, 99))

        eq(font['size'], 99)
        for event, increase, result in tests:
            with self.subTest(event=event):
                self.sizer.zoom(increase)
                eq(font['size'], result)


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

    def test_font_sizing(self):
        text = self.text
        eq = self.assertEqual

        base = [14, 16, 19, 12, 12, 10]
        zoomin = [15, 18, 20, 13, 13, 11]
        zoomout = [13, 15, 17, 11, 11, 9]
        wheel = '<Control-MouseWheel>'
        button4 = '<Control-Button-4>'
        button5 = '<Control-Button-5>'

        tests = ((wheel, {}, zoomout, None),
                 (wheel, {'delta': 10}, base, None),
                 (button4, {}, zoomin, '<ButtonRelease-4>'),
                 (wheel, {'delta': -10}, base, None),
                 (button5, {}, zoomout, '<ButtonRelease-5>'))

        eq(self.get_sizes(), base)

        for event, kw, result, after in tests:
            with self.subTest(event=event):
                text.event_generate(event, **kw)
                eq(self.get_sizes(), result)
                if after:
                    text.event_generate(after)


if __name__ == '__main__':
    unittest.main(verbosity=2)
