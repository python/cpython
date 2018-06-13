"""Tools for displaying tool-tips.

This includes:
 * an abstract base-class for different kidns of tooltips
 * a simple Tooltip class, showing a single line of text
"""
from tkinter import *


class ToolTipBase(object):

    def __init__(self, anchor_widget):
        self.anchor_widget = anchor_widget
        self.tipwindow = None

    def __del__(self):
        self.hidetip()

    def showtip(self):
        if self.tipwindow:
            return
        self.tipwindow = tw = Toplevel(self.anchor_widget)
        # remove border on calltip window
        tw.wm_overrideredirect(1)
        try:
            # This command is only needed and available on Tk >= 8.4.0 for OSX
            # Without it, call tips intrude on the typing process by grabbing
            # the focus.
            tw.tk.call("::tk::unsupported::MacWindowStyle", "style", tw._w,
                       "help", "noActivates")
        except TclError:
            pass
        self.position_window()
        self.showcontents()
        self.tipwindow.lift()  # work around bug in Tk 8.5.18+ (issue #24570)

    def position_window(self):
        x, y = self.get_position()
        root_x = self.anchor_widget.winfo_rootx() + x
        root_y = self.anchor_widget.winfo_rooty() + y
        self.tipwindow.wm_geometry("+%d+%d" % (root_x, root_y))

    def get_position(self):
        """Choose the position of the tooltip"""
        # The tip window must be completely outside the anchor widget;
        # otherwise when the mouse enters the tip window we get
        # a leave event and it disappears, and then we get an enter
        # event and it reappears, and so on forever :-(
        #
        # Note: This is a simplistic implementation; sub-classes will likely
        # want to override this.
        return 20, self.anchor_widget.winfo_height() + 1

    def showcontents(self):
        """content display hook for concrete sub-classes"""
        # See ToolTip for an example
        raise NotImplementedError

    def hidetip(self):
        tw = self.tipwindow
        self.tipwindow = None
        if tw:
            tw.destroy()

    def is_active(self):
        return bool(self.tipwindow)


class OnHoverToolTipBase(ToolTipBase):
    """abstract base class, providing delayed on-hover display"""

    def __init__(self, anchor_widget, hover_delay=1500):
        super(OnHoverToolTipBase, self).__init__(anchor_widget)
        self.hover_delay = hover_delay

        self._after_id = None
        self._id1 = self.anchor_widget.bind("<Enter>", self._handle_enter)
        self._id2 = self.anchor_widget.bind("<Leave>", self._handle_leave)
        self._id3 = self.anchor_widget.bind("<ButtonPress>", self._handle_leave)

    def __del__(self):
        try:
            self.anchor_widget.unbind("<Enter>", self._id1)
            self.anchor_widget.unbind("<Leave>", self._id2)
            self.anchor_widget.unbind("<ButtonPress>", self._id3)
        except TclError:
            pass
        super(OnHoverToolTipBase, self).__del__()

    def _handle_enter(self, event=None):
        self.schedule()

    def _handle_leave(self, event=None):
        self.hidetip()

    def schedule(self):
        self.unschedule()
        self._after_id = self.anchor_widget.after(self.hover_delay,
                                                  self.showtip)

    def unschedule(self):
        after_id = self._after_id
        self._after_id = None
        if after_id:
            self.anchor_widget.after_cancel(after_id)

    def hidetip(self):
        self.unschedule()
        super(OnHoverToolTipBase, self).hidetip()


class TextToolTip(OnHoverToolTipBase):
    """an on-hover tooltip showing some text"""
    def __init__(self, anchor_widget, text, hover_delay=1500):
        super(TextToolTip, self).__init__(anchor_widget,
                                          hover_delay=hover_delay)
        self.text = text

    def showcontents(self):
        label = Label(self.tipwindow, text=self.text, justify=LEFT,
                      background="#ffffe0", relief=SOLID, borderwidth=1)
        label.pack()


def _tooltip(parent):  # htest #
    top = Toplevel(parent)
    top.title("Test tooltip")
    x, y = map(int, parent.geometry().split('+')[1:])
    top.geometry("+%d+%d" % (x, y + 150))
    label = Label(top, text="Place your mouse over buttons")
    label.pack()
    button1 = Button(top, text="Button 1 -- 1/2 second hover delay")
    button1.pack()
    TextToolTip(button1, "This is tooltip text for button1.", hover_delay=500)
    button2 = Button(top, text="Button 2 -- no hover delay")
    button2.pack()
    TextToolTip(button2, "This is tooltip\ntext for button2.", hover_delay=0)


if __name__ == '__main__':
    from idlelib.idle_test.htest import run
    run(_tooltip)
