import contextlib
import os
import sys
import tracemalloc
import unittest
from unittest.mock import patch
from test.script_helper import assert_python_ok, assert_python_failure
from test import script_helper, support
try:
    import threading
except ImportError:
    threading = None

EMPTY_STRING_SIZE = sys.getsizeof(b'')

def get_frames(nframe, lineno_delta):
    frames = []
    frame = sys._getframe(1)
    for index in range(nframe):
        code = frame.f_code
        lineno = frame.f_lineno + lineno_delta
        frames.append((code.co_filename, lineno))
        lineno_delta = 0
        frame = frame.f_back
        if frame is None:
            break
    return tuple(frames)

def allocate_bytes(size):
    nframe = tracemalloc.get_traceback_limit()
    bytes_len = (size - EMPTY_STRING_SIZE)
    frames = get_frames(nframe, 1)
    data = b'x' * bytes_len
    return data, tracemalloc.Traceback(frames)

def create_snapshots():
    traceback_limit = 2

    raw_traces = [
        (10, (('a.py', 2), ('b.py', 4))),
        (10, (('a.py', 2), ('b.py', 4))),
        (10, (('a.py', 2), ('b.py', 4))),

        (2, (('a.py', 5), ('b.py', 4))),

        (66, (('b.py', 1),)),

        (7, (('<unknown>', 0),)),
    ]
    snapshot = tracemalloc.Snapshot(raw_traces, traceback_limit)

    raw_traces2 = [
        (10, (('a.py', 2), ('b.py', 4))),
        (10, (('a.py', 2), ('b.py', 4))),
        (10, (('a.py', 2), ('b.py', 4))),

        (2, (('a.py', 5), ('b.py', 4))),
        (5000, (('a.py', 5), ('b.py', 4))),

        (400, (('c.py', 578),)),
    ]
    snapshot2 = tracemalloc.Snapshot(raw_traces2, traceback_limit)

    return (snapshot, snapshot2)

def frame(filename, lineno):
    return tracemalloc._Frame((filename, lineno))

def traceback(*frames):
    return tracemalloc.Traceback(frames)

def traceback_lineno(filename, lineno):
    return traceback((filename, lineno))

def traceback_filename(filename):
    return traceback_lineno(filename, 0)


