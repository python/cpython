"""Assorted Tk-related subroutines used in Grail."""


from types import *
from Tkinter import *

def _clear_entry_widget(event):
    try:
        widget = event.widget
        widget.delete(0, INSERT)
    except: pass
def install_keybindings(root):
    root.bind_class('Entry', '<Control-u>', _clear_entry_widget)


def make_toplevel(master, title=None, class_=None):
    """Create a Toplevel widget.

    This is a shortcut for a Toplevel() instantiation plus calls to
    set the title and icon name of the widget.

    """

    if class_:
        widget = Toplevel(master, class_=class_)
    else:
        widget = Toplevel(master)
    if title:
        widget.title(title)
        widget.iconname(title)
    return widget

def set_transient(widget, master, relx=0.5, rely=0.3, expose=1):
    """Make an existing toplevel widget transient for a master.

    The widget must exist but should not yet have been placed; in
    other words, this should be called after creating all the
    subwidget but before letting the user interact.
    """

    widget.withdraw() # Remain invisible while we figure out the geometry
    widget.transient(master)
    widget.update_idletasks() # Actualize geometry information
    if master.winfo_ismapped():
        m_width = master.winfo_width()
        m_height = master.winfo_height()
        m_x = master.winfo_rootx()
        m_y = master.winfo_rooty()
    else:
        m_width = master.winfo_screenwidth()
        m_height = master.winfo_screenheight()
        m_x = m_y = 0
    w_width = widget.winfo_reqwidth()
    w_height = widget.winfo_reqheight()
    x = m_x + (m_width - w_width) * relx
    y = m_y + (m_height - w_height) * rely
    widget.geometry("+%d+%d" % (x, y))
    if expose:
        widget.deiconify()      # Become visible at the desired location
    return widget


def make_scrollbars(parent, hbar, vbar, pack=1, class_=None, name=None,
                    takefocus=0):

    """Subroutine to create a frame with scrollbars.

    This is used by make_text_box and similar routines.

    Note: the caller is responsible for setting the x/y scroll command
    properties (e.g. by calling set_scroll_commands()).

    Return a tuple containing the hbar, the vbar, and the frame, where
    hbar and vbar are None if not requested.

    """
    if class_:
        if name: frame = Frame(parent, class_=class_, name=name)
        else: frame = Frame(parent, class_=class_)
    else:
        if name: frame = Frame(parent, name=name)
        else: frame = Frame(parent)

    if pack:
        frame.pack(fill=BOTH, expand=1)

    corner = None
    if vbar:
        if not hbar:
            vbar = Scrollbar(frame, takefocus=takefocus)
            vbar.pack(fill=Y, side=RIGHT)
        else:
            vbarframe = Frame(frame, borderwidth=0)
            vbarframe.pack(fill=Y, side=RIGHT)
            vbar = Scrollbar(frame, name="vbar", takefocus=takefocus)
            vbar.pack(in_=vbarframe, expand=1, fill=Y, side=TOP)
            sbwidth = vbar.winfo_reqwidth()
            corner = Frame(vbarframe, width=sbwidth, height=sbwidth)
            corner.propagate(0)
            corner.pack(side=BOTTOM)
    else:
        vbar = None

    if hbar:
        hbar = Scrollbar(frame, orient=HORIZONTAL, name="hbar",
                         takefocus=takefocus)
        hbar.pack(fill=X, side=BOTTOM)
    else:
        hbar = None

    return hbar, vbar, frame


def set_scroll_commands(widget, hbar, vbar):

    """Link a scrollable widget to its scroll bars.

    The scroll bars may be empty.

    """

    if vbar:
        widget['yscrollcommand'] = (vbar, 'set')
        vbar['command'] = (widget, 'yview')

    if hbar:
        widget['xscrollcommand'] = (hbar, 'set')
        hbar['command'] = (widget, 'xview')

    widget.vbar = vbar
    widget.hbar = hbar


def make_text_box(parent, width=0, height=0, hbar=0, vbar=1,
                  fill=BOTH, expand=1, wrap=WORD, pack=1,
                  class_=None, name=None, takefocus=None):

    """Subroutine to create a text box.

    Create:
    - a both-ways filling and expanding frame, containing:
      - a text widget on the left, and
      - possibly a vertical scroll bar on the right.
      - possibly a horizonta; scroll bar at the bottom.

    Return the text widget and the frame widget.

    """
    hbar, vbar, frame = make_scrollbars(parent, hbar, vbar, pack,
                                        class_=class_, name=name,
                                        takefocus=takefocus)

    widget = Text(frame, wrap=wrap, name="text")
    if width: widget.config(width=width)
    if height: widget.config(height=height)
    widget.pack(expand=expand, fill=fill, side=LEFT)

    set_scroll_commands(widget, hbar, vbar)

    return widget, frame


def make_list_box(parent, width=0, height=0, hbar=0, vbar=1,
                  fill=BOTH, expand=1, pack=1, class_=None, name=None,
                  takefocus=None):

    """Subroutine to create a list box.

    Like make_text_box().
    """
    hbar, vbar, frame = make_scrollbars(parent, hbar, vbar, pack,
                                        class_=class_, name=name,
                                        takefocus=takefocus)

    widget = Listbox(frame, name="listbox")
    if width: widget.config(width=width)
    if height: widget.config(height=height)
    widget.pack(expand=expand, fill=fill, side=LEFT)

    set_scroll_commands(widget, hbar, vbar)

    return widget, frame


