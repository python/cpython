"""Tests for binary format round-trip functionality."""

import os
import random
import tempfile
import unittest
from collections import defaultdict

try:
    import _remote_debugging
    from _remote_debugging import (
        InterpreterInfo,
        ThreadInfo,
        FrameInfo,
        LocationInfo,
        THREAD_STATUS_HAS_GIL,
        THREAD_STATUS_ON_CPU,
        THREAD_STATUS_UNKNOWN,
        THREAD_STATUS_GIL_REQUESTED,
        THREAD_STATUS_HAS_EXCEPTION,
    )
    from profiling.sampling.binary_collector import BinaryCollector
    from profiling.sampling.binary_reader import BinaryReader

    ZSTD_AVAILABLE = _remote_debugging.zstd_available()
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )


def make_frame(filename, lineno, funcname, end_lineno=None, column=None,
               end_column=None, opcode=None):
    """Create a FrameInfo struct sequence with full location info and opcode."""
    if end_lineno is None:
        end_lineno = lineno
    if column is None:
        column = 0
    if end_column is None:
        end_column = 0
    location = LocationInfo((lineno, end_lineno, column, end_column))
    return FrameInfo((filename, location, funcname, opcode))


def make_thread(thread_id, frames, status=0):
    """Create a ThreadInfo struct sequence."""
    return ThreadInfo((thread_id, status, frames))


def make_interpreter(interp_id, threads):
    """Create an InterpreterInfo struct sequence."""
    return InterpreterInfo((interp_id, threads))


def extract_lineno(location):
    """Extract line number from location (tuple or int or None)."""
    if location is None:
        return 0  # Treat None as 0
    if isinstance(location, tuple):
        return location[0] if location[0] is not None else 0
    return location


def extract_location(location):
    """Extract full location info as dict from location tuple or None."""
    if location is None:
        return {"lineno": 0, "end_lineno": 0, "column": 0, "end_column": 0}
    if isinstance(location, tuple) and len(location) >= 4:
        return {
            "lineno": location[0] if location[0] is not None else 0,
            "end_lineno": location[1] if location[1] is not None else 0,
            "column": location[2] if location[2] is not None else 0,
            "end_column": location[3] if location[3] is not None else 0,
        }
    # Fallback for old-style location
    lineno = location[0] if isinstance(location, tuple) else location
    return {"lineno": lineno or 0, "end_lineno": lineno or 0, "column": 0, "end_column": 0}


def frame_to_dict(frame):
    """Convert a FrameInfo to a dict."""
    loc = extract_location(frame.location)
    return {
        "filename": frame.filename,
        "funcname": frame.funcname,
        "lineno": loc["lineno"],
        "end_lineno": loc["end_lineno"],
        "column": loc["column"],
        "end_column": loc["end_column"],
        "opcode": frame.opcode,
    }


class RawCollector:
    """Collector that captures all raw data grouped by thread."""

    def __init__(self):
        # Key: (interpreter_id, thread_id) -> list of samples for that thread
        self.by_thread = defaultdict(list)
        self.total_count = 0

    def collect(self, stack_frames, timestamps_us):
        """Capture the raw sample data."""
        # timestamps_us is a list; add one sample per timestamp
        count = len(timestamps_us)
        for interp in stack_frames:
            for thread in interp.threads:
                frames = [frame_to_dict(f) for f in thread.frame_info]
                key = (interp.interpreter_id, thread.thread_id)
                sample = {"status": thread.status, "frames": frames}
                for _ in range(count):
                    self.by_thread[key].append(sample)
                self.total_count += count

    def export(self, filename):
        pass


def samples_to_by_thread(samples):
    """Convert input samples to by-thread format for comparison."""
    by_thread = defaultdict(list)
    for sample in samples:
        for interp in sample:
            for thread in interp.threads:
                frames = [frame_to_dict(f) for f in thread.frame_info]
                key = (interp.interpreter_id, thread.thread_id)
                by_thread[key].append(
                    {
                        "status": thread.status,
                        "frames": frames,
                    }
                )
    return by_thread


