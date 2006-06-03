#!/usr/bin/env python
#
# $Id$
#
# somebody should probably check the randrange()s...

import curses
from random import randrange

def next_j(j):
    if j == 0:
        j = 4
    else:
        j -= 1

    if curses.has_colors():
        z = randrange(0, 3)
        color = curses.color_pair(z)
        if z:
            color = color | curses.A_BOLD
        stdscr.attrset(color)

    return j

def main(win):
    # we know that the first argument from curses.wrapper() is stdscr.
    # Initialize it globally for convenience.
    global stdscr
    stdscr = win

    if curses.has_colors():
        bg = curses.COLOR_BLACK
        curses.init_pair(1, curses.COLOR_BLUE, bg)
        curses.init_pair(2, curses.COLOR_CYAN, bg)

    curses.nl()
    curses.noecho()
    # XXX curs_set() always returns ERR
    # curses.curs_set(0)
    stdscr.timeout(0)

    c = curses.COLS - 4
    r = curses.LINES - 4
    xpos = [0] * c
    ypos = [0] * r
    for j in range(4, -1, -1):
        xpos[j] = randrange(0, c) + 2
        ypos[j] = randrange(0, r) + 2

    j = 0
    while True:
        x = randrange(0, c) + 2
        y = randrange(0, r) + 2

        stdscr.addch(y, x, ord('.'))

        stdscr.addch(ypos[j], xpos[j], ord('o'))

        j = next_j(j)
        stdscr.addch(ypos[j], xpos[j], ord('O'))

        j = next_j(j)
        stdscr.addch( ypos[j] - 1, xpos[j],     ord('-'))
        stdscr.addstr(ypos[j],     xpos[j] - 1, "|.|")
        stdscr.addch( ypos[j] + 1, xpos[j],     ord('-'))

        j = next_j(j)
        stdscr.addch( ypos[j] - 2, xpos[j],     ord('-'))
        stdscr.addstr(ypos[j] - 1, xpos[j] - 1, "/ \\")
        stdscr.addstr(ypos[j],     xpos[j] - 2, "| O |")
        stdscr.addstr(ypos[j] + 1, xpos[j] - 1, "\\ /")
        stdscr.addch( ypos[j] + 2, xpos[j],     ord('-'))

        j = next_j(j)
        stdscr.addch( ypos[j] - 2, xpos[j],     ord(' '))
        stdscr.addstr(ypos[j] - 1, xpos[j] - 1, "   ")
        stdscr.addstr(ypos[j],     xpos[j] - 2, "     ")
        stdscr.addstr(ypos[j] + 1, xpos[j] - 1, "   ")
        stdscr.addch( ypos[j] + 2, xpos[j],     ord(' '))

        xpos[j] = x
        ypos[j] = y

        ch = stdscr.getch()
        if ch == ord('q') or ch == ord('Q'):
            return
        elif ch == ord('s'):
            stdscr.nodelay(0)
        elif ch == ord(' '):
            stdscr.nodelay(1)

        curses.napms(50)

curses.wrapper(main)
