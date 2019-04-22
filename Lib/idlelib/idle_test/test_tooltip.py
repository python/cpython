from idlelib.tooltip import TooltipBase, Hovertip
from test.support import requires
requires('gui')

from functools import wraps
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

def add_call_counting(func):
    @wraps(func)
    def wrapped_func(*args, **kwargs):
        wrapped_func.call_args_list.append((args, kwargs))
        return func(*args, **kwargs)
    wrapped_func.call_args_list = []
    return wrapped_func


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
            tooltip = TooltipBase(button)
            tooltip.showtip()


class HovertipTest(unittest.TestCase):
    def setUp(self):
        self.top, self.button = _make_top_and_button(self)

    def test_showtip(self):
        tooltip = Hovertip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        tooltip.showtip()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_showtip_twice(self):
        tooltip = Hovertip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        tooltip.showtip()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        orig_tipwindow = tooltip.tipwindow
        tooltip.showtip()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.assertIs(tooltip.tipwindow, orig_tipwindow)

    def test_hidetip(self):
        tooltip = Hovertip(self.button, 'ToolTip text')
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip()
        tooltip.hidetip()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())

    def test_showtip_on_mouse_enter_no_delay(self):
        tooltip = Hovertip(self.button, 'ToolTip text', hover_delay=None)
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip = add_call_counting(tooltip.showtip)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.assertGreater(len(tooltip.showtip.call_args_list), 0)

    def test_showtip_on_mouse_enter_hover_delay(self):
        tooltip = Hovertip(self.button, 'ToolTip text', hover_delay=50)
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip = add_call_counting(tooltip.showtip)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        time.sleep(0.1)
        root_update()
        self.assertTrue(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.assertGreater(len(tooltip.showtip.call_args_list), 0)

    def test_hidetip_on_mouse_leave(self):
        tooltip = Hovertip(self.button, 'ToolTip text', hover_delay=None)
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip = add_call_counting(tooltip.showtip)
        root_update()
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.button.event_generate('<Leave>', x=0, y=0)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.assertGreater(len(tooltip.showtip.call_args_list), 0)

    def test_dont_show_on_mouse_leave_before_delay(self):
        tooltip = Hovertip(self.button, 'ToolTip text', hover_delay=50)
        self.addCleanup(tooltip.hidetip)
        tooltip.showtip = add_call_counting(tooltip.showtip)
        root_update()
        self.button.event_generate('<Enter>', x=0, y=0)
        root_update()
        self.button.event_generate('<Leave>', x=0, y=0)
        root_update()
        time.sleep(0.1)
        root_update()
        self.assertFalse(tooltip.tipwindow and tooltip.tipwindow.winfo_viewable())
        self.assertEqual(tooltip.showtip.call_args_list, [])


if __name__ == '__main__':
    unittest.main(verbosity=2)