class BinaryFormatTestBase(unittest.TestCase):
    """Base class with common setup/teardown for binary format tests."""

    def setUp(self):
        self.temp_files = []

    def tearDown(self):
        for f in self.temp_files:
            if os.path.exists(f):
                os.unlink(f)

    def create_binary_file(self, samples, interval=1000, compression="none"):
        """Create a test binary file and track it for cleanup."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        collector = BinaryCollector(
            filename, interval, compression=compression
        )
        for sample in samples:
            collector.collect(sample)
        collector.export(None)
        return filename

    def roundtrip(self, samples, interval=1000, compression="none"):
        """Write samples to binary and read back."""
        filename = self.create_binary_file(samples, interval, compression)
        collector = RawCollector()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(collector)
        return collector, count

    def assert_samples_equal(self, expected_samples, collector):
        """Assert that roundtripped samples match input exactly, per-thread."""
        expected = samples_to_by_thread(expected_samples)

        # Same threads present
        self.assertEqual(
            set(expected.keys()),
            set(collector.by_thread.keys()),
            "Thread set mismatch",
        )

        # For each thread, samples match in order
        for key in expected:
            exp_samples = expected[key]
            act_samples = collector.by_thread[key]
            interp_id, thread_id = key

            self.assertEqual(
                len(exp_samples),
                len(act_samples),
                f"Thread ({interp_id}, {thread_id}): sample count mismatch "
                f"(expected {len(exp_samples)}, got {len(act_samples)})",
            )

            for i, (exp, act) in enumerate(zip(exp_samples, act_samples)):
                self.assertEqual(
                    exp["status"],
                    act["status"],
                    f"Thread ({interp_id}, {thread_id}), sample {i}: "
                    f"status mismatch (expected {exp['status']}, got {act['status']})",
                )

                self.assertEqual(
                    len(exp["frames"]),
                    len(act["frames"]),
                    f"Thread ({interp_id}, {thread_id}), sample {i}: "
                    f"frame count mismatch",
                )

                for j, (exp_frame, act_frame) in enumerate(
                    zip(exp["frames"], act["frames"])
                ):
                    for field in ("filename", "funcname", "lineno", "end_lineno",
                                  "column", "end_column", "opcode"):
                        self.assertEqual(
                            exp_frame[field],
                            act_frame[field],
                            f"Thread ({interp_id}, {thread_id}), sample {i}, "
                            f"frame {j}: {field} mismatch "
                            f"(expected {exp_frame[field]!r}, got {act_frame[field]!r})",
                        )


class TestBinaryRoundTrip(BinaryFormatTestBase):
    """Tests for exact binary format round-trip."""

    def test_single_sample_single_frame(self):
        """Single sample with one frame roundtrips exactly."""
        samples = [
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            12345, [make_frame("test.py", 42, "myfunc")]
                        )
                    ],
                )
            ]
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_single_sample_multi_frame(self):
        """Single sample with call stack roundtrips exactly."""
        frames = [
            make_frame("inner.py", 10, "inner"),
            make_frame("middle.py", 20, "middle"),
            make_frame("outer.py", 30, "outer"),
        ]
        samples = [[make_interpreter(0, [make_thread(100, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_multiple_samples_same_stack(self):
        """Multiple identical samples roundtrip exactly (tests RLE)."""
        frame = make_frame("hot.py", 99, "hot_func")
        samples = [
            [make_interpreter(0, [make_thread(1, [frame])])]
            for _ in range(100)
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 100)
        self.assert_samples_equal(samples, collector)

    def test_multiple_samples_varying_stacks(self):
        """Multiple samples with varying stacks roundtrip exactly."""
        samples = []
        for i in range(20):
            depth = i % 5 + 1
            frames = [
                make_frame(f"f{j}.py", j * 10 + i, f"func{j}")
                for j in range(depth)
            ]
            samples.append([make_interpreter(0, [make_thread(1, frames)])])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 20)
        self.assert_samples_equal(samples, collector)

    def test_thread_ids_preserved(self):
        """Thread IDs are preserved exactly."""
        thread_ids = [1, 12345, 0x7FFF12345678, 999999]
        samples = []
        for tid in thread_ids:
            samples.append(
                [
                    make_interpreter(
                        0, [make_thread(tid, [make_frame("t.py", 10, "f")])]
                    )
                ]
            )
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(thread_ids))
        self.assert_samples_equal(samples, collector)

    def test_interpreter_ids_preserved(self):
        """Interpreter IDs are preserved exactly."""
        interp_ids = [0, 1, 5, 100]
        samples = []
        for iid in interp_ids:
            samples.append(
                [
                    make_interpreter(
                        iid, [make_thread(1, [make_frame("i.py", 10, "f")])]
                    )
                ]
            )
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(interp_ids))
        self.assert_samples_equal(samples, collector)

    def test_status_flags_preserved(self):
        """All thread status flags are preserved exactly."""
        statuses = [
            0,
            THREAD_STATUS_HAS_GIL,
            THREAD_STATUS_ON_CPU,
            THREAD_STATUS_UNKNOWN,
            THREAD_STATUS_GIL_REQUESTED,
            THREAD_STATUS_HAS_EXCEPTION,
            THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU,
            THREAD_STATUS_HAS_GIL | THREAD_STATUS_HAS_EXCEPTION,
            THREAD_STATUS_HAS_GIL
            | THREAD_STATUS_ON_CPU
            | THREAD_STATUS_GIL_REQUESTED,
        ]
        samples = []
        for i, status in enumerate(statuses):
            samples.append(
                [
                    make_interpreter(
                        0,
                        [
                            make_thread(
                                1, [make_frame("s.py", 10 + i, "f")], status
                            )
                        ],
                    )
                ]
            )
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(statuses))
        self.assert_samples_equal(samples, collector)

    def test_multiple_threads_per_sample(self):
        """Multiple threads in one sample roundtrip exactly."""
        threads = [
            make_thread(
                1, [make_frame("t1.py", 10, "t1")], THREAD_STATUS_HAS_GIL
            ),
            make_thread(
                2, [make_frame("t2.py", 20, "t2")], THREAD_STATUS_ON_CPU
            ),
            make_thread(3, [make_frame("t3.py", 30, "t3")], 0),
        ]
        samples = [[make_interpreter(0, threads)] for _ in range(10)]
        collector, count = self.roundtrip(samples)
        # 10 samples × 3 threads = 30 thread-samples
        self.assertEqual(count, 30)
        self.assert_samples_equal(samples, collector)

    def test_multiple_interpreters_per_sample(self):
        """Multiple interpreters in one sample roundtrip exactly."""
        samples = [
            [
                make_interpreter(
                    0, [make_thread(1, [make_frame("i0.py", 10, "i0")])]
                ),
                make_interpreter(
                    1, [make_thread(2, [make_frame("i1.py", 20, "i1")])]
                ),
            ]
            for _ in range(5)
        ]
        collector, count = self.roundtrip(samples)
        # 5 samples × 2 interpreters × 1 thread = 10 thread-samples
        self.assertEqual(count, 10)
        self.assert_samples_equal(samples, collector)

    def test_same_thread_id_different_interpreters(self):
        """Same thread_id in different interpreters must be tracked separately."""
        # This test catches bugs where thread state is keyed only by thread_id
        # without considering interpreter_id
        samples = []
        # Interleave samples from interpreter 0 and 1, both using thread_id=1
        for i in range(20):
            interp_id = i % 2  # Alternate between interpreter 0 and 1
            frame = make_frame(
                f"interp{interp_id}.py", 10 + i, f"func{interp_id}"
            )
            samples.append(
                [make_interpreter(interp_id, [make_thread(1, [frame])])]
            )

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 20)
        self.assert_samples_equal(samples, collector)

        # Verify both interpreters are present
        keys = set(collector.by_thread.keys())
        self.assertIn((0, 1), keys)  # interpreter 0, thread 1
        self.assertIn((1, 1), keys)  # interpreter 1, thread 1

        # Verify each interpreter got 10 samples
        self.assertEqual(len(collector.by_thread[(0, 1)]), 10)
        self.assertEqual(len(collector.by_thread[(1, 1)]), 10)

        # Verify the samples are in the right order for each interpreter
        for i, sample in enumerate(collector.by_thread[(0, 1)]):
            expected_lineno = 10 + i * 2  # 10, 12, 14, ...
            self.assertEqual(sample["frames"][0]["lineno"], expected_lineno)
            self.assertEqual(sample["frames"][0]["filename"], "interp0.py")

        for i, sample in enumerate(collector.by_thread[(1, 1)]):
            expected_lineno = 11 + i * 2  # 11, 13, 15, ...
            self.assertEqual(sample["frames"][0]["lineno"], expected_lineno)
            self.assertEqual(sample["frames"][0]["filename"], "interp1.py")

    def test_deep_call_stack(self):
        """Deep call stack roundtrips exactly."""
        depth = 100
        frames = [
            make_frame(f"f{i}.py", i + 1, f"func{i}") for i in range(depth)
        ]
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_line_numbers_preserved(self):
        """Various line numbers are preserved exactly."""
        linenos = [1, 100, 1000, 65535, 100000]
        samples = []
        for lineno in linenos:
            samples.append(
                [
                    make_interpreter(
                        0, [make_thread(1, [make_frame("l.py", lineno, "f")])]
                    )
                ]
            )
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(linenos))
        self.assert_samples_equal(samples, collector)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_zstd_compression_roundtrip(self):
        """Zstd compressed data roundtrips exactly."""
        samples = []
        for i in range(200):
            frames = [
                make_frame(f"z{j}.py", j * 10 + i + 1, f"zfunc{j}")
                for j in range(3)
            ]
            samples.append([make_interpreter(0, [make_thread(1, frames)])])
        collector, count = self.roundtrip(samples, compression="zstd")
        self.assertEqual(count, 200)
        self.assert_samples_equal(samples, collector)

    def test_sample_interval_preserved(self):
        """Sample interval is preserved in file metadata."""
        intervals = [100, 500, 1000, 5000, 10000]
        for interval in intervals:
            with self.subTest(interval=interval):
                samples = [
                    [
                        make_interpreter(
                            0, [make_thread(1, [make_frame("i.py", 1, "f")])]
                        )
                    ]
                ]
                filename = self.create_binary_file(samples, interval=interval)
                with BinaryReader(filename) as reader:
                    info = reader.get_info()
                    self.assertEqual(info["sample_interval_us"], interval)

    def test_threads_interleaved_samples(self):
        """Multiple threads with interleaved varying samples."""
        samples = []
        for i in range(30):
            threads = [
                make_thread(
                    1,
                    [make_frame("t1.py", 10 + i, "t1")],
                    THREAD_STATUS_HAS_GIL if i % 2 == 0 else 0,
                ),
                make_thread(
                    2,
                    [make_frame("t2.py", 20 + i, "t2")],
                    THREAD_STATUS_ON_CPU if i % 3 == 0 else 0,
                ),
            ]
            samples.append([make_interpreter(0, threads)])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 60)
        self.assert_samples_equal(samples, collector)

    def test_full_location_roundtrip(self):
        """Full source location (end_lineno, column, end_column) roundtrips."""
        frames = [
            make_frame("test.py", 10, "func1", end_lineno=12, column=4, end_column=20),
            make_frame("test.py", 20, "func2", end_lineno=20, column=8, end_column=45),
            make_frame("test.py", 30, "func3", end_lineno=35, column=0, end_column=100),
        ]
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_opcode_roundtrip(self):
        """Opcode values roundtrip exactly."""
        opcodes = [0, 1, 50, 100, 150, 200, 254]  # Valid Python opcodes
        samples = []
        for opcode in opcodes:
            frame = make_frame("test.py", 10, "func", opcode=opcode)
            samples.append([make_interpreter(0, [make_thread(1, [frame])])])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(opcodes))
        self.assert_samples_equal(samples, collector)

    def test_opcode_none_roundtrip(self):
        """Opcode=None (sentinel 255) roundtrips as None."""
        frame = make_frame("test.py", 10, "func", opcode=None)
        samples = [[make_interpreter(0, [make_thread(1, [frame])])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_mixed_location_and_opcode(self):
        """Mixed full location and opcode data roundtrips."""
        frames = [
            make_frame("a.py", 10, "a", end_lineno=15, column=4, end_column=30, opcode=100),
            make_frame("b.py", 20, "b", end_lineno=20, column=0, end_column=50, opcode=None),
            make_frame("c.py", 30, "c", end_lineno=32, column=8, end_column=25, opcode=50),
        ]
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_delta_encoding_multiline(self):
        """Multi-line spans (large end_lineno delta) roundtrip correctly."""
        # This tests the delta encoding: end_lineno = lineno + delta
        frames = [
            make_frame("test.py", 1, "small", end_lineno=1, column=0, end_column=10),
            make_frame("test.py", 100, "medium", end_lineno=110, column=0, end_column=50),
            make_frame("test.py", 1000, "large", end_lineno=1500, column=0, end_column=200),
        ]
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_column_positions_preserved(self):
        """Various column positions are preserved exactly."""
        columns = [(0, 10), (4, 50), (8, 100), (100, 200)]
        samples = []
        for col, end_col in columns:
            frame = make_frame("test.py", 10, "func", column=col, end_column=end_col)
            samples.append([make_interpreter(0, [make_thread(1, [frame])])])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(columns))
        self.assert_samples_equal(samples, collector)

    def test_same_line_different_opcodes(self):
        """Same line with different opcodes creates distinct frames."""
        # This tests that opcode is part of the frame key
        frames = [
            make_frame("test.py", 10, "func", opcode=100),
            make_frame("test.py", 10, "func", opcode=101),
            make_frame("test.py", 10, "func", opcode=102),
        ]
        samples = [[make_interpreter(0, [make_thread(1, [f])]) for f in frames]]
        collector, count = self.roundtrip(samples)
        # Verify all three opcodes are preserved distinctly
        self.assertEqual(count, 3)

    def test_same_line_different_columns(self):
        """Same line with different columns creates distinct frames."""
        frames = [
            make_frame("test.py", 10, "func", column=0, end_column=10),
            make_frame("test.py", 10, "func", column=15, end_column=25),
            make_frame("test.py", 10, "func", column=30, end_column=40),
        ]
        samples = [[make_interpreter(0, [make_thread(1, [f])]) for f in frames]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 3)


class TestBinaryEdgeCases(BinaryFormatTestBase):
    """Tests for edge cases in binary format."""

    def test_unicode_filenames(self):
        """Unicode filenames roundtrip exactly."""
        filenames = [
            "/путь/файл.py",
            "/路径/文件.py",
            "/パス/ファイル.py",
            "/chemin/café.py",
        ]
        for fname in filenames:
            with self.subTest(filename=fname):
                samples = [
                    [
                        make_interpreter(
                            0, [make_thread(1, [make_frame(fname, 1, "func")])]
                        )
                    ]
                ]
                collector, count = self.roundtrip(samples)
                self.assertEqual(count, 1)
                self.assert_samples_equal(samples, collector)

    def test_unicode_funcnames(self):
        """Unicode function names roundtrip exactly."""
        funcnames = [
            "функция",
            "函数",
            "関数",
            "función",
        ]
        for funcname in funcnames:
            with self.subTest(funcname=funcname):
                samples = [
                    [
                        make_interpreter(
                            0,
                            [
                                make_thread(
                                    1, [make_frame("test.py", 1, funcname)]
                                )
                            ],
                        )
                    ]
                ]
                collector, count = self.roundtrip(samples)
                self.assertEqual(count, 1)
                self.assert_samples_equal(samples, collector)

    def test_special_char_filenames(self):
        """Filenames with special characters roundtrip exactly."""
        filenames = [
            "/path/with spaces/file.py",
            "/path/with\ttab/file.py",
            "/path/with'quote/file.py",
            '/path/with"double/file.py',
            "/path/with\\backslash/file.py",
        ]
        for fname in filenames:
            with self.subTest(filename=fname):
                samples = [
                    [
                        make_interpreter(
                            0, [make_thread(1, [make_frame(fname, 1, "func")])]
                        )
                    ]
                ]
                collector, count = self.roundtrip(samples)
                self.assertEqual(count, 1)
                self.assert_samples_equal(samples, collector)

    def test_special_funcnames(self):
        """Function names with special characters roundtrip exactly."""
        funcnames = [
            "<lambda>",
            "<listcomp>",
            "<genexpr>",
            "<module>",
            "__init__",
            "func.inner",
        ]
        for funcname in funcnames:
            with self.subTest(funcname=funcname):
                samples = [
                    [
                        make_interpreter(
                            0,
                            [
                                make_thread(
                                    1, [make_frame("test.py", 1, funcname)]
                                )
                            ],
                        )
                    ]
                ]
                collector, count = self.roundtrip(samples)
                self.assertEqual(count, 1)
                self.assert_samples_equal(samples, collector)

    def test_long_filename(self):
        """Long filename roundtrips exactly."""
        long_file = "/very/long/path/" + "sub/" * 50 + "file.py"
        samples = [
            [
                make_interpreter(
                    0, [make_thread(1, [make_frame(long_file, 1, "func")])]
                )
            ]
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_long_funcname(self):
        """Long function name roundtrips exactly."""
        long_func = "very_long_function_name_" + "x" * 200
        samples = [
            [
                make_interpreter(
                    0, [make_thread(1, [make_frame("test.py", 1, long_func)])]
                )
            ]
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_empty_funcname(self):
        """Empty function name roundtrips exactly."""
        samples = [
            [
                make_interpreter(
                    0, [make_thread(1, [make_frame("test.py", 1, "")])]
                )
            ]
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_large_sample_count(self):
        """Large number of samples roundtrips exactly."""
        num = 5000
        samples = [
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            1, [make_frame("test.py", (i % 100) + 1, "func")]
                        )
                    ],
                )
            ]
            for i in range(num)
        ]
        collector, count = self.roundtrip(samples, compression="zstd")
        self.assertEqual(count, num)
        self.assert_samples_equal(samples, collector)

    def test_context_manager_cleanup(self):
        """Reader cleans up on context exit."""
        samples = [
            [
                make_interpreter(
                    0, [make_thread(1, [make_frame("t.py", 1, "f")])]
                )
            ]
        ]
        filename = self.create_binary_file(samples)
        reader = BinaryReader(filename)
        with reader:
            collector = RawCollector()
            count = reader.replay_samples(collector)
            self.assertEqual(count, 1)
        with self.assertRaises(RuntimeError):
            reader.replay_samples(collector)

    def test_invalid_file_path(self):
        """Invalid file path raises appropriate error."""
        with self.assertRaises((FileNotFoundError, OSError, ValueError)):
            with BinaryReader("/nonexistent/path/file.bin") as reader:
                reader.replay_samples(RawCollector())


class TestBinaryEncodings(BinaryFormatTestBase):
    """Tests specifically targeting different stack encodings."""

    def test_stack_full_encoding(self):
        """First sample uses STACK_FULL encoding and roundtrips."""
        frames = [make_frame(f"f{i}.py", i + 1, f"func{i}") for i in range(5)]
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)

    def test_stack_repeat_encoding(self):
        """Identical consecutive samples use RLE and roundtrip."""
        frame = make_frame("repeat.py", 42, "repeat_func")
        samples = [
            [make_interpreter(0, [make_thread(1, [frame])])]
            for _ in range(1000)
        ]
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1000)
        self.assert_samples_equal(samples, collector)

    def test_stack_suffix_encoding(self):
        """Samples sharing suffix use STACK_SUFFIX and roundtrip."""
        samples = []
        for i in range(10):
            frames = [make_frame(f"new{i}.py", i + 1, f"new{i}")]
            frames.extend(
                [
                    make_frame(f"shared{j}.py", j + 1, f"shared{j}")
                    for j in range(5)
                ]
            )
            samples.append([make_interpreter(0, [make_thread(1, frames)])])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 10)
        self.assert_samples_equal(samples, collector)

    def test_stack_pop_push_encoding(self):
        """Samples with pop+push pattern roundtrip."""
        samples = []
        base_frames = [make_frame("base.py", 10, "base")]

        # Call deeper
        samples.append([make_interpreter(0, [make_thread(1, base_frames)])])
        samples.append(
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            1,
                            [make_frame("call1.py", 20, "call1")]
                            + base_frames,
                        )
                    ],
                )
            ]
        )
        samples.append(
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            1,
                            [
                                make_frame("call2.py", 30, "call2"),
                                make_frame("call1.py", 20, "call1"),
                            ]
                            + base_frames,
                        )
                    ],
                )
            ]
        )
        # Return
        samples.append(
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            1,
                            [make_frame("call1.py", 25, "call1")]
                            + base_frames,
                        )
                    ],
                )
            ]
        )
        samples.append([make_interpreter(0, [make_thread(1, base_frames)])])

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 5)
        self.assert_samples_equal(samples, collector)

    def test_mixed_encodings(self):
        """Mix of different encoding patterns roundtrips."""
        samples = []
        # Some repeated samples (RLE)
        frame1 = make_frame("hot.py", 1, "hot")
        for _ in range(20):
            samples.append([make_interpreter(0, [make_thread(1, [frame1])])])
        # Some varying samples
        for i in range(20):
            frames = [make_frame(f"vary{i}.py", i + 1, f"vary{i}")]
            samples.append([make_interpreter(0, [make_thread(1, frames)])])
        # More repeated
        for _ in range(20):
            samples.append([make_interpreter(0, [make_thread(1, [frame1])])])

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 60)
        self.assert_samples_equal(samples, collector)

    def test_alternating_threads_status_changes(self):
        """Alternating thread status changes roundtrip correctly."""
        samples = []
        for i in range(50):
            status1 = THREAD_STATUS_HAS_GIL if i % 2 == 0 else 0
            status2 = (
                THREAD_STATUS_ON_CPU if i % 3 == 0 else THREAD_STATUS_HAS_GIL
            )
            threads = [
                make_thread(1, [make_frame("t1.py", 10, "t1")], status1),
                make_thread(2, [make_frame("t2.py", 20, "t2")], status2),
            ]
            samples.append([make_interpreter(0, threads)])
        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 100)
        self.assert_samples_equal(samples, collector)


class TestBinaryStress(BinaryFormatTestBase):
    """Randomized stress tests for binary format."""

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_random_samples_stress(self):
        """Stress test with random samples - exercises hash table resizing."""
        random.seed(42)  # Reproducible

        # Large pools to force hash table resizing (exceeds initial 8192/4096 sizes)
        filenames = [f"file{i}.py" for i in range(200)]
        funcnames = [f"func{i}" for i in range(300)]
        thread_ids = list(range(1, 50))
        interp_ids = list(range(10))
        statuses = [
            0,
            THREAD_STATUS_HAS_GIL,
            THREAD_STATUS_ON_CPU,
            THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU,
            THREAD_STATUS_HAS_EXCEPTION,
        ]

        samples = []
        for _ in range(1000):
            num_interps = random.randint(1, 3)
            interps = []
            for _ in range(num_interps):
                iid = random.choice(interp_ids)
                num_threads = random.randint(1, 5)
                threads = []
                for _ in range(num_threads):
                    tid = random.choice(thread_ids)
                    status = random.choice(statuses)
                    depth = random.randint(1, 15)
                    frames = []
                    for _ in range(depth):
                        fname = random.choice(filenames)
                        func = random.choice(funcnames)
                        # Wide line number range to create many unique frames
                        lineno = random.randint(1, 5000)
                        frames.append(make_frame(fname, lineno, func))
                    threads.append(make_thread(tid, frames, status))
                interps.append(make_interpreter(iid, threads))
            samples.append(interps)

        collector, count = self.roundtrip(samples, compression="zstd")
        self.assertGreater(count, 0)
        self.assert_samples_equal(samples, collector)

    def test_rle_stress(self):
        """Stress test RLE encoding with identical samples."""
        random.seed(123)

        # Create a few distinct stacks
        stacks = []
        for i in range(5):
            depth = random.randint(1, 8)
            frames = [
                make_frame(f"rle{j}.py", j * 10, f"rle{j}")
                for j in range(depth)
            ]
            stacks.append(frames)

        # Generate samples with repeated stacks (should trigger RLE)
        samples = []
        for _ in range(100):
            stack = random.choice(stacks)
            repeat = random.randint(1, 50)
            for _ in range(repeat):
                samples.append([make_interpreter(0, [make_thread(1, stack)])])

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(samples))
        self.assert_samples_equal(samples, collector)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_multi_thread_stress(self):
        """Stress test with many threads and interleaved samples."""
        random.seed(456)

        thread_ids = list(range(1, 20))
        samples = []

        for i in range(300):
            # Randomly select 1-5 threads for this sample
            num_threads = random.randint(1, 5)
            selected = random.sample(thread_ids, num_threads)
            threads = []
            for tid in selected:
                status = random.choice(
                    [0, THREAD_STATUS_HAS_GIL, THREAD_STATUS_ON_CPU]
                )
                depth = random.randint(1, 5)
                frames = [
                    make_frame(f"mt{tid}_{j}.py", i + j, f"f{j}")
                    for j in range(depth)
                ]
                threads.append(make_thread(tid, frames, status))
            samples.append([make_interpreter(0, threads)])

        collector, count = self.roundtrip(samples, compression="zstd")
        self.assertGreater(count, 0)
        self.assert_samples_equal(samples, collector)

    def test_encoding_transitions_stress(self):
        """Stress test stack encoding transitions."""
        random.seed(789)

        base_frames = [
            make_frame(f"base{i}.py", i, f"base{i}") for i in range(5)
        ]
        samples = []

        for i in range(200):
            choice = random.randint(0, 4)
            if choice == 0:
                # Full new stack
                depth = random.randint(1, 8)
                frames = [
                    make_frame(f"new{i}_{j}.py", j, f"new{j}")
                    for j in range(depth)
                ]
            elif choice == 1:
                # Repeat previous (will use RLE if identical)
                frames = base_frames[: random.randint(1, 5)]
            elif choice == 2:
                # Add frames on top (suffix encoding)
                extra = random.randint(1, 3)
                frames = [
                    make_frame(f"top{i}_{j}.py", j, f"top{j}")
                    for j in range(extra)
                ]
                frames.extend(base_frames[: random.randint(2, 4)])
            else:
                # Pop and push (pop-push encoding)
                keep = random.randint(1, 3)
                push = random.randint(0, 2)
                frames = [
                    make_frame(f"push{i}_{j}.py", j, f"push{j}")
                    for j in range(push)
                ]
                frames.extend(base_frames[:keep])

            samples.append([make_interpreter(0, [make_thread(1, frames)])])

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, len(samples))
        self.assert_samples_equal(samples, collector)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_same_thread_id_multiple_interpreters_stress(self):
        """Stress test: same thread_id across multiple interpreters with interleaved samples.

        This test catches bugs where thread state is keyed only by thread_id
        without considering interpreter_id (both in writer and reader).
        """
        random.seed(999)

        # Multiple interpreters, each with overlapping thread_ids
        interp_ids = [0, 1, 2, 3]
        # Same thread_ids used across all interpreters
        shared_thread_ids = [1, 2, 3]

        filenames = [f"file{i}.py" for i in range(10)]
        funcnames = [f"func{i}" for i in range(15)]
        statuses = [0, THREAD_STATUS_HAS_GIL, THREAD_STATUS_ON_CPU]

        samples = []
        for i in range(1000):
            # Randomly pick an interpreter
            iid = random.choice(interp_ids)
            # Randomly pick 1-3 threads (from shared pool)
            num_threads = random.randint(1, 3)
            selected_tids = random.sample(shared_thread_ids, num_threads)

            threads = []
            for tid in selected_tids:
                status = random.choice(statuses)
                depth = random.randint(1, 6)
                frames = []
                for d in range(depth):
                    # Include interpreter and thread info in frame data for verification
                    fname = f"i{iid}_t{tid}_{random.choice(filenames)}"
                    func = random.choice(funcnames)
                    lineno = i * 10 + d + 1  # Unique per sample
                    frames.append(make_frame(fname, lineno, func))
                threads.append(make_thread(tid, frames, status))

            samples.append([make_interpreter(iid, threads)])

        collector, count = self.roundtrip(samples, compression="zstd")
        self.assertGreater(count, 0)
        self.assert_samples_equal(samples, collector)

        # Verify that we have samples from multiple (interpreter, thread) combinations
        # with the same thread_id
        keys = set(collector.by_thread.keys())
        # Should have samples for same thread_id in different interpreters
        for tid in shared_thread_ids:
            interps_with_tid = [iid for (iid, t) in keys if t == tid]
            self.assertGreater(
                len(interps_with_tid),
                1,
                f"Thread {tid} should appear in multiple interpreters",
            )


class TimestampCollector:
    """Collector that captures timestamps for verification."""

    def __init__(self):
        self.all_timestamps = []

    def collect(self, stack_frames, timestamps_us=None):
        if timestamps_us is not None:
            self.all_timestamps.extend(timestamps_us)

    def export(self, filename):
        pass


class TestTimestampPreservation(BinaryFormatTestBase):
    """Tests for timestamp preservation during binary round-trip."""

    def test_timestamp_preservation(self):
        """Timestamps are preserved during round-trip."""
        frame = make_frame("test.py", 10, "func")
        timestamps = [1000000, 2000000, 3000000]

        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        collector = BinaryCollector(filename, 1000, compression="none")
        for ts in timestamps:
            sample = [make_interpreter(0, [make_thread(1, [frame])])]
            collector.collect(sample, timestamp_us=ts)
        collector.export(None)

        ts_collector = TimestampCollector()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(ts_collector)

        self.assertEqual(count, 3)
        self.assertEqual(ts_collector.all_timestamps, timestamps)

    def test_timestamp_preservation_with_rle(self):
        """RLE-batched samples preserve individual timestamps."""
        frame = make_frame("rle.py", 42, "rle_func")

        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        # Identical samples (triggers RLE) with different timestamps
        collector = BinaryCollector(filename, 1000, compression="none")
        expected_timestamps = []
        for i in range(50):
            ts = 1000000 + i * 100
            expected_timestamps.append(ts)
            sample = [make_interpreter(0, [make_thread(1, [frame])])]
            collector.collect(sample, timestamp_us=ts)
        collector.export(None)

        ts_collector = TimestampCollector()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(ts_collector)

        self.assertEqual(count, 50)
        self.assertEqual(ts_collector.all_timestamps, expected_timestamps)


if __name__ == "__main__":
    unittest.main()