def make_canvas(parent, width=0, height=0, hbar=1, vbar=1,
                fill=BOTH, expand=1, pack=1, class_=None, name=None,
                takefocus=None):

    """Subroutine to create a canvas.

    Like make_text_box().

    """

    hbar, vbar, frame = make_scrollbars(parent, hbar, vbar, pack,
                                        class_=class_, name=name,
                                        takefocus=takefocus)

    widget = Canvas(frame, scrollregion=(0, 0, width, height), name="canvas")
    if width: widget.config(width=width)
    if height: widget.config(height=height)
    widget.pack(expand=expand, fill=fill, side=LEFT)

    set_scroll_commands(widget, hbar, vbar)

    return widget, frame



def make_form_entry(parent, label, borderwidth=None):

    """Subroutine to create a form entry.

    Create:
    - a horizontally filling and expanding frame, containing:
      - a label on the left, and
      - a text entry on the right.

    Return the entry widget and the frame widget.

    """

    frame = Frame(parent)
    frame.pack(fill=X)

    label = Label(frame, text=label)
    label.pack(side=LEFT)

    if borderwidth is None:
        entry = Entry(frame, relief=SUNKEN)
    else:
        entry = Entry(frame, relief=SUNKEN, borderwidth=borderwidth)
    entry.pack(side=LEFT, fill=X, expand=1)

    return entry, frame

# This is a slightly modified version of the function above.  This
# version does the proper alighnment of labels with their fields.  It
# should probably eventually replace make_form_entry altogether.
#
# The one annoying bug is that the text entry field should be
# expandable while still aligning the colons.  This doesn't work yet.
#
def make_labeled_form_entry(parent, label, entrywidth=20, entryheight=1,
                            labelwidth=0, borderwidth=None,
                            takefocus=None):
    """Subroutine to create a form entry.

    Create:
    - a horizontally filling and expanding frame, containing:
      - a label on the left, and
      - a text entry on the right.

    Return the entry widget and the frame widget.
    """
    if label and label[-1] != ':': label = label + ':'

    frame = Frame(parent)

    label = Label(frame, text=label, width=labelwidth, anchor=E)
    label.pack(side=LEFT)
    if entryheight == 1:
        if borderwidth is None:
            entry = Entry(frame, relief=SUNKEN, width=entrywidth)
        else:
            entry = Entry(frame, relief=SUNKEN, width=entrywidth,
                          borderwidth=borderwidth)
        entry.pack(side=RIGHT, expand=1, fill=X)
        frame.pack(fill=X)
    else:
        entry = make_text_box(frame, entrywidth, entryheight, 1, 1,
                              takefocus=takefocus)
        frame.pack(fill=BOTH, expand=1)

    return entry, frame, label


def make_double_frame(master=None, class_=None, name=None, relief=RAISED,
                      borderwidth=1):
    """Create a pair of frames suitable for 'hosting' a dialog."""
    if name:
        if class_: frame = Frame(master, class_=class_, name=name)
        else: frame = Frame(master, name=name)
    else:
        if class_: frame = Frame(master, class_=class_)
        else: frame = Frame(master)
    top = Frame(frame, name="topframe", relief=relief,
                borderwidth=borderwidth)
    bottom = Frame(frame, name="bottomframe")
    bottom.pack(fill=X, padx='1m', pady='1m', side=BOTTOM)
    top.pack(expand=1, fill=BOTH, padx='1m', pady='1m')
    frame.pack(expand=1, fill=BOTH)
    top = Frame(top)
    top.pack(expand=1, fill=BOTH, padx='2m', pady='2m')

    return frame, top, bottom


def make_group_frame(master, name=None, label=None, fill=Y,
                     side=None, expand=None, font=None):
    """Create nested frames with a border and optional label.

    The outer frame is only used to provide the decorative border, to
    control packing, and to host the label.  The inner frame is packed
    to fill the outer frame and should be used as the parent of all
    sub-widgets.  Only the inner frame is returned.

    """
    font = font or "-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*"
    outer = Frame(master, borderwidth=2, relief=GROOVE)
    outer.pack(expand=expand, fill=fill, side=side)
    if label:
        Label(outer, text=label, font=font, anchor=W).pack(fill=X)
    inner = Frame(master, borderwidth='1m', name=name)
    inner.pack(expand=1, fill=BOTH, in_=outer)
    inner.forget = outer.forget
    return inner


def unify_button_widths(*buttons):
    """Make buttons passed in all have the same width.

    Works for labels and other widgets with the 'text' option.

    """
    wid = 0
    for btn in buttons:
        wid = max(wid, len(btn["text"]))
    for btn in buttons:
        btn["width"] = wid


def flatten(msg):
    """Turn a list or tuple into a single string -- recursively."""
    t = type(msg)
    if t in (ListType, TupleType):
        msg = ' '.join(map(flatten, msg))
    elif t is ClassType:
        msg = msg.__name__
    else:
        msg = str(msg)
    return msg


def boolean(s):
    """Test whether a string is a Tk boolean, without error checking."""
    if s.lower() in ('', '0', 'no', 'off', 'false'): return 0
    else: return 1


def test():
    """Test make_text_box(), make_form_entry(), flatten(), boolean()."""
    import sys
    root = Tk()
    entry, eframe = make_form_entry(root, 'Boolean:')
    text, tframe = make_text_box(root)
    def enter(event, entry=entry, text=text):
        s = boolean(entry.get()) and '\nyes' or '\nno'
        text.insert('end', s)
    entry.bind('<Return>', enter)
    entry.insert(END, flatten(sys.argv))
    root.mainloop()


if __name__ == '__main__':
    test()
