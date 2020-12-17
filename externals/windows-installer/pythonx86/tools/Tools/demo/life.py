#!/usr/bin/env python3

"""
A curses-based version of Conway's Game of Life.

An empty board will be displayed, and the following commands are available:
 E : Erase the board
 R : Fill the board randomly
 S : Step for a single generation
 C : Update continuously until a key is struck
 Q : Quit
 Cursor keys :  Move the cursor around the board
 Space or Enter : Toggle the contents of the cursor's position

Contributed by Andrew Kuchling, Mouse support and color by Dafydd Crosby.
"""

import curses
import random


class LifeBoard:
    """Encapsulates a Life board

    Attributes:
    X,Y : horizontal and vertical size of the board
    state : dictionary mapping (x,y) to 0 or 1

    Methods:
    display(update_board) -- If update_board is true, compute the
                             next generation.  Then display the state
                             of the board and refresh the screen.
    erase() -- clear the entire board
    make_random() -- fill the board randomly
    set(y,x) -- set the given cell to Live; doesn't refresh the screen
    toggle(y,x) -- change the given cell from live to dead, or vice
                   versa, and refresh the screen display

    """
    def __init__(self, scr, char=ord('*')):
        """Create a new LifeBoard instance.

        scr -- curses screen object to use for display
        char -- character used to render live cells (default: '*')
        """
        self.state = {}
        self.scr = scr
        Y, X = self.scr.getmaxyx()
        self.X, self.Y = X - 2, Y - 2 - 1
        self.char = char
        self.scr.clear()

        # Draw a border around the board
        border_line = '+' + (self.X * '-') + '+'
        self.scr.addstr(0, 0, border_line)
        self.scr.addstr(self.Y + 1, 0, border_line)
        for y in range(0, self.Y):
            self.scr.addstr(1 + y, 0, '|')
            self.scr.addstr(1 + y, self.X + 1, '|')
        self.scr.refresh()

    def set(self, y, x):
        """Set a cell to the live state"""
        if x < 0 or self.X <= x or y < 0 or self.Y <= y:
            raise ValueError("Coordinates out of range %i,%i" % (y, x))
        self.state[x, y] = 1

    def toggle(self, y, x):
        """Toggle a cell's state between live and dead"""
        if x < 0 or self.X <= x or y < 0 or self.Y <= y:
            raise ValueError("Coordinates out of range %i,%i" % (y, x))
        if (x, y) in self.state:
            del self.state[x, y]
            self.scr.addch(y + 1, x + 1, ' ')
        else:
            self.state[x, y] = 1
            if curses.has_colors():
                # Let's pick a random color!
                self.scr.attrset(curses.color_pair(random.randrange(1, 7)))
            self.scr.addch(y + 1, x + 1, self.char)
            self.scr.attrset(0)
        self.scr.refresh()

    def erase(self):
        """Clear the entire board and update the board display"""
        self.state = {}
        self.display(update_board=False)

    def display(self, update_board=True):
        """Display the whole board, optionally computing one generation"""
        M, N = self.X, self.Y
        if not update_board:
            for i in range(0, M):
                for j in range(0, N):
                    if (i, j) in self.state:
                        self.scr.addch(j + 1, i + 1, self.char)
                    else:
                        self.scr.addch(j + 1, i + 1, ' ')
            self.scr.refresh()
            return

        d = {}
        self.boring = 1
        for i in range(0, M):
            L = range(max(0, i - 1), min(M, i + 2))
            for j in range(0, N):
                s = 0
                live = (i, j) in self.state
                for k in range(max(0, j - 1), min(N, j + 2)):
                    for l in L:
                        if (l, k) in self.state:
                            s += 1
                s -= live
                if s == 3:
                    # Birth
                    d[i, j] = 1
                    if curses.has_colors():
                        # Let's pick a random color!
                        self.scr.attrset(curses.color_pair(
                            random.randrange(1, 7)))
                    self.scr.addch(j + 1, i + 1, self.char)
                    self.scr.attrset(0)
                    if not live:
                        self.boring = 0
                elif s == 2 and live:
                    # Survival
                    d[i, j] = 1
                elif live:
                    # Death
                    self.scr.addch(j + 1, i + 1, ' ')
                    self.boring = 0
        self.state = d
        self.scr.refresh()

    def make_random(self):
        "Fill the board with a random pattern"
        self.state = {}
        for i in range(0, self.X):
            for j in range(0, self.Y):
                if random.random() > 0.5:
                    self.set(j, i)


