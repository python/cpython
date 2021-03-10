import functools
import inspect
import os
import string
import sys
import tempfile
import unittest

from test.support import requires, verbose, SaveSignals
from test.support.import_helper import import_module

# Optionally test curses module.  This currently requires that the
# 'curses' resource be given on the regrtest command line using the -u
# option.  If not available, nothing after this line will be executed.
requires('curses')

# If either of these don't exist, skip the tests.
curses = import_module('curses')
import_module('curses.ascii')
import_module('curses.textpad')
try:
    import curses.panel
except ImportError:
    pass

def requires_curses_func(name):
    return unittest.skipUnless(hasattr(curses, name),
                               'requires curses.%s' % name)

def requires_curses_window_meth(name):
    def deco(test):
        @functools.wraps(test)
        def wrapped(self, *args, **kwargs):
            if not hasattr(self.stdscr, name):
                raise unittest.SkipTest('requires curses.window.%s' % name)
            test(self, *args, **kwargs)
        return wrapped
    return deco


def requires_colors(test):
    @functools.wraps(test)
    def wrapped(self, *args, **kwargs):
        if not curses.has_colors():
            self.skipTest('requires colors support')
        curses.start_color()
        test(self, *args, **kwargs)
    return wrapped

term = os.environ.get('TERM')
SHORT_MAX = 0x7fff

# If newterm was supported we could use it instead of initscr and not exit
@unittest.skipIf(not term or term == 'unknown',
                 "$TERM=%r, calling initscr() may cause exit" % term)
@unittest.skipIf(sys.platform == "cygwin",
                 "cygwin's curses mostly just hangs")
