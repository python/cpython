# asciixmas
# December 1989             Larry Bartz           Indianapolis, IN
#
# $Id$
#
# I'm dreaming of an ascii character-based monochrome Christmas,
# Just like the ones I used to know!
# Via a full duplex communications channel,
# At 9600 bits per second,
# Even though it's kinda slow.
#
# I'm dreaming of an ascii character-based monochrome Christmas,
# With ev'ry C program I write!
# May your screen be merry and bright!
# And may all your Christmases be amber or green,
# (for reduced eyestrain and improved visibility)!
#
#
# Notes on the Python version:
# I used a couple of `try...except curses.error' to get around some functions
# returning ERR. The errors come from using wrapping functions to fill
# windows to the last character cell. The C version doesn't have this problem,
# it simply ignores any return values.
#

import curses
import sys

FROMWHO = "Thomas Gellekum <tg@FreeBSD.org>"

def set_color(win, color):
    if curses.has_colors():
        n = color + 1
        curses.init_pair(n, color, my_bg)
        win.attroff(curses.A_COLOR)
        win.attron(curses.color_pair(n))

def unset_color(win):
    if curses.has_colors():
        win.attrset(curses.color_pair(0))

def look_out(msecs):
    curses.napms(msecs)
    if stdscr.getch() != -1:
        curses.beep()
        sys.exit(0)

def boxit():
    for y in range(0, 20):
        stdscr.addch(y, 7, ord('|'))

    for x in range(8, 80):
        stdscr.addch(19, x, ord('_'))

    for x in range(0, 80):
        stdscr.addch(22, x, ord('_'))

    return

def seas():
    stdscr.addch(4, 1, ord('S'))
    stdscr.addch(6, 1, ord('E'))
    stdscr.addch(8, 1, ord('A'))
    stdscr.addch(10, 1, ord('S'))
    stdscr.addch(12, 1, ord('O'))
    stdscr.addch(14, 1, ord('N'))
    stdscr.addch(16, 1, ord("'"))
    stdscr.addch(18, 1, ord('S'))

    return

def greet():
    stdscr.addch(3, 5, ord('G'))
    stdscr.addch(5, 5, ord('R'))
    stdscr.addch(7, 5, ord('E'))
    stdscr.addch(9, 5, ord('E'))
    stdscr.addch(11, 5, ord('T'))
    stdscr.addch(13, 5, ord('I'))
    stdscr.addch(15, 5, ord('N'))
    stdscr.addch(17, 5, ord('G'))
    stdscr.addch(19, 5, ord('S'))

    return

def fromwho():
    stdscr.addstr(21, 13, FROMWHO)
    return

def tree():
    set_color(treescrn, curses.COLOR_GREEN)
    treescrn.addch(1, 11, ord('/'))
    treescrn.addch(2, 11, ord('/'))
    treescrn.addch(3, 10, ord('/'))
    treescrn.addch(4, 9, ord('/'))
    treescrn.addch(5, 9, ord('/'))
    treescrn.addch(6, 8, ord('/'))
    treescrn.addch(7, 7, ord('/'))
    treescrn.addch(8, 6, ord('/'))
    treescrn.addch(9, 6, ord('/'))
    treescrn.addch(10, 5, ord('/'))
    treescrn.addch(11, 3, ord('/'))
    treescrn.addch(12, 2, ord('/'))

    treescrn.addch(1, 13, ord('\\'))
    treescrn.addch(2, 13, ord('\\'))
    treescrn.addch(3, 14, ord('\\'))
    treescrn.addch(4, 15, ord('\\'))
    treescrn.addch(5, 15, ord('\\'))
    treescrn.addch(6, 16, ord('\\'))
    treescrn.addch(7, 17, ord('\\'))
    treescrn.addch(8, 18, ord('\\'))
    treescrn.addch(9, 18, ord('\\'))
    treescrn.addch(10, 19, ord('\\'))
    treescrn.addch(11, 21, ord('\\'))
    treescrn.addch(12, 22, ord('\\'))

    treescrn.addch(4, 10, ord('_'))
    treescrn.addch(4, 14, ord('_'))
    treescrn.addch(8, 7, ord('_'))
    treescrn.addch(8, 17, ord('_'))

    treescrn.addstr(13, 0, "//////////// \\\\\\\\\\\\\\\\\\\\\\\\")

    treescrn.addstr(14, 11, "| |")
    treescrn.addstr(15, 11, "|_|")

    unset_color(treescrn)
    treescrn.refresh()
    w_del_msg.refresh()

    return

