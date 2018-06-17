"""A CallTip window class for Tkinter/IDLE.

After tooltip.py, which uses ideas gleaned from PySol
Used by the calltips IDLE extension.
"""
from tkinter import Label, LEFT, SOLID, TclError

from idlelib.tooltip import ToolTipBase

HIDE_VIRTUAL_EVENT_NAME = "<<calltipwindow-hide>>"
HIDE_SEQUENCES = ("<Key-Escape>", "<FocusOut>")
CHECKHIDE_VIRTUAL_EVENT_NAME = "<<calltipwindow-checkhide>>"
CHECKHIDE_SEQUENCES = ("<KeyRelease>", "<ButtonRelease>")
CHECKHIDE_TIME = 100  # milliseconds

MARK_RIGHT = "calltipwindowregion_right"


class CallTip(ToolTipBase):
    """a call-tip widget for tkinter text widgets"""

    def __init__(self, text_widget):
        super(CallTip, self).__init__(text_widget)
        self.text_widget = self.anchor_widget
        self.label = self.text = None
        self.parenline = self.parencol = self.lastline = None
        self.hideid = self.checkhideid = None
        self.checkhide_after_id = None

    def __del__(self):
        super(CallTip, self).__del__()

    def get_position(self):
        """Choose the position of the calltip"""
        curline = int(self.text_widget.index("insert").split('.')[0])
        if curline == self.parenline:
            box = self.text_widget.bbox("%d.%d" % (self.parenline, self.parencol))
        else:
            box = self.text_widget.bbox("%d.0" % curline)
        if not box:
            box = list(self.text_widget.bbox("insert"))
            # align to left of window
            box[0] = 0
            box[2] = 0
        return box[0] + 2, box[1] + box[3]

    def position_window(self):
        """Check if needs to reposition the window, and if so - do it."""
        curline = int(self.text_widget.index("insert").split('.')[0])
        if curline == self.lastline:
            return
        self.lastline = curline
        self.text_widget.see("insert")
        super(CallTip, self).position_window()

    def showtip(self, text, parenleft, parenright):
        """Show the calltip, bind events which will close it and reposition it.
        """
        # Only called in CallTips, where lines are truncated
        self.text = text
        if self.tipwindow or not self.text:
            return

        self.text_widget.mark_set(MARK_RIGHT, parenright)
        self.parenline, self.parencol = map(
            int, self.text_widget.index(parenleft).split("."))

        super(CallTip, self).showtip()

        self._bind_events()

    def showcontents(self):
        self.label = Label(self.tipwindow, text=self.text, justify=LEFT,
                           background="#ffffe0", relief=SOLID, borderwidth=1,
                           font=self.text_widget['font'])
        self.label.pack()

    def checkhide_event(self, event=None):
        if not self.tipwindow:
            # If the event was triggered by the same event that unbinded
            # this function, the function will be called nevertheless,
            # so do nothing in this case.
            return None
        curline, curcol = map(int, self.text_widget.index("insert").split('.'))
        if curline < self.parenline or \
           (curline == self.parenline and curcol <= self.parencol) or \
           self.text_widget.compare("insert", ">", MARK_RIGHT):
            self.hidetip()
            return "break"
        else:
            self.position_window()
            if self.checkhide_after_id is not None:
                self.text_widget.after_cancel(self.checkhide_after_id)
            self.checkhide_after_id = \
                self.text_widget.after(CHECKHIDE_TIME, self.checkhide_event)
            return None

    def hide_event(self, event):
        if not self.tipwindow:
            # See the explanation in checkhide_event.
            return None
        self.hidetip()
        return "break"

    def hidetip(self):
        if not self.tipwindow:
            return

        try:
            self.label.destroy()
        except TclError:
            pass
        self.label = None

        self.parenline = self.parencol = self.lastline = None
        try:
            self.text_widget.mark_unset(MARK_RIGHT)
        except TclError:
            pass

        try:
            self._unbind_events()
        except (TclError, ValueError):
            # ValueError may be raised by MultiCall
            pass

        super(CallTip, self).hidetip()

    def _bind_events(self):
        self.checkhideid = self.text_widget.bind(CHECKHIDE_VIRTUAL_EVENT_NAME,
                                                  self.checkhide_event)
        for seq in CHECKHIDE_SEQUENCES:
            self.text_widget.event_add(CHECKHIDE_VIRTUAL_EVENT_NAME, seq)
        self.text_widget.after(CHECKHIDE_TIME, self.checkhide_event)
        self.hideid = self.text_widget.bind(HIDE_VIRTUAL_EVENT_NAME,
                                             self.hide_event)
        for seq in HIDE_SEQUENCES:
            self.text_widget.event_add(HIDE_VIRTUAL_EVENT_NAME, seq)

    def _unbind_events(self):
        for seq in CHECKHIDE_SEQUENCES:
            self.text_widget.event_delete(CHECKHIDE_VIRTUAL_EVENT_NAME, seq)
        self.text_widget.unbind(CHECKHIDE_VIRTUAL_EVENT_NAME, self.checkhideid)
        self.checkhideid = None
        for seq in HIDE_SEQUENCES:
            self.text_widget.event_delete(HIDE_VIRTUAL_EVENT_NAME, seq)
        self.text_widget.unbind(HIDE_VIRTUAL_EVENT_NAME, self.hideid)
        self.hideid = None


def _calltip_window(parent):  # htest #
    from tkinter import Toplevel, Text, LEFT, BOTH

    top = Toplevel(parent)
    top.title("Test calltips")
    x, y = map(int, parent.geometry().split('+')[1:])
    top.geometry("200x100+%d+%d" % (x + 250, y + 175))
    text = Text(top)
    text.pack(side=LEFT, fill=BOTH, expand=1)
    text.insert("insert", "string.split")
    top.update()

    calltip = CallTip(text)
    def calltip_show(event):
        calltip.showtip("(s='Hello world')", "insert", "end")
    def calltip_hide(event):
        calltip.hidetip()
    text.event_add("<<calltip-show>>", "(")
    text.event_add("<<calltip-hide>>", ")")
    text.bind("<<calltip-show>>", calltip_show)
    text.bind("<<calltip-hide>>", calltip_hide)

    text.focus_set()

if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_calltips', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_calltip_window)
