#
# Test script for the curses module
#
# This script doesn't actually display anything very coherent. but it
# does call every method and function.
#
# Functions not tested: {def,reset}_{shell,prog}_mode, getch(), getstr(),
# getmouse(), ungetmouse(), init_color()
#

import curses, sys, tempfile

# Optionally test curses module.  This currently requires that the
# 'curses' resource be given on the regrtest command line using the -u
# option.  If not available, nothing after this line will be executed.

import test_support
test_support.requires('curses')

def window_funcs(stdscr):
    "Test the methods of windows"
    win = curses.newwin(10,10)
    win = curses.newwin(5,5, 5,5)
    win2 = curses.newwin(15,15, 5,5)

    for meth in [stdscr.addch, stdscr.addstr]:
        for args in [('a'), ('a', curses.A_BOLD),
                     (4,4, 'a'), (5,5, 'a', curses.A_BOLD)]:
            apply(meth, args)

    for meth in [stdscr.box, stdscr.clear, stdscr.clrtobot,
                 stdscr.clrtoeol, stdscr.cursyncup, stdscr.delch,
                 stdscr.deleteln, stdscr.erase, stdscr.getbegyx,
                 stdscr.getbkgd, stdscr.getkey, stdscr.getmaxyx,
                 stdscr.getparyx, stdscr.getyx, stdscr.inch,
                 stdscr.insertln, stdscr.instr, stdscr.is_wintouched,
                 win.noutrefresh, stdscr.redrawwin, stdscr.refresh,
                 stdscr.standout, stdscr.standend, stdscr.syncdown,
                 stdscr.syncup, stdscr.touchwin, stdscr.untouchwin]:
        meth()

    stdscr.addnstr('1234', 3)
    stdscr.addnstr('1234', 3, curses.A_BOLD)
    stdscr.addnstr(4,4, '1234', 3)
    stdscr.addnstr(5,5, '1234', 3, curses.A_BOLD)

    stdscr.attron(curses.A_BOLD)
    stdscr.attroff(curses.A_BOLD)
    stdscr.attrset(curses.A_BOLD)
    stdscr.bkgd(' ')
    stdscr.bkgd(' ', curses.A_REVERSE)
    stdscr.bkgdset(' ')
    stdscr.bkgdset(' ', curses.A_REVERSE)

    win.border(65, 66, 67, 68,
               69, 70, 71, 72)
    win.border('|', '!', '-', '_',
               '+', '\\', '#', '/')
    try:
        win.border(65, 66, 67, 68,
                   69, [], 71, 72)
    except TypeError:
        pass
    else:
        raise RuntimeError, "Expected win.border() to raise TypeError"

    stdscr.clearok(1)

    win4 = stdscr.derwin(2,2)
    win4 = stdscr.derwin(1,1, 5,5)
    win4.mvderwin(9,9)

    stdscr.echochar('a')
    stdscr.echochar('a', curses.A_BOLD)
    stdscr.hline('-', 5)
    stdscr.hline('-', 5, curses.A_BOLD)
    stdscr.hline(1,1,'-', 5)
    stdscr.hline(1,1,'-', 5, curses.A_BOLD)

    stdscr.idcok(1)
    stdscr.idlok(1)
    stdscr.immedok(1)
    stdscr.insch('c')
    stdscr.insdelln(1)
    stdscr.insnstr('abc', 3)
    stdscr.insnstr('abc', 3, curses.A_BOLD)
    stdscr.insnstr(5, 5, 'abc', 3)
    stdscr.insnstr(5, 5, 'abc', 3, curses.A_BOLD)

    stdscr.insstr('def')
    stdscr.insstr('def', curses.A_BOLD)
    stdscr.insstr(5, 5, 'def')
    stdscr.insstr(5, 5, 'def', curses.A_BOLD)
    stdscr.is_linetouched(0)
    stdscr.keypad(1)
    stdscr.leaveok(1)
    stdscr.move(3,3)
    win.mvwin(2,2)
    stdscr.nodelay(1)
    stdscr.notimeout(1)
    win2.overlay(win)
    win2.overwrite(win)
    stdscr.redrawln(1,2)

    stdscr.scrollok(1)
    stdscr.scroll()
    stdscr.scroll(2)
    stdscr.scroll(-3)

    stdscr.setscrreg(10,15)
    win3 = stdscr.subwin(10,10)
    win3 = stdscr.subwin(10,10, 5,5)
    stdscr.syncok(1)
    stdscr.timeout(5)
    stdscr.touchline(5,5)
    stdscr.touchline(5,5,0)
    stdscr.vline('a', 3)
    stdscr.vline('a', 3, curses.A_STANDOUT)
    stdscr.vline(1,1, 'a', 3)
    stdscr.vline(1,1, 'a', 3, curses.A_STANDOUT)

    if hasattr(curses, 'resize'):
        stdscr.resize()
    if hasattr(curses, 'enclose'):
        stdscr.enclose()


def module_funcs(stdscr):
    "Test module-level functions"

    for func in [curses.baudrate, curses.beep, curses.can_change_color,
                 curses.cbreak, curses.def_prog_mode, curses.doupdate,
                 curses.filter, curses.flash, curses.flushinp,
                 curses.has_colors, curses.has_ic, curses.has_il,
                 curses.isendwin, curses.killchar, curses.longname,
                 curses.nocbreak, curses.noecho, curses.nonl,
                 curses.noqiflush, curses.noraw,
                 curses.reset_prog_mode, curses.termattrs,
                 curses.termname, curses.erasechar, curses.getsyx]:
        func()

    # Functions that actually need arguments
    curses.curs_set(1)
    curses.delay_output(1)
    curses.echo() ; curses.echo(1)

    f = tempfile.TemporaryFile()
    stdscr.putwin(f)
    f.seek(0)
    curses.getwin(f)
    f.close()

    curses.halfdelay(1)
    curses.intrflush(1)
    curses.meta(1)
    curses.napms(100)
    curses.newpad(50,50)
    win = curses.newwin(5,5)
    win = curses.newwin(5,5, 1,1)
    curses.nl() ; curses.nl(1)
    curses.putp('abc')
    curses.qiflush()
    curses.raw() ; curses.raw(1)
    curses.setsyx(5,5)
    curses.setupterm(fd=sys.__stdout__.fileno())
    curses.tigetflag('hc')
    curses.tigetnum('co')
    curses.tigetstr('cr')
    curses.tparm('cr')
    curses.typeahead(sys.__stdin__.fileno())
    curses.unctrl('a')
    curses.ungetch('a')
    curses.use_env(1)

    # Functions only available on a few platforms
    if curses.has_colors():
        curses.start_color()
        curses.init_pair(2, 1,1)
        curses.color_content(1)
        curses.color_pair(2)
        curses.pair_content(curses.COLOR_PAIRS)
        curses.pair_number(0)

    if hasattr(curses, 'keyname'):
        curses.keyname(13)

    if hasattr(curses, 'has_key'):
        curses.has_key(13)

    if hasattr(curses, 'getmouse'):
        curses.mousemask(curses.BUTTON1_PRESSED)
        curses.mouseinterval(10)


def main(stdscr):
    curses.savetty()
    try:
        module_funcs(stdscr)
        window_funcs(stdscr)
    finally:
        curses.resetty()

if __name__ == '__main__':
    curses.wrapper(main)
else:
    try:
        stdscr = curses.initscr()
        main(stdscr)
    finally:
        curses.endwin()
