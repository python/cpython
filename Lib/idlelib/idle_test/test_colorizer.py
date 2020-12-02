"Test colorizer, coverage 93%."

from idlelib import colorizer
from test.support import requires
import unittest
from unittest import mock

from functools import partial
from tkinter import Tk, Text
from idlelib import config
from idlelib.percolator import Percolator


usercfg = colorizer.idleConf.userCfg
testcfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}

source = (
    "if True: int ('1') # keyword, builtin, string, comment\n"
    "elif False: print(0)  # 'string' in comment\n"
    "else: float(None)  # if in comment\n"
    "if iF + If + IF: 'keyword matching must respect case'\n"
    "if'': x or''  # valid string-keyword no-space combinations\n"
    "async def f(): await g()\n"
    "'x', '''x''', \"x\", \"\"\"x\"\"\"\n"
    )


def setUpModule():
    colorizer.idleConf.userCfg = testcfg


def tearDownModule():
    colorizer.idleConf.userCfg = usercfg


class FunctionTest(unittest.TestCase):

    def test_any(self):
        self.assertEqual(colorizer.any('test', ('a', 'b', 'cd')),
                         '(?P<test>a|b|cd)')

    def test_make_pat(self):
        # Tested in more detail by testing prog.
        self.assertTrue(colorizer.make_pat())

    def test_prog(self):
        prog = colorizer.prog
        eq = self.assertEqual
        line = 'def f():\n    print("hello")\n'
        m = prog.search(line)
        eq(m.groupdict()['KEYWORD'], 'def')
        m = prog.search(line, m.end())
        eq(m.groupdict()['SYNC'], '\n')
        m = prog.search(line, m.end())
        eq(m.groupdict()['BUILTIN'], 'print')
        m = prog.search(line, m.end())
        eq(m.groupdict()['STRING'], '"hello"')
        m = prog.search(line, m.end())
        eq(m.groupdict()['SYNC'], '\n')

    def test_idprog(self):
        idprog = colorizer.idprog
        m = idprog.match('nospace')
        self.assertIsNone(m)
        m = idprog.match(' space')
        self.assertEqual(m.group(0), ' space')


class ColorConfigTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = Tk()
        root.withdraw()
        cls.text = Text(root)

    @classmethod
    def tearDownClass(cls):
        del cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_color_config(self):
        text = self.text
        eq = self.assertEqual
        colorizer.color_config(text)
        # Uses IDLE Classic theme as default.
        eq(text['background'], '#ffffff')
        eq(text['foreground'], '#000000')
        eq(text['selectbackground'], 'gray')
        eq(text['selectforeground'], '#000000')
        eq(text['insertbackground'], 'black')
        eq(text['inactiveselectbackground'], 'gray')


class ColorDelegatorInstantiationTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = Tk()
        root.withdraw()
        text = cls.text = Text(root)

    @classmethod
    def tearDownClass(cls):
        del cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.color = colorizer.ColorDelegator()

    def tearDown(self):
        self.color.close()
        self.text.delete('1.0', 'end')
        self.color.resetcache()
        del self.color

    def test_init(self):
        color = self.color
        self.assertIsInstance(color, colorizer.ColorDelegator)

    def test_init_state(self):
        # init_state() is called during the instantiation of
        # ColorDelegator in setUp().
        color = self.color
        self.assertIsNone(color.after_id)
        self.assertTrue(color.allow_colorizing)
        self.assertFalse(color.colorizing)
        self.assertFalse(color.stop_colorizing)


class ColorDelegatorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = Tk()
        root.withdraw()
        text = cls.text = Text(root)
        cls.percolator = Percolator(text)
        # Delegator stack = [Delegator(text)]

    @classmethod
    def tearDownClass(cls):
        cls.percolator.redir.close()
        del cls.percolator, cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.color = colorizer.ColorDelegator()
        self.percolator.insertfilter(self.color)
        # Calls color.setdelegate(Delegator(text)).

    def tearDown(self):
        self.color.close()
        self.percolator.removefilter(self.color)
        self.text.delete('1.0', 'end')
        self.color.resetcache()
        del self.color

    def test_setdelegate(self):
        # Called in setUp when filter is attached to percolator.
        color = self.color
        self.assertIsInstance(color.delegate, colorizer.Delegator)
        # It is too late to mock notify_range, so test side effect.
        self.assertEqual(self.root.tk.call(
            'after', 'info', color.after_id)[1], 'timer')

    def test_LoadTagDefs(self):
        highlight = partial(config.idleConf.GetHighlight, theme='IDLE Classic')
        for tag, colors in self.color.tagdefs.items():
            with self.subTest(tag=tag):
                self.assertIn('background', colors)
                self.assertIn('foreground', colors)
                if tag not in ('SYNC', 'TODO'):
                    self.assertEqual(colors, highlight(element=tag.lower()))

    def test_config_colors(self):
        text = self.text
        highlight = partial(config.idleConf.GetHighlight, theme='IDLE Classic')
        for tag in self.color.tagdefs:
            for plane in ('background', 'foreground'):
                with self.subTest(tag=tag, plane=plane):
                    if tag in ('SYNC', 'TODO'):
                        self.assertEqual(text.tag_cget(tag, plane), '')
                    else:
                        self.assertEqual(text.tag_cget(tag, plane),
                                         highlight(element=tag.lower())[plane])
        # 'sel' is marked as the highest priority.
        self.assertEqual(text.tag_names()[-1], 'sel')

    @mock.patch.object(colorizer.ColorDelegator, 'notify_range')
    def test_insert(self, mock_notify):
        text = self.text
        # Initial text.
        text.insert('insert', 'foo')
        self.assertEqual(text.get('1.0', 'end'), 'foo\n')
        mock_notify.assert_called_with('1.0', '1.0+3c')
        # Additional text.
        text.insert('insert', 'barbaz')
        self.assertEqual(text.get('1.0', 'end'), 'foobarbaz\n')
        mock_notify.assert_called_with('1.3', '1.3+6c')

    @mock.patch.object(colorizer.ColorDelegator, 'notify_range')
    def test_delete(self, mock_notify):
        text = self.text
        # Initialize text.
        text.insert('insert', 'abcdefghi')
        self.assertEqual(text.get('1.0', 'end'), 'abcdefghi\n')
        # Delete single character.
        text.delete('1.7')
        self.assertEqual(text.get('1.0', 'end'), 'abcdefgi\n')
        mock_notify.assert_called_with('1.7')
        # Delete multiple characters.
        text.delete('1.3', '1.6')
        self.assertEqual(text.get('1.0', 'end'), 'abcgi\n')
        mock_notify.assert_called_with('1.3')

    def test_notify_range(self):
        text = self.text
        color = self.color
        eq = self.assertEqual

        # Colorizing already scheduled.
        save_id = color.after_id
        eq(self.root.tk.call('after', 'info', save_id)[1], 'timer')
        self.assertFalse(color.colorizing)
        self.assertFalse(color.stop_colorizing)
        self.assertTrue(color.allow_colorizing)

        # Coloring scheduled and colorizing in progress.
        color.colorizing = True
        color.notify_range('1.0', 'end')
        self.assertFalse(color.stop_colorizing)
        eq(color.after_id, save_id)

        # No colorizing scheduled and colorizing in progress.
        text.after_cancel(save_id)
        color.after_id = None
        color.notify_range('1.0', '1.0+3c')
        self.assertTrue(color.stop_colorizing)
        self.assertIsNotNone(color.after_id)
        eq(self.root.tk.call('after', 'info', color.after_id)[1], 'timer')
        # New event scheduled.
        self.assertNotEqual(color.after_id, save_id)

        # No colorizing scheduled and colorizing off.
        text.after_cancel(color.after_id)
        color.after_id = None
        color.allow_colorizing = False
        color.notify_range('1.4', '1.4+10c')
        # Nothing scheduled when colorizing is off.
        self.assertIsNone(color.after_id)

    def test_toggle_colorize_event(self):
        color = self.color
        eq = self.assertEqual

        # Starts with colorizing allowed and scheduled.
        self.assertFalse(color.colorizing)
        self.assertFalse(color.stop_colorizing)
        self.assertTrue(color.allow_colorizing)
        eq(self.root.tk.call('after', 'info', color.after_id)[1], 'timer')

        # Toggle colorizing off.
        color.toggle_colorize_event()
        self.assertIsNone(color.after_id)
        self.assertFalse(color.colorizing)
        self.assertFalse(color.stop_colorizing)
        self.assertFalse(color.allow_colorizing)

        # Toggle on while colorizing in progress (doesn't add timer).
        color.colorizing = True
        color.toggle_colorize_event()
        self.assertIsNone(color.after_id)
        self.assertTrue(color.colorizing)
        self.assertFalse(color.stop_colorizing)
        self.assertTrue(color.allow_colorizing)

        # Toggle off while colorizing in progress.
        color.toggle_colorize_event()
        self.assertIsNone(color.after_id)
        self.assertTrue(color.colorizing)
        self.assertTrue(color.stop_colorizing)
        self.assertFalse(color.allow_colorizing)

        # Toggle on while colorizing not in progress.
        color.colorizing = False
        color.toggle_colorize_event()
        eq(self.root.tk.call('after', 'info', color.after_id)[1], 'timer')
        self.assertFalse(color.colorizing)
        self.assertTrue(color.stop_colorizing)
        self.assertTrue(color.allow_colorizing)

    @mock.patch.object(colorizer.ColorDelegator, 'recolorize_main')
    def test_recolorize(self, mock_recmain):
        text = self.text
        color = self.color
        eq = self.assertEqual
        # Call recolorize manually and not scheduled.
        text.after_cancel(color.after_id)

        # No delegate.
        save_delegate = color.delegate
        color.delegate = None
        color.recolorize()
        mock_recmain.assert_not_called()
        color.delegate = save_delegate

        # Toggle off colorizing.
        color.allow_colorizing = False
        color.recolorize()
        mock_recmain.assert_not_called()
        color.allow_colorizing = True

        # Colorizing in progress.
        color.colorizing = True
        color.recolorize()
        mock_recmain.assert_not_called()
        color.colorizing = False

        # Colorizing is done, but not completed, so rescheduled.
        color.recolorize()
        self.assertFalse(color.stop_colorizing)
        self.assertFalse(color.colorizing)
        mock_recmain.assert_called()
        eq(mock_recmain.call_count, 1)
        # Rescheduled when TODO tag still exists.
        eq(self.root.tk.call('after', 'info', color.after_id)[1], 'timer')

        # No changes to text, so no scheduling added.
        text.tag_remove('TODO', '1.0', 'end')
        color.recolorize()
        self.assertFalse(color.stop_colorizing)
        self.assertFalse(color.colorizing)
        mock_recmain.assert_called()
        eq(mock_recmain.call_count, 2)
        self.assertIsNone(color.after_id)

    @mock.patch.object(colorizer.ColorDelegator, 'notify_range')
    def test_recolorize_main(self, mock_notify):
        text = self.text
        color = self.color
        eq = self.assertEqual

        text.insert('insert', source)
        expected = (('1.0', ('KEYWORD',)), ('1.2', ()), ('1.3', ('KEYWORD',)),
                    ('1.7', ()), ('1.9', ('BUILTIN',)), ('1.14', ('STRING',)),
                    ('1.19', ('COMMENT',)),
                    ('2.1', ('KEYWORD',)), ('2.18', ()), ('2.25', ('COMMENT',)),
                    ('3.6', ('BUILTIN',)), ('3.12', ('KEYWORD',)), ('3.21', ('COMMENT',)),
                    ('4.0', ('KEYWORD',)), ('4.3', ()), ('4.6', ()),
                    ('5.2', ('STRING',)), ('5.8', ('KEYWORD',)), ('5.10', ('STRING',)),
                    ('6.0', ('KEYWORD',)), ('6.10', ('DEFINITION',)), ('6.11', ()),
                    ('7.0', ('STRING',)), ('7.4', ()), ('7.5', ('STRING',)),
                    ('7.12', ()), ('7.14', ('STRING',)),
                    # SYNC at the end of every line.
                    ('1.55', ('SYNC',)), ('2.50', ('SYNC',)), ('3.34', ('SYNC',)),
                   )

        # Nothing marked to do therefore no tags in text.
        text.tag_remove('TODO', '1.0', 'end')
        color.recolorize_main()
        for tag in text.tag_names():
            with self.subTest(tag=tag):
                eq(text.tag_ranges(tag), ())

        # Source marked for processing.
        text.tag_add('TODO', '1.0', 'end')
        # Check some indexes.
        color.recolorize_main()
        for index, expected_tags in expected:
            with self.subTest(index=index):
                eq(text.tag_names(index), expected_tags)

        # Check for some tags for ranges.
        eq(text.tag_nextrange('TODO', '1.0'), ())
        eq(text.tag_nextrange('KEYWORD', '1.0'), ('1.0', '1.2'))
        eq(text.tag_nextrange('COMMENT', '2.0'), ('2.22', '2.43'))
        eq(text.tag_nextrange('SYNC', '2.0'), ('2.43', '3.0'))
        eq(text.tag_nextrange('STRING', '2.0'), ('4.17', '4.53'))
        eq(text.tag_nextrange('STRING', '7.0'), ('7.0', '7.3'))
        eq(text.tag_nextrange('STRING', '7.3'), ('7.5', '7.12'))
        eq(text.tag_nextrange('STRING', '7.12'), ('7.14', '7.17'))
        eq(text.tag_nextrange('STRING', '7.17'), ('7.19', '7.26'))
        eq(text.tag_nextrange('SYNC', '7.0'), ('7.26', '9.0'))

    @mock.patch.object(colorizer.ColorDelegator, 'recolorize')
    @mock.patch.object(colorizer.ColorDelegator, 'notify_range')
    def test_removecolors(self, mock_notify, mock_recolorize):
        text = self.text
        color = self.color
        text.insert('insert', source)

        color.recolorize_main()
        # recolorize_main doesn't add these tags.
        text.tag_add("ERROR", "1.0")
        text.tag_add("TODO", "1.0")
        text.tag_add("hit", "1.0")
        for tag in color.tagdefs:
            with self.subTest(tag=tag):
                self.assertNotEqual(text.tag_ranges(tag), ())

        color.removecolors()
        for tag in color.tagdefs:
            with self.subTest(tag=tag):
                self.assertEqual(text.tag_ranges(tag), ())


if __name__ == '__main__':
    unittest.main(verbosity=2)
