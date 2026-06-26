import functools
import inspect
import os
import select
import string
import sys
import tempfile
import threading
import unittest
from unittest.mock import MagicMock

from test.support import (requires, verbose, SaveSignals, cpython_only,
                          check_disallow_instantiation, MISSING_C_DOCSTRINGS,
                          gc_collect, SHORT_TIMEOUT)
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

# newterm() is used when available (it reports errors instead of exiting), but
# initscr() is still the fallback, and an unusable $TERM has no terminal to
# drive either way.
@unittest.skipIf(not term or term == 'unknown',
                 "$TERM=%r, no usable terminal" % term)
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
        if hasattr(curses, 'newterm'):
            # Use newterm() rather than initscr(): it reports errors instead of
            # exiting, and gives each test a fresh screen, which also lets
            # ScreenTests run newterm()/set_term() in the same process.
            try:
                infd = sys.__stdin__.fileno()
            except (AttributeError, ValueError, OSError):
                infd = stdout_fd
            self.screen = curses.newterm(term, stdout_fd, infd)
            self.stdscr = self.screen.stdscr
            # Close the screen after the test to break its window<->screen
            # reference cycle deterministically, rather than leaving it for the
            # cyclic GC to collect during a much later test (where a window's
            # delwin() can fail -- an unraisable error on macOS).
            self.addCleanup(self.screen.close)
            self.addCleanup(setattr, self, 'screen', None)
            self.addCleanup(setattr, self, 'stdscr', None)
        else:
            self.stdscr = curses.initscr()
        if self.isatty:
            curses.savetty()
            self.addCleanup(curses.endwin)
            self.addCleanup(curses.resetty)
        self.stdscr.erase()

    @requires_curses_func('filter')
    def test_filter(self):
        # filter() must be called before initscr()/newterm(); it confines
        # curses to a single line.  Undo it with nofilter() afterwards so that
        # it does not shrink the screens created by later tests.
        curses.filter()
        if hasattr(curses, 'nofilter'):
            self.addCleanup(curses.nofilter)

    @requires_curses_func('use_env')
    def test_use_env(self):
        # TODO: Should be called before initscr() or newterm() are called.
        # TODO: use_tioctl()
        curses.use_env(False)
        curses.use_env(True)

    def test_error(self):
        self.assertIsSubclass(curses.error, Exception)

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

    def test_subwindows_references(self):
        win = curses.newwin(5, 10)
        win2 = win.subwin(3, 7)
        del win
        gc_collect()
        del win2
        gc_collect()

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

    def _encodable(self, s):
        # Wide characters are only supported in a locale that can encode them.
        try:
            s.encode(self.stdscr.encoding)
        except UnicodeEncodeError:
            return False
        return True

    @requires_curses_window_meth('get_wch')
    def test_addch_combining(self):
        stdscr = self.stdscr
        stdscr.move(0, 0)
        # A character cell may hold a spacing char plus combining marks.
        if self._encodable('e\u0301'):
            stdscr.addch('e\u0301')              # 'e' + COMBINING ACUTE ACCENT
        if self._encodable('a\u0323\u0300'):
            stdscr.addch(1, 0, 'a\u0323\u0300')  # base plus two combining marks
        # Too many code points to fit in a single character cell.
        self.assertRaises(TypeError, stdscr.addch, 'e' + '\u0301' * 10)
        # Only the first code point may be a spacing character.
        self.assertRaises(ValueError, stdscr.addch, 'ab')
        self.assertRaises(ValueError, stdscr.addch, 'a\u0301b')
        # A lone control character is allowed (like addch(ord('\n'))), but it
        # cannot be combined with other characters, as base or otherwise.
        stdscr.addch('\n')
        self.assertRaises(ValueError, stdscr.addch, 'a\n')
        self.assertRaises(ValueError, stdscr.addch, '\n\u0301')
        self.assertRaises(ValueError, stdscr.addch, '\ne\u0301')

    @requires_curses_window_meth('get_wch')
    def test_addch_emoji(self):
        # curses has no grapheme-cluster support: a cell holds one spacing
        # character plus zero-width combining characters.  A lone emoji fits,
        # as does an emoji with a zero-width variation selector.
        stdscr = self.stdscr
        if self._encodable('\U0001f600'):
            stdscr.addch(0, 0, '\U0001f600')          # single emoji
        if self._encodable('\u263a\ufe0f'):
            stdscr.addch(1, 0, '\u263a\ufe0f')        # WHITE SMILING FACE + VS-16
        # An emoji ZWJ sequence or an emoji with a modifier is more than one
        # spacing character and cannot share a single cell.
        self.assertRaises(ValueError, stdscr.addch,
                          '\U0001f44d\U0001f3fd')          # thumbs up + skin tone
        self.assertRaises(ValueError, stdscr.addch,
                          '\U0001f468\u200d\U0001f469')    # man ZWJ woman

    @requires_curses_window_meth('get_wch')
    def test_wide_characters(self):
        # Wide and combining characters in the character-cell methods.
        stdscr = self.stdscr
        combining = 'e\u0301'              # 'e' + COMBINING ACUTE ACCENT
        vline, hline = '\u2502', '\u2500'  # box-drawing vertical/horizontal
        stdscr.move(0, 0)
        if self._encodable(combining):
            stdscr.echochar(combining)
            stdscr.insch(1, 0, combining)
            stdscr.bkgdset(combining)
            stdscr.bkgd(combining)
        if self._encodable(hline):
            stdscr.hline(2, 0, hline, 5)
        if self._encodable(vline):
            stdscr.vline(3, 0, vline, 3)
        if self._encodable(vline + hline):
            stdscr.border(vline, vline, hline, hline)
            stdscr.box(vline, hline)
        # border() and box() cannot mix integer and wide-string characters.
        self.assertRaises(TypeError, stdscr.box, vline, ord('-'))

    @requires_curses_func('complexchar')
    def test_complexchar_in_cell_methods(self):
        # Every single-character-cell method also accepts a complexchar, whose
        # attributes and color pair come from the cell itself.
        stdscr = self.stdscr
        cc = curses.complexchar('A', curses.A_BOLD)
        v = curses.complexchar('|')
        h = curses.complexchar('-')
        stdscr.move(0, 0)
        stdscr.addch(0, 0, cc)
        self.assertEqual(str(stdscr.in_wch(0, 0)), 'A')
        self.assertTrue(stdscr.in_wch(0, 0).attr & curses.A_BOLD)
        stdscr.insch(1, 0, cc)
        stdscr.echochar(cc)
        stdscr.bkgdset(cc)
        stdscr.bkgd(cc)
        stdscr.hline(2, 0, h, 3)
        stdscr.vline(3, 0, v, 3)
        stdscr.border(v, v, h, h)
        stdscr.box(v, h)
        # A complexchar already carries its rendition, so combining it with an
        # explicit attr argument is rejected.
        self.assertRaises(TypeError, stdscr.addch, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.addch, 0, 0, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.insch, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.echochar, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.bkgd, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.bkgdset, cc, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.hline, h, 3, curses.A_BOLD)
        self.assertRaises(TypeError, stdscr.vline, v, 3, curses.A_BOLD)

    @requires_curses_window_meth('in_wstr')
    def test_in_wstr(self):
        # The wide-character window read returns a str (instr returns bytes).
        stdscr = self.stdscr
        s = 'a\u00e9\u2502z'  # 'a', 'e'+acute (precomposed), box vline, 'z'
        stdscr.addstr(0, 0, s)
        self.assertEqual(stdscr.in_wstr(0, 0, len(s)), s)
        self.assertIsInstance(stdscr.instr(0, 0, len(s)), bytes)

    @requires_curses_func('complexchar')
    def test_complexchar(self):
        # A complexchar is a styled wide-character cell: str() is its text,
        # and the attr and pair attributes are its rendition.
        cc = curses.complexchar('A', curses.A_BOLD)
        self.assertEqual(str(cc), 'A')
        self.assertTrue(cc.attr & curses.A_BOLD)
        self.assertEqual(cc.pair, 0)
        # A spacing character optionally followed by combining characters.
        if self._encodable('e\u0301'):
            self.assertEqual(str(curses.complexchar('e\u0301')), 'e\u0301')
        # Defaults: no attributes, color pair 0.
        cc = curses.complexchar('z')
        self.assertEqual(str(cc), 'z')
        self.assertEqual(cc.attr, 0)
        self.assertEqual(cc.pair, 0)
        # Immutable rendition.
        self.assertRaises(AttributeError, setattr, cc, 'attr', 1)
        self.assertRaises(AttributeError, setattr, cc, 'pair', 1)
        # Equality and hashing compare text, attributes and color pair.
        self.assertEqual(curses.complexchar('A', curses.A_BOLD),
                         curses.complexchar('A', curses.A_BOLD))
        self.assertEqual(hash(curses.complexchar('A', curses.A_BOLD)),
                         hash(curses.complexchar('A', curses.A_BOLD)))
        self.assertNotEqual(curses.complexchar('A'),
                            curses.complexchar('A', curses.A_BOLD))
        self.assertNotEqual(curses.complexchar('A'), curses.complexchar('B'))
        # repr() shows only a non-default attr/pair, and is a constructor call.
        ns = {'_curses': sys.modules[type(cc).__module__]}
        self.assertNotIn('attr=', repr(curses.complexchar('z')))
        self.assertNotIn('pair=', repr(curses.complexchar('z')))
        r = repr(curses.complexchar('A', curses.A_BOLD))
        self.assertIn('attr=', r)
        self.assertNotIn('pair=', r)
        self.assertEqual(eval(r, ns), curses.complexchar('A', curses.A_BOLD))
        # Invalid arguments.
        self.assertRaises(TypeError, curses.complexchar, 65)
        self.assertRaises(TypeError, curses.complexchar, 'A', 'bold')
        self.assertRaises(OverflowError, curses.complexchar, 'A', -1)
        self.assertRaises(OverflowError, curses.complexchar, 'A', 1 << 64)
        self.assertRaises(ValueError, curses.complexchar, 'A', 0, -1)
        self.assertRaises(ValueError, curses.complexchar, 'ab')

    @requires_curses_window_meth('in_wch')
    def test_in_wch(self):
        # in_wch() returns the styled wide cell as a complexchar -- something
        # inch() (a packed chtype) cannot represent.
        stdscr = self.stdscr
        stdscr.addch(0, 0, curses.complexchar('A', curses.A_UNDERLINE))
        cc = stdscr.in_wch(0, 0)
        self.assertEqual(str(cc), 'A')
        self.assertTrue(cc.attr & curses.A_UNDERLINE)
        if self._encodable('\u00e9'):  # precomposed, for a portable round-trip
            stdscr.addch(3, 0, curses.complexchar('\u00e9'))
            self.assertEqual(str(stdscr.in_wch(3, 0)), '\u00e9')
        # in_wch() without coordinates reads at the cursor position.
        stdscr.move(0, 0)
        self.assertEqual(str(stdscr.in_wch()), 'A')

    @requires_curses_window_meth('in_wch')
    @requires_colors
    def test_in_wch_color(self):
        # Unlike the chtype methods (which pack the pair into the value via
        # COLOR_PAIR), a complex character carries its color pair separately.
        stdscr = self.stdscr
        curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
        stdscr.addch(0, 0, curses.complexchar('A', curses.A_BOLD, 1))
        cc = stdscr.in_wch(0, 0)
        self.assertEqual(str(cc), 'A')
        self.assertTrue(cc.attr & curses.A_BOLD)
        self.assertEqual(cc.pair, 1)
        self.assertEqual(curses.complexchar('A', 0, 1).pair, 1)

    @requires_curses_window_meth('getbkgrnd')
    def test_getbkgrnd(self):
        # getbkgrnd() returns the background as a complexchar (getbkgd() can
        # only return a packed chtype).
        stdscr = self.stdscr
        stdscr.bkgdset(curses.complexchar(' ', curses.A_DIM))
        stdscr.bkgd(curses.complexchar(' ', curses.A_BOLD))
        cc = stdscr.getbkgrnd()
        self.assertEqual(str(cc), ' ')
        self.assertTrue(cc.attr & curses.A_BOLD)

    @requires_curses_func('complexstr')
    def test_complexstr(self):
        # A complexstr is an immutable run of styled wide-character cells: the
        # string counterpart of complexchar (as str is to a single character).
        cc = curses.complexchar
        B = curses.A_BOLD
        # Built from an iterable whose items are complexchar or str cells.
        s = curses.complexstr([cc('A', B), 'b', cc('c')])
        self.assertEqual(len(s), 3)
        self.assertEqual(str(s), 'Abc')
        # Indexing yields a complexchar carrying the cell's rendition.
        self.assertIsInstance(s[0], curses.complexchar)
        self.assertEqual(str(s[0]), 'A')
        self.assertTrue(s[0].attr & B)
        self.assertEqual(s[-1], cc('c'))
        self.assertRaises(IndexError, lambda: s[3])
        # Iteration walks the cells.
        self.assertEqual([str(c) for c in s], ['A', 'b', 'c'])
        # Slicing and concatenation produce new complexstr instances.
        self.assertIsInstance(s[1:], curses.complexstr)
        self.assertEqual(str(s[1:]), 'bc')
        self.assertEqual(str(s[::-1]), 'cbA')
        self.assertEqual(str(s + curses.complexstr(['Z'])), 'AbcZ')
        # The empty complexstr.
        self.assertEqual(len(curses.complexstr([])), 0)
        self.assertEqual(str(curses.complexstr('')), '')
        # Equality and hashing compare the cells (text, attributes, pair).
        self.assertEqual(s, curses.complexstr([cc('A', B), 'b', cc('c')]))
        self.assertEqual(hash(s),
                         hash(curses.complexstr([cc('A', B), 'b', cc('c')])))
        self.assertNotEqual(s, curses.complexstr([cc('A'), 'b', cc('c')]))
        self.assertNotEqual(s, curses.complexstr([cc('A', B), 'b']))
        # A spacing character optionally followed by combining characters.
        if self._encodable('é'):
            self.assertEqual(str(curses.complexstr(['é', 'x'])),
                             'éx')
        # cells is positional-only.
        self.assertRaises(TypeError, lambda: curses.complexstr(cells=['x']))
        # Invalid arguments.
        self.assertRaises(TypeError, curses.complexstr, 5)
        self.assertRaises(TypeError, curses.complexstr, [65])
        self.assertRaises(ValueError, curses.complexstr, ['ab'])

        # A string is split into character cells, grouping each base character
        # with the combining characters that follow it (not one cell per code
        # point), unlike a generic sequence whose items are each one cell.
        self.assertEqual(len(curses.complexstr('abc')), 3)
        self.assertEqual(str(curses.complexstr('abc')), 'abc')
        self.assertEqual(len(curses.complexstr('')), 0)
        base = 'é'  # 'e' + combining acute: two code points, one cell
        if self._encodable(base):
            self.assertEqual(len(curses.complexstr(base)), 1)
            self.assertEqual(curses.complexstr(base)[0], cc(base))
            self.assertEqual(len(curses.complexstr('a' + base + 'b')), 3)
            # A combining character cannot begin a cell: one that leads the
            # string, or overflows a base's combining slots, has no base.
            self.assertRaises(ValueError, curses.complexstr, '\u0301')
            self.assertRaises(ValueError, curses.complexstr, 'e' + '\u0301' * 10)
            # A control character may stand alone but not carry combining marks.
            self.assertRaises(ValueError, curses.complexstr, '\n\u0301')
        # attr and pair apply to every cell of a string; pair is optional.
        styled = curses.complexstr('hi', B, 0)
        self.assertTrue(all(styled[i].attr & B for i in range(len(styled))))
        self.assertEqual(curses.complexstr('x', B)[0], cc('x', B))
        self.assertEqual(curses.complexstr('x', B, 0)[0], cc('x', B, 0))
        # attr and pair may also be passed by keyword.
        self.assertEqual(curses.complexstr('x', attr=B)[0], cc('x', B))
        self.assertEqual(curses.complexstr('x', attr=B, pair=0)[0], cc('x', B, 0))
        self.assertEqual(curses.complexstr('x', pair=0)[0], cc('x', 0, 0))
        # cells is positional-only.
        self.assertRaises(TypeError, lambda: curses.complexstr(cells='x'))
        self.assertRaises(ValueError, curses.complexstr, 'a', 0, -1)
        self.assertRaises(ValueError, lambda: curses.complexstr('a', pair=-1))
        # For a non-string, giving attr/pair at all is an error (the cells
        # carry their own rendition) -- even attr=0.
        self.assertRaises(TypeError, curses.complexstr, [cc('A')], B)
        self.assertRaises(TypeError, curses.complexstr, [cc('A')], 0)
        self.assertRaises(TypeError, curses.complexstr, ['A'], 0, 0)
        self.assertRaises(TypeError,
                          lambda: curses.complexstr([cc('A')], attr=B))
        self.assertRaises(TypeError,
                          lambda: curses.complexstr(['A'], pair=0))

    @requires_curses_window_meth('in_wchstr')
    def test_in_wchstr(self):
        # in_wchstr() returns a complexstr -- the styled-cell counterpart of
        # instr() (bytes) and in_wstr() (str), which both strip the rendition.
        stdscr = self.stdscr
        cc = curses.complexchar
        B = curses.A_BOLD
        s = curses.complexstr([cc('A', B), cc('b'), cc('C', B)])
        stdscr.addstr(0, 0, s)
        r = stdscr.in_wchstr(0, 0, 3)
        self.assertIsInstance(r, curses.complexstr)
        # A read followed by a re-write is an exact round-trip.
        self.assertEqual(r, s)
        self.assertEqual(str(r), 'AbC')
        self.assertTrue(r[0].attr & B)
        self.assertFalse(r[1].attr & B)
        # The count is optional and reads to the end of the line by default.
        stdscr.move(0, 0)
        self.assertEqual(str(stdscr.in_wchstr())[:3], 'AbC')

    @requires_curses_window_meth('in_wchstr')
    def test_complexstr_in_write_methods(self):
        # addstr/addnstr/insstr/insnstr also accept a complexstr, written via
        # the wide-character functions; a plain str keeps its current meaning.
        stdscr = self.stdscr
        cc = curses.complexchar
        B = curses.A_BOLD
        s = curses.complexstr([cc('A', B), cc('b'), cc('C', B)])
        # addstr with a complexstr round-trips.
        stdscr.addstr(0, 0, s)
        self.assertEqual(stdscr.in_wchstr(0, 0, 3), s)
        # addnstr writes at most n cells.
        stdscr.addstr(2, 0, '....')
        stdscr.addnstr(2, 0, s, 2)
        self.assertEqual(str(stdscr.in_wchstr(2, 0, 4)), 'Ab..')
        # insstr inserts the cells in order.
        stdscr.move(3, 0)
        stdscr.addstr('END')
        stdscr.insstr(3, 0, curses.complexstr([cc('P'), cc('Q')]))
        self.assertEqual(str(stdscr.in_wchstr(3, 0, 5)), 'PQEND')
        # insnstr inserts at most n cells.
        stdscr.move(4, 0)
        stdscr.addstr('END')
        stdscr.insnstr(4, 0, curses.complexstr(['1', '2', '3']), 2)
        self.assertEqual(str(stdscr.in_wchstr(4, 0, 5)), '12END')
        # An empty run is accepted (and still honours the move).
        stdscr.addstr(5, 0, curses.complexstr([]))
        stdscr.insstr(5, 0, curses.complexstr([]))
        # Cells carry their own rendition, so an explicit attr is rejected.
        self.assertRaises(TypeError, stdscr.addstr, s, B)
        self.assertRaises(TypeError, stdscr.addnstr, s, 2, B)
        self.assertRaises(TypeError, stdscr.insstr, s, B)
        self.assertRaises(TypeError, stdscr.insnstr, s, 2, B)
        # A bare sequence of cells is not accepted; build a complexstr first.
        self.assertRaises(TypeError, stdscr.addstr, [cc('A'), 'b'])
        self.assertRaises(TypeError, stdscr.insstr, [cc('A'), 'b'])

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
        c = '\u0114'
        try:
            stdscr.echochar(c)
        except UnicodeEncodeError:
            # The character is not encodable with the current encoding.
            self.assertRaises(UnicodeEncodeError, c.encode, encoding)
        except OverflowError:
            # The character is encoded to a multibyte sequence.
            encoded = c.encode(encoding)
            self.assertNotEqual(len(encoded), 1, repr(encoded))
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

    def test_add_string_behavior(self):
        # addstr() advances the cursor past the written text; addnstr()
        # writes at most n characters.
        win = curses.newwin(1, 10, 0, 0)
        win.addstr(0, 0, 'abc')
        self.assertEqual(win.getyx(), (0, 3))
        win.erase()
        win.addnstr(0, 0, 'abcdef', 3)
        self.assertEqual(win.instr(0, 0), b'abc       ')

    def test_insert_string_behavior(self):
        # insstr()/insnstr() insert at the cursor, shift the rest of the
        # line right (losing characters off the edge), and leave the cursor
        # where it was.
        win = curses.newwin(1, 10, 0, 0)
        win.addstr(0, 0, 'abcde')
        win.move(0, 1)
        win.insstr('XY')
        self.assertEqual(win.getyx(), (0, 1))   # cursor did not advance
        self.assertEqual(win.instr(0, 0), b'aXYbcde   ')

        win.erase()
        win.addstr(0, 0, 'ZZZZZ')
        win.move(0, 0)
        win.insnstr('abcdef', 3)           # at most 3 characters
        self.assertEqual(win.instr(0, 0), b'abcZZZZZ  ')

    def test_insch(self):
        # insch() inserts a single character at the cursor (or at y, x),
        # shifting the rest of the line right.
        win = curses.newwin(2, 10, 0, 0)
        win.addstr(0, 0, 'abc')
        win.move(0, 1)
        win.insch(ord('X'))
        self.assertEqual(win.instr(0, 0), b'aXbc      ')
        win.insch(1, 0, 'Y', curses.A_BOLD)
        self.assertEqual(win.inch(1, 0), b'Y'[0] | curses.A_BOLD)

    def test_pad(self):
        pad = curses.newpad(10, 20)
        pad.addstr(0, 0, 'PADTEXT')
        self.assertEqual(pad.instr(0, 0, 7), b'PADTEXT')

        # subpad() creates a pad within the parent pad.  Cell sharing with
        # the parent is implementation-defined, so write to the subpad itself.
        sub = pad.subpad(3, 5, 0, 0)
        self.assertEqual(sub.getmaxyx(), (3, 5))
        sub.addstr(1, 0, 'sub')
        self.assertEqual(sub.instr(1, 0, 3), b'sub')

        # A pad is refreshed onto an explicit screen rectangle; the
        # 6-argument form is required (and rejected for ordinary windows).
        pad.refresh(0, 0, 0, 0, 4, 10)
        pad.noutrefresh(0, 0, 0, 0, 4, 10)
        curses.doupdate()
        self.assertRaises(TypeError, pad.refresh)
        win = curses.newwin(5, 5, 0, 0)
        self.assertRaises(TypeError, win.refresh, 0, 0, 0, 0, 4, 4)

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

    def test_coordinate_errors(self):
        # Addressing a cell outside the window raises curses.error.
        win = curses.newwin(5, 10, 0, 0)
        self.assertRaises(curses.error, win.move, 100, 100)
        self.assertRaises(curses.error, win.move, -1, -1)
        self.assertRaises(curses.error, win.addch, 100, 100, ord('x'))
        self.assertRaises(curses.error, win.inch, 100, 100)
        if hasattr(win, 'chgat'):       # chgat() requires wchgat()
            self.assertRaises(curses.error, win.chgat, 100, 0, curses.A_BOLD)

    def test_argument_errors(self):
        win = curses.newwin(5, 10, 0, 0)
        # A character argument must be an int, a byte or a one-element string.
        self.assertRaises(TypeError, win.addch, [])
        self.assertRaises(OverflowError, win.addch, 2**64)
        # A string method rejects a non-string, non-bytes argument.
        self.assertRaises(TypeError, win.addstr, 5)
        self.assertRaises(TypeError, win.addstr)
        # Wrong number of positional arguments.
        self.assertRaises(TypeError, win.instr, 0, 0, 0, 0)

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
        win = curses.newwin(5, 15, 5, 2)
        win.attron(curses.A_BOLD)
        win.attroff(curses.A_BOLD)
        win.attrset(curses.A_BOLD)

        win.standout()
        win.standend()

        # The attr_*() family works on attr_t attributes paired with a color
        # pair, unlike the chtype-based attron()/attroff()/attrset().
        win.attr_set(curses.A_BOLD | curses.A_UNDERLINE)
        attrs, pair = win.attr_get()
        self.assertTrue(attrs & curses.A_BOLD)
        self.assertTrue(attrs & curses.A_UNDERLINE)
        self.assertEqual(pair, 0)
        self.assertEqual(win.getattrs(), attrs)

        win.attr_on(curses.A_REVERSE)
        self.assertTrue(win.attr_get()[0] & curses.A_REVERSE)
        win.attr_off(curses.A_REVERSE)
        self.assertFalse(win.attr_get()[0] & curses.A_REVERSE)

        # color_set() with a real pair needs start_color(); see
        # test_attr_color_pair.  Here only the argument validation is checked,
        # which fails before wcolor_set() is reached.
        self.assertRaises(TypeError, win.attr_set, 'x')
        self.assertRaises(TypeError, win.attr_set, curses.A_BOLD, 'x')
        self.assertRaises(TypeError, win.attr_on, 'x')
        self.assertRaises(TypeError, win.color_set, 'x')
        self.assertRaises(ValueError, win.color_set, -1)
        self.assertRaises(ValueError, win.attr_set, curses.A_BOLD, -1)
        # attr_t is unsigned: a negative or too-large attribute overflows.
        self.assertRaises(OverflowError, win.attr_set, -1)
        self.assertRaises(OverflowError, win.attr_on, -1)
        self.assertRaises(OverflowError, win.attr_set, 1 << 64)

    @requires_colors
    def test_attr_color_pair(self):
        win = curses.newwin(5, 15, 5, 2)
        curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
        win.attr_set(curses.A_BOLD, 1)
        attrs, pair = win.attr_get()
        self.assertTrue(attrs & curses.A_BOLD)
        self.assertEqual(pair, 1)
        win.color_set(0)
        self.assertEqual(win.attr_get()[1], 0)

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

        # attr_t is unsigned: a negative or too-large attribute overflows.
        self.assertRaises(TypeError, win.chgat, 'x')
        self.assertRaises(OverflowError, win.chgat, -1)
        self.assertRaises(OverflowError, win.chgat, 1 << 64)

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
        self.assertIs(win.enclose(2, 5), True)
        self.assertIs(win.enclose(1, 5), False)
        self.assertIs(win.enclose(2, 4), False)
        self.assertIs(win.enclose(6, 19), True)
        self.assertIs(win.enclose(7, 19), False)
        self.assertIs(win.enclose(6, 20), False)

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

    @requires_curses_func('wunctrl')
    def test_wunctrl(self):
        # The wide-character variant of unctrl() returns a str.
        self.assertEqual(curses.wunctrl(b'A'), 'A')
        self.assertEqual(curses.wunctrl('A'), 'A')
        self.assertEqual(curses.wunctrl(65), 'A')
        self.assertEqual(curses.wunctrl('\n'), '^J')
        self.assertEqual(curses.wunctrl(10), '^J')
        self.assertEqual(curses.wunctrl('é'), 'é')  # printable
        self.assertRaises(TypeError, curses.wunctrl, b'')
        self.assertRaises(TypeError, curses.wunctrl, b'AB')
        self.assertRaises(TypeError, curses.wunctrl, '')
        # More than one spacing character is not a single cell.
        self.assertRaises(ValueError, curses.wunctrl, 'AB')
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
        # TODO: term_attrs()
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

        # The erase and kill characters are a property of the controlling
        # terminal: the wide variants report ERR (raising curses.error) without
        # one, while the narrow variants above return an unspecified byte.
        try:
            tty_fd = os.open(os.ctermid(), os.O_RDONLY)
        except OSError:
            tty_fd = None
        if tty_fd is not None:
            os.close(tty_fd)
            if hasattr(curses, 'erasewchar'):
                c = curses.erasewchar()
                self.assertIsInstance(c, str)
                self.assertEqual(len(c), 1)
            if hasattr(curses, 'killwchar'):
                c = curses.killwchar()
                self.assertIsInstance(c, str)
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

    @requires_curses_window_meth('is_scrollok')
    def test_state_getters(self):
        stdscr = self.stdscr
        # Each is_*() getter returns the value set by the matching setter.
        for setter, getter in [
            ('clearok', 'is_cleared'),
            ('idcok', 'is_idcok'),
            ('idlok', 'is_idlok'),
            ('keypad', 'is_keypad'),
            ('leaveok', 'is_leaveok'),
            ('nodelay', 'is_nodelay'),
            ('notimeout', 'is_notimeout'),
            ('scrollok', 'is_scrollok'),
        ]:
            getattr(stdscr, setter)(True)
            self.assertIs(getattr(stdscr, getter)(), True)
            getattr(stdscr, setter)(False)
            self.assertIs(getattr(stdscr, getter)(), False)
        if hasattr(stdscr, 'immedok'):
            stdscr.immedok(True)
            self.assertIs(stdscr.is_immedok(), True)
            stdscr.immedok(False)
        if hasattr(stdscr, 'syncok'):
            stdscr.syncok(True)
            self.assertIs(stdscr.is_syncok(), True)
            stdscr.syncok(False)

        # getdelay() reflects timeout()/nodelay().
        stdscr.timeout(100)
        self.assertEqual(stdscr.getdelay(), 100)
        stdscr.nodelay(True)
        self.assertEqual(stdscr.getdelay(), 0)
        stdscr.timeout(-1)
        self.assertEqual(stdscr.getdelay(), -1)

        # getscrreg() reflects setscrreg().
        stdscr.setscrreg(5, 10)
        self.assertEqual(stdscr.getscrreg(), (5, 10))

        # is_pad()/is_subwin()/getparent().
        self.assertIs(stdscr.is_pad(), False)
        self.assertIs(stdscr.is_subwin(), False)
        self.assertIsNone(stdscr.getparent())
        sub = stdscr.subwin(3, 3, 0, 0)
        self.assertIs(sub.is_subwin(), True)
        self.assertIs(sub.getparent(), stdscr)
        pad = curses.newpad(5, 5)
        self.assertIs(pad.is_pad(), True)

    @requires_curses_func('is_cbreak')
    def test_global_state_getters(self):
        if self.isatty:
            curses.cbreak()
            self.assertIs(curses.is_cbreak(), True)
            curses.nocbreak()
            self.assertIs(curses.is_cbreak(), False)
            curses.raw()
            self.assertIs(curses.is_raw(), True)
            curses.noraw()
            self.assertIs(curses.is_raw(), False)
        curses.echo()
        self.assertIs(curses.is_echo(), True)
        curses.noecho()
        self.assertIs(curses.is_echo(), False)
        curses.nl()
        self.assertIs(curses.is_nl(), True)
        curses.nonl()
        self.assertIs(curses.is_nl(), False)

    @requires_curses_func('typeahead')
    def test_typeahead(self):
        curses.typeahead(sys.__stdin__.fileno())
        curses.typeahead(-1)

    def test_prog_mode(self):
        if not self.isatty:
            self.skipTest('requires terminal')
        curses.def_prog_mode()
        curses.reset_prog_mode()
        # def_shell_mode()/reset_shell_mode() are intentionally not exercised
        # here: they capture and restore curses' "shell mode" terminal state,
        # which is only meaningful before initscr().  Calling them mid-suite
        # corrupts the modes that endwin() restores and breaks later tests.

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

    @requires_curses_func('alloc_pair')
    @requires_colors
    def test_dynamic_color_pairs(self):
        # alloc_pair()/find_pair()/free_pair() (extended-color extension).
        fg = bg = curses.COLORS - 1
        pair = curses.alloc_pair(fg, bg)
        self.assertGreater(pair, 0)
        self.assertEqual(curses.pair_content(pair), (fg, bg))
        # The same combination of colors reuses the same pair.
        self.assertEqual(curses.alloc_pair(fg, bg), pair)
        self.assertEqual(curses.find_pair(fg, bg), pair)
        # Once freed, the pair is no longer found.
        self.assertIsNone(curses.free_pair(pair))
        self.assertEqual(curses.find_pair(fg, bg), -1)

        # Error paths.
        for color in self.bad_colors2():
            self.assertRaises(ValueError, curses.alloc_pair, color, 0)
            self.assertRaises(ValueError, curses.alloc_pair, 0, color)
            self.assertRaises(ValueError, curses.find_pair, color, 0)
            self.assertRaises(ValueError, curses.find_pair, 0, color)
        for pair in self.bad_pairs():
            self.assertRaises(ValueError, curses.free_pair, pair)
        # Color pair 0 is reserved and cannot be freed.
        self.assertRaises(curses.error, curses.free_pair, 0)

        # Invalid number or type of arguments.
        self.assertRaises(TypeError, curses.alloc_pair)
        self.assertRaises(TypeError, curses.alloc_pair, 0)
        self.assertRaises(TypeError, curses.alloc_pair, 0, 0, 0)
        self.assertRaises(TypeError, curses.alloc_pair, 'red', 0)
        self.assertRaises(TypeError, curses.alloc_pair, 0, 'red')
        self.assertRaises(TypeError, curses.alloc_pair, fg=0, bg=0)
        self.assertRaises(TypeError, curses.find_pair)
        self.assertRaises(TypeError, curses.find_pair, 0)
        self.assertRaises(TypeError, curses.find_pair, 0, 0, 0)
        self.assertRaises(TypeError, curses.find_pair, 'red', 0)
        self.assertRaises(TypeError, curses.find_pair, 0, 'red')
        self.assertRaises(TypeError, curses.free_pair)
        self.assertRaises(TypeError, curses.free_pair, 1, 2)
        self.assertRaises(TypeError, curses.free_pair, 'red')

    @requires_curses_func('reset_color_pairs')
    @requires_colors
    def test_reset_color_pairs(self):
        self.assertIsNone(curses.reset_color_pairs())
        self.assertRaises(TypeError, curses.reset_color_pairs, 0)

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
        try:
            curses.use_default_colors()
        except curses.error:
            self.skipTest('cannot change color (use_default_colors() failed)')
        self.assertEqual(curses.pair_content(0), (-1, -1))

    @requires_curses_window_meth('use')
    def test_use_window(self):
        win = self.stdscr
        self.assertEqual(win.use(lambda w, a, b: (w is win, a, b), 5, b=6),
                         (True, 5, 6))
        with self.assertRaises(ZeroDivisionError):
            win.use(lambda w: 1 / 0)

    @unittest.skipUnless(hasattr(curses.screen, 'use'),
                         'requires screen.use()')
    def test_use_screen(self):
        screen = self.screen
        self.assertEqual(
            screen.use(lambda sc, flag: (sc is screen, flag), flag=True),
            (True, True))

    @requires_curses_func('assume_default_colors')
    @requires_colors
    def test_assume_default_colors(self):
        try:
            curses.assume_default_colors(-1, -1)
        except curses.error:
            self.skipTest('cannot change color (assume_default_colors() failed)')
        self.assertEqual(curses.pair_content(0), (-1, -1))
        curses.assume_default_colors(curses.COLOR_YELLOW, curses.COLOR_BLUE)
        self.assertEqual(curses.pair_content(0), (curses.COLOR_YELLOW, curses.COLOR_BLUE))
        curses.assume_default_colors(curses.COLOR_RED, -1)
        self.assertEqual(curses.pair_content(0), (curses.COLOR_RED, -1))
        curses.assume_default_colors(-1, curses.COLOR_GREEN)
        self.assertEqual(curses.pair_content(0), (-1, curses.COLOR_GREEN))
        curses.assume_default_colors(-1, -1)

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
        self.assertIsInstance(curses.has_key(13), bool)
        self.assertIsInstance(curses.has_key(curses.KEY_LEFT), bool)

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
        # set_userptr(A()) makes a panel<->userptr reference cycle (A.__del__
        # closes over panel); clean it up so the panel and its window do not
        # linger until a later test collects them.
        self.addCleanup(self._delete_panels, panel)
        class A:
            def __del__(self):
                panel.set_userptr(None)
        panel.set_userptr(A())
        panel.set_userptr(None)

    @cpython_only
    @requires_curses_func('panel')
    def test_disallow_instantiation(self):
        # Ensure that the type disallows instantiation (bpo-43916)
        w = curses.newwin(10, 10)
        panel = curses.panel.new_panel(w)
        check_disallow_instantiation(self, type(panel))

    @requires_curses_func('panel')
    def test_panel_stack(self):
        panel = curses.panel
        # new_panel() puts the panel on top of the stack, so the three
        # panels end up ordered bottom -> top as p1, p2, p3.
        p1 = panel.new_panel(curses.newwin(3, 6, 0, 0))
        p2 = panel.new_panel(curses.newwin(3, 6, 1, 1))
        p3 = panel.new_panel(curses.newwin(3, 6, 2, 2))
        self.addCleanup(self._delete_panels, p1, p2, p3)

        # The most recently created panel is on top.
        self.assertIs(panel.top_panel(), p3)
        # window() returns the wrapped window.
        self.assertEqual(p2.window().getbegyx(), (1, 1))

        # above()/below() walk the stack one step at a time.
        self.assertIs(p1.above(), p2)
        self.assertIs(p2.above(), p3)
        self.assertIsNone(p3.above())     # nothing above the top panel
        self.assertIs(p3.below(), p2)
        self.assertIs(p2.below(), p1)

        # top() raises a panel to the top, bottom() lowers it to the bottom.
        p1.top()
        self.assertIs(panel.top_panel(), p1)
        self.assertIsNone(p1.above())
        p1.bottom()
        self.assertIs(panel.bottom_panel(), p1)
        self.assertIsNone(p1.below())

        # update_panels() refreshes the virtual screen from the stack.
        panel.update_panels()

    @requires_curses_func('panel')
    def test_panel_hide_show(self):
        p = curses.panel.new_panel(curses.newwin(3, 6, 0, 0))
        self.addCleanup(self._delete_panels, p)
        self.assertIs(p.hidden(), False)
        p.hide()
        self.assertIs(p.hidden(), True)
        p.show()
        self.assertIs(p.hidden(), False)

    @requires_curses_func('panel')
    def test_panel_move(self):
        win = curses.newwin(3, 6, 1, 2)
        p = curses.panel.new_panel(win)
        self.addCleanup(self._delete_panels, p)
        self.assertEqual(win.getbegyx(), (1, 2))
        p.move(4, 5)
        self.assertEqual(win.getbegyx(), (4, 5))

    @requires_curses_func('panel')
    def test_panel_replace(self):
        win1 = curses.newwin(3, 6, 0, 0)
        win2 = curses.newwin(4, 8, 1, 1)
        p = curses.panel.new_panel(win1)
        self.addCleanup(self._delete_panels, p)
        self.assertIs(p.window(), win1)
        p.replace(win2)
        self.assertIs(p.window(), win2)

    @requires_curses_func('panel')
    def test_panel_userptr(self):
        p = curses.panel.new_panel(curses.newwin(3, 6, 0, 0))
        self.addCleanup(self._delete_panels, p)
        obj = ['userptr']
        p.set_userptr(obj)
        self.assertIs(p.userptr(), obj)

    def _delete_panels(self, *panels):
        # Drop the panels from the global stack so they do not leak into
        # later tests that inspect top_panel()/bottom_panel().
        for p in panels:
            try:
                p.bottom()
            except curses.panel.error:
                pass
        del panels
        gc_collect()

    def _make_textbox(self, nlines, ncols, *, insert_mode=False, stripspaces=1):
        win = curses.newwin(nlines, ncols, 0, 0)
        box = curses.textpad.Textbox(win, insert_mode=insert_mode)
        box.stripspaces = stripspaces
        return box, win

    def _type(self, box, text):
        for ch in text:
            box.do_command(ch if isinstance(ch, int) else ord(ch))

    def test_textbox_gather(self):
        # Typed text is read back by gather().  With stripspaces on (the
        # default) gather() keeps a single trailing blank on a line and
        # drops trailing empty lines.
        box, win = self._make_textbox(3, 10)
        self._type(box, 'Hello')
        self.assertEqual(box.gather(), 'Hello \n')

    def test_textbox_gather_multiline(self):
        box, win = self._make_textbox(3, 10)
        self._type(box, 'ab')
        box.do_command(curses.ascii.NL)    # ^j -> start of next line
        self._type(box, 'cd')
        self.assertEqual(box.gather(), 'ab \ncd \n')

    def test_textbox_stripspaces(self):
        box, win = self._make_textbox(1, 8, stripspaces=1)
        self._type(box, 'hi')
        self.assertEqual(box.gather(), 'hi ')

        box, win = self._make_textbox(1, 8, stripspaces=0)
        self._type(box, 'hi')
        self.assertEqual(box.gather(), 'hi      ')

    def test_textbox_insert_mode(self):
        # In insert mode a typed character shifts the rest of the line right.
        box, win = self._make_textbox(1, 10, insert_mode=True)
        self._type(box, 'aXc')
        win.move(0, 1)
        self._type(box, 'b')
        self.assertEqual(box.gather(), 'abXc ')

    def test_textbox_movement(self):
        box, win = self._make_textbox(3, 10)
        self._type(box, 'abc')
        box.do_command(curses.ascii.SOH)   # ^a -> left edge
        self.assertEqual(win.getyx(), (0, 0))
        box.do_command(curses.ascii.ENQ)   # ^e -> end of line
        self.assertEqual(win.getyx(), (0, 3))

    def test_textbox_kill_to_eol(self):
        box, win = self._make_textbox(1, 10)
        self._type(box, 'abcdef')
        win.move(0, 3)
        box.do_command(curses.ascii.VT)    # ^k -> clear to end of line
        self.assertEqual(box.gather(), 'abc ')

    def test_textbox_backspace(self):
        box, win = self._make_textbox(1, 10)
        self._type(box, 'abc')
        box.do_command(curses.ascii.BS)    # ^h -> delete backward
        self.assertEqual(box.gather(), 'ab ')

    def test_textbox_edit(self):
        # edit() reads characters until Ctrl-G and returns the contents.
        box, win = self._make_textbox(1, 10)
        for ch in reversed('Hi' + chr(curses.ascii.BEL)):
            curses.ungetch(ch)
        self.assertEqual(box.edit(), 'Hi ')

    def test_textbox_edit_validate(self):
        # The validate hook can rewrite an incoming keystroke.
        box, win = self._make_textbox(1, 10)
        for ch in reversed('abc' + chr(curses.ascii.BEL)):
            curses.ungetch(ch)
        box.edit(lambda ch: ord('X') if ch == ord('b') else ch)
        self.assertEqual(box.gather(), 'aXc ')

    def test_textpad_rectangle(self):
        # rectangle() draws a box with ACS line/corner characters.
        win = curses.newwin(6, 12, 0, 0)
        curses.textpad.rectangle(win, 0, 0, 4, 8)
        chartext = curses.A_CHARTEXT
        self.assertEqual(win.inch(0, 0) & chartext,
                         curses.ACS_ULCORNER & chartext)
        self.assertEqual(win.inch(0, 8) & chartext,
                         curses.ACS_URCORNER & chartext)
        self.assertEqual(win.inch(4, 0) & chartext,
                         curses.ACS_LLCORNER & chartext)
        self.assertEqual(win.inch(4, 8) & chartext,
                         curses.ACS_LRCORNER & chartext)
        self.assertEqual(win.inch(0, 1) & chartext,
                         curses.ACS_HLINE & chartext)
        self.assertEqual(win.inch(1, 0) & chartext,
                         curses.ACS_VLINE & chartext)

    def test_wrapper(self):
        # wrapper() sets up curses, passes the screen to the callable along
        # with extra arguments, returns its result and restores the terminal.
        if not self.isatty:
            self.skipTest('requires terminal')

        def body(stdscr, a, b):
            self.assertIsInstance(stdscr, type(self.stdscr))
            self.assertIs(curses.isendwin(), False)
            return a + b

        self.assertEqual(curses.wrapper(body, 2, 3), 5)
        self.assertIs(curses.isendwin(), True)
        # wrapper() left the screen ended; revive it so the per-test
        # endwin() cleanup does not fail with ERR.
        curses.doupdate()

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

        with self.assertRaises(OverflowError):
            curses.resize_term(35000, 1)
        with self.assertRaises(OverflowError):
            curses.resize_term(1, 35000)
        # GH-120378: a failed resize can leave refresh broken; restore the
        # original size to recover.  Avoid initscr(), which would switch away
        # from the shared newterm() screen and corrupt later tests.
        curses.resize_term(lines, cols)
        self.stdscr.erase()

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

        with self.assertRaises(OverflowError):
            curses.resizeterm(35000, 1)
        with self.assertRaises(OverflowError):
            curses.resizeterm(1, 35000)
        # GH-120378: a failed resize can leave refresh broken; restore the
        # original size to recover.  Avoid initscr(), which would switch away
        # from the shared newterm() screen and corrupt later tests.
        curses.resizeterm(lines, cols)
        self.stdscr.erase()

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

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
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

    @requires_curses_func('update_lines_cols')
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
            self.assertHasAttr(curses.ascii, name)

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