def balls():
    treescrn.overlay(treescrn2)

    set_color(treescrn2, curses.COLOR_BLUE)
    treescrn2.addch(3, 9, ord('@'))
    treescrn2.addch(3, 15, ord('@'))
    treescrn2.addch(4, 8, ord('@'))
    treescrn2.addch(4, 16, ord('@'))
    treescrn2.addch(5, 7, ord('@'))
    treescrn2.addch(5, 17, ord('@'))
    treescrn2.addch(7, 6, ord('@'))
    treescrn2.addch(7, 18, ord('@'))
    treescrn2.addch(8, 5, ord('@'))
    treescrn2.addch(8, 19, ord('@'))
    treescrn2.addch(10, 4, ord('@'))
    treescrn2.addch(10, 20, ord('@'))
    treescrn2.addch(11, 2, ord('@'))
    treescrn2.addch(11, 22, ord('@'))
    treescrn2.addch(12, 1, ord('@'))
    treescrn2.addch(12, 23, ord('@'))

    unset_color(treescrn2)
    treescrn2.refresh()
    w_del_msg.refresh()
    return

def star():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_YELLOW)

    treescrn2.addch(0, 12, ord('*'))
    treescrn2.standend()

    unset_color(treescrn2)
    treescrn2.refresh()
    w_del_msg.refresh()
    return

def strng1():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_WHITE)

    treescrn2.addch(3, 13, ord('\''))
    treescrn2.addch(3, 12, ord(':'))
    treescrn2.addch(3, 11, ord('.'))

    treescrn2.attroff(curses.A_BOLD | curses.A_BLINK)
    unset_color(treescrn2)

    treescrn2.refresh()
    w_del_msg.refresh()
    return

def strng2():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_WHITE)

    treescrn2.addch(5, 14, ord('\''))
    treescrn2.addch(5, 13, ord(':'))
    treescrn2.addch(5, 12, ord('.'))
    treescrn2.addch(5, 11, ord(','))
    treescrn2.addch(6, 10, ord('\''))
    treescrn2.addch(6, 9, ord(':'))

    treescrn2.attroff(curses.A_BOLD | curses.A_BLINK)
    unset_color(treescrn2)

    treescrn2.refresh()
    w_del_msg.refresh()
    return

def strng3():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_WHITE)

    treescrn2.addch(7, 16, ord('\''))
    treescrn2.addch(7, 15, ord(':'))
    treescrn2.addch(7, 14, ord('.'))
    treescrn2.addch(7, 13, ord(','))
    treescrn2.addch(8, 12, ord('\''))
    treescrn2.addch(8, 11, ord(':'))
    treescrn2.addch(8, 10, ord('.'))
    treescrn2.addch(8, 9, ord(','))

    treescrn2.attroff(curses.A_BOLD | curses.A_BLINK)
    unset_color(treescrn2)

    treescrn2.refresh()
    w_del_msg.refresh()
    return

def strng4():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_WHITE)

    treescrn2.addch(9, 17, ord('\''))
    treescrn2.addch(9, 16, ord(':'))
    treescrn2.addch(9, 15, ord('.'))
    treescrn2.addch(9, 14, ord(','))
    treescrn2.addch(10, 13, ord('\''))
    treescrn2.addch(10, 12, ord(':'))
    treescrn2.addch(10, 11, ord('.'))
    treescrn2.addch(10, 10, ord(','))
    treescrn2.addch(11, 9, ord('\''))
    treescrn2.addch(11, 8, ord(':'))
    treescrn2.addch(11, 7, ord('.'))
    treescrn2.addch(11, 6, ord(','))
    treescrn2.addch(12, 5, ord('\''))

    treescrn2.attroff(curses.A_BOLD | curses.A_BLINK)
    unset_color(treescrn2)

    treescrn2.refresh()
    w_del_msg.refresh()
    return

