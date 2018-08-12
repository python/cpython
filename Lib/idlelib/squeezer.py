"""An IDLE extension to avoid having very long texts printed in the shell.

A common problem in IDLE's interactive shell is printing of large amounts of
text into the shell. This makes looking at the previous history difficult.
Worse, this can cause IDLE to become very slow, even to the point of being
completely unusable.

This extension will automatically replace long texts with a small button.
Double-cliking this button will remove it and insert the original text instead.
Middle-clicking will copy the text to the clipboard. Right-clicking will open
the text in a separate viewing window.

Additionally, any output can be manually "squeezed" by the user. This includes
output written to the standard error stream ("stderr"), such as exception
messages and their tracebacks.
"""
import re

import tkinter as tk
from tkinter.font import Font

from idlelib.pyshell import PyShell
from idlelib.config import idleConf
from idlelib.textview import view_text
from idlelib.tooltip import ToolTip


def _add_to_rmenu(editwin, specs):
    """Utility func: Add specs to the right-click menu of the given editwin."""
    # Important: don't use += or .append() here!!!
    # rmenu_specs has a default value set as a class attribute, so we must be
    # sure to create an instance attribute here, without changing the class
    # attribute.
    editwin.rmenu_specs = editwin.rmenu_specs + specs


def count_lines_with_wrapping(s, linewidth=80, tabwidth=8):
    """Count the number of lines in a given string.

    Lines are counted as if the string was wrapped so that lines are never over
    linewidth characters long.

    Tabs are considered tabwidth characters long.
    """
    pos = 0
    linecount = 1
    current_column = 0

    for m in re.finditer(r"[\t\n]", s):
        # process the normal chars up to tab or newline
        numchars = m.start() - pos
        pos += numchars
        current_column += numchars

        # deal with tab or newline
        if s[pos] == '\n':
            linecount += 1
            current_column = 0
        else:
            assert s[pos] == '\t'
            current_column += tabwidth - (current_column % tabwidth)

            # if a tab passes the end of the line, consider the entire tab as
            # being on the next line
            if current_column > linewidth:
                linecount += 1
                current_column = tabwidth

        pos += 1 # after the tab or newline

        # avoid divmod(-1, linewidth)
        if current_column > 0:
            # If the length was exactly linewidth, divmod would give (1,0),
            # even though a new line hadn't yet been started. The same is true
            # if length is any exact multiple of linewidth. Therefore, subtract
            # 1 before doing divmod, and later add 1 to the column to
            # compensate.
            lines, column = divmod(current_column - 1, linewidth)
            linecount += lines
            current_column = column + 1

    # process remaining chars (no more tabs or newlines)
    current_column += len(s) - pos
    # avoid divmod(-1, linewidth)
    if current_column > 0:
        linecount += (current_column - 1) // linewidth
    else:
        # the text ended with a newline; don't count an extra line after it
        linecount -= 1

    return linecount


# define the extension's classes

class ExpandingButton(tk.Button):
    """Class for the "squeezed" text buttons used by Squeezer

    These buttons are displayed inside a Tk Text widget in place of text. A
    user can then use the button to replace it with the original text, copy
    the original text to the clipboard or preview the original text in a
    separate window.

    Each button is tied to a Squeezer instance, and it knows to update the
    Squeezer instance when it is expanded (and therefore removed).
    """
    def __init__(self, s, tags, numoflines, squeezer):
        self.s = s
        self.tags = tags
        self.squeezer = squeezer
        self.editwin = editwin = squeezer.editwin
        self.text = text = editwin.text

        # the base Text widget of the PyShell object, used to change text
        # before the iomark
        self.base_text = editwin.per.bottom

        button_text = "Squeezed text (%d lines)." % numoflines
        tk.Button.__init__(self, text, text=button_text,
                           background="#FFFFC0", activebackground="#FFFFE0")

        if self.squeezer.get_show_tooltip():
            button_tooltip_text = (
                "Double-click to expand, middle-click to copy, " +
                "right-click to preview."
            )
            ToolTip(self, button_tooltip_text,
                    delay=self.squeezer.get_tooltip_delay())

        self.bind("<Double-Button-1>", self.expand)
        self.bind("<Button-2>", self.copy)
        self.bind("<Button-3>", self.preview)
        self.selection_handle(
            lambda offset, length: s[int(offset):int(offset) + int(length)])

    def expand(self, event):
        """expand event handler

        This inserts the original text in place of the button in the Text
        widget, removes the button and updates the Squeezer instance.
        """
        self.base_text.insert(self.text.index(self), self.s, self.tags)
        self.base_text.delete(self)
        self.squeezer.expandingbuttons.remove(self)

    def copy(self, event):
        """copy event handler

        Copy the original text to the clipboard.
        """
        self.clipboard_clear()
        self.clipboard_append(self.s)

    def preview(self, event):
        """preview event handler

        View the original text in a separate text viewer window.
        """
        view_text(self.text, "Squeezed Output Viewer", self.s)