class TextboxTest(unittest.TestCase):
    def setUp(self):
        self.mock_win = MagicMock(spec=curses.window)
        self.mock_win.getyx.return_value = (1, 1)
        self.mock_win.getmaxyx.return_value = (10, 20)
        self.textbox = curses.textpad.Textbox(self.mock_win)

    def test_init(self):
        """Test textbox initialization."""
        self.mock_win.reset_mock()
        tb = curses.textpad.Textbox(self.mock_win)
        self.mock_win.getmaxyx.assert_called_once_with()
        self.mock_win.keypad.assert_called_once_with(1)
        self.assertEqual(tb.insert_mode, False)
        self.assertEqual(tb.stripspaces, 1)
        self.assertIsNone(tb.lastcmd)
        self.mock_win.reset_mock()

    def test_insert(self):
        """Test inserting a printable character."""
        self.mock_win.reset_mock()
        self.textbox.do_command(ord('a'))
        self.mock_win.addch.assert_called_with(ord('a'))
        self.textbox.do_command(ord('b'))
        self.mock_win.addch.assert_called_with(ord('b'))
        self.textbox.do_command(ord('c'))
        self.mock_win.addch.assert_called_with(ord('c'))
        self.mock_win.reset_mock()

    def test_delete(self):
        """Test deleting a character."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.ascii.BS)
        self.textbox.do_command(curses.KEY_BACKSPACE)
        self.textbox.do_command(curses.ascii.DEL)
        assert self.mock_win.delch.call_count == 3
        self.mock_win.reset_mock()

    def test_move_left(self):
        """Test moving the cursor left."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.KEY_LEFT)
        self.mock_win.move.assert_called_with(1, 0)
        self.mock_win.reset_mock()

    def test_move_right(self):
        """Test moving the cursor right."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.KEY_RIGHT)
        self.mock_win.move.assert_called_with(1, 2)
        self.mock_win.reset_mock()

    def test_move_left_and_right(self):
        """Test moving the cursor left and then right."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.KEY_LEFT)
        self.mock_win.move.assert_called_with(1, 0)
        self.textbox.do_command(curses.KEY_RIGHT)
        self.mock_win.move.assert_called_with(1, 2)
        self.mock_win.reset_mock()

    def test_move_up(self):
        """Test moving the cursor up."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.KEY_UP)
        self.mock_win.move.assert_called_with(0, 1)
        self.mock_win.reset_mock()

    def test_move_down(self):
        """Test moving the cursor down."""
        self.mock_win.reset_mock()
        self.textbox.do_command(curses.KEY_DOWN)
        self.mock_win.move.assert_called_with(2, 1)
        self.mock_win.reset_mock()


@unittest.skipUnless(hasattr(curses, 'newterm'), 'requires curses.newterm()')
@unittest.skipIf(not term or term == 'unknown',
                 "$TERM=%r, newterm() may not work" % term)
@unittest.skipIf(sys.platform == "cygwin",
                 "cygwin's curses mostly just hangs")
class ScreenTests(unittest.TestCase):
    # newterm()/set_term() mutate global curses state, but each test drives its
    # own pseudo-terminal(s) and never touches the screen shared by TestCurses,
    # whose setUp() makes that screen current again.  So these can run in this
    # process, without a real terminal and without a subprocess.

    def setUp(self):
        # newterm() may install signal handlers; restore them afterwards.
        self.save_signals = SaveSignals()
        self.save_signals.save()
        self.addCleanup(self.save_signals.restore)

    def tearDown(self):
        # Leave visual mode and reclaim the test's screens while their
        # pseudo-terminals are still open (make_pty() closes them later).
        try:
            curses.endwin()
        except curses.error:
            pass
        gc_collect()

    @staticmethod
    def _drain_pty(master, stop):
        # Read and discard whatever curses writes to the screen, until asked to
        # stop and nothing more is pending.  poll() rather than a blocking
        # read() so we can stop without closing the fd (closing it while this
        # thread is blocked in read() hangs on macOS).
        poller = select.poll()
        poller.register(master, select.POLLIN)
        while True:
            if poller.poll(100):
                try:
                    if not os.read(master, 1024):
                        break  # EOF
                except OSError:
                    break
            elif stop.is_set():
                break

    def make_pty(self):
        master, slave = os.openpty()
        # Nothing reads the master end, so writing to the slave and the
        # tcdrain() in endwin() can block on macOS once the pty buffer fills;
        # drain it from a background thread (endwin() releases the GIL).
        stop = threading.Event()
        reader = threading.Thread(target=self._drain_pty, args=(master, stop),
                                  daemon=True)
        reader.start()
        # Stop and join the reader before closing the fds: on macOS, closing
        # either end while the reader is blocked in read() hangs.
        def stop_reader():
            stop.set()
            reader.join(SHORT_TIMEOUT)
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, slave)
        self.addCleanup(stop_reader)
        return slave

    def test_newterm(self):
        s = self.make_pty()
        screen = curses.newterm('xterm', s, s)
        self.assertIsInstance(screen, curses.screen)
        win = screen.stdscr
        self.assertIsInstance(win, curses.window)
        self.assertEqual(win.getmaxyx(), (24, 80))
        win.addstr(0, 0, 'hello')
        win.refresh()

    def test_newterm_file_object(self):
        # type=None uses $TERM; the file arguments accept file objects too.
        s = self.make_pty()
        out = os.fdopen(os.dup(s), 'wb', buffering=0)
        self.addCleanup(out.close)
        screen = curses.newterm(None, out, s)
        self.assertIsInstance(screen, curses.screen)

    def test_set_term(self):
        s = self.make_pty()
        s2 = self.make_pty()
        a = curses.newterm('xterm', s, s)     # current screen is a
        b = curses.newterm('xterm', s2, s2)   # current screen is b
        self.assertIs(curses.set_term(a), b)  # returns the previous one
        self.assertIs(curses.set_term(b), a)

    def test_window_keeps_screen_alive(self):
        # The standard window keeps its screen alive; dropping every other
        # reference and collecting must not invalidate the window.
        s = self.make_pty()
        win = curses.newterm('xterm', s, s).stdscr
        gc_collect()
        win.addstr(0, 0, 'still alive')
        win.refresh()

    def test_screen_freed(self):
        # Dropping all references to a (non-current) screen and its windows
        # frees it without error.
        s = self.make_pty()
        s2 = self.make_pty()
        a = curses.newterm('xterm', s, s)
        b = curses.newterm('xterm', s2, s2)   # a is no longer current
        del a
        gc_collect()

    def test_close(self):
        s = self.make_pty()
        screen = curses.newterm('xterm', s, s)
        win = screen.stdscr
        self.assertIsInstance(win, curses.window)
        screen.close()
        # After close() the standard window is detached and unusable, and
        # stdscr is None.  No reference cycle remains.
        self.assertIsNone(screen.stdscr)
        self.assertRaises(curses.error, win.addstr, 0, 0, 'x')
        # close() is idempotent.
        screen.close()

    @unittest.skipUnless(hasattr(curses, 'new_prescr'),
                         'requires curses.new_prescr()')
    def test_new_prescr(self):
        screen = curses.new_prescr()
        self.assertIsInstance(screen, curses.screen)
        self.assertIsNone(screen.stdscr)
        del screen
        gc_collect()

    @cpython_only
    def test_disallow_instantiation(self):
        # The screen type cannot be instantiated directly (bpo-43916).
        check_disallow_instantiation(self, curses.screen)


if __name__ == '__main__':
    unittest.main()
