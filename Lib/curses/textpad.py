"""curses.textpad

"""

import sys, curses, ascii

def rectangle(win, uly, ulx, lry, lrx):
    "Draw a rectangle."
    win.vline(uly+1, ulx, curses.ACS_VLINE, lry - uly - 1)
    win.hline(uly, ulx+1, curses.ACS_HLINE, lrx - ulx - 1)
    win.hline(lry, ulx+1, curses.ACS_HLINE, lrx - ulx - 1)
    win.vline(uly+1, lrx, curses.ACS_VLINE, lry - uly - 1)
    win.addch(uly, ulx, curses.ACS_ULCORNER)
    win.addch(uly, lrx, curses.ACS_URCORNER)
    win.addch(lry, lrx, curses.ACS_LRCORNER)
    win.addch(lry, ulx, curses.ACS_LLCORNER)

class textbox:
    """Editing widget using the interior of a window object.
     Supports the following Emacs-like key bindings:

    Ctrl-A      Go to left edge of window.
    Ctrl-B      Cursor left, wrapping to previous line if appropriate.
    Ctrl-D      Delete character under cursor.
    Ctrl-E      Go to right edge (nospaces off) or end of line (nospaces on).
    Ctrl-F      Cursor right, wrapping to next line when appropriate.
    Ctrl-G      Terminate, returning the window contents.
    Ctrl-J      Terminate if the window is 1 line, otherwise insert newline.
    Ctrl-K      If line is blank, delete it, otherwise clear to end of line.
    Ctrl-L      Refresh screen
    Ctrl-N      Cursor down; move down one line.
    Ctrl-O      Insert a blank line at cursor location.
    Ctrl-P      Cursor up; move up one line.

    Move operations do nothing if the cursor is at an edge where the movement
    is not possible.  The following synonyms are supported where possible:

    KEY_LEFT = Ctrl-B, KEY_RIGHT = Ctrl-F, KEY_UP = Ctrl-P, KEY_DOWN = Ctrl-N
    """
    def __init__(self, win):
        self.win = win
        (self.maxy, self.maxx) = win.getmaxyx()
        self.maxy = self.maxy - 1
        self.maxx = self.maxx - 1
        self.stripspaces = 1
        win.keypad(1)

    def firstblank(self, y):
        "Go to the location of the first blank on the given line."
        (oldy, oldx) = self.win.getyx()
        self.win.move(y, self.maxx-1)
        last = self.maxx-1
        while 1:
            if last == 0:
                break
            if ascii.ascii(self.win.inch(y, last)) != ascii.SP:
                last = last + 1
                break
            last = last - 1
        self.win.move(oldy, oldx)
        return last

    def do_command(self, ch):
        "Process a single editing command."
        (y, x) = self.win.getyx()
        if ascii.isprint(ch):
            if y < self.maxy or x < self.maxx:
                # The try-catch ignores the error we trigger from some curses
                # versions by trying to write into the lowest-rightmost spot
                # in the self.window.
                try:
                    self.win.addch(ch)
                except ERR:
                    pass
        elif ch == ascii.SOH:				# Ctrl-a
            self.win.move(y, 0)
        elif ch in (ascii.STX, curses.KEY_LEFT):	# Ctrl-b
            if x > 0:
                self.win.move(y, x-1)
            elif y == 0:
                pass
            elif self.stripspaces:
                self.win.move(y-1, self.firstblank(y-1))
            else:
                self.win.move(y-1, self.maxx)
        elif ch == ascii.EOT:				# Ctrl-d
            self.win.delch()
        elif ch == ascii.ENQ:				# Ctrl-e
            if self.stripspaces:
                self.win.move(y, self.firstblank(y, maxx))
            else:
                self.win.move(y, self.maxx)
        elif ch in (ascii.ACK, curses.KEY_RIGHT):	# Ctrl-f
            if x < self.maxx:
                self.win.move(y, x+1)
            elif y == self.maxx:
                pass
            else:
                self.win.move(y+1, 0)
        elif ch == ascii.BEL:				# Ctrl-g
            return 0
        elif ch == ascii.NL:				# Ctrl-j
            if self.maxy == 0:
                return 0
            elif y < self.maxy:
                self.win.move(y+1, 0)
        elif ch == ascii.VT:				# Ctrl-k
            if x == 0 and self.firstblank(y) == 0:
                self.win.deleteln()
            else:
                self.win.clrtoeol()
        elif ch == ascii.FF:				# Ctrl-l
            self.win.refresh()
        elif ch in (ascii.SO, curses.KEY_DOWN):		# Ctrl-n
            if y < self.maxy:
                self.win.move(y+1, x)
        elif ch == ascii.SI:				# Ctrl-o
            self.win.insertln()
        elif ch in (ascii.DLE, curses.KEY_UP):		# Ctrl-p
            if y > 0:
                self.win.move(y-1, x)
        self.win.refresh()
        return 1
        
    def gather(self):
        "Collect and return the contents of the window."
        result = ""
        for y in range(self.maxy+1):
            self.win.move(y, 0)
            stop = self.firstblank(y)
            if stop == 0 and self.stripspaces:
                continue
            for x in range(self.maxx+1):
                if self.stripspaces and x == stop:
                    break
                result = result + chr(ascii.ascii(self.win.inch(y, x)))
            if self.maxy > 0:
                result = result + "\n"
        return result

    def edit(self, validate=None):
        "Edit in the widget window and collect the results."
        while 1:
            ch = self.win.getch()
            if validate:
                ch = validate(ch)
            if not self.do_command(ch):
                break
        return self.gather()

if __name__ == '__main__':
    def test_editbox(stdscr):
        win = curses.newwin(4, 9, 15, 20)
        rectangle(stdscr, 14, 19, 19, 29)
        stdscr.refresh()
        return textbox(win).edit()

    str = curses.wrapper(test_editbox)
    print str
