"""Simple textbox editing widget with Emacs-like keybindings."""

import curses
import curses.ascii

def rectangle(win, uly, ulx, lry, lrx):
    """Draw a rectangle with corners at the provided upper-left
    and lower-right coordinates.
    """
    win.vline(uly+1, ulx, curses.ACS_VLINE, lry - uly - 1)
    win.hline(uly, ulx+1, curses.ACS_HLINE, lrx - ulx - 1)
    win.hline(lry, ulx+1, curses.ACS_HLINE, lrx - ulx - 1)
    win.vline(uly+1, lrx, curses.ACS_VLINE, lry - uly - 1)
    win.addch(uly, ulx, curses.ACS_ULCORNER)
    win.addch(uly, lrx, curses.ACS_URCORNER)
    win.addch(lry, lrx, curses.ACS_LRCORNER)
    win.addch(lry, ulx, curses.ACS_LLCORNER)

class Textbox:
    """Editing widget using the interior of a window object.
     Supports the following Emacs-like key bindings:

    Ctrl-A      Go to left edge of window.
    Ctrl-B      Cursor left, wrapping to previous line if appropriate.
    Ctrl-D      Delete character under cursor.
    Ctrl-E      Go to right edge (stripspaces off) or end of line
                (stripspaces on).
    Ctrl-F      Cursor right, wrapping to next line when appropriate.
    Ctrl-G      Terminate, returning the window contents.
    Ctrl-H      Delete character backward.
    Ctrl-J      Terminate if the window is 1 line, otherwise move to start
                of next line.
    Ctrl-K      If line is blank, delete it, otherwise clear to end of line.
    Ctrl-L      Refresh screen.
    Ctrl-N      Cursor down; move down one line.
    Ctrl-O      Insert a blank line at cursor location.
    Ctrl-P      Cursor up; move up one line.

    Move operations do nothing if the cursor is at an edge where the
    movement is not possible.  The following synonyms are supported where
    possible:

    KEY_LEFT = Ctrl-B, KEY_RIGHT = Ctrl-F, KEY_UP = Ctrl-P,
    KEY_DOWN = Ctrl-N, KEY_BACKSPACE = Ctrl-h
    """
    def __init__(self, win, insert_mode=False):
        self.win = win
        self.insert_mode = insert_mode
        self._update_max_yx()
        self.stripspaces = 1
        self.lastcmd = None
        win.keypad(1)

    def _update_max_yx(self):
        maxy, maxx = self.win.getmaxyx()
        self.maxy = maxy - 1
        self.maxx = maxx - 1

    def _decode(self, ch):
        # Decode an integer keystroke or byte to text with the window's encoding.
        # A_CHARTEXT drops any attribute bits.
        return bytes([ch & curses.A_CHARTEXT]).decode(self.win.encoding, 'replace')

    def _printable_key(self, ch):
        # Whether the integer keystroke is a printable character, not a key code:
        # 0..255 are character bytes, larger values are function keys.
        return ch <= 0xff and self._decode(ch).isprintable()

    def _end_of_line(self, y):
        """Go to the location of the first blank on the given line,
        returning the index of the last non-blank character."""
        self._update_max_yx()
        last = self.maxx
        while True:
            # The text of the cell at (y, last).
            if str(self.win.in_wch(y, last)) != ' ':
                last = min(self.maxx, last+1)
                break
            elif last == 0:
                break
            last = last - 1
        return last

    def _insert_printable_char(self, ch):
        self._update_max_yx()
        (y, x) = self.win.getyx()
        backyx = None
        while True:
            if self.insert_mode:
                # The displaced cell, as a complexchar so addch() can rewrite it
                # with its rendition.
                oldch = self.win.in_wch()
            if y >= self.maxy and x >= self.maxx:
                # Use insch() in the lower-right cell; addch() there would push
                # the cursor out of the window (an error, and it scrolls a
                # scrollable window).  insch() does not decode an int byte
                # through the locale on a wide build, so pass it as text.
                if isinstance(ch, int):
                    self.win.insch(self._decode(ch), ch & curses.A_ATTRIBUTES)
                else:
                    self.win.insch(ch)
                break
            self.win.addch(ch)
            # In insert mode keep shifting cells right until a blank one.
            if not self.insert_mode or not str(oldch).isprintable():
                break
            ch = oldch
            (y, x) = self.win.getyx()
            # Remember where to put the cursor back since we are in insert_mode
            if backyx is None:
                backyx = y, x

        if backyx is not None:
            self.win.move(*backyx)

    def do_command(self, ch):
        "Process a single editing command."
        self._update_max_yx()
        (y, x) = self.win.getyx()
        self.lastcmd = ch
        if isinstance(ch, str):
            # A character from get_wch(); a control character is dispatched
            # below by its code point.
            if ch.isprintable():
                self._insert_printable_char(ch)
                return 1
            ch = ord(ch)
        elif self._printable_key(ch):
            self._insert_printable_char(ch)
            return 1
        if ch == curses.ascii.SOH:                             # ^a
            self.win.move(y, 0)
        elif ch in (curses.ascii.STX,curses.KEY_LEFT,
                    curses.ascii.BS,
                    curses.KEY_BACKSPACE,
                    curses.ascii.DEL):
            if x > 0:
                self.win.move(y, x-1)
            elif y == 0:
                pass
            elif self.stripspaces:
                self.win.move(y-1, self._end_of_line(y-1))
            else:
                self.win.move(y-1, self.maxx)
            if ch in (curses.ascii.BS, curses.KEY_BACKSPACE, curses.ascii.DEL):
                self.win.delch()
        elif ch == curses.ascii.EOT:                           # ^d
            self.win.delch()
        elif ch == curses.ascii.ENQ:                           # ^e
            if self.stripspaces:
                self.win.move(y, self._end_of_line(y))
            else:
                self.win.move(y, self.maxx)
        elif ch in (curses.ascii.ACK, curses.KEY_RIGHT):       # ^f
            if x < self.maxx:
                self.win.move(y, x+1)
            elif y == self.maxy:
                pass
            else:
                self.win.move(y+1, 0)
        elif ch == curses.ascii.BEL:                           # ^g
            return 0
        elif ch == curses.ascii.NL:                            # ^j
            if self.maxy == 0:
                return 0
            elif y < self.maxy:
                self.win.move(y+1, 0)
        elif ch == curses.ascii.VT:                            # ^k
            if x == 0 and self._end_of_line(y) == 0:
                self.win.deleteln()
            else:
                # first undo the effect of self._end_of_line
                self.win.move(y, x)
                self.win.clrtoeol()
        elif ch == curses.ascii.FF:                            # ^l
            self.win.refresh()
        elif ch in (curses.ascii.SO, curses.KEY_DOWN):         # ^n
            if y < self.maxy:
                self.win.move(y+1, x)
                if x > self._end_of_line(y+1):
                    self.win.move(y+1, self._end_of_line(y+1))
        elif ch == curses.ascii.SI:                            # ^o
            self.win.insertln()
        elif ch in (curses.ascii.DLE, curses.KEY_UP):          # ^p
            if y > 0:
                self.win.move(y-1, x)
                if x > self._end_of_line(y-1):
                    self.win.move(y-1, self._end_of_line(y-1))
        return 1

    def gather(self):
        "Collect and return the contents of the window."
        result = ""
        self._update_max_yx()
        for y in range(self.maxy+1):
            self.win.move(y, 0)
            stop = self._end_of_line(y)
            if stop == 0 and self.stripspaces:
                continue
            for x in range(self.maxx+1):
                if self.stripspaces and x > stop:
                    break
                result = result + str(self.win.in_wch(y, x))
            if self.maxy > 0:
                result = result + "\n"
        return result

    def edit(self, validate=None):
        "Edit in the widget window and collect the results."
        while 1:
            ch = self.win.get_wch()
            # Represent an ASCII keystroke by its code point, the way getch()
            # always has, so that existing validators and the command dispatch
            # keep working; only non-ASCII characters are passed as strings.
            if isinstance(ch, str) and ch.isascii():
                ch = ord(ch)
            if validate:
                ch = validate(ch)
            if not ch:
                continue
            if not self.do_command(ch):
                break
            self.win.refresh()
        return self.gather()

if __name__ == '__main__':
    def test_editbox(stdscr):
        ncols, nlines = 9, 4
        uly, ulx = 15, 20
        stdscr.addstr(uly-2, ulx, "Use Ctrl-G to end editing.")
        win = curses.newwin(nlines, ncols, uly, ulx)
        rectangle(stdscr, uly-1, ulx-1, uly + nlines, ulx + ncols)
        stdscr.refresh()
        return Textbox(win).edit()

    str = curses.wrapper(test_editbox)
    print('Contents of text box:', repr(str))
