import unittest
import Tkinter
import ttk
from test.test_support import requires, run_unittest

import support

requires('gui')

class StyleTest(unittest.TestCase):

    def setUp(self):
        self.style = ttk.Style()


    def test_configure(self):
        style = self.style
        style.configure('TButton', background='yellow')
        self.assertEqual(style.configure('TButton', 'background'),
            'yellow')
        self.assertTrue(isinstance(style.configure('TButton'), dict))


    def test_map(self):
        style = self.style
        style.map('TButton', background=[('active', 'background', 'blue')])
        self.assertEqual(style.map('TButton', 'background'),
            [('active', 'background', 'blue')])
        self.assertTrue(isinstance(style.map('TButton'), dict))


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
        self.assertRaises(Tkinter.TclError, style.layout, 'NotALayout')
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
        self.assertTrue(isinstance(style.layout('TButton'), list))

        # correct layout, but "option" doesn't exist as option
        self.assertRaises(Tkinter.TclError, style.layout, 'Treeview',
            [('name', {'option': 'inexistent'})])


    def test_theme_use(self):
        self.assertRaises(Tkinter.TclError, self.style.theme_use,
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


tests_gui = (StyleTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