class TestTracemallocEnabled(unittest.TestCase):
    def setUp(self):
        if tracemalloc.is_tracing():
            self.skipTest("tracemalloc must be stopped before the test")

        tracemalloc.start(1)

    def tearDown(self):
        tracemalloc.stop()

    def test_get_tracemalloc_memory(self):
        data = [allocate_bytes(123) for count in range(1000)]
        size = tracemalloc.get_tracemalloc_memory()
        self.assertGreaterEqual(size, 0)

        tracemalloc.clear_traces()
        size2 = tracemalloc.get_tracemalloc_memory()
        self.assertGreaterEqual(size2, 0)
        self.assertLessEqual(size2, size)

    def test_get_object_traceback(self):
        tracemalloc.clear_traces()
        obj_size = 12345
        obj, obj_traceback = allocate_bytes(obj_size)
        traceback = tracemalloc.get_object_traceback(obj)
        self.assertEqual(traceback, obj_traceback)

    def test_set_traceback_limit(self):
        obj_size = 10

        tracemalloc.stop()
        self.assertRaises(ValueError, tracemalloc.start, -1)

        tracemalloc.stop()
        tracemalloc.start(10)
        obj2, obj2_traceback = allocate_bytes(obj_size)
        traceback = tracemalloc.get_object_traceback(obj2)
        self.assertEqual(len(traceback), 10)
        self.assertEqual(traceback, obj2_traceback)

        tracemalloc.stop()
        tracemalloc.start(1)
        obj, obj_traceback = allocate_bytes(obj_size)
        traceback = tracemalloc.get_object_traceback(obj)
        self.assertEqual(len(traceback), 1)
        self.assertEqual(traceback, obj_traceback)

    def find_trace(self, traces, traceback):
        for trace in traces:
            if trace[1] == traceback._frames:
                return trace

        self.fail("trace not found")

    def test_get_traces(self):
        tracemalloc.clear_traces()
        obj_size = 12345
        obj, obj_traceback = allocate_bytes(obj_size)

        traces = tracemalloc._get_traces()
        trace = self.find_trace(traces, obj_traceback)

        self.assertIsInstance(trace, tuple)
        size, traceback = trace
        self.assertEqual(size, obj_size)
        self.assertEqual(traceback, obj_traceback._frames)

        tracemalloc.stop()
        self.assertEqual(tracemalloc._get_traces(), [])

    def test_get_traces_intern_traceback(self):
        # dummy wrappers to get more useful and identical frames in the traceback
        def allocate_bytes2(size):
            return allocate_bytes(size)
        def allocate_bytes3(size):
            return allocate_bytes2(size)
        def allocate_bytes4(size):
            return allocate_bytes3(size)

        # Ensure that two identical tracebacks are not duplicated
        tracemalloc.stop()
        tracemalloc.start(4)
        obj_size = 123
        obj1, obj1_traceback = allocate_bytes4(obj_size)
        obj2, obj2_traceback = allocate_bytes4(obj_size)

        traces = tracemalloc._get_traces()

        trace1 = self.find_trace(traces, obj1_traceback)
        trace2 = self.find_trace(traces, obj2_traceback)
        size1, traceback1 = trace1
        size2, traceback2 = trace2
        self.assertEqual(traceback2, traceback1)
        self.assertIs(traceback2, traceback1)

    def test_get_traced_memory(self):
        # Python allocates some internals objects, so the test must tolerate
        # a small difference between the expected size and the real usage
        max_error = 2048

        # allocate one object
        obj_size = 1024 * 1024
        tracemalloc.clear_traces()
        obj, obj_traceback = allocate_bytes(obj_size)
        size, peak_size = tracemalloc.get_traced_memory()
        self.assertGreaterEqual(size, obj_size)
        self.assertGreaterEqual(peak_size, size)

        self.assertLessEqual(size - obj_size, max_error)
        self.assertLessEqual(peak_size - size, max_error)

        # destroy the object
        obj = None
        size2, peak_size2 = tracemalloc.get_traced_memory()
        self.assertLess(size2, size)
        self.assertGreaterEqual(size - size2, obj_size - max_error)
        self.assertGreaterEqual(peak_size2, peak_size)

        # clear_traces() must reset traced memory counters
        tracemalloc.clear_traces()
        self.assertEqual(tracemalloc.get_traced_memory(), (0, 0))

        # allocate another object
        obj, obj_traceback = allocate_bytes(obj_size)
        size, peak_size = tracemalloc.get_traced_memory()
        self.assertGreaterEqual(size, obj_size)

        # stop() also resets traced memory counters
        tracemalloc.stop()
        self.assertEqual(tracemalloc.get_traced_memory(), (0, 0))

    def test_clear_traces(self):
        obj, obj_traceback = allocate_bytes(123)
        traceback = tracemalloc.get_object_traceback(obj)
        self.assertIsNotNone(traceback)

        tracemalloc.clear_traces()
        traceback2 = tracemalloc.get_object_traceback(obj)
        self.assertIsNone(traceback2)

    def test_is_tracing(self):
        tracemalloc.stop()
        self.assertFalse(tracemalloc.is_tracing())

        tracemalloc.start()
        self.assertTrue(tracemalloc.is_tracing())

    def test_snapshot(self):
        obj, source = allocate_bytes(123)

        # take a snapshot
        snapshot = tracemalloc.take_snapshot()

        # write on disk
        snapshot.dump(support.TESTFN)
        self.addCleanup(support.unlink, support.TESTFN)

        # load from disk
        snapshot2 = tracemalloc.Snapshot.load(support.TESTFN)
        self.assertEqual(snapshot2.traces, snapshot.traces)

        # tracemalloc must be tracing memory allocations to take a snapshot
        tracemalloc.stop()
        with self.assertRaises(RuntimeError) as cm:
            tracemalloc.take_snapshot()
        self.assertEqual(str(cm.exception),
                         "the tracemalloc module must be tracing memory "
                         "allocations to take a snapshot")

    def test_snapshot_save_attr(self):
        # take a snapshot with a new attribute
        snapshot = tracemalloc.take_snapshot()
        snapshot.test_attr = "new"
        snapshot.dump(support.TESTFN)
        self.addCleanup(support.unlink, support.TESTFN)

        # load() should recreates the attribute
        snapshot2 = tracemalloc.Snapshot.load(support.TESTFN)
        self.assertEqual(snapshot2.test_attr, "new")

    def fork_child(self):
        if not tracemalloc.is_tracing():
            return 2

        obj_size = 12345
        obj, obj_traceback = allocate_bytes(obj_size)
        traceback = tracemalloc.get_object_traceback(obj)
        if traceback is None:
            return 3

        # everything is fine
        return 0

    @unittest.skipUnless(hasattr(os, 'fork'), 'need os.fork()')
    def test_fork(self):
        # check that tracemalloc is still working after fork
        pid = os.fork()
        if not pid:
            # child
            exitcode = 1
            try:
                exitcode = self.fork_child()
            finally:
                os._exit(exitcode)
        else:
            pid2, status = os.waitpid(pid, 0)
            self.assertTrue(os.WIFEXITED(status))
            exitcode = os.WEXITSTATUS(status)
            self.assertEqual(exitcode, 0)


