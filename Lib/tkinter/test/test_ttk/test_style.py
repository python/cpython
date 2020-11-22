import unittest
import tkinter
from tkinter import ttk
from test import support
from test.support import requires, run_unittest
from tkinter.test.support import AbstractTkTest

requires('gui')

CLASS_NAMES = [
    '.', 'ComboboxPopdownFrame', 'Heading',
    'Horizontal.TProgressbar', 'Horizontal.TScale', 'Item', 'Sash',
    'TButton', 'TCheckbutton', 'TCombobox', 'TEntry',
    'TLabelframe', 'TLabelframe.Label', 'TMenubutton',
    'TNotebook', 'TNotebook.Tab', 'Toolbutton', 'TProgressbar',
    'TRadiobutton', 'Treeview', 'TScale', 'TScrollbar', 'TSpinbox',
    'Vertical.TProgressbar', 'Vertical.TScale'
]

class StyleTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.style = ttk.Style(self.root)


    def test_configure(self):
        style = self.style
        style.configure('TButton', background='yellow')
        self.assertEqual(style.configure('TButton', 'background'),
            'yellow')
        self.assertIsInstance(style.configure('TButton'), dict)


    def test_map(self):
        style = self.style

        # Single state
        for states in ['active'], [('active',)]:
            with self.subTest(states=states):
                style.map('TButton', background=[(*states, 'white')])
                expected = [('active', 'white')]
                self.assertEqual(style.map('TButton', 'background'), expected)
                m = style.map('TButton')
                self.assertIsInstance(m, dict)
                self.assertEqual(m['background'], expected)

        # Multiple states
        for states in ['pressed', '!disabled'], ['pressed !disabled'], [('pressed', '!disabled')]:
            with self.subTest(states=states):
                style.map('TButton', background=[(*states, 'black')])
                expected = [('pressed', '!disabled', 'black')]
                self.assertEqual(style.map('TButton', 'background'), expected)
                m = style.map('TButton')
                self.assertIsInstance(m, dict)
                self.assertEqual(m['background'], expected)

        # Default state
        for states in [], [''], [()]:
            with self.subTest(states=states):
                style.map('TButton', background=[(*states, 'grey')])
                expected = [('grey',)]
                self.assertEqual(style.map('TButton', 'background'), expected)
                m = style.map('TButton')
                self.assertIsInstance(m, dict)
                self.assertEqual(m['background'], expected)


    def test_lookup(self):
        style = self.style
        style.configure('TButton', background='yellow')
        style.map('TButton', background=[('active', 'background', 'blue')])

        self.assertEqual(style.lookup('TButton', 'background'), 'yellow')
        self.assertEqual(style.lookup('TButton', 'background',
            ['active', 'background']), 'blue')
        self.assertEqual(style.lookup('TButton', 'optionnotdefined',
            default='iknewit'), 'iknewit')


    def test_layout(self):
        style = self.style
        self.assertRaises(tkinter.TclError, style.layout, 'NotALayout')
        tv_style = style.layout('Treeview')

        # "erase" Treeview layout
        style.layout('Treeview', '')
        self.assertEqual(style.layout('Treeview'),
            [('null', {'sticky': 'nswe'})]
        )

        # restore layout
        style.layout('Treeview', tv_style)
        self.assertEqual(style.layout('Treeview'), tv_style)

        # should return a list
        self.assertIsInstance(style.layout('TButton'), list)

        # correct layout, but "option" doesn't exist as option
        self.assertRaises(tkinter.TclError, style.layout, 'Treeview',
            [('name', {'option': 'inexistent'})])


    def test_theme_use(self):
        self.assertRaises(tkinter.TclError, self.style.theme_use,
            'nonexistingname')

        curr_theme = self.style.theme_use()
        new_theme = None
        for theme in self.style.theme_names():
            if theme != curr_theme:
                new_theme = theme
                self.style.theme_use(theme)
                break
        else:
            # just one theme available, can't go on with tests
            return

        self.assertFalse(curr_theme == new_theme)
        self.assertFalse(new_theme != self.style.theme_use())

        self.style.theme_use(curr_theme)


    def test_configure_custom_copy(self):
        style = self.style

        curr_theme = self.style.theme_use()
        self.addCleanup(self.style.theme_use, curr_theme)
        for theme in self.style.theme_names():
            self.style.theme_use(theme)
            for name in CLASS_NAMES:
                default = style.configure(name)
                if not default:
                    continue
                with self.subTest(theme=theme, name=name):
                    if support.verbose >= 2:
                        print('configure', theme, name, default)
                    newname = f'C.{name}'
                    self.assertEqual(style.configure(newname), None)
                    style.configure(newname, **default)
                    self.assertEqual(style.configure(newname), default)
                    for key, value in default.items():
                        self.assertEqual(style.configure(newname, key), value)


    def test_map_custom_copy(self):
        style = self.style

        curr_theme = self.style.theme_use()
        self.addCleanup(self.style.theme_use, curr_theme)
        for theme in self.style.theme_names():
            self.style.theme_use(theme)
            for name in CLASS_NAMES:
                default = style.map(name)
                if not default:
                    continue
                with self.subTest(theme=theme, name=name):
                    if support.verbose >= 2:
                        print('map', theme, name, default)
                    newname = f'C.{name}'
                    self.assertEqual(style.map(newname), {})
                    style.map(newname, **default)
                    self.assertEqual(style.map(newname), default)
                    for key, value in default.items():
                        self.assertEqual(style.map(newname, key), value)


tests_gui = (StyleTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
