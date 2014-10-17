#
# Test script for the curses module
#
# This script doesn't actually display anything very coherent. but it
# does call (nearly) every method and function.
#
# Functions not tested: {def,reset}_{shell,prog}_mode, getch(), getstr(),
# init_color()
# Only called, not tested: getmouse(), ungetmouse()
#

import os
import sys
import tempfile
import unittest

from test.support import requires, import_module, verbose

# Optionally test curses module.  This currently requires that the
# 'curses' resource be given on the regrtest command line using the -u
# option.  If not available, nothing after this line will be executed.
import inspect
requires('curses')

# If either of these don't exist, skip the tests.
curses = import_module('curses')
curses.panel = import_module('curses.panel')

term = os.environ.get('TERM', 'unknown')

@unittest.skipUnless(sys.__stdout__.isatty(), 'sys.__stdout__ is not a tty')
@unittest.skipIf(term == 'unknown',
                 "$TERM=%r, calling initscr() may cause exit" % term)
@unittest.skipIf(sys.platform == "cygwin",
                 "cygwin's curses mostly just hangs")
class TestCurses(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        curses.setupterm(fd=sys.__stdout__.fileno())

    def setUp(self):
        if verbose:
            # just to make the test output a little more readable
            print()
        self.stdscr = curses.initscr()
        curses.savetty()

    def tearDown(self):
        curses.resetty()
        curses.endwin()

    def test_window_funcs(self):
        "Test the methods of windows"
        stdscr = self.stdscr
        win = curses.newwin(10,10)
        win = curses.newwin(5,5, 5,5)
        win2 = curses.newwin(15,15, 5,5)

        for meth in [stdscr.addch, stdscr.addstr]:
            for args in [('a'), ('a', curses.A_BOLD),
                         (4,4, 'a'), (5,5, 'a', curses.A_BOLD)]:
                meth(*args)

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
        with self.assertRaises(TypeError,
                               msg="Expected win.border() to raise TypeError"):
            win.border(65, 66, 67, 68,
                       69, [], 71, 72)

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
        win2.overlay(win, 1, 2, 2, 1, 3, 3)
        win2.overwrite(win, 1, 2, 2, 1, 3, 3)
        stdscr.redrawln(1,2)

        stdscr.scrollok(1)
        stdscr.scroll()
        stdscr.scroll(2)
        stdscr.scroll(-3)

        stdscr.move(12, 2)
        stdscr.setscrreg(10,15)
        win3 = stdscr.subwin(10,10)
        win3 = stdscr.subwin(10,10, 5,5)
        stdscr.syncok(1)
        stdscr.timeout(5)
        stdscr.touchline(5,5)
        stdscr.touchline(5,5,0)
        stdscr.vline('a', 3)
        stdscr.vline('a', 3, curses.A_STANDOUT)
        stdscr.chgat(5, 2, 3, curses.A_BLINK)
        stdscr.chgat(3, curses.A_BOLD)
        stdscr.chgat(5, 8, curses.A_UNDERLINE)
        stdscr.chgat(curses.A_BLINK)
        stdscr.refresh()

        stdscr.vline(1,1, 'a', 3)
        stdscr.vline(1,1, 'a', 3, curses.A_STANDOUT)

        if hasattr(curses, 'resize'):
            stdscr.resize()
        if hasattr(curses, 'enclose'):
            stdscr.enclose()


    def test_module_funcs(self):
        "Test module-level functions"
        stdscr = self.stdscr
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
        if curses.tigetstr("cnorm"):
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
        curses.putp(b'abc')
        curses.qiflush()
        curses.raw() ; curses.raw(1)
        curses.setsyx(5,5)
        curses.tigetflag('hc')
        curses.tigetnum('co')
        curses.tigetstr('cr')
        curses.tparm(b'cr')
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
            curses.pair_content(curses.COLOR_PAIRS - 1)
            curses.pair_number(0)

            if hasattr(curses, 'use_default_colors'):
                curses.use_default_colors()

        if hasattr(curses, 'keyname'):
            curses.keyname(13)

        if hasattr(curses, 'has_key'):
            curses.has_key(13)

        if hasattr(curses, 'getmouse'):
            (availmask, oldmask) = curses.mousemask(curses.BUTTON1_PRESSED)
            # availmask indicates that mouse stuff not available.
            if availmask != 0:
                curses.mouseinterval(10)
                # just verify these don't cause errors
                curses.ungetmouse(0, 0, 0, 0, curses.BUTTON1_PRESSED)
                m = curses.getmouse()

        if hasattr(curses, 'is_term_resized'):
            curses.is_term_resized(*stdscr.getmaxyx())
        if hasattr(curses, 'resizeterm'):
            curses.resizeterm(*stdscr.getmaxyx())
        if hasattr(curses, 'resize_term'):
            curses.resize_term(*stdscr.getmaxyx())

    def test_unctrl(self):
        from curses import ascii
        for ch, expected in [('a', 'a'), ('A', 'A'),
                             (';', ';'), (' ', ' '),
                             ('\x7f', '^?'), ('\n', '^J'), ('\0', '^@'),
                             # Meta-bit characters
                             ('\x8a', '!^J'), ('\xc1', '!A'),
                             ]:
            self.assertEqual(ascii.unctrl(ch), expected,
                             'curses.unctrl fails on character %r' % ch)


    def test_userptr_without_set(self):
        w = curses.newwin(10, 10)
        p = curses.panel.new_panel(w)
        # try to access userptr() before calling set_userptr() -- segfaults
        with self.assertRaises(curses.panel.error,
                               msg='userptr should fail since not set'):
            p.userptr()

    def test_userptr_memory_leak(self):
        w = curses.newwin(10, 10)
        p = curses.panel.new_panel(w)
        obj = object()
        nrefs = sys.getrefcount(obj)
        for i in range(100):
            p.set_userptr(obj)

        p.set_userptr(None)
        self.assertEqual(sys.getrefcount(obj), nrefs,
                         "set_userptr leaked references")

    def test_userptr_segfault(self):
        panel = curses.panel.new_panel(self.stdscr)
        class A:
            def __del__(self):
                panel.set_userptr(None)
        panel.set_userptr(A())
        panel.set_userptr(None)

    @unittest.skipUnless(hasattr(curses, 'resizeterm'),
                           'resizeterm not available')
    def test_resize_term(self):
        lines, cols = curses.LINES, curses.COLS
        new_lines = lines - 1
        new_cols = cols + 1
        curses.resizeterm(new_lines, new_cols)

        self.assertEqual(curses.LINES, new_lines)
        self.assertEqual(curses.COLS, new_cols)

    def test_issue6243(self):
        curses.ungetch(1025)
        self.stdscr.getkey()

    @unittest.skipUnless(hasattr(curses, 'unget_wch'),
                         'unget_wch not available')
    def test_unget_wch(self):
        stdscr = self.stdscr
        encoding = stdscr.encoding
        for ch in ('a', '\xe9', '\u20ac', '\U0010FFFF'):
            try:
                ch.encode(encoding)
            except UnicodeEncodeError:
                continue
            try:
                curses.unget_wch(ch)
            except Exception as err:
                self.fail("unget_wch(%a) failed with encoding %s: %s"
                          % (ch, stdscr.encoding, err))
            read = stdscr.get_wch()
            self.assertEqual(read, ch)

            code = ord(ch)
            curses.unget_wch(code)
            read = stdscr.get_wch()
            self.assertEqual(read, ch)

    def test_issue10570(self):
        b = curses.tparm(curses.tigetstr("cup"), 5, 3)
        self.assertIs(type(b), bytes)
        curses.putp(b)

    def test_encoding(self):
        stdscr = self.stdscr
        import codecs
        encoding = stdscr.encoding
        codecs.lookup(encoding)

        with self.assertRaises(TypeError):
            stdscr.encoding = 10

        stdscr.encoding = encoding
        with self.assertRaises(TypeError):
            del stdscr.encoding

    def test_issue21088(self):
        stdscr = self.stdscr
        #
        # http://bugs.python.org/issue21088
        #
        # the bug:
        # when converting curses.window.addch to Argument Clinic
        # the first two parameters were switched.

        # if someday we can represent the signature of addch
        # we will need to rewrite this test.
        try:
            signature = inspect.signature(stdscr.addch)
            self.assertFalse(signature)
        except ValueError:
            # not generating a signature is fine.
            pass

        # So.  No signature for addch.
        # But Argument Clinic gave us a human-readable equivalent
        # as the first line of the docstring.  So we parse that,
        # and ensure that the parameters appear in the correct order.
        # Since this is parsing output from Argument Clinic, we can
        # be reasonably certain the generated parsing code will be
        # correct too.
        human_readable_signature = stdscr.addch.__doc__.split("\n")[0]
        offset = human_readable_signature.find("[y, x,]")
        assert offset >= 0, ""


if __name__ == '__main__':
    unittest.main()