class Squeezer:
    """An IDLE extension for "squeezing" long texts into a simple button."""
    @classmethod
    def get_auto_squeeze_min_lines(cls):
        return idleConf.GetOption(
            "extensions", "Squeezer", "auto-squeeze-min-lines",
            type="int", default=30,
        )

    @classmethod
    def get_show_tooltip(cls):
        return idleConf.GetOption(
            "extensions", "Squeezer", "show-tooltip",
            type="bool", default=True,
        )

    @classmethod
    def get_tooltip_delay(cls):
        return idleConf.GetOption(
            "extensions", "Squeezer", "tooltip-delay",
            type="int", default=0,
        )

    menudefs = [
        ('edit', [
            None,   # Separator
            ("Expand last squeezed text", "<<expand-last-squeezed>>"),
            ("Preview last squeezed text", "<<preview-last-squeezed>>"),
        ]),
    ]

    def __init__(self, editwin):
        self.editwin = editwin
        self.text = text = editwin.text

        # Get the base Text widget of the PyShell object, used to change text
        # before the iomark. PyShell deliberately disables changing text before
        # the iomark via its 'text' attribute, which is actually a wrapper for
        # the actual Text widget. But Squeezer deliberately needs to make such
        # changes.
        self.base_text = editwin.per.bottom

        self.expandingbuttons = []
        if isinstance(editwin, PyShell):
            # If we get a PyShell instance, replace its write method with a
            # wrapper, which inserts an ExpandingButton instead of a long text.
            def mywrite(s, tags=(), write=editwin.write):
                # only auto-squeeze text which has just the "stdout" tag
                if tags != "stdout":
                    return write(s, tags)

                # only auto-squeeze text with at least the minimum
                # configured number of lines
                numoflines = self.count_lines(s)
                if numoflines < self.get_auto_squeeze_min_lines():
                    return write(s, tags)

                # create an ExpandingButton instance
                expandingbutton = ExpandingButton(s, tags, numoflines,
                                                  self)

                # insert the ExpandingButton into the Text widget
                text.mark_gravity("iomark", tk.RIGHT)
                text.window_create("iomark", window=expandingbutton,
                                   padx=3, pady=5)
                text.see("iomark")
                text.update()
                text.mark_gravity("iomark", tk.LEFT)

                # add the ExpandingButton to the Squeezer's list
                self.expandingbuttons.append(expandingbutton)

            editwin.write = mywrite

            # Add squeeze-current-text to the right-click menu
            text.bind("<<squeeze-current-text>>",
                      self.squeeze_current_text_event)
            _add_to_rmenu(editwin, [("Squeeze current text",
                                     "<<squeeze-current-text>>")])

    def count_lines(self, s):
        """Count the number of lines in a given text.

        Before calculation, the tab width and line length of the text are
        fetched, so that up-to-date values are used.

        Lines are counted as if the string was wrapped so that lines are never
        over linewidth characters long.

        Tabs are considered tabwidth characters long.
        """
        # Tab width is configurable
        tabwidth = self.editwin.get_tk_tabwidth()

        # Get the Text widget's size
        linewidth = self.editwin.text.winfo_width()
        # Deduct the border and padding
        linewidth -= 2*sum([int(self.editwin.text.cget(opt))
                            for opt in ('border', 'padx')])

        # Get the Text widget's font
        font = Font(self.editwin.text, name=self.editwin.text.cget('font'))
        # Divide the size of the Text widget by the font's width.
        # According to Tk8.5 docs, the Text widget's width is set
        # according to the width of its font's '0' (zero) character,
        # so we will use this as an approximation.
        # see: http://www.tcl.tk/man/tcl8.5/TkCmd/text.htm#M-width
        linewidth //= font.measure('0')

        return count_lines_with_wrapping(s, linewidth, tabwidth)

    def expand_last_squeezed_event(self, event):
        """expand-last-squeezed event handler

        Expand the last squeezed text in the Text widget.

        If there is no such squeezed text, give the user a small warning and
        do nothing.
        """
        if len(self.expandingbuttons) > 0:
            self.expandingbuttons[-1].expand(event)
        else:
            self.text.bell()
        return "break"

    def preview_last_squeezed_event(self, event):
        """preview-last-squeezed event handler

        Preview the last squeezed text in the Text widget.

        If there is no such squeezed text, give the user a small warning and
        do nothing.
        """
        if len(self.expandingbuttons) > 0:
            self.expandingbuttons[-1].preview(event)
        else:
            self.text.bell()
        return "break"

    def squeeze_current_text_event(self, event):
        """squeeze-current-text event handler

        Squeeze the block of text inside which contains the "insert" cursor.

        If the insert cursor is not in a squeezable block of text, give the
        user a small warning and do nothing.
        """
        # set tag_name to the first valid tag found on the "insert" cursor
        tag_names = self.text.tag_names(tk.INSERT)
        for tag_name in ("stdout", "stderr"):
            if tag_name in tag_names:
                break
        else:
            # the insert cursor doesn't have a "stdout" or "stderr" tag
            self.text.bell()
            return "break"

        # find the range to squeeze
        start, end = self.text.tag_prevrange(tag_name, tk.INSERT + "+1c")
        s = self.text.get(start, end)

        # if the last char is a newline, remove it from the range
        if len(s) > 0 and s[-1] == '\n':
            end = self.text.index("%s-1c" % end)
            s = s[:-1]

        # delete the text
        self.base_text.delete(start, end)

        # prepare an ExpandingButton
        numoflines = self.count_lines(s)
        expandingbutton = ExpandingButton(s, tag_name, numoflines, self)

        # insert the ExpandingButton to the Text
        self.text.window_create(start, window=expandingbutton,
                                padx=3, pady=5)

        # insert the ExpandingButton to the list of ExpandingButtons, while
        # keeping the list ordered according to the position of the buttons in
        # the Text widget
        i = len(self.expandingbuttons)
        while i > 0 and self.text.compare(self.expandingbuttons[i-1],
                                          ">", expandingbutton):
            i -= 1
        self.expandingbuttons.insert(i, expandingbutton)

        return "break"