class TestCurses(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if verbose:
            print(f'TERM={term}', file=sys.stderr, flush=True)
        # testing setupterm() inside initscr/endwin
        # causes terminal breakage
        stdout_fd = sys.__stdout__.fileno()
        curses.setupterm(fd=stdout_fd)

    def setUp(self):
        self.isatty = True
        self.output = sys.__stdout__
        stdout_fd = sys.__stdout__.fileno()
        if not sys.__stdout__.isatty():
            # initstr() unconditionally uses C stdout.
            # If it is redirected to file or pipe, try to attach it
            # to terminal.
            # First, save a copy of the file descriptor of stdout, so it
            # can be restored after finishing the test.
            dup_fd = os.dup(stdout_fd)
            self.addCleanup(os.close, dup_fd)
            self.addCleanup(os.dup2, dup_fd, stdout_fd)

            if sys.__stderr__.isatty():
                # If stderr is connected to terminal, use it.
                tmp = sys.__stderr__
                self.output = sys.__stderr__
            else:
                try:
                    # Try to open the terminal device.
                    tmp = open('/dev/tty', 'wb', buffering=0)
                except OSError:
                    # As a fallback, use regular file to write control codes.
                    # Some functions (like savetty) will not work, but at
                    # least the garbage control sequences will not be mixed
                    # with the testing report.
                    tmp = tempfile.TemporaryFile(mode='wb', buffering=0)
                    self.isatty = False
                self.addCleanup(tmp.close)
                self.output = None
            os.dup2(tmp.fileno(), stdout_fd)

        self.save_signals = SaveSignals()
        self.save_signals.save()
        self.addCleanup(self.save_signals.restore)
        if verbose and self.output is not None:
            # just to make the test output a little more readable
            sys.stderr.flush()
            sys.stdout.flush()
            print(file=self.output, flush=True)
        self.stdscr = curses.initscr()
        if self.isatty:
            curses.savetty()
            self.addCleanup(curses.endwin)
            self.addCleanup(curses.resetty)
        self.stdscr.erase()

    @requires_curses_func('filter')
    def test_filter(self):
        # TODO: Should be called before initscr() or newterm() are called.
        # TODO: nofilter()
        curses.filter()

    @requires_curses_func('use_env')
    def test_use_env(self):
        # TODO: Should be called before initscr() or newterm() are called.
        # TODO: use_tioctl()
        curses.use_env(False)
        curses.use_env(True)

    def test_create_windows(self):
        win = curses.newwin(5, 10)
        self.assertEqual(win.getbegyx(), (0, 0))
        self.assertEqual(win.getparyx(), (-1, -1))
        self.assertEqual(win.getmaxyx(), (5, 10))

        win = curses.newwin(10, 15, 2, 5)
        self.assertEqual(win.getbegyx(), (2, 5))
        self.assertEqual(win.getparyx(), (-1, -1))
        self.assertEqual(win.getmaxyx(), (10, 15))

        win2 = win.subwin(3, 7)
        self.assertEqual(win2.getbegyx(), (3, 7))
        self.assertEqual(win2.getparyx(), (1, 2))
        self.assertEqual(win2.getmaxyx(), (9, 13))

        win2 = win.subwin(5, 10, 3, 7)
        self.assertEqual(win2.getbegyx(), (3, 7))
        self.assertEqual(win2.getparyx(), (1, 2))
        self.assertEqual(win2.getmaxyx(), (5, 10))

        win3 = win.derwin(2, 3)
        self.assertEqual(win3.getbegyx(), (4, 8))
        self.assertEqual(win3.getparyx(), (2, 3))
        self.assertEqual(win3.getmaxyx(), (8, 12))

        win3 = win.derwin(6, 11, 2, 3)
        self.assertEqual(win3.getbegyx(), (4, 8))
        self.assertEqual(win3.getparyx(), (2, 3))
        self.assertEqual(win3.getmaxyx(), (6, 11))

        win.mvwin(0, 1)
        self.assertEqual(win.getbegyx(), (0, 1))
        self.assertEqual(win.getparyx(), (-1, -1))
        self.assertEqual(win.getmaxyx(), (10, 15))
        self.assertEqual(win2.getbegyx(), (3, 7))
        self.assertEqual(win2.getparyx(), (1, 2))
        self.assertEqual(win2.getmaxyx(), (5, 10))
        self.assertEqual(win3.getbegyx(), (4, 8))
        self.assertEqual(win3.getparyx(), (2, 3))
        self.assertEqual(win3.getmaxyx(), (6, 11))

        win2.mvderwin(2, 1)
        self.assertEqual(win2.getbegyx(), (3, 7))
        self.assertEqual(win2.getparyx(), (2, 1))
        self.assertEqual(win2.getmaxyx(), (5, 10))

        win3.mvderwin(2, 1)
        self.assertEqual(win3.getbegyx(), (4, 8))
        self.assertEqual(win3.getparyx(), (2, 1))
        self.assertEqual(win3.getmaxyx(), (6, 11))

    def test_move_cursor(self):
        stdscr = self.stdscr
        win = stdscr.subwin(10, 15, 2, 5)
        stdscr.move(1, 2)
        win.move(2, 4)
        self.assertEqual(stdscr.getyx(), (1, 2))
        self.assertEqual(win.getyx(), (2, 4))

        win.cursyncup()
        self.assertEqual(stdscr.getyx(), (4, 9))

    def test_refresh_control(self):
        stdscr = self.stdscr
        # touchwin()/untouchwin()/is_wintouched()
        stdscr.refresh()
        self.assertIs(stdscr.is_wintouched(), False)
        stdscr.touchwin()
        self.assertIs(stdscr.is_wintouched(), True)
        stdscr.refresh()
        self.assertIs(stdscr.is_wintouched(), False)
        stdscr.touchwin()
        self.assertIs(stdscr.is_wintouched(), True)
        stdscr.untouchwin()
        self.assertIs(stdscr.is_wintouched(), False)

        # touchline()/untouchline()/is_linetouched()
        stdscr.touchline(5, 2)
        self.assertIs(stdscr.is_linetouched(5), True)
        self.assertIs(stdscr.is_linetouched(6), True)
        self.assertIs(stdscr.is_wintouched(), True)
        stdscr.touchline(5, 1, False)
        self.assertIs(stdscr.is_linetouched(5), False)

        # syncup()
        win = stdscr.subwin(10, 15, 2, 5)
        win2 = win.subwin(5, 10, 3, 7)
        win2.touchwin()
        stdscr.untouchwin()
        win2.syncup()
        self.assertIs(win.is_wintouched(), True)
        self.assertIs(stdscr.is_wintouched(), True)

        # syncdown()
        stdscr.touchwin()
        win.untouchwin()
        win2.untouchwin()
        win2.syncdown()
        self.assertIs(win2.is_wintouched(), True)

        # syncok()
        if hasattr(stdscr, 'syncok') and not sys.platform.startswith("sunos"):
            win.untouchwin()
            stdscr.untouchwin()
            for syncok in [False, True]:
                win2.syncok(syncok)
                win2.addch('a')
                self.assertIs(win.is_wintouched(), syncok)
                self.assertIs(stdscr.is_wintouched(), syncok)

    def test_output_character(self):
        stdscr = self.stdscr
        encoding = stdscr.encoding
        # addch()
        stdscr.refresh()
        stdscr.move(0, 0)
        stdscr.addch('A')
        stdscr.addch(b'A')
        stdscr.addch(65)
        c = '\u20ac'
        try:
            stdscr.addch(c)
        except UnicodeEncodeError:
            self.assertRaises(UnicodeEncodeError, c.encode, encoding)
        except OverflowError:
            encoded = c.encode(encoding)
            self.assertNotEqual(len(encoded), 1, repr(encoded))
        stdscr.addch('A', curses.A_BOLD)
        stdscr.addch(1, 2, 'A')
        stdscr.addch(2, 3, 'A', curses.A_BOLD)
        self.assertIs(stdscr.is_wintouched(), True)

        # echochar()
        stdscr.refresh()
        stdscr.move(0, 0)
        stdscr.echochar('A')
        stdscr.echochar(b'A')
        stdscr.echochar(65)
        with self.assertRaises((UnicodeEncodeError, OverflowError)):
            stdscr.echochar('\u20ac')
        stdscr.echochar('A', curses.A_BOLD)
        self.assertIs(stdscr.is_wintouched(), False)

    def test_output_string(self):
        stdscr = self.stdscr
        encoding = stdscr.encoding
        # addstr()/insstr()
        for func in [stdscr.addstr, stdscr.insstr]:
            with self.subTest(func.__qualname__):
                stdscr.move(0, 0)
                func('abcd')
                func(b'abcd')
                s = 'àßçđ'
                try:
                    func(s)
                except UnicodeEncodeError:
                    self.assertRaises(UnicodeEncodeError, s.encode, encoding)
                func('abcd', curses.A_BOLD)
                func(1, 2, 'abcd')
                func(2, 3, 'abcd', curses.A_BOLD)

        # addnstr()/insnstr()
        for func in [stdscr.addnstr, stdscr.insnstr]:
            with self.subTest(func.__qualname__):
                stdscr.move(0, 0)
                func('1234', 3)
                func(b'1234', 3)
                s = '\u0661\u0662\u0663\u0664'
                try:
                    func(s, 3)
                except UnicodeEncodeError:
                    self.assertRaises(UnicodeEncodeError, s.encode, encoding)
                func('1234', 5)
                func('1234', 3, curses.A_BOLD)
                func(1, 2, '1234', 3)
                func(2, 3, '1234', 3, curses.A_BOLD)

    def test_output_string_embedded_null_chars(self):
        # reject embedded null bytes and characters
        stdscr = self.stdscr
        for arg in ['a\0', b'a\0']:
            with self.subTest(arg=arg):
                self.assertRaises(ValueError, stdscr.addstr, arg)
                self.assertRaises(ValueError, stdscr.addnstr, arg, 1)
                self.assertRaises(ValueError, stdscr.insstr, arg)
                self.assertRaises(ValueError, stdscr.insnstr, arg, 1)

    def test_read_from_window(self):
        stdscr = self.stdscr
        stdscr.addstr(0, 1, 'ABCD', curses.A_BOLD)
        # inch()
        stdscr.move(0, 1)
        self.assertEqual(stdscr.inch(), 65 | curses.A_BOLD)
        self.assertEqual(stdscr.inch(0, 3), 67 | curses.A_BOLD)
        stdscr.move(0, 0)
        # instr()
        self.assertEqual(stdscr.instr()[:6], b' ABCD ')
        self.assertEqual(stdscr.instr(3)[:6], b' AB')
        self.assertEqual(stdscr.instr(0, 2)[:4], b'BCD ')
        self.assertEqual(stdscr.instr(0, 2, 4), b'BCD ')
        self.assertRaises(ValueError, stdscr.instr, -2)
        self.assertRaises(ValueError, stdscr.instr, 0, 2, -2)

    def test_getch(self):
        win = curses.newwin(5, 12, 5, 2)

        # TODO: Test with real input by writing to master fd.
        for c in 'spam\n'[::-1]:
            curses.ungetch(c)
        self.assertEqual(win.getch(3, 1), b's'[0])
        self.assertEqual(win.getyx(), (3, 1))
        self.assertEqual(win.getch(3, 4), b'p'[0])
        self.assertEqual(win.getyx(), (3, 4))
        self.assertEqual(win.getch(), b'a'[0])
        self.assertEqual(win.getyx(), (3, 4))
        self.assertEqual(win.getch(), b'm'[0])
        self.assertEqual(win.getch(), b'\n'[0])

    def test_getstr(self):
        win = curses.newwin(5, 12, 5, 2)
        curses.echo()
        self.addCleanup(curses.noecho)

        self.assertRaises(ValueError, win.getstr, -400)
        self.assertRaises(ValueError, win.getstr, 2, 3, -400)

        # TODO: Test with real input by writing to master fd.
        for c in 'Lorem\nipsum\ndolor\nsit\namet\n'[::-1]:
            curses.ungetch(c)
        self.assertEqual(win.getstr(3, 1, 2), b'Lo')
        self.assertEqual(win.instr(3, 0), b' Lo         ')
        self.assertEqual(win.getstr(3, 5, 10), b'ipsum')
        self.assertEqual(win.instr(3, 0), b' Lo  ipsum  ')
        self.assertEqual(win.getstr(1, 5), b'dolor')
        self.assertEqual(win.instr(1, 0), b'     dolor  ')
        self.assertEqual(win.getstr(2), b'si')
        self.assertEqual(win.instr(1, 0), b'si   dolor  ')
        self.assertEqual(win.getstr(), b'amet')
        self.assertEqual(win.instr(1, 0), b'amet dolor  ')

    def test_clear(self):
        win = curses.newwin(5, 15, 5, 2)
        lorem_ipsum(win)

        win.move(0, 8)
        win.clrtoeol()
        self.assertEqual(win.instr(0, 0).rstrip(), b'Lorem ip')
        self.assertEqual(win.instr(1, 0).rstrip(), b'dolor sit amet,')

        win.move(0, 3)
        win.clrtobot()
        self.assertEqual(win.instr(0, 0).rstrip(), b'Lor')
        self.assertEqual(win.instr(1, 0).rstrip(), b'')

        for func in [win.erase, win.clear]:
            lorem_ipsum(win)
            func()
            self.assertEqual(win.instr(0, 0).rstrip(), b'')
            self.assertEqual(win.instr(1, 0).rstrip(), b'')

    def test_insert_delete(self):
        win = curses.newwin(5, 15, 5, 2)
        lorem_ipsum(win)

        win.move(0, 2)
        win.delch()
        self.assertEqual(win.instr(0, 0), b'Loem ipsum     ')
        win.delch(0, 7)
        self.assertEqual(win.instr(0, 0), b'Loem ipum      ')

        win.move(1, 5)
        win.deleteln()
        self.assertEqual(win.instr(0, 0), b'Loem ipum      ')
        self.assertEqual(win.instr(1, 0), b'consectetur    ')
        self.assertEqual(win.instr(2, 0), b'adipiscing elit')
        self.assertEqual(win.instr(3, 0), b'sed do eiusmod ')
        self.assertEqual(win.instr(4, 0), b'               ')

        win.move(1, 5)
        win.insertln()
        self.assertEqual(win.instr(0, 0), b'Loem ipum      ')
        self.assertEqual(win.instr(1, 0), b'               ')
        self.assertEqual(win.instr(2, 0), b'consectetur    ')

        win.clear()
        lorem_ipsum(win)
        win.move(1, 5)
        win.insdelln(2)
        self.assertEqual(win.instr(0, 0), b'Lorem ipsum    ')
        self.assertEqual(win.instr(1, 0), b'               ')
        self.assertEqual(win.instr(2, 0), b'               ')
        self.assertEqual(win.instr(3, 0), b'dolor sit amet,')

        win.clear()
        lorem_ipsum(win)
        win.move(1, 5)
        win.insdelln(-2)
        self.assertEqual(win.instr(0, 0), b'Lorem ipsum    ')
        self.assertEqual(win.instr(1, 0), b'adipiscing elit')
        self.assertEqual(win.instr(2, 0), b'sed do eiusmod ')
        self.assertEqual(win.instr(3, 0), b'               ')

    def test_scroll(self):
        win = curses.newwin(5, 15, 5, 2)
        lorem_ipsum(win)
        win.scrollok(True)
        win.scroll()
        self.assertEqual(win.instr(0, 0), b'dolor sit amet,')
        win.scroll(2)
        self.assertEqual(win.instr(0, 0), b'adipiscing elit')
        win.scroll(-3)
        self.assertEqual(win.instr(0, 0), b'               ')
        self.assertEqual(win.instr(2, 0), b'               ')
        self.assertEqual(win.instr(3, 0), b'adipiscing elit')
        win.scrollok(False)

    def test_attributes(self):
        # TODO: attr_get(), attr_set(), ...
        win = curses.newwin(5, 15, 5, 2)
        win.attron(curses.A_BOLD)
        win.attroff(curses.A_BOLD)
        win.attrset(curses.A_BOLD)

        win.standout()
        win.standend()

    @requires_curses_window_meth('chgat')
    def test_chgat(self):
        win = curses.newwin(5, 15, 5, 2)
        win.addstr(2, 0, 'Lorem ipsum')
        win.addstr(3, 0, 'dolor sit amet')

        win.move(2, 8)
        win.chgat(curses.A_BLINK)
        self.assertEqual(win.inch(2, 7), b'p'[0])
        self.assertEqual(win.inch(2, 8), b's'[0] | curses.A_BLINK)
        self.assertEqual(win.inch(2, 14), b' '[0] | curses.A_BLINK)

        win.move(2, 1)
        win.chgat(3, curses.A_BOLD)
        self.assertEqual(win.inch(2, 0), b'L'[0])
        self.assertEqual(win.inch(2, 1), b'o'[0] | curses.A_BOLD)
        self.assertEqual(win.inch(2, 3), b'e'[0] | curses.A_BOLD)
        self.assertEqual(win.inch(2, 4), b'm'[0])

        win.chgat(3, 2, curses.A_UNDERLINE)
        self.assertEqual(win.inch(3, 1), b'o'[0])
        self.assertEqual(win.inch(3, 2), b'l'[0] | curses.A_UNDERLINE)
        self.assertEqual(win.inch(3, 14), b' '[0] | curses.A_UNDERLINE)

        win.chgat(3, 4, 7, curses.A_BLINK)
        self.assertEqual(win.inch(3, 3), b'o'[0] | curses.A_UNDERLINE)
        self.assertEqual(win.inch(3, 4), b'r'[0] | curses.A_BLINK)
        self.assertEqual(win.inch(3, 10), b'a'[0] | curses.A_BLINK)
        self.assertEqual(win.inch(3, 11), b'm'[0] | curses.A_UNDERLINE)
        self.assertEqual(win.inch(3, 14), b' '[0] | curses.A_UNDERLINE)

    def test_background(self):
        win = curses.newwin(5, 15, 5, 2)
        win.addstr(0, 0, 'Lorem ipsum')

        self.assertIn(win.getbkgd(), (0, 32))

        # bkgdset()
        win.bkgdset('_')
        self.assertEqual(win.getbkgd(), b'_'[0])
        win.bkgdset(b'#')
        self.assertEqual(win.getbkgd(), b'#'[0])
        win.bkgdset(65)
        self.assertEqual(win.getbkgd(), 65)
        win.bkgdset(0)
        self.assertEqual(win.getbkgd(), 32)

        win.bkgdset('#', curses.A_REVERSE)
        self.assertEqual(win.getbkgd(), b'#'[0] | curses.A_REVERSE)
        self.assertEqual(win.inch(0, 0), b'L'[0])
        self.assertEqual(win.inch(0, 5), b' '[0])
        win.bkgdset(0)

        # bkgd()
        win.bkgd('_')
        self.assertEqual(win.getbkgd(), b'_'[0])
        self.assertEqual(win.inch(0, 0), b'L'[0])
        self.assertEqual(win.inch(0, 5), b'_'[0])

        win.bkgd('#', curses.A_REVERSE)
        self.assertEqual(win.getbkgd(), b'#'[0] | curses.A_REVERSE)
        self.assertEqual(win.inch(0, 0), b'L'[0] | curses.A_REVERSE)
        self.assertEqual(win.inch(0, 5), b'#'[0] | curses.A_REVERSE)

    def test_overlay(self):
        srcwin = curses.newwin(5, 18, 3, 4)
        lorem_ipsum(srcwin)
        dstwin = curses.newwin(7, 17, 5, 7)
        for i in range(6):
            dstwin.addstr(i, 0, '_'*17)

        srcwin.overlay(dstwin)
        self.assertEqual(dstwin.instr(0, 0), b'sectetur_________')
        self.assertEqual(dstwin.instr(1, 0), b'piscing_elit,____')
        self.assertEqual(dstwin.instr(2, 0), b'_do_eiusmod______')
        self.assertEqual(dstwin.instr(3, 0), b'_________________')

        srcwin.overwrite(dstwin)
        self.assertEqual(dstwin.instr(0, 0), b'sectetur       __')
        self.assertEqual(dstwin.instr(1, 0), b'piscing elit,  __')
        self.assertEqual(dstwin.instr(2, 0), b' do eiusmod    __')
        self.assertEqual(dstwin.instr(3, 0), b'_________________')

        srcwin.overlay(dstwin, 1, 4, 3, 2, 4, 11)
        self.assertEqual(dstwin.instr(3, 0), b'__r_sit_amet_____')
        self.assertEqual(dstwin.instr(4, 0), b'__ectetur________')
        self.assertEqual(dstwin.instr(5, 0), b'_________________')

        srcwin.overwrite(dstwin, 1, 4, 3, 2, 4, 11)
        self.assertEqual(dstwin.instr(3, 0), b'__r sit amet_____')
        self.assertEqual(dstwin.instr(4, 0), b'__ectetur   _____')
        self.assertEqual(dstwin.instr(5, 0), b'_________________')

    def test_refresh(self):
        win = curses.newwin(5, 15, 2, 5)
        win.noutrefresh()
        win.redrawln(1, 2)
        win.redrawwin()
        win.refresh()
        curses.doupdate()

    @requires_curses_window_meth('resize')
    def test_resize(self):
        win = curses.newwin(5, 15, 2, 5)
        win.resize(4, 20)
        self.assertEqual(win.getmaxyx(), (4, 20))
        win.resize(5, 15)
        self.assertEqual(win.getmaxyx(), (5, 15))

    @requires_curses_window_meth('enclose')
    def test_enclose(self):
        win = curses.newwin(5, 15, 2, 5)
        # TODO: Return bool instead of 1/0
        self.assertTrue(win.enclose(2, 5))
        self.assertFalse(win.enclose(1, 5))
        self.assertFalse(win.enclose(2, 4))
        self.assertTrue(win.enclose(6, 19))
        self.assertFalse(win.enclose(7, 19))
        self.assertFalse(win.enclose(6, 20))

    def test_putwin(self):
        win = curses.newwin(5, 12, 1, 2)
        win.addstr(2, 1, 'Lorem ipsum')
        with tempfile.TemporaryFile() as f:
            win.putwin(f)
            del win
            f.seek(0)
            win = curses.getwin(f)
            self.assertEqual(win.getbegyx(), (1, 2))
            self.assertEqual(win.getmaxyx(), (5, 12))
            self.assertEqual(win.instr(2, 0), b' Lorem ipsum')

    def test_borders_and_lines(self):
        win = curses.newwin(5, 10, 5, 2)
        win.border('|', '!', '-', '_',
                   '+', '\\', '#', '/')
        self.assertEqual(win.instr(0, 0), b'+--------\\')
        self.assertEqual(win.instr(1, 0), b'|        !')
        self.assertEqual(win.instr(4, 0), b'#________/')
        win.border(b'|', b'!', b'-', b'_',
                   b'+', b'\\', b'#', b'/')
        win.border(65, 66, 67, 68,
                   69, 70, 71, 72)
        self.assertRaises(TypeError, win.border,
                          65, 66, 67, 68, 69, [], 71, 72)
        self.assertRaises(TypeError, win.border,
                          65, 66, 67, 68, 69, 70, 71, 72, 73)
        self.assertRaises(TypeError, win.border,
                          65, 66, 67, 68, 69, 70, 71, 72, 73)
        win.border(65, 66, 67, 68, 69, 70, 71)
        win.border(65, 66, 67, 68, 69, 70)
        win.border(65, 66, 67, 68, 69)
        win.border(65, 66, 67, 68)
        win.border(65, 66, 67)
        win.border(65, 66)
        win.border(65)
        win.border()

        win.box(':', '~')
        self.assertEqual(win.instr(0, 1, 8), b'~~~~~~~~')
        self.assertEqual(win.instr(1, 0),   b':        :')
        self.assertEqual(win.instr(4, 1, 8), b'~~~~~~~~')
        win.box(b':', b'~')
        win.box(65, 67)
        self.assertRaises(TypeError, win.box, 65, 66, 67)
        self.assertRaises(TypeError, win.box, 65)
        win.box()

        win.move(1, 2)
        win.hline('-', 5)
        self.assertEqual(win.instr(1, 1, 7), b' ----- ')
        win.hline(b'-', 5)
        win.hline(45, 5)
        win.hline('-', 5, curses.A_BOLD)
        win.hline(1, 1, '-', 5)
        win.hline(1, 1, '-', 5, curses.A_BOLD)

        win.move(1, 2)
        win.vline('a', 3)
        win.vline(b'a', 3)
        win.vline(97, 3)
        win.vline('a', 3, curses.A_STANDOUT)
        win.vline(1, 1, 'a', 3)
        win.vline(1, 1, ';', 2, curses.A_STANDOUT)
        self.assertEqual(win.inch(1, 1), b';'[0] | curses.A_STANDOUT)
        self.assertEqual(win.inch(2, 1), b';'[0] | curses.A_STANDOUT)
        self.assertEqual(win.inch(3, 1), b'a'[0])

    def test_unctrl(self):
        # TODO: wunctrl()
        self.assertEqual(curses.unctrl(b'A'), b'A')
        self.assertEqual(curses.unctrl('A'), b'A')
        self.assertEqual(curses.unctrl(65), b'A')
        self.assertEqual(curses.unctrl(b'\n'), b'^J')
        self.assertEqual(curses.unctrl('\n'), b'^J')
        self.assertEqual(curses.unctrl(10), b'^J')
        self.assertRaises(TypeError, curses.unctrl, b'')
        self.assertRaises(TypeError, curses.unctrl, b'AB')
        self.assertRaises(TypeError, curses.unctrl, '')
        self.assertRaises(TypeError, curses.unctrl, 'AB')
        self.assertRaises(OverflowError, curses.unctrl, 2**64)

    def test_endwin(self):
        if not self.isatty:
            self.skipTest('requires terminal')
        self.assertIs(curses.isendwin(), False)
        curses.endwin()
        self.assertIs(curses.isendwin(), True)
        curses.doupdate()
        self.assertIs(curses.isendwin(), False)

    def test_terminfo(self):
        self.assertIsInstance(curses.tigetflag('hc'), int)
        self.assertEqual(curses.tigetflag('cols'), -1)
        self.assertEqual(curses.tigetflag('cr'), -1)

        self.assertIsInstance(curses.tigetnum('cols'), int)
        self.assertEqual(curses.tigetnum('hc'), -2)
        self.assertEqual(curses.tigetnum('cr'), -2)

        self.assertIsInstance(curses.tigetstr('cr'), (bytes, type(None)))
        self.assertIsNone(curses.tigetstr('hc'))
        self.assertIsNone(curses.tigetstr('cols'))

        cud = curses.tigetstr('cud')
        if cud is not None:
            # See issue10570.
            self.assertIsInstance(cud, bytes)
            curses.tparm(cud, 2)
            cud_2 = curses.tparm(cud, 2)
            self.assertIsInstance(cud_2, bytes)
            curses.putp(cud_2)

        curses.putp(b'abc\n')

    def test_misc_module_funcs(self):
        curses.delay_output(1)
        curses.flushinp()

        curses.doupdate()
        self.assertIs(curses.isendwin(), False)

        curses.napms(100)

        curses.newpad(50, 50)

    def test_env_queries(self):
        # TODO: term_attrs(), erasewchar(), killwchar()
        self.assertIsInstance(curses.termname(), bytes)
        self.assertIsInstance(curses.longname(), bytes)
        self.assertIsInstance(curses.baudrate(), int)
        self.assertIsInstance(curses.has_ic(), bool)
        self.assertIsInstance(curses.has_il(), bool)
        self.assertIsInstance(curses.termattrs(), int)

        c = curses.killchar()
        self.assertIsInstance(c, bytes)
        self.assertEqual(len(c), 1)
        c = curses.erasechar()
        self.assertIsInstance(c, bytes)
        self.assertEqual(len(c), 1)

    def test_output_options(self):
        stdscr = self.stdscr

        stdscr.clearok(True)
        stdscr.clearok(False)

        stdscr.idcok(True)
        stdscr.idcok(False)

        stdscr.idlok(False)
        stdscr.idlok(True)

        if hasattr(stdscr, 'immedok'):
            stdscr.immedok(True)
            stdscr.immedok(False)

        stdscr.leaveok(True)
        stdscr.leaveok(False)

        stdscr.scrollok(True)
        stdscr.scrollok(False)

        stdscr.setscrreg(5, 10)

        curses.nonl()
        curses.nl(True)
        curses.nl(False)
        curses.nl()


    def test_input_options(self):
        stdscr = self.stdscr

        if self.isatty:
            curses.nocbreak()
            curses.cbreak()
            curses.cbreak(False)
            curses.cbreak(True)

            curses.intrflush(True)
            curses.intrflush(False)

            curses.raw()
            curses.raw(False)
            curses.raw(True)
            curses.noraw()

        curses.noecho()
        curses.echo()
        curses.echo(False)
        curses.echo(True)

        curses.halfdelay(255)
        curses.halfdelay(1)

        stdscr.keypad(True)
        stdscr.keypad(False)

        curses.meta(True)
        curses.meta(False)

        stdscr.nodelay(True)
        stdscr.nodelay(False)

        curses.noqiflush()
        curses.qiflush(True)
        curses.qiflush(False)
        curses.qiflush()

        stdscr.notimeout(True)
        stdscr.notimeout(False)

        stdscr.timeout(-1)
        stdscr.timeout(0)
        stdscr.timeout(5)

    @requires_curses_func('typeahead')
    def test_typeahead(self):
        curses.typeahead(sys.__stdin__.fileno())
        curses.typeahead(-1)

    def test_prog_mode(self):
        if not self.isatty:
            self.skipTest('requires terminal')
        curses.def_prog_mode()
        curses.reset_prog_mode()

    def test_beep(self):
        if (curses.tigetstr("bel") is not None
            or curses.tigetstr("flash") is not None):
            curses.beep()
        else:
            try:
                curses.beep()
            except curses.error:
                self.skipTest('beep() failed')

    def test_flash(self):
        if (curses.tigetstr("bel") is not None
            or curses.tigetstr("flash") is not None):
            curses.flash()
        else:
            try:
                curses.flash()
            except curses.error:
                self.skipTest('flash() failed')

    def test_curs_set(self):
        for vis, cap in [(0, 'civis'), (2, 'cvvis'), (1, 'cnorm')]:
            if curses.tigetstr(cap) is not None:
                curses.curs_set(vis)
            else:
                try:
                    curses.curs_set(vis)
                except curses.error:
                    pass

    @requires_curses_func('get_escdelay')
    def test_escdelay(self):
        escdelay = curses.get_escdelay()
        self.assertIsInstance(escdelay, int)
        curses.set_escdelay(25)
        self.assertEqual(curses.get_escdelay(), 25)
        curses.set_escdelay(escdelay)

    @requires_curses_func('get_tabsize')
    def test_tabsize(self):
        tabsize = curses.get_tabsize()
        self.assertIsInstance(tabsize, int)
        curses.set_tabsize(4)
        self.assertEqual(curses.get_tabsize(), 4)
        curses.set_tabsize(tabsize)

    @requires_curses_func('getsyx')
    def test_getsyx(self):
        y, x = curses.getsyx()
        self.assertIsInstance(y, int)
        self.assertIsInstance(x, int)
        curses.setsyx(4, 5)
        self.assertEqual(curses.getsyx(), (4, 5))

    def bad_colors(self):
        return (-1, curses.COLORS, -2**31 - 1, 2**31, -2**63 - 1, 2**63, 2**64)

    def bad_colors2(self):
        return (curses.COLORS, 2**31, 2**63, 2**64)

    def bad_pairs(self):
        return (-1, -2**31 - 1, 2**31, -2**63 - 1, 2**63, 2**64)

    def test_has_colors(self):
        self.assertIsInstance(curses.has_colors(), bool)
        self.assertIsInstance(curses.can_change_color(), bool)

    def test_start_color(self):
        if not curses.has_colors():
            self.skipTest('requires colors support')
        curses.start_color()
        if verbose:
            print(f'COLORS = {curses.COLORS}', file=sys.stderr)
            print(f'COLOR_PAIRS = {curses.COLOR_PAIRS}', file=sys.stderr)

    @requires_colors
    def test_color_content(self):
        self.assertEqual(curses.color_content(curses.COLOR_BLACK), (0, 0, 0))
        curses.color_content(0)
        maxcolor = curses.COLORS - 1
        curses.color_content(maxcolor)

        for color in self.bad_colors():
            self.assertRaises(ValueError, curses.color_content, color)

    @requires_colors
    def test_init_color(self):
        if not curses.can_change_color():
            self.skipTest('cannot change color')

        old = curses.color_content(0)
        try:
            curses.init_color(0, *old)
        except curses.error:
            self.skipTest('cannot change color (init_color() failed)')
        self.addCleanup(curses.init_color, 0, *old)
        curses.init_color(0, 0, 0, 0)
        self.assertEqual(curses.color_content(0), (0, 0, 0))
        curses.init_color(0, 1000, 1000, 1000)
        self.assertEqual(curses.color_content(0), (1000, 1000, 1000))

        maxcolor = curses.COLORS - 1
        old = curses.color_content(maxcolor)
        curses.init_color(maxcolor, *old)
        self.addCleanup(curses.init_color, maxcolor, *old)
        curses.init_color(maxcolor, 0, 500, 1000)
        self.assertEqual(curses.color_content(maxcolor), (0, 500, 1000))

        for color in self.bad_colors():
            self.assertRaises(ValueError, curses.init_color, color, 0, 0, 0)
        for comp in (-1, 1001):
            self.assertRaises(ValueError, curses.init_color, 0, comp, 0, 0)
            self.assertRaises(ValueError, curses.init_color, 0, 0, comp, 0)
            self.assertRaises(ValueError, curses.init_color, 0, 0, 0, comp)

    def get_pair_limit(self):
        pair_limit = curses.COLOR_PAIRS
        if hasattr(curses, 'ncurses_version'):
            if curses.has_extended_color_support():
                pair_limit += 2*curses.COLORS + 1
            if (not curses.has_extended_color_support()
                    or (6, 1) <= curses.ncurses_version < (6, 2)):
                pair_limit = min(pair_limit, SHORT_MAX)
            # If use_default_colors() is called, the upper limit of the extended
            # range may be restricted, so we need to check if the limit is still
            # correct
            try:
                curses.init_pair(pair_limit - 1, 0, 0)
            except ValueError:
                pair_limit = curses.COLOR_PAIRS
        return pair_limit

    @requires_colors
    def test_pair_content(self):
        if not hasattr(curses, 'use_default_colors'):
            self.assertEqual(curses.pair_content(0),
                             (curses.COLOR_WHITE, curses.COLOR_BLACK))
        curses.pair_content(0)
        maxpair = self.get_pair_limit() - 1
        if maxpair > 0:
            curses.pair_content(maxpair)

        for pair in self.bad_pairs():
            self.assertRaises(ValueError, curses.pair_content, pair)

    @requires_colors
    def test_init_pair(self):
        old = curses.pair_content(1)
        curses.init_pair(1, *old)
        self.addCleanup(curses.init_pair, 1, *old)

        curses.init_pair(1, 0, 0)
        self.assertEqual(curses.pair_content(1), (0, 0))
        maxcolor = curses.COLORS - 1
        curses.init_pair(1, maxcolor, 0)
        self.assertEqual(curses.pair_content(1), (maxcolor, 0))
        curses.init_pair(1, 0, maxcolor)
        self.assertEqual(curses.pair_content(1), (0, maxcolor))
        maxpair = self.get_pair_limit() - 1
        if maxpair > 1:
            curses.init_pair(maxpair, 0, 0)
            self.assertEqual(curses.pair_content(maxpair), (0, 0))

        for pair in self.bad_pairs():
            self.assertRaises(ValueError, curses.init_pair, pair, 0, 0)
        for color in self.bad_colors2():
            self.assertRaises(ValueError, curses.init_pair, 1, color, 0)
            self.assertRaises(ValueError, curses.init_pair, 1, 0, color)

    @requires_colors
    def test_color_attrs(self):
        for pair in 0, 1, 255:
            attr = curses.color_pair(pair)
            self.assertEqual(curses.pair_number(attr), pair, attr)
            self.assertEqual(curses.pair_number(attr | curses.A_BOLD), pair)
        self.assertEqual(curses.color_pair(0), 0)
        self.assertEqual(curses.pair_number(0), 0)

    @requires_curses_func('use_default_colors')
    @requires_colors
    def test_use_default_colors(self):
        old = curses.pair_content(0)
        try:
            curses.use_default_colors()
        except curses.error:
            self.skipTest('cannot change color (use_default_colors() failed)')
        self.assertEqual(curses.pair_content(0), (-1, -1))
        self.assertIn(old, [(curses.COLOR_WHITE, curses.COLOR_BLACK), (-1, -1), (0, 0)])

    def test_keyname(self):
        # TODO: key_name()
        self.assertEqual(curses.keyname(65), b'A')
        self.assertEqual(curses.keyname(13), b'^M')
        self.assertEqual(curses.keyname(127), b'^?')
        self.assertEqual(curses.keyname(0), b'^@')
        self.assertRaises(ValueError, curses.keyname, -1)
        self.assertIsInstance(curses.keyname(256), bytes)

    @requires_curses_func('has_key')
    def test_has_key(self):
        curses.has_key(13)

    @requires_curses_func('getmouse')
    def test_getmouse(self):
        (availmask, oldmask) = curses.mousemask(curses.BUTTON1_PRESSED)
        if availmask == 0:
            self.skipTest('mouse stuff not available')
        curses.mouseinterval(10)
        # just verify these don't cause errors
        curses.ungetmouse(0, 0, 0, 0, curses.BUTTON1_PRESSED)
        m = curses.getmouse()

    @requires_curses_func('panel')
    def test_userptr_without_set(self):
        w = curses.newwin(10, 10)
        p = curses.panel.new_panel(w)
        # try to access userptr() before calling set_userptr() -- segfaults
        with self.assertRaises(curses.panel.error,
                               msg='userptr should fail since not set'):
            p.userptr()

    @requires_curses_func('panel')
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

    @requires_curses_func('panel')
    def test_userptr_segfault(self):
        w = curses.newwin(10, 10)
        panel = curses.panel.new_panel(w)
        class A:
            def __del__(self):
                panel.set_userptr(None)
        panel.set_userptr(A())
        panel.set_userptr(None)

    @requires_curses_func('panel')
    def test_new_curses_panel(self):
        w = curses.newwin(10, 10)
        panel = curses.panel.new_panel(w)
        self.assertRaises(TypeError, type(panel))

    @requires_curses_func('is_term_resized')
    def test_is_term_resized(self):
        lines, cols = curses.LINES, curses.COLS
        self.assertIs(curses.is_term_resized(lines, cols), False)
        self.assertIs(curses.is_term_resized(lines-1, cols-1), True)

    @requires_curses_func('resize_term')
    def test_resize_term(self):
        curses.update_lines_cols()
        lines, cols = curses.LINES, curses.COLS
        new_lines = lines - 1
        new_cols = cols + 1
        curses.resize_term(new_lines, new_cols)
        self.assertEqual(curses.LINES, new_lines)
        self.assertEqual(curses.COLS, new_cols)

        curses.resize_term(lines, cols)
        self.assertEqual(curses.LINES, lines)
        self.assertEqual(curses.COLS, cols)

    @requires_curses_func('resizeterm')
    def test_resizeterm(self):
        curses.update_lines_cols()
        lines, cols = curses.LINES, curses.COLS
        new_lines = lines - 1
        new_cols = cols + 1
        curses.resizeterm(new_lines, new_cols)
        self.assertEqual(curses.LINES, new_lines)
        self.assertEqual(curses.COLS, new_cols)

        curses.resizeterm(lines, cols)
        self.assertEqual(curses.LINES, lines)
        self.assertEqual(curses.COLS, cols)

    def test_ungetch(self):
        curses.ungetch(b'A')
        self.assertEqual(self.stdscr.getkey(), 'A')
        curses.ungetch('B')
        self.assertEqual(self.stdscr.getkey(), 'B')
        curses.ungetch(67)
        self.assertEqual(self.stdscr.getkey(), 'C')

    def test_issue6243(self):
        curses.ungetch(1025)
        self.stdscr.getkey()

    @requires_curses_func('unget_wch')
    @unittest.skipIf(getattr(curses, 'ncurses_version', (99,)) < (5, 8),
                     "unget_wch is broken in ncurses 5.7 and earlier")
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
        self.assertIn("[y, x,]", human_readable_signature)

    @requires_curses_window_meth('resize')
    def test_issue13051(self):
        win = curses.newwin(5, 15, 2, 5)
        box = curses.textpad.Textbox(win, insert_mode=True)
        lines, cols = win.getmaxyx()
        win.resize(lines-2, cols-2)
        # this may cause infinite recursion, leading to a RuntimeError
        box._insert_printable_char('a')


class MiscTests(unittest.TestCase):

    def test_update_lines_cols(self):
        curses.update_lines_cols()
        lines, cols = curses.LINES, curses.COLS
        curses.LINES = curses.COLS = 0
        curses.update_lines_cols()
        self.assertEqual(curses.LINES, lines)
        self.assertEqual(curses.COLS, cols)

    @requires_curses_func('ncurses_version')
    def test_ncurses_version(self):
        v = curses.ncurses_version
        if verbose:
            print(f'ncurses_version = {curses.ncurses_version}', flush=True)
        self.assertIsInstance(v[:], tuple)
        self.assertEqual(len(v), 3)
        self.assertIsInstance(v[0], int)
        self.assertIsInstance(v[1], int)
        self.assertIsInstance(v[2], int)
        self.assertIsInstance(v.major, int)
        self.assertIsInstance(v.minor, int)
        self.assertIsInstance(v.patch, int)
        self.assertEqual(v[0], v.major)
        self.assertEqual(v[1], v.minor)
        self.assertEqual(v[2], v.patch)
        self.assertGreaterEqual(v.major, 0)
        self.assertGreaterEqual(v.minor, 0)
        self.assertGreaterEqual(v.patch, 0)

    def test_has_extended_color_support(self):
        r = curses.has_extended_color_support()
        self.assertIsInstance(r, bool)


class TestAscii(unittest.TestCase):

    def test_controlnames(self):
        for name in curses.ascii.controlnames:
            self.assertTrue(hasattr(curses.ascii, name), name)

    def test_ctypes(self):
        def check(func, expected):
            with self.subTest(ch=c, func=func):
                self.assertEqual(func(i), expected)
                self.assertEqual(func(c), expected)

        for i in range(256):
            c = chr(i)
            b = bytes([i])
            check(curses.ascii.isalnum, b.isalnum())
            check(curses.ascii.isalpha, b.isalpha())
            check(curses.ascii.isdigit, b.isdigit())
            check(curses.ascii.islower, b.islower())
            check(curses.ascii.isspace, b.isspace())
            check(curses.ascii.isupper, b.isupper())

            check(curses.ascii.isascii, i < 128)
            check(curses.ascii.ismeta, i >= 128)
            check(curses.ascii.isctrl, i < 32)
            check(curses.ascii.iscntrl, i < 32 or i == 127)
            check(curses.ascii.isblank, c in ' \t')
            check(curses.ascii.isgraph, 32 < i <= 126)
            check(curses.ascii.isprint, 32 <= i <= 126)
            check(curses.ascii.ispunct, c in string.punctuation)
            check(curses.ascii.isxdigit, c in string.hexdigits)

        for i in (-2, -1, 256, sys.maxunicode, sys.maxunicode+1):
            self.assertFalse(curses.ascii.isalnum(i))
            self.assertFalse(curses.ascii.isalpha(i))
            self.assertFalse(curses.ascii.isdigit(i))
            self.assertFalse(curses.ascii.islower(i))
            self.assertFalse(curses.ascii.isspace(i))
            self.assertFalse(curses.ascii.isupper(i))

            self.assertFalse(curses.ascii.isascii(i))
            self.assertFalse(curses.ascii.isctrl(i))
            self.assertFalse(curses.ascii.iscntrl(i))
            self.assertFalse(curses.ascii.isblank(i))
            self.assertFalse(curses.ascii.isgraph(i))
            self.assertFalse(curses.ascii.isprint(i))
            self.assertFalse(curses.ascii.ispunct(i))
            self.assertFalse(curses.ascii.isxdigit(i))

        self.assertFalse(curses.ascii.ismeta(-1))

    def test_ascii(self):
        ascii = curses.ascii.ascii
        self.assertEqual(ascii('\xc1'), 'A')
        self.assertEqual(ascii('A'), 'A')
        self.assertEqual(ascii(ord('\xc1')), ord('A'))

    def test_ctrl(self):
        ctrl = curses.ascii.ctrl
        self.assertEqual(ctrl('J'), '\n')
        self.assertEqual(ctrl('\n'), '\n')
        self.assertEqual(ctrl('@'), '\0')
        self.assertEqual(ctrl(ord('J')), ord('\n'))

    def test_alt(self):
        alt = curses.ascii.alt
        self.assertEqual(alt('\n'), '\x8a')
        self.assertEqual(alt('A'), '\xc1')
        self.assertEqual(alt(ord('A')), 0xc1)

    def test_unctrl(self):
        unctrl = curses.ascii.unctrl
        self.assertEqual(unctrl('a'), 'a')
        self.assertEqual(unctrl('A'), 'A')
        self.assertEqual(unctrl(';'), ';')
        self.assertEqual(unctrl(' '), ' ')
        self.assertEqual(unctrl('\x7f'), '^?')
        self.assertEqual(unctrl('\n'), '^J')
        self.assertEqual(unctrl('\0'), '^@')
        self.assertEqual(unctrl(ord('A')), 'A')
        self.assertEqual(unctrl(ord('\n')), '^J')
        # Meta-bit characters
        self.assertEqual(unctrl('\x8a'), '!^J')
        self.assertEqual(unctrl('\xc1'), '!A')
        self.assertEqual(unctrl(ord('\x8a')), '!^J')
        self.assertEqual(unctrl(ord('\xc1')), '!A')


def lorem_ipsum(win):
    text = [
        'Lorem ipsum',
        'dolor sit amet,',
        'consectetur',
        'adipiscing elit,',
        'sed do eiusmod',
        'tempor incididunt',
        'ut labore et',
        'dolore magna',
        'aliqua.',
    ]
    maxy, maxx = win.getmaxyx()
    for y, line in enumerate(text[:maxy]):
        win.addstr(y, 0, line[:maxx - (y == maxy - 1)])

if __name__ == '__main__':
    unittest.main()