def strng5():
    treescrn2.attrset(curses.A_BOLD | curses.A_BLINK)
    set_color(treescrn2, curses.COLOR_WHITE)

    treescrn2.addch(11, 19, ord('\''))
    treescrn2.addch(11, 18, ord(':'))
    treescrn2.addch(11, 17, ord('.'))
    treescrn2.addch(11, 16, ord(','))
    treescrn2.addch(12, 15, ord('\''))
    treescrn2.addch(12, 14, ord(':'))
    treescrn2.addch(12, 13, ord('.'))
    treescrn2.addch(12, 12, ord(','))

    treescrn2.attroff(curses.A_BOLD | curses.A_BLINK)
    unset_color(treescrn2)

    # save a fully lit tree
    treescrn2.overlay(treescrn)

    treescrn2.refresh()
    w_del_msg.refresh()
    return

def blinkit():
    treescrn8.touchwin()

    for cycle in range(5):
        if cycle == 0:
            treescrn3.overlay(treescrn8)
            treescrn8.refresh()
            w_del_msg.refresh()
            break
        elif cycle == 1:
            treescrn4.overlay(treescrn8)
            treescrn8.refresh()
            w_del_msg.refresh()
            break
        elif cycle == 2:
            treescrn5.overlay(treescrn8)
            treescrn8.refresh()
            w_del_msg.refresh()
            break
        elif cycle == 3:
            treescrn6.overlay(treescrn8)
            treescrn8.refresh()
            w_del_msg.refresh()
            break
        elif cycle == 4:
            treescrn7.overlay(treescrn8)
            treescrn8.refresh()
            w_del_msg.refresh()
            break

        treescrn8.touchwin()

    # ALL ON
    treescrn.overlay(treescrn8)
    treescrn8.refresh()
    w_del_msg.refresh()

    return

def deer_step(win, y, x):
    win.mvwin(y, x)
    win.refresh()
    w_del_msg.refresh()
    look_out(5)

def reindeer():
    y_pos = 0

    for x_pos in range(70, 62, -1):
        if x_pos < 66: y_pos = 1
        for looper in range(0, 4):
            dotdeer0.addch(y_pos, x_pos, ord('.'))
            dotdeer0.refresh()
            w_del_msg.refresh()
            dotdeer0.erase()
            dotdeer0.refresh()
            w_del_msg.refresh()
            look_out(50)

    y_pos = 2

    for x_pos in range(x_pos - 1, 50, -1):
        for looper in range(0, 4):
            if x_pos < 56:
                y_pos = 3

                try:
                    stardeer0.addch(y_pos, x_pos, ord('*'))
                except curses.error:
                    pass
                stardeer0.refresh()
                w_del_msg.refresh()
                stardeer0.erase()
                stardeer0.refresh()
                w_del_msg.refresh()
            else:
                dotdeer0.addch(y_pos, x_pos, ord('*'))
                dotdeer0.refresh()
                w_del_msg.refresh()
                dotdeer0.erase()
                dotdeer0.refresh()
                w_del_msg.refresh()

    x_pos = 58

    for y_pos in range(2, 5):
        lildeer0.touchwin()
        lildeer0.refresh()
        w_del_msg.refresh()

        for looper in range(0, 4):
            deer_step(lildeer3, y_pos, x_pos)
            deer_step(lildeer2, y_pos, x_pos)
            deer_step(lildeer1, y_pos, x_pos)
            deer_step(lildeer2, y_pos, x_pos)
            deer_step(lildeer3, y_pos, x_pos)

            lildeer0.touchwin()
            lildeer0.refresh()
            w_del_msg.refresh()

            x_pos -= 2

    x_pos = 35

    for y_pos in range(5, 10):

        middeer0.touchwin()
        middeer0.refresh()
        w_del_msg.refresh()

        for looper in range(2):
            deer_step(middeer3, y_pos, x_pos)
            deer_step(middeer2, y_pos, x_pos)
            deer_step(middeer1, y_pos, x_pos)
            deer_step(middeer2, y_pos, x_pos)
            deer_step(middeer3, y_pos, x_pos)

            middeer0.touchwin()
            middeer0.refresh()
            w_del_msg.refresh()

            x_pos -= 3

    look_out(300)

    y_pos = 1

    for x_pos in range(8, 16):
        deer_step(bigdeer4, y_pos, x_pos)
        deer_step(bigdeer3, y_pos, x_pos)
        deer_step(bigdeer2, y_pos, x_pos)
        deer_step(bigdeer1, y_pos, x_pos)
        deer_step(bigdeer2, y_pos, x_pos)
        deer_step(bigdeer3, y_pos, x_pos)
        deer_step(bigdeer4, y_pos, x_pos)
        deer_step(bigdeer0, y_pos, x_pos)

    x_pos -= 1

    for looper in range(0, 6):
        deer_step(lookdeer4, y_pos, x_pos)
        deer_step(lookdeer3, y_pos, x_pos)
        deer_step(lookdeer2, y_pos, x_pos)
        deer_step(lookdeer1, y_pos, x_pos)
        deer_step(lookdeer2, y_pos, x_pos)
        deer_step(lookdeer3, y_pos, x_pos)
        deer_step(lookdeer4, y_pos, x_pos)

    deer_step(lookdeer0, y_pos, x_pos)

    for y_pos in range(y_pos, 10):
        for looper in range(0, 2):
            deer_step(bigdeer4, y_pos, x_pos)
            deer_step(bigdeer3, y_pos, x_pos)
            deer_step(bigdeer2, y_pos, x_pos)
            deer_step(bigdeer1, y_pos, x_pos)
            deer_step(bigdeer2, y_pos, x_pos)
            deer_step(bigdeer3, y_pos, x_pos)
            deer_step(bigdeer4, y_pos, x_pos)
        deer_step(bigdeer0, y_pos, x_pos)

    y_pos -= 1

    deer_step(lookdeer3, y_pos, x_pos)
    return

