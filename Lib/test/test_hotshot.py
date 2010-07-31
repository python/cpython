import os
import pprint
import unittest
import tempfile
import _hotshot
import gc

from test import test_support

# Silence Py3k warning
hotshot = test_support.import_module('hotshot', deprecated=True)
from hotshot.log import ENTER, EXIT, LINE
from hotshot import stats


def shortfilename(fn):
    # We use a really shortened filename since an exact match is made,
    # and the source may be either a Python source file or a
    # pre-compiled bytecode file.
    if fn:
        return os.path.splitext(os.path.basename(fn))[0]
    else:
        return fn


class UnlinkingLogReader(hotshot.log.LogReader):
    """Extend the LogReader so the log file is unlinked when we're
    done with it."""

    def __init__(self, logfn):
        self.__logfn = logfn
        hotshot.log.LogReader.__init__(self, logfn)

    def next(self, index=None):
        try:
            return hotshot.log.LogReader.next(self)
        except StopIteration:
            self.close()
            os.unlink(self.__logfn)
            raise


class HotShotTestCase(unittest.TestCase):
    def new_profiler(self, lineevents=0, linetimings=1):
        self.logfn = test_support.TESTFN
        return hotshot.Profile(self.logfn, lineevents, linetimings)

    def get_logreader(self):
        return UnlinkingLogReader(self.logfn)

    def get_events_wotime(self):
        L = []
        for event in self.get_logreader():
            what, (filename, lineno, funcname), tdelta = event
            L.append((what, (shortfilename(filename), lineno, funcname)))
        return L

    def check_events(self, expected):
        events = self.get_events_wotime()
        if events != expected:
            self.fail(
                "events did not match expectation; got:\n%s\nexpected:\n%s"
                % (pprint.pformat(events), pprint.pformat(expected)))

    def run_test(self, callable, events, profiler=None):
        if profiler is None:
            profiler = self.new_profiler()
        self.assertTrue(not profiler._prof.closed)
        profiler.runcall(callable)
        self.assertTrue(not profiler._prof.closed)
        profiler.close()
        self.assertTrue(profiler._prof.closed)
        self.check_events(events)

    def test_addinfo(self):
        def f(p):
            p.addinfo("test-key", "test-value")
        profiler = self.new_profiler()
        profiler.runcall(f, profiler)
        profiler.close()
        log = self.get_logreader()
        info = log._info
        list(log)
        self.assertTrue(info["test-key"] == ["test-value"])

    def test_line_numbers(self):
        def f():
            y = 2
            x = 1
        def g():
            f()
        f_lineno = f.func_code.co_firstlineno
        g_lineno = g.func_code.co_firstlineno
        events = [(ENTER, ("test_hotshot", g_lineno, "g")),
                  (LINE,  ("test_hotshot", g_lineno+1, "g")),
                  (ENTER, ("test_hotshot", f_lineno, "f")),
                  (LINE,  ("test_hotshot", f_lineno+1, "f")),
                  (LINE,  ("test_hotshot", f_lineno+2, "f")),
                  (EXIT,  ("test_hotshot", f_lineno, "f")),
                  (EXIT,  ("test_hotshot", g_lineno, "g")),
                  ]
        self.run_test(g, events, self.new_profiler(lineevents=1))

    def test_start_stop(self):
        # Make sure we don't return NULL in the start() and stop()
        # methods when there isn't an error.  Bug in 2.2 noted by
        # Anthony Baxter.
        profiler = self.new_profiler()
        profiler.start()
        profiler.stop()
        profiler.close()
        os.unlink(self.logfn)

    def test_bad_sys_path(self):
        import sys
        import os
        orig_path = sys.path
        coverage = hotshot._hotshot.coverage
        try:
            # verify we require a list for sys.path
            sys.path = 'abc'
            self.assertRaises(RuntimeError, coverage, test_support.TESTFN)
            # verify that we require sys.path exists
            del sys.path
            self.assertRaises(RuntimeError, coverage, test_support.TESTFN)
        finally:
            sys.path = orig_path
            if os.path.exists(test_support.TESTFN):
                os.remove(test_support.TESTFN)

    def test_logreader_eof_error(self):
        emptyfile = tempfile.NamedTemporaryFile()
        try:
            self.assertRaises((IOError, EOFError), _hotshot.logreader,
                              emptyfile.name)
        finally:
            emptyfile.close()
        gc.collect()

    def test_load_stats(self):
        def start(prof):
            prof.start()
        # Make sure stats can be loaded when start and stop of profiler
        # are not executed in the same stack frame.
        profiler = self.new_profiler()
        start(profiler)
        profiler.stop()
        profiler.close()
        stats.load(self.logfn)
        os.unlink(self.logfn)


def test_main():
    test_support.run_unittest(HotShotTestCase)


if __name__ == "__main__":
    test_main()