def erase_menu(stdscr, menu_y):
    "Clear the space where the menu resides"
    stdscr.move(menu_y, 0)
    stdscr.clrtoeol()
    stdscr.move(menu_y + 1, 0)
    stdscr.clrtoeol()


def display_menu(stdscr, menu_y):
    "Display the menu of possible keystroke commands"
    erase_menu(stdscr, menu_y)

    # If color, then light the menu up :-)
    if curses.has_colors():
        stdscr.attrset(curses.color_pair(1))
    stdscr.addstr(menu_y, 4,
        'Use the cursor keys to move, and space or Enter to toggle a cell.')
    stdscr.addstr(menu_y + 1, 4,
        'E)rase the board, R)andom fill, S)tep once or C)ontinuously, Q)uit')
    stdscr.attrset(0)


def keyloop(stdscr):
    # Clear the screen and display the menu of keys
    stdscr.clear()
    stdscr_y, stdscr_x = stdscr.getmaxyx()
    menu_y = (stdscr_y - 3) - 1
    display_menu(stdscr, menu_y)

    # If color, then initialize the color pairs
    if curses.has_colors():
        curses.init_pair(1, curses.COLOR_BLUE, 0)
        curses.init_pair(2, curses.COLOR_CYAN, 0)
        curses.init_pair(3, curses.COLOR_GREEN, 0)
        curses.init_pair(4, curses.COLOR_MAGENTA, 0)
        curses.init_pair(5, curses.COLOR_RED, 0)
        curses.init_pair(6, curses.COLOR_YELLOW, 0)
        curses.init_pair(7, curses.COLOR_WHITE, 0)

    # Set up the mask to listen for mouse events
    curses.mousemask(curses.BUTTON1_CLICKED)

    # Allocate a subwindow for the Life board and create the board object
    subwin = stdscr.subwin(stdscr_y - 3, stdscr_x, 0, 0)
    board = LifeBoard(subwin, char=ord('*'))
    board.display(update_board=False)

    # xpos, ypos are the cursor's position
    xpos, ypos = board.X // 2, board.Y // 2

    # Main loop:
    while True:
        stdscr.move(1 + ypos, 1 + xpos)   # Move the cursor
        c = stdscr.getch()                # Get a keystroke
        if 0 < c < 256:
            c = chr(c)
            if c in ' \n':
                board.toggle(ypos, xpos)
            elif c in 'Cc':
                erase_menu(stdscr, menu_y)
                stdscr.addstr(menu_y, 6, ' Hit any key to stop continuously '
                              'updating the screen.')
                stdscr.refresh()
                # Activate nodelay mode; getch() will return -1
                # if no keystroke is available, instead of waiting.
                stdscr.nodelay(1)
                while True:
                    c = stdscr.getch()
                    if c != -1:
                        break
                    stdscr.addstr(0, 0, '/')
                    stdscr.refresh()
                    board.display()
                    stdscr.addstr(0, 0, '+')
                    stdscr.refresh()

                stdscr.nodelay(0)       # Disable nodelay mode
                display_menu(stdscr, menu_y)

            elif c in 'Ee':
                board.erase()
            elif c in 'Qq':
                break
            elif c in 'Rr':
                board.make_random()
                board.display(update_board=False)
            elif c in 'Ss':
                board.display()
            else:
                # Ignore incorrect keys
                pass
        elif c == curses.KEY_UP and ypos > 0:
            ypos -= 1
        elif c == curses.KEY_DOWN and ypos + 1 < board.Y:
            ypos += 1
        elif c == curses.KEY_LEFT and xpos > 0:
            xpos -= 1
        elif c == curses.KEY_RIGHT and xpos + 1 < board.X:
            xpos += 1
        elif c == curses.KEY_MOUSE:
            mouse_id, mouse_x, mouse_y, mouse_z, button_state = curses.getmouse()
            if (mouse_x > 0 and mouse_x < board.X + 1 and
                mouse_y > 0 and mouse_y < board.Y + 1):
                xpos = mouse_x - 1
                ypos = mouse_y - 1
                board.toggle(ypos, xpos)
            else:
                # They've clicked outside the board
                curses.flash()
        else:
            # Ignore incorrect keys
            pass


def main(stdscr):
    keyloop(stdscr)                 # Enter the main loop

if __name__ == '__main__':
    curses.wrapper(main)