class TestSnapshot(unittest.TestCase):
    maxDiff = 4000

    def test_create_snapshot(self):
        raw_traces = [(5, (('a.py', 2),))]

        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(tracemalloc, 'is_tracing',
                                             return_value=True))
            stack.enter_context(patch.object(tracemalloc, 'get_traceback_limit',
                                             return_value=5))
            stack.enter_context(patch.object(tracemalloc, '_get_traces',
                                             return_value=raw_traces))

            snapshot = tracemalloc.take_snapshot()
            self.assertEqual(snapshot.traceback_limit, 5)
            self.assertEqual(len(snapshot.traces), 1)
            trace = snapshot.traces[0]
            self.assertEqual(trace.size, 5)
            self.assertEqual(len(trace.traceback), 1)
            self.assertEqual(trace.traceback[0].filename, 'a.py')
            self.assertEqual(trace.traceback[0].lineno, 2)

    def test_filter_traces(self):
        snapshot, snapshot2 = create_snapshots()
        filter1 = tracemalloc.Filter(False, "b.py")
        filter2 = tracemalloc.Filter(True, "a.py", 2)
        filter3 = tracemalloc.Filter(True, "a.py", 5)

        original_traces = list(snapshot.traces._traces)

        # exclude b.py
        snapshot3 = snapshot.filter_traces((filter1,))
        self.assertEqual(snapshot3.traces._traces, [
            (10, (('a.py', 2), ('b.py', 4))),
            (10, (('a.py', 2), ('b.py', 4))),
            (10, (('a.py', 2), ('b.py', 4))),
            (2, (('a.py', 5), ('b.py', 4))),
            (7, (('<unknown>', 0),)),
        ])

        # filter_traces() must not touch the original snapshot
        self.assertEqual(snapshot.traces._traces, original_traces)

        # only include two lines of a.py
        snapshot4 = snapshot3.filter_traces((filter2, filter3))
        self.assertEqual(snapshot4.traces._traces, [
            (10, (('a.py', 2), ('b.py', 4))),
            (10, (('a.py', 2), ('b.py', 4))),
            (10, (('a.py', 2), ('b.py', 4))),
            (2, (('a.py', 5), ('b.py', 4))),
        ])

        # No filter: just duplicate the snapshot
        snapshot5 = snapshot.filter_traces(())
        self.assertIsNot(snapshot5, snapshot)
        self.assertIsNot(snapshot5.traces, snapshot.traces)
        self.assertEqual(snapshot5.traces, snapshot.traces)

        self.assertRaises(TypeError, snapshot.filter_traces, filter1)

    def test_snapshot_group_by_line(self):
        snapshot, snapshot2 = create_snapshots()
        tb_0 = traceback_lineno('<unknown>', 0)
        tb_a_2 = traceback_lineno('a.py', 2)
        tb_a_5 = traceback_lineno('a.py', 5)
        tb_b_1 = traceback_lineno('b.py', 1)
        tb_c_578 = traceback_lineno('c.py', 578)

        # stats per file and line
        stats1 = snapshot.statistics('lineno')
        self.assertEqual(stats1, [
            tracemalloc.Statistic(tb_b_1, 66, 1),
            tracemalloc.Statistic(tb_a_2, 30, 3),
            tracemalloc.Statistic(tb_0, 7, 1),
            tracemalloc.Statistic(tb_a_5, 2, 1),
        ])

        # stats per file and line (2)
        stats2 = snapshot2.statistics('lineno')
        self.assertEqual(stats2, [
            tracemalloc.Statistic(tb_a_5, 5002, 2),
            tracemalloc.Statistic(tb_c_578, 400, 1),
            tracemalloc.Statistic(tb_a_2, 30, 3),
        ])

        # stats diff per file and line
        statistics = snapshot2.compare_to(snapshot, 'lineno')
        self.assertEqual(statistics, [
            tracemalloc.StatisticDiff(tb_a_5, 5002, 5000, 2, 1),
            tracemalloc.StatisticDiff(tb_c_578, 400, 400, 1, 1),
            tracemalloc.StatisticDiff(tb_b_1, 0, -66, 0, -1),
            tracemalloc.StatisticDiff(tb_0, 0, -7, 0, -1),
            tracemalloc.StatisticDiff(tb_a_2, 30, 0, 3, 0),
        ])

    def test_snapshot_group_by_file(self):
        snapshot, snapshot2 = create_snapshots()
        tb_0 = traceback_filename('<unknown>')
        tb_a = traceback_filename('a.py')
        tb_b = traceback_filename('b.py')
        tb_c = traceback_filename('c.py')

        # stats per file
        stats1 = snapshot.statistics('filename')
        self.assertEqual(stats1, [
            tracemalloc.Statistic(tb_b, 66, 1),
            tracemalloc.Statistic(tb_a, 32, 4),
            tracemalloc.Statistic(tb_0, 7, 1),
        ])

        # stats per file (2)
        stats2 = snapshot2.statistics('filename')
        self.assertEqual(stats2, [
            tracemalloc.Statistic(tb_a, 5032, 5),
            tracemalloc.Statistic(tb_c, 400, 1),
        ])

        # stats diff per file
        diff = snapshot2.compare_to(snapshot, 'filename')
        self.assertEqual(diff, [
            tracemalloc.StatisticDiff(tb_a, 5032, 5000, 5, 1),
            tracemalloc.StatisticDiff(tb_c, 400, 400, 1, 1),
            tracemalloc.StatisticDiff(tb_b, 0, -66, 0, -1),
            tracemalloc.StatisticDiff(tb_0, 0, -7, 0, -1),
        ])

    def test_snapshot_group_by_traceback(self):
        snapshot, snapshot2 = create_snapshots()

        # stats per file
        tb1 = traceback(('a.py', 2), ('b.py', 4))
        tb2 = traceback(('a.py', 5), ('b.py', 4))
        tb3 = traceback(('b.py', 1))
        tb4 = traceback(('<unknown>', 0))
        stats1 = snapshot.statistics('traceback')
        self.assertEqual(stats1, [
            tracemalloc.Statistic(tb3, 66, 1),
            tracemalloc.Statistic(tb1, 30, 3),
            tracemalloc.Statistic(tb4, 7, 1),
            tracemalloc.Statistic(tb2, 2, 1),
        ])

        # stats per file (2)
        tb5 = traceback(('c.py', 578))
        stats2 = snapshot2.statistics('traceback')
        self.assertEqual(stats2, [
            tracemalloc.Statistic(tb2, 5002, 2),
            tracemalloc.Statistic(tb5, 400, 1),
            tracemalloc.Statistic(tb1, 30, 3),
        ])

        # stats diff per file
        diff = snapshot2.compare_to(snapshot, 'traceback')
        self.assertEqual(diff, [
            tracemalloc.StatisticDiff(tb2, 5002, 5000, 2, 1),
            tracemalloc.StatisticDiff(tb5, 400, 400, 1, 1),
            tracemalloc.StatisticDiff(tb3, 0, -66, 0, -1),
            tracemalloc.StatisticDiff(tb4, 0, -7, 0, -1),
            tracemalloc.StatisticDiff(tb1, 30, 0, 3, 0),
        ])

        self.assertRaises(ValueError,
                          snapshot.statistics, 'traceback', cumulative=True)

    def test_snapshot_group_by_cumulative(self):
        snapshot, snapshot2 = create_snapshots()
        tb_0 = traceback_filename('<unknown>')
        tb_a = traceback_filename('a.py')
        tb_b = traceback_filename('b.py')
        tb_a_2 = traceback_lineno('a.py', 2)
        tb_a_5 = traceback_lineno('a.py', 5)
        tb_b_1 = traceback_lineno('b.py', 1)
        tb_b_4 = traceback_lineno('b.py', 4)

        # per file
        stats = snapshot.statistics('filename', True)
        self.assertEqual(stats, [
            tracemalloc.Statistic(tb_b, 98, 5),
            tracemalloc.Statistic(tb_a, 32, 4),
            tracemalloc.Statistic(tb_0, 7, 1),
        ])

        # per line
        stats = snapshot.statistics('lineno', True)
        self.assertEqual(stats, [
            tracemalloc.Statistic(tb_b_1, 66, 1),
            tracemalloc.Statistic(tb_b_4, 32, 4),
            tracemalloc.Statistic(tb_a_2, 30, 3),
            tracemalloc.Statistic(tb_0, 7, 1),
            tracemalloc.Statistic(tb_a_5, 2, 1),
        ])

    def test_trace_format(self):
        snapshot, snapshot2 = create_snapshots()
        trace = snapshot.traces[0]
        self.assertEqual(str(trace), 'a.py:2: 10 B')
        traceback = trace.traceback
        self.assertEqual(str(traceback), 'a.py:2')
        frame = traceback[0]
        self.assertEqual(str(frame), 'a.py:2')

    def test_statistic_format(self):
        snapshot, snapshot2 = create_snapshots()
        stats = snapshot.statistics('lineno')
        stat = stats[0]
        self.assertEqual(str(stat),
                         'b.py:1: size=66 B, count=1, average=66 B')

    def test_statistic_diff_format(self):
        snapshot, snapshot2 = create_snapshots()
        stats = snapshot2.compare_to(snapshot, 'lineno')
        stat = stats[0]
        self.assertEqual(str(stat),
                         'a.py:5: size=5002 B (+5000 B), count=2 (+1), average=2501 B')

    def test_slices(self):
        snapshot, snapshot2 = create_snapshots()
        self.assertEqual(snapshot.traces[:2],
                         (snapshot.traces[0], snapshot.traces[1]))

        traceback = snapshot.traces[0].traceback
        self.assertEqual(traceback[:2],
                         (traceback[0], traceback[1]))

    def test_format_traceback(self):
        snapshot, snapshot2 = create_snapshots()
        def getline(filename, lineno):
            return '  <%s, %s>' % (filename, lineno)
        with unittest.mock.patch('tracemalloc.linecache.getline',
                                 side_effect=getline):
            tb = snapshot.traces[0].traceback
            self.assertEqual(tb.format(),
                             ['  File "a.py", line 2',
                              '    <a.py, 2>',
                              '  File "b.py", line 4',
                              '    <b.py, 4>'])

            self.assertEqual(tb.format(limit=1),
                             ['  File "a.py", line 2',
                              '    <a.py, 2>'])

            self.assertEqual(tb.format(limit=-1),
                             [])


