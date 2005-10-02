"""CodeContext - Display the block context of code at top of edit window

Once code has scrolled off the top of the screen, it can be difficult
to determine which block you are in.  This extension implements a pane
at the top of each IDLE edit window which provides block structure
hints.  These hints are the lines which contain the block opening
keywords, e.g. 'if', for the enclosing block.  The number of hint lines
is determined by the numlines variable in the CodeContext section of
config-extensions.def. Lines which do not open blocks are not shown in
the context hints pane.

"""
import Tkinter
from configHandler import idleConf
from sets import Set
import re
from sys import maxint as INFINITY

BLOCKOPENERS = Set(["class", "def", "elif", "else", "except", "finally", "for",
                    "if", "try", "while"])
UPDATEINTERVAL = 100 # millisec
FONTUPDATEINTERVAL = 1000 # millisec

getspacesfirstword = lambda s, c=re.compile(r"^(\s*)(\w*)"): c.match(s).groups()

class CodeContext:
    menudefs = [('options', [('!Code Conte_xt', '<<toggle-code-context>>')])]

    numlines = idleConf.GetOption("extensions", "CodeContext",
                                  "numlines", type="int", default=3)
    bgcolor = idleConf.GetOption("extensions", "CodeContext",
                                 "bgcolor", type="str", default="LightGray")
    fgcolor = idleConf.GetOption("extensions", "CodeContext",
                                 "fgcolor", type="str", default="Black")
    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.textfont = self.text["font"]
        self.label = None
        # self.info holds information about the context lines of line number
        # self.lastfirstline. The information is a tuple of the line's
        # indentation, the line's text and the keyword at the beginning of the
        # line, as returned by get_line_info. At the beginning of the list
        # there's a dummy line, which starts the "block" of the whole document.
        self.info = [(0, -1, "", False)]
        self.lastfirstline = 1
        visible = idleConf.GetOption("extensions", "CodeContext",
                                     "visible", type="bool", default=False)
        if visible:
            self.toggle_code_context_event()
            self.editwin.setvar('<<toggle-code-context>>', True)
        # Start two update cycles, one for context lines, one for font changes.
        self.text.after(UPDATEINTERVAL, self.timer_event)
        self.text.after(FONTUPDATEINTERVAL, self.font_timer_event)

    def toggle_code_context_event(self, event=None):
        if not self.label:
            self.label = Tkinter.Label(self.editwin.top,
                                      text="\n" * (self.numlines - 1),
                                      anchor="w", justify="left",
                                      font=self.textfont,
                                      bg=self.bgcolor, fg=self.fgcolor,
                                      relief="sunken",
                                      width=1, # Don't request more than we get
                                      )
            self.label.pack(side="top", fill="x", expand=0,
                            after=self.editwin.status_bar)
        else:
            self.label.destroy()
            self.label = None
        idleConf.SetOption("extensions", "CodeContext", "visible",
                           str(self.label is not None))
        idleConf.SaveUserCfgFiles()

    def get_line_info(self, linenum):
        """Get the line indent value, text, and any block start keyword

        If the line does not start a block, the keyword value is False.
        The indentation of empty lines (or comment lines) is INFINITY.
        """
        text = self.text.get("%d.0" % linenum, "%d.end" % linenum)
        spaces, firstword = getspacesfirstword(text)
        opener = firstword in BLOCKOPENERS and firstword
        if len(text) == len(spaces) or text[len(spaces)] == '#':
            indent = INFINITY
        else:
            indent = len(spaces)
        return indent, text, opener

    def interesting_lines(self, firstline, stopline=1, stopindent=0):
        """
        Find the context lines, starting at firstline.
        Will not return lines whose index is smaller than stopline or whose
        indentation is smaller than stopindent.
        stopline should always be >= 1, so the dummy block start will never
        be returned (This function doesn't know what to do about it.)
        Returns a list with the context lines, starting from the first (top),
        and a number which all context lines above the inspected region should
        have a smaller indentation than it.
        """
        lines = []
        # The indentation level we are currently in:
        lastindent = INFINITY
        # For a line to be interesting, it must begin with a block opening
        # keyword, and have less indentation than lastindent.
        for line_index in xrange(firstline, stopline-1, -1):
            indent, text, opener = self.get_line_info(line_index)
            if indent < lastindent:
                lastindent = indent
                if opener in ("else", "elif"):
                    # We also show the if statement
                    lastindent += 1
                if opener and line_index < firstline and indent >= stopindent:
                    lines.append((line_index, indent, text, opener))
                if lastindent <= stopindent:
                    break
        lines.reverse()
        return lines, lastindent

    def update_label(self):
        """Update the CodeContext label, if needed.
        """
        firstline = int(self.text.index("@0,0").split('.')[0])
        if self.lastfirstline == firstline:
            return
        if self.lastfirstline < firstline:
            lines, lastindent = self.interesting_lines(firstline,
                                                       self.lastfirstline)
            while self.info[-1][1] >= lastindent:
                del self.info[-1]
            self.info.extend(lines)
        else:
            stopindent = self.info[-1][1] + 1
            while self.info[-1][0] >= firstline:
                stopindent = self.info[-1][1]
                del self.info[-1]
            lines, lastindent = self.interesting_lines(
                firstline, self.info[-1][0]+1, stopindent)
            self.info.extend(lines)
        self.lastfirstline = firstline
        lines = [""] * max(0, self.numlines - len(self.info)) + \
                [x[2] for x in self.info[-self.numlines:]]
        self.label["text"] = '\n'.join(lines)

    def timer_event(self):
        if self.label:
            self.update_label()
        self.text.after(UPDATEINTERVAL, self.timer_event)

    def font_timer_event(self):
        newtextfont = self.text["font"]
        if self.label and newtextfont != self.textfont:
            self.textfont = newtextfont
            self.label["font"] = self.textfont
        self.text.after(FONTUPDATEINTERVAL, self.font_timer_event)
