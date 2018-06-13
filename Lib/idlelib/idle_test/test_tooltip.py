from idlelib.tooltip import TextToolTip, ToolTipBase
from test.support import requires
requires('gui')

import time
from tkinter import Button, Tk, Toplevel
import unittest


def setUpModule():
    global root
    root = Tk()

def root_update():
    global root
    root.update()

def tearDownModule():
    global root
    root.update_idletasks()
    root.destroy()
    del root


def _make_top_and_button(testobj):
    global root
    top = Toplevel(root)
    testobj.addCleanup(top.destroy)
    top.title("Test tooltip")
    button = Button(top, text='ToolTip test button')
    button.pack()
    testobj.addCleanup(button.destroy)
    top.lift()
    return top, button


class ToolTipBaseTest(unittest.TestCase):
    def setUp(self):
        self.top, self.button = _make_top_and_button(self)

    def test_base_class_is_unusable(self):
        global root
        top = Toplevel(root)
        self.addCleanup(top.destroy)

        button = Button(top, text='ToolTip test button')
        button.pack()
        self.addCleanup(button.destroy)

        with self.assertRaises(NotImplementedError):
            tooltip = ToolTipBase(button)
            tooltip.showtip()


class TextToolTipTest(unittest.TestCase):
    def setUp(self):
        self.top, self.button = _make_top_and_button(self)

    def test_showtip(self):
        tooltip = TextToolTip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        tooltip.showtip()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_hidetip(self):
        tooltip = TextToolTip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip()
        tooltip.hidetip()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_is_active(self):
        tooltip = TextToolTip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        self.assertFalse(tooltip.is_active())
        tooltip.showtip()
        self.assertTrue(tooltip.is_active())
        tooltip.hidetip()
        self.assertFalse(tooltip.is_active())

    def test_showtip_on_mouse_enter_no_delay(self):
        tooltip = TextToolTip(self.button, 'ToolTip text', hover_delay=None)
        self.addCleanup(tooltip.hidetip)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_showtip_on_mouse_enter_hover_delay(self):
        tooltip = TextToolTip(self.button, 'ToolTip text', hover_delay=50)
        self.addCleanup(tooltip.hidetip)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        time.sleep(0.1)
        root_update()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_hidetip_on_mouse_leave(self):
        tooltip = TextToolTip(self.button, 'ToolTip text', hover_delay=None)
        self.addCleanup(tooltip.hidetip)
        root_update()
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.button.event_generate('<Leave>', x=0, y=0)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