class TestFilters(unittest.TestCase):
    maxDiff = 2048

    def test_filter_attributes(self):
        # test default values
        f = tracemalloc.Filter(True, "abc")
        self.assertEqual(f.inclusive, True)
        self.assertEqual(f.filename_pattern, "abc")
        self.assertIsNone(f.lineno)
        self.assertEqual(f.all_frames, False)

        # test custom values
        f = tracemalloc.Filter(False, "test.py", 123, True)
        self.assertEqual(f.inclusive, False)
        self.assertEqual(f.filename_pattern, "test.py")
        self.assertEqual(f.lineno, 123)
        self.assertEqual(f.all_frames, True)

        # parameters passed by keyword
        f = tracemalloc.Filter(inclusive=False, filename_pattern="test.py", lineno=123, all_frames=True)
        self.assertEqual(f.inclusive, False)
        self.assertEqual(f.filename_pattern, "test.py")
        self.assertEqual(f.lineno, 123)
        self.assertEqual(f.all_frames, True)

        # read-only attribute
        self.assertRaises(AttributeError, setattr, f, "filename_pattern", "abc")

    def test_filter_match(self):
        # filter without line number
        f = tracemalloc.Filter(True, "abc")
        self.assertTrue(f._match_frame("abc", 0))
        self.assertTrue(f._match_frame("abc", 5))
        self.assertTrue(f._match_frame("abc", 10))
        self.assertFalse(f._match_frame("12356", 0))
        self.assertFalse(f._match_frame("12356", 5))
        self.assertFalse(f._match_frame("12356", 10))

        f = tracemalloc.Filter(False, "abc")
        self.assertFalse(f._match_frame("abc", 0))
        self.assertFalse(f._match_frame("abc", 5))
        self.assertFalse(f._match_frame("abc", 10))
        self.assertTrue(f._match_frame("12356", 0))
        self.assertTrue(f._match_frame("12356", 5))
        self.assertTrue(f._match_frame("12356", 10))

        # filter with line number > 0
        f = tracemalloc.Filter(True, "abc", 5)
        self.assertFalse(f._match_frame("abc", 0))
        self.assertTrue(f._match_frame("abc", 5))
        self.assertFalse(f._match_frame("abc", 10))
        self.assertFalse(f._match_frame("12356", 0))
        self.assertFalse(f._match_frame("12356", 5))
        self.assertFalse(f._match_frame("12356", 10))

        f = tracemalloc.Filter(False, "abc", 5)
        self.assertTrue(f._match_frame("abc", 0))
        self.assertFalse(f._match_frame("abc", 5))
        self.assertTrue(f._match_frame("abc", 10))
        self.assertTrue(f._match_frame("12356", 0))
        self.assertTrue(f._match_frame("12356", 5))
        self.assertTrue(f._match_frame("12356", 10))

        # filter with line number 0
        f = tracemalloc.Filter(True, "abc", 0)
        self.assertTrue(f._match_frame("abc", 0))
        self.assertFalse(f._match_frame("abc", 5))
        self.assertFalse(f._match_frame("abc", 10))
        self.assertFalse(f._match_frame("12356", 0))
        self.assertFalse(f._match_frame("12356", 5))
        self.assertFalse(f._match_frame("12356", 10))

        f = tracemalloc.Filter(False, "abc", 0)
        self.assertFalse(f._match_frame("abc", 0))
        self.assertTrue(f._match_frame("abc", 5))
        self.assertTrue(f._match_frame("abc", 10))
        self.assertTrue(f._match_frame("12356", 0))
        self.assertTrue(f._match_frame("12356", 5))
        self.assertTrue(f._match_frame("12356", 10))

    def test_filter_match_filename(self):
        def fnmatch(inclusive, filename, pattern):
            f = tracemalloc.Filter(inclusive, pattern)
            return f._match_frame(filename, 0)

        self.assertTrue(fnmatch(True, "abc", "abc"))
        self.assertFalse(fnmatch(True, "12356", "abc"))
        self.assertFalse(fnmatch(True, "<unknown>", "abc"))

        self.assertFalse(fnmatch(False, "abc", "abc"))
        self.assertTrue(fnmatch(False, "12356", "abc"))
        self.assertTrue(fnmatch(False, "<unknown>", "abc"))

    def test_filter_match_filename_joker(self):
        def fnmatch(filename, pattern):
            filter = tracemalloc.Filter(True, pattern)
            return filter._match_frame(filename, 0)

        # empty string
        self.assertFalse(fnmatch('abc', ''))
        self.assertFalse(fnmatch('', 'abc'))
        self.assertTrue(fnmatch('', ''))
        self.assertTrue(fnmatch('', '*'))

        # no *
        self.assertTrue(fnmatch('abc', 'abc'))
        self.assertFalse(fnmatch('abc', 'abcd'))
        self.assertFalse(fnmatch('abc', 'def'))

        # a*
        self.assertTrue(fnmatch('abc', 'a*'))
        self.assertTrue(fnmatch('abc', 'abc*'))
        self.assertFalse(fnmatch('abc', 'b*'))
        self.assertFalse(fnmatch('abc', 'abcd*'))

        # a*b
        self.assertTrue(fnmatch('abc', 'a*c'))
        self.assertTrue(fnmatch('abcdcx', 'a*cx'))
        self.assertFalse(fnmatch('abb', 'a*c'))
        self.assertFalse(fnmatch('abcdce', 'a*cx'))

        # a*b*c
        self.assertTrue(fnmatch('abcde', 'a*c*e'))
        self.assertTrue(fnmatch('abcbdefeg', 'a*bd*eg'))
        self.assertFalse(fnmatch('abcdd', 'a*c*e'))
        self.assertFalse(fnmatch('abcbdefef', 'a*bd*eg'))

        # replace .pyc and .pyo suffix with .py
        self.assertTrue(fnmatch('a.pyc', 'a.py'))
        self.assertTrue(fnmatch('a.pyo', 'a.py'))
        self.assertTrue(fnmatch('a.py', 'a.pyc'))
        self.assertTrue(fnmatch('a.py', 'a.pyo'))

        if os.name == 'nt':
            # case insensitive
            self.assertTrue(fnmatch('aBC', 'ABc'))
            self.assertTrue(fnmatch('aBcDe', 'Ab*dE'))

            self.assertTrue(fnmatch('a.pyc', 'a.PY'))
            self.assertTrue(fnmatch('a.PYO', 'a.py'))
            self.assertTrue(fnmatch('a.py', 'a.PYC'))
            self.assertTrue(fnmatch('a.PY', 'a.pyo'))
        else:
            # case sensitive
            self.assertFalse(fnmatch('aBC', 'ABc'))
            self.assertFalse(fnmatch('aBcDe', 'Ab*dE'))

            self.assertFalse(fnmatch('a.pyc', 'a.PY'))
            self.assertFalse(fnmatch('a.PYO', 'a.py'))
            self.assertFalse(fnmatch('a.py', 'a.PYC'))
            self.assertFalse(fnmatch('a.PY', 'a.pyo'))

        if os.name == 'nt':
            # normalize alternate separator "/" to the standard separator "\"
            self.assertTrue(fnmatch(r'a/b', r'a\b'))
            self.assertTrue(fnmatch(r'a\b', r'a/b'))
            self.assertTrue(fnmatch(r'a/b\c', r'a\b/c'))
            self.assertTrue(fnmatch(r'a/b/c', r'a\b\c'))
        else:
            # there is no alternate separator
            self.assertFalse(fnmatch(r'a/b', r'a\b'))
            self.assertFalse(fnmatch(r'a\b', r'a/b'))
            self.assertFalse(fnmatch(r'a/b\c', r'a\b/c'))
            self.assertFalse(fnmatch(r'a/b/c', r'a\b\c'))

    def test_filter_match_trace(self):
        t1 = (("a.py", 2), ("b.py", 3))
        t2 = (("b.py", 4), ("b.py", 5))
        t3 = (("c.py", 5), ('<unknown>', 0))
        unknown = (('<unknown>', 0),)

        f = tracemalloc.Filter(True, "b.py", all_frames=True)
        self.assertTrue(f._match_traceback(t1))
        self.assertTrue(f._match_traceback(t2))
        self.assertFalse(f._match_traceback(t3))
        self.assertFalse(f._match_traceback(unknown))

        f = tracemalloc.Filter(True, "b.py", all_frames=False)
        self.assertFalse(f._match_traceback(t1))
        self.assertTrue(f._match_traceback(t2))
        self.assertFalse(f._match_traceback(t3))
        self.assertFalse(f._match_traceback(unknown))

        f = tracemalloc.Filter(False, "b.py", all_frames=True)
        self.assertFalse(f._match_traceback(t1))
        self.assertFalse(f._match_traceback(t2))
        self.assertTrue(f._match_traceback(t3))
        self.assertTrue(f._match_traceback(unknown))

        f = tracemalloc.Filter(False, "b.py", all_frames=False)
        self.assertTrue(f._match_traceback(t1))
        self.assertFalse(f._match_traceback(t2))
        self.assertTrue(f._match_traceback(t3))
        self.assertTrue(f._match_traceback(unknown))

        f = tracemalloc.Filter(False, "<unknown>", all_frames=False)
        self.assertTrue(f._match_traceback(t1))
        self.assertTrue(f._match_traceback(t2))
        self.assertTrue(f._match_traceback(t3))
        self.assertFalse(f._match_traceback(unknown))

        f = tracemalloc.Filter(True, "<unknown>", all_frames=True)
        self.assertFalse(f._match_traceback(t1))
        self.assertFalse(f._match_traceback(t2))
        self.assertTrue(f._match_traceback(t3))
        self.assertTrue(f._match_traceback(unknown))

        f = tracemalloc.Filter(False, "<unknown>", all_frames=True)
        self.assertTrue(f._match_traceback(t1))
        self.assertTrue(f._match_traceback(t2))
        self.assertFalse(f._match_traceback(t3))
        self.assertFalse(f._match_traceback(unknown))


