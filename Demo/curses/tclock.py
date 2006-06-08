#!/usr/bin/env python
#
# $Id$
#
# From tclock.c, Copyright Howard Jones <ha.jones@ic.ac.uk>, September 1994.

from math import *
import curses, time

ASPECT = 2.2

def sign(_x):
    if _x < 0: return -1
    return 1

def A2XY(angle, radius):
    return (int(round(ASPECT * radius * sin(angle))),
            int(round(radius * cos(angle))))

def plot(x, y, col):
    stdscr.addch(y, x, col)

# draw a diagonal line using Bresenham's algorithm
def dline(pair, from_x, from_y, x2, y2, ch):
    if curses.has_colors():
        stdscr.attrset(curses.color_pair(pair))

    dx = x2 - from_x
    dy = y2 - from_y

    ax = abs(dx * 2)
    ay = abs(dy * 2)

    sx = sign(dx)
    sy = sign(dy)

    x = from_x
    y = from_y

    if ax > ay:
        d = ay - ax // 2

        while True:
            plot(x, y, ch)
            if x == x2:
                return

            if d >= 0:
                y += sy
                d -= ax
            x += sx
            d += ay
    else:
        d = ax - ay // 2

        while True:
            plot(x, y, ch)
            if y == y2:
                return

            if d >= 0:
                x += sx
                d -= ay
            y += sy
            d += ax

def main(win):
    global stdscr
    stdscr = win

    lastbeep = -1
    my_bg = curses.COLOR_BLACK

    stdscr.nodelay(1)
    stdscr.timeout(0)
#    curses.curs_set(0)
    if curses.has_colors():
        curses.init_pair(1, curses.COLOR_RED, my_bg)
        curses.init_pair(2, curses.COLOR_MAGENTA, my_bg)
        curses.init_pair(3, curses.COLOR_GREEN, my_bg)

    cx = (curses.COLS - 1) // 2
    cy = curses.LINES // 2
    ch = min( cy-1, int(cx // ASPECT) - 1)
    mradius = (3 * ch) // 4
    hradius = ch // 2
    sradius = 5 * ch // 6

    for i in range(0, 12):
        sangle = (i + 1) * 2.0 * pi / 12.0
        sdx, sdy = A2XY(sangle, sradius)

        stdscr.addstr(cy - sdy, cx + sdx, "%d" % (i + 1))

    stdscr.addstr(0, 0,
                  "ASCII Clock by Howard Jones <ha.jones@ic.ac.uk>, 1994")

    sradius = max(sradius-4, 8)

    while True:
        curses.napms(1000)

        tim = time.time()
        t = time.localtime(tim)

        hours = t[3] + t[4] / 60.0
        if hours > 12.0:
            hours -= 12.0

        mangle = t[4] * 2 * pi / 60.0
        mdx, mdy = A2XY(mangle, mradius)

        hangle = hours * 2 * pi / 12.0
        hdx, hdy = A2XY(hangle, hradius)

        sangle = t[5] * 2 * pi / 60.0
        sdx, sdy = A2XY(sangle, sradius)

        dline(3, cx, cy, cx + mdx, cy - mdy, ord('#'))

        stdscr.attrset(curses.A_REVERSE)
        dline(2, cx, cy, cx + hdx, cy - hdy, ord('.'))
        stdscr.attroff(curses.A_REVERSE)

        if curses.has_colors():
            stdscr.attrset(curses.color_pair(1))

        plot(cx + sdx, cy - sdy, ord('O'))

        if curses.has_colors():
            stdscr.attrset(curses.color_pair(0))

        stdscr.addstr(curses.LINES - 2, 0, time.ctime(tim))
        stdscr.refresh()
        if (t[5] % 5) == 0 and t[5] != lastbeep:
            lastbeep = t[5]
            curses.beep()

        ch = stdscr.getch()
        if ch == ord('q'):
            return 0

        plot(cx + sdx, cy - sdy, ord(' '))
        dline(0, cx, cy, cx + hdx, cy - hdy, ord(' '))
        dline(0, cx, cy, cx + mdx, cy - mdy, ord(' '))

curses.wrapper(main)