def main(win):
    global stdscr
    stdscr = win

    global my_bg, y_pos, x_pos
    global treescrn, treescrn2, treescrn3, treescrn4
    global treescrn5, treescrn6, treescrn7, treescrn8
    global dotdeer0, stardeer0
    global lildeer0, lildeer1, lildeer2, lildeer3
    global middeer0, middeer1, middeer2, middeer3
    global bigdeer0, bigdeer1, bigdeer2, bigdeer3, bigdeer4
    global lookdeer0, lookdeer1, lookdeer2, lookdeer3, lookdeer4
    global w_holiday, w_del_msg

    my_bg = curses.COLOR_BLACK
    # curses.curs_set(0)

    treescrn = curses.newwin(16, 27, 3, 53)
    treescrn2 = curses.newwin(16, 27, 3, 53)
    treescrn3 = curses.newwin(16, 27, 3, 53)
    treescrn4 = curses.newwin(16, 27, 3, 53)
    treescrn5 = curses.newwin(16, 27, 3, 53)
    treescrn6 = curses.newwin(16, 27, 3, 53)
    treescrn7 = curses.newwin(16, 27, 3, 53)
    treescrn8 = curses.newwin(16, 27, 3, 53)

    dotdeer0 = curses.newwin(3, 71, 0, 8)

    stardeer0 = curses.newwin(4, 56, 0, 8)

    lildeer0 = curses.newwin(7, 53, 0, 8)
    lildeer1 = curses.newwin(2, 4, 0, 0)
    lildeer2 = curses.newwin(2, 4, 0, 0)
    lildeer3 = curses.newwin(2, 4, 0, 0)

    middeer0 = curses.newwin(15, 42, 0, 8)
    middeer1 = curses.newwin(3, 7, 0, 0)
    middeer2 = curses.newwin(3, 7, 0, 0)
    middeer3 = curses.newwin(3, 7, 0, 0)

    bigdeer0 = curses.newwin(10, 23, 0, 0)
    bigdeer1 = curses.newwin(10, 23, 0, 0)
    bigdeer2 = curses.newwin(10, 23, 0, 0)
    bigdeer3 = curses.newwin(10, 23, 0, 0)
    bigdeer4 = curses.newwin(10, 23, 0, 0)

    lookdeer0 = curses.newwin(10, 25, 0, 0)
    lookdeer1 = curses.newwin(10, 25, 0, 0)
    lookdeer2 = curses.newwin(10, 25, 0, 0)
    lookdeer3 = curses.newwin(10, 25, 0, 0)
    lookdeer4 = curses.newwin(10, 25, 0, 0)

    w_holiday = curses.newwin(1, 27, 3, 27)

    w_del_msg = curses.newwin(1, 20, 23, 60)

    try:
        w_del_msg.addstr(0, 0, "Hit any key to quit")
    except curses.error:
        pass

    try:
        w_holiday.addstr(0, 0, "H A P P Y  H O L I D A Y S")
    except curses.error:
        pass

    # set up the windows for our various reindeer
    lildeer1.addch(0, 0, ord('V'))
    lildeer1.addch(1, 0, ord('@'))
    lildeer1.addch(1, 1, ord('<'))
    lildeer1.addch(1, 2, ord('>'))
    try:
        lildeer1.addch(1, 3, ord('~'))
    except curses.error:
        pass

    lildeer2.addch(0, 0, ord('V'))
    lildeer2.addch(1, 0, ord('@'))
    lildeer2.addch(1, 1, ord('|'))
    lildeer2.addch(1, 2, ord('|'))
    try:
        lildeer2.addch(1, 3, ord('~'))
    except curses.error:
        pass

    lildeer3.addch(0, 0, ord('V'))
    lildeer3.addch(1, 0, ord('@'))
    lildeer3.addch(1, 1, ord('>'))
    lildeer3.addch(1, 2, ord('<'))
    try:
        lildeer2.addch(1, 3, ord('~'))  # XXX
    except curses.error:
        pass

    middeer1.addch(0, 2, ord('y'))
    middeer1.addch(0, 3, ord('y'))
    middeer1.addch(1, 2, ord('0'))
    middeer1.addch(1, 3, ord('('))
    middeer1.addch(1, 4, ord('='))
    middeer1.addch(1, 5, ord(')'))
    middeer1.addch(1, 6, ord('~'))
    middeer1.addch(2, 3, ord('\\'))
    middeer1.addch(2, 5, ord('/'))

    middeer2.addch(0, 2, ord('y'))
    middeer2.addch(0, 3, ord('y'))
    middeer2.addch(1, 2, ord('0'))
    middeer2.addch(1, 3, ord('('))
    middeer2.addch(1, 4, ord('='))
    middeer2.addch(1, 5, ord(')'))
    middeer2.addch(1, 6, ord('~'))
    middeer2.addch(2, 3, ord('|'))
    middeer2.addch(2, 5, ord('|'))

    middeer3.addch(0, 2, ord('y'))
    middeer3.addch(0, 3, ord('y'))
    middeer3.addch(1, 2, ord('0'))
    middeer3.addch(1, 3, ord('('))
    middeer3.addch(1, 4, ord('='))
    middeer3.addch(1, 5, ord(')'))
    middeer3.addch(1, 6, ord('~'))
    middeer3.addch(2, 3, ord('/'))
    middeer3.addch(2, 5, ord('\\'))

    bigdeer1.addch(0, 17, ord('\\'))
    bigdeer1.addch(0, 18, ord('/'))
    bigdeer1.addch(0, 19, ord('\\'))
    bigdeer1.addch(0, 20, ord('/'))
    bigdeer1.addch(1, 18, ord('\\'))
    bigdeer1.addch(1, 20, ord('/'))
    bigdeer1.addch(2, 19, ord('|'))
    bigdeer1.addch(2, 20, ord('_'))
    bigdeer1.addch(3, 18, ord('/'))
    bigdeer1.addch(3, 19, ord('^'))
    bigdeer1.addch(3, 20, ord('0'))
    bigdeer1.addch(3, 21, ord('\\'))
    bigdeer1.addch(4, 17, ord('/'))
    bigdeer1.addch(4, 18, ord('/'))
    bigdeer1.addch(4, 19, ord('\\'))
    bigdeer1.addch(4, 22, ord('\\'))
    bigdeer1.addstr(5, 7, "^~~~~~~~~//  ~~U")
    bigdeer1.addstr(6, 7, "( \\_____( /")       # ))
    bigdeer1.addstr(7, 8, "( )    /")
    bigdeer1.addstr(8, 9, "\\\\   /")
    bigdeer1.addstr(9, 11, "\\>/>")

    bigdeer2.addch(0, 17, ord('\\'))
    bigdeer2.addch(0, 18, ord('/'))
    bigdeer2.addch(0, 19, ord('\\'))
    bigdeer2.addch(0, 20, ord('/'))
    bigdeer2.addch(1, 18, ord('\\'))
    bigdeer2.addch(1, 20, ord('/'))
    bigdeer2.addch(2, 19, ord('|'))
    bigdeer2.addch(2, 20, ord('_'))
    bigdeer2.addch(3, 18, ord('/'))
    bigdeer2.addch(3, 19, ord('^'))
    bigdeer2.addch(3, 20, ord('0'))
    bigdeer2.addch(3, 21, ord('\\'))
    bigdeer2.addch(4, 17, ord('/'))
    bigdeer2.addch(4, 18, ord('/'))
    bigdeer2.addch(4, 19, ord('\\'))
    bigdeer2.addch(4, 22, ord('\\'))
    bigdeer2.addstr(5, 7, "^~~~~~~~~//  ~~U")
    bigdeer2.addstr(6, 7, "(( )____( /")        # ))
    bigdeer2.addstr(7, 7, "( /    |")
    bigdeer2.addstr(8, 8, "\\/    |")
    bigdeer2.addstr(9, 9, "|>   |>")

    bigdeer3.addch(0, 17, ord('\\'))
    bigdeer3.addch(0, 18, ord('/'))
    bigdeer3.addch(0, 19, ord('\\'))
    bigdeer3.addch(0, 20, ord('/'))
    bigdeer3.addch(1, 18, ord('\\'))
    bigdeer3.addch(1, 20, ord('/'))
    bigdeer3.addch(2, 19, ord('|'))
    bigdeer3.addch(2, 20, ord('_'))
    bigdeer3.addch(3, 18, ord('/'))
    bigdeer3.addch(3, 19, ord('^'))
    bigdeer3.addch(3, 20, ord('0'))
    bigdeer3.addch(3, 21, ord('\\'))
    bigdeer3.addch(4, 17, ord('/'))
    bigdeer3.addch(4, 18, ord('/'))
    bigdeer3.addch(4, 19, ord('\\'))
    bigdeer3.addch(4, 22, ord('\\'))
    bigdeer3.addstr(5, 7, "^~~~~~~~~//  ~~U")
    bigdeer3.addstr(6, 6, "( ()_____( /")       # ))
    bigdeer3.addstr(7, 6, "/ /       /")
    bigdeer3.addstr(8, 5, "|/          \\")
    bigdeer3.addstr(9, 5, "/>           \\>")

    bigdeer4.addch(0, 17, ord('\\'))
    bigdeer4.addch(0, 18, ord('/'))
    bigdeer4.addch(0, 19, ord('\\'))
    bigdeer4.addch(0, 20, ord('/'))
    bigdeer4.addch(1, 18, ord('\\'))
    bigdeer4.addch(1, 20, ord('/'))
    bigdeer4.addch(2, 19, ord('|'))
    bigdeer4.addch(2, 20, ord('_'))
    bigdeer4.addch(3, 18, ord('/'))
    bigdeer4.addch(3, 19, ord('^'))
    bigdeer4.addch(3, 20, ord('0'))
    bigdeer4.addch(3, 21, ord('\\'))
    bigdeer4.addch(4, 17, ord('/'))
    bigdeer4.addch(4, 18, ord('/'))
    bigdeer4.addch(4, 19, ord('\\'))
    bigdeer4.addch(4, 22, ord('\\'))
    bigdeer4.addstr(5, 7, "^~~~~~~~~//  ~~U")
    bigdeer4.addstr(6, 6, "( )______( /")       # )
    bigdeer4.addstr(7, 5, "(/          \\")     # )
    bigdeer4.addstr(8, 0, "v___=             ----^")

    lookdeer1.addstr(0, 16, "\\/     \\/")
    lookdeer1.addstr(1, 17, "\\Y/ \\Y/")
    lookdeer1.addstr(2, 19, "\\=/")
    lookdeer1.addstr(3, 17, "^\\o o/^")
    lookdeer1.addstr(4, 17, "//( )")
    lookdeer1.addstr(5, 7, "^~~~~~~~~// \\O/")
    lookdeer1.addstr(6, 7, "( \\_____( /")      # ))
    lookdeer1.addstr(7, 8, "( )    /")
    lookdeer1.addstr(8, 9, "\\\\   /")
    lookdeer1.addstr(9, 11, "\\>/>")

    lookdeer2.addstr(0, 16, "\\/     \\/")
    lookdeer2.addstr(1, 17, "\\Y/ \\Y/")
    lookdeer2.addstr(2, 19, "\\=/")
    lookdeer2.addstr(3, 17, "^\\o o/^")
    lookdeer2.addstr(4, 17, "//( )")
    lookdeer2.addstr(5, 7, "^~~~~~~~~// \\O/")
    lookdeer2.addstr(6, 7, "(( )____( /")       # ))
    lookdeer2.addstr(7, 7, "( /    |")
    lookdeer2.addstr(8, 8, "\\/    |")
    lookdeer2.addstr(9, 9, "|>   |>")

    lookdeer3.addstr(0, 16, "\\/     \\/")
    lookdeer3.addstr(1, 17, "\\Y/ \\Y/")
    lookdeer3.addstr(2, 19, "\\=/")
    lookdeer3.addstr(3, 17, "^\\o o/^")
    lookdeer3.addstr(4, 17, "//( )")
    lookdeer3.addstr(5, 7, "^~~~~~~~~// \\O/")
    lookdeer3.addstr(6, 6, "( ()_____( /")      # ))
    lookdeer3.addstr(7, 6, "/ /       /")
    lookdeer3.addstr(8, 5, "|/          \\")
    lookdeer3.addstr(9, 5, "/>           \\>")

    lookdeer4.addstr(0, 16, "\\/     \\/")
    lookdeer4.addstr(1, 17, "\\Y/ \\Y/")
    lookdeer4.addstr(2, 19, "\\=/")
    lookdeer4.addstr(3, 17, "^\\o o/^")
    lookdeer4.addstr(4, 17, "//( )")
    lookdeer4.addstr(5, 7, "^~~~~~~~~// \\O/")
    lookdeer4.addstr(6, 6, "( )______( /")      # )
    lookdeer4.addstr(7, 5, "(/          \\")    # )
    lookdeer4.addstr(8, 0, "v___=             ----^")

    ###############################################
    curses.cbreak()
    stdscr.nodelay(1)

    while 1:
        stdscr.clear()
        treescrn.erase()
        w_del_msg.touchwin()
        treescrn.touchwin()
        treescrn2.erase()
        treescrn2.touchwin()
        treescrn8.erase()
        treescrn8.touchwin()
        stdscr.refresh()
        look_out(150)
        boxit()
        stdscr.refresh()
        look_out(150)
        seas()
        stdscr.refresh()
        greet()
        stdscr.refresh()
        look_out(150)
        fromwho()
        stdscr.refresh()
        look_out(150)
        tree()
        look_out(150)
        balls()
        look_out(150)
        star()
        look_out(150)
        strng1()
        strng2()
        strng3()
        strng4()
        strng5()

        # set up the windows for our blinking trees
        #
        # treescrn3
        treescrn.overlay(treescrn3)

        # balls
        treescrn3.addch(4, 18, ord(' '))
        treescrn3.addch(7, 6, ord(' '))
        treescrn3.addch(8, 19, ord(' '))
        treescrn3.addch(11, 22, ord(' '))

        # star
        treescrn3.addch(0, 12, ord('*'))

        # strng1
        treescrn3.addch(3, 11, ord(' '))

        # strng2
        treescrn3.addch(5, 13, ord(' '))
        treescrn3.addch(6, 10, ord(' '))

        # strng3
        treescrn3.addch(7, 16, ord(' '))
        treescrn3.addch(7, 14, ord(' '))

        # strng4
        treescrn3.addch(10, 13, ord(' '))
        treescrn3.addch(10, 10, ord(' '))
        treescrn3.addch(11, 8, ord(' '))

        # strng5
        treescrn3.addch(11, 18, ord(' '))
        treescrn3.addch(12, 13, ord(' '))

        # treescrn4
        treescrn.overlay(treescrn4)

        # balls
        treescrn4.addch(3, 9, ord(' '))
        treescrn4.addch(4, 16, ord(' '))
        treescrn4.addch(7, 6, ord(' '))
        treescrn4.addch(8, 19, ord(' '))
        treescrn4.addch(11, 2, ord(' '))
        treescrn4.addch(12, 23, ord(' '))

        # star
        treescrn4.standout()
        treescrn4.addch(0, 12, ord('*'))
        treescrn4.standend()

        # strng1
        treescrn4.addch(3, 13, ord(' '))

        # strng2

        # strng3
        treescrn4.addch(7, 15, ord(' '))
        treescrn4.addch(8, 11, ord(' '))

        # strng4
        treescrn4.addch(9, 16, ord(' '))
        treescrn4.addch(10, 12, ord(' '))
        treescrn4.addch(11, 8, ord(' '))

        # strng5
        treescrn4.addch(11, 18, ord(' '))
        treescrn4.addch(12, 14, ord(' '))

        # treescrn5
        treescrn.overlay(treescrn5)

        # balls
        treescrn5.addch(3, 15, ord(' '))
        treescrn5.addch(10, 20, ord(' '))
        treescrn5.addch(12, 1, ord(' '))

        # star
        treescrn5.addch(0, 12, ord(' '))

        # strng1
        treescrn5.addch(3, 11, ord(' '))

        # strng2
        treescrn5.addch(5, 12, ord(' '))

        # strng3
        treescrn5.addch(7, 14, ord(' '))
        treescrn5.addch(8, 10, ord(' '))

        # strng4
        treescrn5.addch(9, 15, ord(' '))
        treescrn5.addch(10, 11, ord(' '))
        treescrn5.addch(11, 7, ord(' '))

        # strng5
        treescrn5.addch(11, 17, ord(' '))
        treescrn5.addch(12, 13, ord(' '))

        # treescrn6
        treescrn.overlay(treescrn6)

        # balls
        treescrn6.addch(6, 7, ord(' '))
        treescrn6.addch(7, 18, ord(' '))
        treescrn6.addch(10, 4, ord(' '))
        treescrn6.addch(11, 23, ord(' '))

        # star
        treescrn6.standout()
        treescrn6.addch(0, 12, ord('*'))
        treescrn6.standend()

        # strng1

        # strng2
        treescrn6.addch(5, 11, ord(' '))

        # strng3
        treescrn6.addch(7, 13, ord(' '))
        treescrn6.addch(8, 9, ord(' '))

        # strng4
        treescrn6.addch(9, 14, ord(' '))
        treescrn6.addch(10, 10, ord(' '))
        treescrn6.addch(11, 6, ord(' '))

        # strng5
        treescrn6.addch(11, 16, ord(' '))
        treescrn6.addch(12, 12, ord(' '))

        #  treescrn7

        treescrn.overlay(treescrn7)

        # balls
        treescrn7.addch(3, 15, ord(' '))
        treescrn7.addch(6, 7, ord(' '))
        treescrn7.addch(7, 18, ord(' '))
        treescrn7.addch(10, 4, ord(' '))
        treescrn7.addch(11, 22, ord(' '))

        # star
        treescrn7.addch(0, 12, ord('*'))

        # strng1
        treescrn7.addch(3, 12, ord(' '))

        # strng2
        treescrn7.addch(5, 13, ord(' '))
        treescrn7.addch(6, 9, ord(' '))

        # strng3
        treescrn7.addch(7, 15, ord(' '))
        treescrn7.addch(8, 11, ord(' '))

        # strng4
        treescrn7.addch(9, 16, ord(' '))
        treescrn7.addch(10, 12, ord(' '))
        treescrn7.addch(11, 8, ord(' '))

        # strng5
        treescrn7.addch(11, 18, ord(' '))
        treescrn7.addch(12, 14, ord(' '))

        look_out(150)
        reindeer()

        w_holiday.touchwin()
        w_holiday.refresh()
        w_del_msg.refresh()

        look_out(500)
        for i in range(0, 20):
            blinkit()

curses.wrapper(main)