class TestCommandLine(unittest.TestCase):
    def test_env_var_disabled_by_default(self):
        # not tracing by default
        code = 'import tracemalloc; print(tracemalloc.is_tracing())'
        ok, stdout, stderr = assert_python_ok('-c', code)
        stdout = stdout.rstrip()
        self.assertEqual(stdout, b'False')

    @unittest.skipIf(script_helper._interpreter_requires_environment(),
                     'Cannot run -E tests when PYTHON env vars are required.')
    def test_env_var_ignored_with_E(self):
        """PYTHON* environment variables must be ignored when -E is present."""
        code = 'import tracemalloc; print(tracemalloc.is_tracing())'
        ok, stdout, stderr = assert_python_ok('-E', '-c', code, PYTHONTRACEMALLOC='1')
        stdout = stdout.rstrip()
        self.assertEqual(stdout, b'False')

    def test_env_var_enabled_at_startup(self):
        # tracing at startup
        code = 'import tracemalloc; print(tracemalloc.is_tracing())'
        ok, stdout, stderr = assert_python_ok('-c', code, PYTHONTRACEMALLOC='1')
        stdout = stdout.rstrip()
        self.assertEqual(stdout, b'True')

    def test_env_limit(self):
        # start and set the number of frames
        code = 'import tracemalloc; print(tracemalloc.get_traceback_limit())'
        ok, stdout, stderr = assert_python_ok('-c', code, PYTHONTRACEMALLOC='10')
        stdout = stdout.rstrip()
        self.assertEqual(stdout, b'10')

    def test_env_var_invalid(self):
        for nframe in (-1, 0, 2**30):
            with self.subTest(nframe=nframe):
                with support.SuppressCrashReport():
                    ok, stdout, stderr = assert_python_failure(
                        '-c', 'pass',
                        PYTHONTRACEMALLOC=str(nframe))
                    self.assertIn(b'PYTHONTRACEMALLOC: invalid '
                                  b'number of frames',
                                  stderr)

    def test_sys_xoptions(self):
        for xoptions, nframe in (
            ('tracemalloc', 1),
            ('tracemalloc=1', 1),
            ('tracemalloc=15', 15),
        ):
            with self.subTest(xoptions=xoptions, nframe=nframe):
                code = 'import tracemalloc; print(tracemalloc.get_traceback_limit())'
                ok, stdout, stderr = assert_python_ok('-X', xoptions, '-c', code)
                stdout = stdout.rstrip()
                self.assertEqual(stdout, str(nframe).encode('ascii'))

    def test_sys_xoptions_invalid(self):
        for nframe in (-1, 0, 2**30):
            with self.subTest(nframe=nframe):
                with support.SuppressCrashReport():
                    args = ('-X', 'tracemalloc=%s' % nframe, '-c', 'pass')
                    ok, stdout, stderr = assert_python_failure(*args)
                    self.assertIn(b'-X tracemalloc=NFRAME: invalid '
                                  b'number of frames',
                                  stderr)

    def test_pymem_alloc0(self):
        # Issue #21639: Check that PyMem_Malloc(0) with tracemalloc enabled
        # does not crash.
        code = 'import _testcapi; _testcapi.test_pymem_alloc0(); 1'
        assert_python_ok('-X', 'tracemalloc', '-c', code)


def test_main():
    support.run_unittest(
        TestTracemallocEnabled,
        TestSnapshot,
        TestFilters,
        TestCommandLine,
    )

if __name__ == "__main__":
    test_main()
