import unittest
import tkinter
from tkinter import ttk
from test.support import requires, run_unittest

import tkinter.test.support as support

requires('gui')

class StyleTest(unittest.TestCase):

    def setUp(self):
        self.style = ttk.Style()


    def test_configure(self):
        style = self.style
        style.configure('TButton', background='yellow')
        self.failUnlessEqual(style.configure('TButton', 'background'),
            'yellow')
        self.failUnless(isinstance(style.configure('TButton'), dict))


    def test_map(self):
        style = self.style
        style.map('TButton', background=[('active', 'background', 'blue')])
        self.failUnlessEqual(style.map('TButton', 'background'),
            [('active', 'background', 'blue')])
        self.failUnless(isinstance(style.map('TButton'), dict))


    def test_lookup(self):
        style = self.style
        style.configure('TButton', background='yellow')
        style.map('TButton', background=[('active', 'background', 'blue')])

        self.failUnlessEqual(style.lookup('TButton', 'background'), 'yellow')
        self.failUnlessEqual(style.lookup('TButton', 'background',
            ['active', 'background']), 'blue')
        self.failUnlessEqual(style.lookup('TButton', 'optionnotdefined',
            default='iknewit'), 'iknewit')


    def test_layout(self):
        style = self.style
        self.failUnlessRaises(tkinter.TclError, style.layout, 'NotALayout')
        tv_style = style.layout('Treeview')

        # "erase" Treeview layout
        style.layout('Treeview', '')
        self.failUnlessEqual(style.layout('Treeview'),
            [('null', {'sticky': 'nswe'})]
        )

        # restore layout
        style.layout('Treeview', tv_style)
        self.failUnlessEqual(style.layout('Treeview'), tv_style)

        # should return a list
        self.failUnless(isinstance(style.layout('TButton'), list))

        # correct layout, but "option" doesn't exist as option
        self.failUnlessRaises(tkinter.TclError, style.layout, 'Treeview',
            [('name', {'option': 'inexistent'})])


    def test_theme_use(self):
        self.failUnlessRaises(tkinter.TclError, self.style.theme_use,
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

        self.failIf(curr_theme == new_theme)
        self.failIf(new_theme != self.style.theme_use())

        self.style.theme_use(curr_theme)


tests_gui = (StyleTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
