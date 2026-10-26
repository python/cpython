"""Tests for binary format round-trip functionality."""

import json
import os
import pathlib
import random
import struct
import tempfile
import unittest
from unittest import mock
from collections import defaultdict

from test.support import captured_stderr, import_helper

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
        THREAD_STATUS_MAIN_THREAD,
    )
    from profiling.sampling.binary_collector import BinaryCollector
    from profiling.sampling.binary_reader import BinaryReader, convert_binary_to_format
    from profiling.sampling.constants import PROFILING_MODE_CPU
    from profiling.sampling.gecko_collector import GeckoCollector

    ZSTD_AVAILABLE = _remote_debugging.zstd_available()
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from .helpers import jsonl_tables


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


def encode_varint(value):
    """Encode an unsigned varint."""
    out = bytearray()
    while value >= 0x80:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def decode_varint(data, offset):
    """Decode an unsigned varint, returning (value, next offset)."""
    value = shift = 0
    while True:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if byte < 0x80:
            return value, offset


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

    HDR_OFF_STR_TABLE = 40
    FILE_HEADER_PLACEHOLDER_SIZE = 64

    # Sample block layout (see binary_io.h).
    BLOCK_MAGIC = b"TBLK"
    COLUMN_COUNT = 6
    COLUMN_STREAM_IDS = 1
    COLUMN_TIMESTAMP_RESIDUALS = 2
    COLUMN_STATUS_BITMAPS = 3
    COLUMN_STACK_INSTRUCTIONS = 4
    COLUMN_FRAME_IDS = 5
    COLUMN_ZSTD = 1
    COLUMN_CONSTANT = 2

    def setUp(self):
        self.temp_files = []

    def tearDown(self):
        for f in self.temp_files:
            if os.path.exists(f):
                os.unlink(f)

    def temp_filename(self):
        """Return a new temporary file name that is removed on cleanup."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)
        return filename

    def create_binary_file(self, samples, interval=1000, compression="none",
                           mode=None, capture_config=None):
        """Create a test binary file and track it for cleanup."""
        filename, _ = self.write_binary_file(
            samples, interval, compression, mode, capture_config
        )
        return filename

    def write_binary_file(self, samples, interval=1000, compression="none",
                          mode=None, capture_config=None):
        """Like create_binary_file but also returns the writer collector."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        collector = BinaryCollector(
            filename, interval, compression=compression, mode=mode,
            capture_config=capture_config,
        )
        for sample in samples:
            collector.collect(sample)
        collector.export(None)
        return filename, collector

    def read_block_columns(self, filename):
        """Return the header flags and (decoded size, encoding) per column.

        Each block is a dict with frame_width, common_status_flags,
        status_bitmap_flags and columns.
        """
        with open(filename, "rb") as raw:
            data = raw.read()
        sample_end = struct.unpack_from("=Q", data, self.HDR_OFF_STR_TABLE)[0]
        offset = self.FILE_HEADER_PLACEHOLDER_SIZE
        blocks = []
        while offset < sample_end:
            magic_end = offset + len(self.BLOCK_MAGIC)
            self.assertEqual(data[offset:magic_end], self.BLOCK_MAGIC)
            _, offset = decode_varint(data, magic_end)  # sample count
            _, offset = decode_varint(data, offset)  # base timestamp
            block = {"frame_width": data[offset],
                     "common_status_flags": data[offset + 1],
                     "status_bitmap_flags": data[offset + 2]}
            offset += 3
            columns = []
            stored_total = 0
            for _ in range(self.COLUMN_COUNT):
                decoded_size, offset = decode_varint(data, offset)
                encoding = data[offset]
                offset += 1
                if encoding == self.COLUMN_ZSTD:
                    stored_size, offset = decode_varint(data, offset)
                elif encoding == self.COLUMN_CONSTANT:
                    stored_size = 1
                else:
                    stored_size = decoded_size
                columns.append((decoded_size, encoding))
                stored_total += stored_size
            offset += stored_total
            block["columns"] = columns
            blocks.append(block)
        return blocks

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
            THREAD_STATUS_MAIN_THREAD,
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

    def test_binary_replay_preserves_main_thread_for_gecko(self):
        """Binary replay preserves main thread identity for GeckoCollector."""
        samples = [
            [
                make_interpreter(
                    0,
                    [
                        make_thread(
                            1,
                            [make_frame("main.py", 10, "main")],
                            THREAD_STATUS_MAIN_THREAD,
                        ),
                        make_thread(2, [make_frame("worker.py", 20, "worker")]),
                    ],
                )
            ]
        ]
        filename = self.create_binary_file(samples)
        collector = GeckoCollector(1000)

        with BinaryReader(filename) as reader:
            count = reader.replay_samples(collector)

        self.assertEqual(count, 2)
        profile = collector._build_profile()
        threads = {thread["tid"]: thread for thread in profile["threads"]}
        self.assertTrue(threads[1]["isMainThread"])
        self.assertFalse(threads[2]["isMainThread"])

    def test_binary_replay_starts_gecko_at_earliest_sample(self):
        """Binary replay uses the earliest sample as Gecko's time origin."""
        filename = self.temp_filename()
        collector = BinaryCollector(filename, 1000, compression="none")
        for thread_id, name, timestamp in [(1, "first", 1000),
                                           (2, "second", 2000),
                                           (2, "changed", 3000)]:
            frame = make_frame(f"{name}.py", 10, name)
            collector.collect(
                [make_interpreter(0, [make_thread(thread_id, [frame])])],
                timestamp_us=timestamp,
            )
        collector.export(None)

        gecko = GeckoCollector(1000)
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(gecko), 3)

        profile = gecko._build_profile()
        threads = {thread["tid"]: thread for thread in profile["threads"]}
        self.assertEqual(threads[1]["samples"]["time"], [0.0])
        self.assertEqual(threads[2]["samples"]["time"], [1.0, 2.0])

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

    def test_profiling_mode_preserved(self):
        filename = self.create_binary_file([], mode=PROFILING_MODE_CPU)
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.get_info()["mode"], PROFILING_MODE_CPU)

    def test_missing_profiling_mode_is_unknown(self):
        filename = self.create_binary_file([])
        with BinaryReader(filename) as reader:
            self.assertIsNone(reader.get_info()["mode"])

    def test_capture_config_preserved(self):
        capture_config = {
            "all_threads": True,
            "native": True,
            "gc": False,
            "opcodes": True,
            "blocking": False,
        }
        filename = self.create_binary_file(
            [], capture_config=capture_config
        )
        with BinaryReader(filename) as reader:
            self.assertEqual(
                reader.get_info()["capture_config"], capture_config
            )

    def test_missing_capture_config_is_unknown(self):
        filename = self.create_binary_file([])
        with BinaryReader(filename) as reader:
            self.assertIsNone(reader.get_info()["capture_config"])

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

    def test_synthetic_frames_roundtrip(self):
        """Degraded/sentinel frames (location=None) survive the binary format."""
        frames = [
            FrameInfo(("~", None, name, None))
            for name in (
                "<GC>",
                "<native>",
                "<unknown function>",
                "<unknown file>",
                "<unreadable frame>",
            )
        ]
        frames.append(FrameInfo(("app.py", None, "<unknown function>", None)))
        frames.append(FrameInfo(("<unknown file>", None, "real_func", None)))
        samples = [[make_interpreter(0, [make_thread(1, frames)])]]

        collector, count = self.roundtrip(samples)
        self.assertEqual(count, 1)
        self.assert_samples_equal(samples, collector)


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

    def test_path_arguments_round_trip(self):
        """Reader and writer accept str, bytes or os.PathLike."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        for path_arg in (filename, os.fsencode(filename), pathlib.Path(filename)):
            with self.subTest(path_type=type(path_arg).__name__):
                writer = _remote_debugging.BinaryWriter(path_arg, 1000, 0)
                writer.finalize()
                reader = _remote_debugging.BinaryReader(path_arg)
                info = reader.get_info()
                reader.close()
                self.assertEqual(info["sample_count"], 0)

    def test_rejects_non_pathlike(self):
        """Reader and writer raise TypeError on non-path-like filenames."""
        with self.assertRaises(TypeError):
            _remote_debugging.BinaryWriter(123, 1000, 0)
        with self.assertRaises(TypeError):
            _remote_debugging.BinaryReader(123)

    def test_writer_rejects_invalid_compression(self):
        """BinaryWriter rejects an unknown compression type."""
        with self.assertRaisesRegex(ValueError, "compression"):
            _remote_debugging.BinaryWriter(
                self.temp_filename(), 1000, 0, compression=2
            )

    def test_writer_rejects_status_outside_byte_range(self):
        """A status above 255 raises OverflowError and drops that sample."""
        filename = self.temp_filename()
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        valid = [make_interpreter(0, [make_thread(1, [], status=1)])]
        invalid = [make_interpreter(0, [make_thread(1, [], status=256)])]
        writer.write_sample(valid, 1000)
        with self.assertRaisesRegex(OverflowError, "thread status"):
            writer.write_sample(invalid, 2000)
        self.assertTrue(writer.limit_reached)
        writer.write_sample(valid, 3000)
        writer.finalize()
        self.assertEqual(writer.total_samples, 2)

        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(RawCollector()), 2)

    def test_writer_reports_stack_reuse(self):
        """A stack seen earlier in the block is stored as STACK_REUSE."""
        filename = self.temp_filename()
        frames = [make_frame("shared.py", line, "shared") for line in range(3)]
        sample = [make_interpreter(0, [
            make_thread(1, frames),
            make_thread(2, frames),
        ])]
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        writer.write_sample(sample, 1000)
        stats = writer.get_stats()
        self.assertEqual(stats["reuse_records"], 1)
        self.assertEqual(stats["total_records"], 2)
        self.assertEqual(stats["total_samples"], 2)
        writer.close()

    def test_writer_reuses_colliding_stacks(self):
        """Stacks sharing a cache bucket are both stored as STACK_REUSE."""
        buckets = {}
        for frame_id in range(2, 4099):
            bucket = hash(struct.pack("=3I", 0, 1, frame_id)) % 4096
            if bucket in buckets:
                first_id = buckets[bucket]
                second_id = frame_id
                break
            buckets[bucket] = frame_id

        frames = [make_frame("a.py", line, "f")
                  for line in range(second_id + 1)]
        first = [frames[0], frames[1], frames[first_id]]
        second = [frames[0], frames[1], frames[second_id]]

        filename = self.temp_filename()
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        timestamp = 1000
        # Assign frame IDs without exceeding MAX_STACK_DEPTH.
        for start in range(0, len(frames), 256):
            stack = frames[start:start + 256]
            writer.write_sample([make_interpreter(0, [make_thread(2, stack)])],
                                timestamp)
            timestamp += 1000
        for stack in (first, second, first, second):
            writer.write_sample([make_interpreter(0, [make_thread(1, stack)])],
                                timestamp)
            timestamp += 1000
        self.assertEqual(writer.get_stats()["reuse_records"], 2)
        writer.close()

    def test_writer_buffered_bytes_match_flushed_bytes(self):
        """bytes_written before finalize() matches the flushed size."""
        filename = self.temp_filename()
        frames = [make_frame("a.py", line, "f") for line in range(300)]
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        for i in range(100):
            sample = [make_interpreter(0, [
                make_thread(1, frames[i:i + 5], status=i % 3),
                make_thread(2, frames[:3]),
            ])]
            writer.write_sample(sample, 1000 * (i + 1))
        buffered = writer.get_stats()["bytes_written"]
        writer.finalize()

        [block] = self.read_block_columns(filename)
        self.assertEqual(buffered,
                         sum(size for size, _ in block["columns"]))

    def _write_block_spanning_file(self):
        # A block accepts at most 4,096 streams.
        frame = make_frame("many_threads.py", 1, "work")
        samples = [
            [make_interpreter(0, [make_thread(i + 1, [frame])])]
            for i in range(4097)
        ]
        return samples, self.create_binary_file(samples)

    def test_samples_span_multiple_blocks(self):
        """Samples filling more than one block roundtrip."""
        samples, filename = self._write_block_spanning_file()
        self.assertGreaterEqual(len(self.read_block_columns(filename)), 2)

        collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(collector), len(samples))
        self.assert_samples_equal(samples, collector)

    def test_block_size_limit_starts_new_block(self):
        """A thread whose samples exceed 1 MiB is split across blocks."""
        # Each sample writes a new 256-frame stack, about 1 KiB while
        # buffered, so 1,100 samples need more than one block.
        frames = [make_frame("deep.py", line, "f") for line in range(1356)]
        samples = [
            [make_interpreter(0, [make_thread(1, frames[i:i + 256])])]
            for i in range(1100)
        ]
        filename = self.create_binary_file(samples)
        blocks = self.read_block_columns(filename)
        self.assertGreaterEqual(len(blocks), 2)
        for block in blocks:
            self.assertLessEqual(sum(size for size, _ in block["columns"]),
                                 1024 * 1024)
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(RawCollector()),
                             len(samples))

    def test_writer_uses_constant_columns_and_narrow_frame_ids(self):
        """Repeated bytes are stored once and frame IDs use the least width."""
        frame = make_frame("steady.py", 1, "steady")
        sample = [make_interpreter(0, [make_thread(1, [frame])])]
        filename = self.temp_filename()
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        for i in range(100):
            writer.write_sample(sample, 1000 * (i + 1))
        writer.finalize()
        # One stream and on-time samples: every stream ID and residual is 0
        [block] = self.read_block_columns(filename)
        self.assertEqual(block["frame_width"], 1)
        self.assertEqual(block["columns"][self.COLUMN_STREAM_IDS],
                         (100, self.COLUMN_CONSTANT))
        self.assertEqual(block["columns"][self.COLUMN_TIMESTAMP_RESIDUALS],
                         (100, self.COLUMN_CONSTANT))

        # 300 distinct frames need two bytes per frame ID
        frames = [make_frame("wide.py", line, "f") for line in range(300)]
        samples = [
            [make_interpreter(0, [make_thread(1, frames[:150])])],
            [make_interpreter(0, [make_thread(1, frames[150:])])],
        ]
        filename = self.create_binary_file(samples)
        [block] = self.read_block_columns(filename)
        self.assertEqual(block["frame_width"], 2)
        self.assertEqual(block["columns"][self.COLUMN_FRAME_IDS][0], 300 * 2)

    def test_writer_stores_common_status_bits_once(self):
        """Status bits set in every sample go in the header, not a bitmap."""
        frame = make_frame("status.py", 1, "f")
        filename = self.temp_filename()
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        for i in range(20):
            # HAS_GIL in every sample; ON_CPU in every other one
            status = THREAD_STATUS_HAS_GIL | (THREAD_STATUS_ON_CPU * (i % 2))
            sample = [make_interpreter(0, [make_thread(1, [frame], status)])]
            writer.write_sample(sample, 1000 * (i + 1))
        writer.finalize()
        [block] = self.read_block_columns(filename)
        self.assertEqual(block["common_status_flags"], THREAD_STATUS_HAS_GIL)
        self.assertEqual(block["status_bitmap_flags"], THREAD_STATUS_ON_CPU)
        # One bitmap of ceil(20 / 8) bytes, for ON_CPU only
        self.assertEqual(block["columns"][self.COLUMN_STATUS_BITMAPS][0], 3)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_writer_compresses_repetitive_columns(self):
        """Compressed files store repetitive columns with zstd."""
        root = [make_frame("root.py", line, "root") for line in range(20)]
        leaves = [make_frame("leaf.py", 1, "even"),
                  make_frame("leaf.py", 2, "odd")]
        samples = [
            [make_interpreter(0, [make_thread(1, [leaves[i % 2]] + root)])]
            for i in range(2000)
        ]
        filename = self.create_binary_file(samples, compression="zstd")
        [block] = self.read_block_columns(filename)
        columns = block["columns"]
        self.assertEqual(columns[self.COLUMN_STACK_INSTRUCTIONS][1],
                         self.COLUMN_ZSTD)
        self.assertEqual(columns[self.COLUMN_FRAME_IDS][1], self.COLUMN_ZSTD)

    def test_replay_byte_swapped_file(self):
        """A file written with the other byte order replays the same."""
        samples = [[make_interpreter(0, [
            make_thread(1, [make_frame("a.py", 1, "f")]),
            make_thread(2, [make_frame("b.py", 2, "g")]),
        ])] for _ in range(3)]
        filename = self.temp_filename()
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        for i, sample in enumerate(samples):
            writer.write_sample(sample, 1000 * (i + 1))
        writer.finalize()

        data = bytearray(pathlib.Path(filename).read_bytes())
        # Header fields: magic, version, start time, interval, sample
        # count, thread count, table offsets, compression and config
        fields = [(0, 4), (4, 4), (12, 8), (20, 8), (28, 8), (36, 4),
                  (40, 8), (48, 8), (56, 4), (60, 4)]
        # Footer string and frame counts
        fields += [(len(data) - 32, 4), (len(data) - 28, 4)]
        # The block's stream table entries follow its header
        offset = self.FILE_HEADER_PLACEHOLDER_SIZE + len(self.BLOCK_MAGIC)
        _, offset = decode_varint(data, offset)  # sample count
        _, offset = decode_varint(data, offset)  # base timestamp
        offset += 3
        for _ in range(self.COLUMN_COUNT):
            _, offset = decode_varint(data, offset)
            offset += 1  # raw encoding, no stored size
        for entry in range(2):
            fields += [(offset + entry * 12, 8), (offset + entry * 12 + 8, 4)]
        for start, size in fields:
            data[start:start + size] = data[start:start + size][::-1]
        pathlib.Path(filename).write_bytes(data)

        collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(collector), 6)
        self.assert_samples_equal(samples, collector)

    def test_progress_updates_across_blocks(self):
        """Progress is reported at fixed intervals across block boundaries."""
        samples, filename = self._write_block_spanning_file()
        calls = []
        with BinaryReader(filename) as reader:
            reader.replay_samples(
                RawCollector(),
                lambda current, total: calls.append((current, total)))
        total = len(samples)
        self.assertEqual(
            calls,
            [(0, total), (1000, total), (2000, total), (3000, total),
             (4000, total), (total, total)],
        )

    def test_reader_replays_twice(self):
        """Replaying twice gives the same samples and statistics."""
        class RetainingCollector(RawCollector):
            def __init__(self):
                super().__init__()
                self.frame_lists = []

            def collect(self, stack_frames, timestamps_us):
                self.frame_lists.extend(
                    thread.frame_info
                    for interp in stack_frames
                    for thread in interp.threads
                )
                super().collect(stack_frames, timestamps_us)

        samples = [
            [make_interpreter(0, [
                make_thread(1, [make_frame("a.py", i % 3, "f")]),
                make_thread(2, [make_frame("b.py", 1, "g")]),
            ])]
            for i in range(10)
        ]
        filename = self.create_binary_file(samples)
        with BinaryReader(filename) as reader:
            first = RetainingCollector()
            second = RetainingCollector()
            self.assertEqual(reader.replay_samples(first), 20)
            self.assertEqual(reader.replay_samples(second), 20)
            self.assertEqual(reader.get_stats()["total_samples"], 20)
        self.assertEqual(first.by_thread, second.by_thread)
        for first_frames, second_frames in zip(
            first.frame_lists, second.frame_lists, strict=True
        ):
            self.assertIsNot(first_frames, second_frames)
            self.assertEqual(len(first_frames), len(second_frames))
            for first_frame, second_frame in zip(
                first_frames, second_frames, strict=True
            ):
                self.assertIs(first_frame, second_frame)

    def test_writer_repeat_run_spans_interleaved_streams(self):
        """A thread's repeat run continues across other threads' samples."""
        filename = self.temp_filename()
        idle = [make_frame("idle.py", 1, "wait")]
        busy = [[make_frame("busy.py", 1, "even")],
                [make_frame("busy.py", 2, "odd")]]
        writer = _remote_debugging.BinaryWriter(filename, 1000, 0)
        for i in range(10):
            sample = [make_interpreter(0, [
                make_thread(1, idle),
                make_thread(2, busy[i % 2]),
            ])]
            writer.write_sample(sample, 1000 * (i + 1))
        stats = writer.get_stats()
        self.assertEqual(stats["repeat_records"], 1)
        self.assertEqual(stats["repeat_samples"], 9)
        writer.finalize()

        collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(collector), 20)
            self.assertEqual(reader.get_stats()["repeat_samples"], 9)
        idle_frames = [frame_to_dict(frame) for frame in idle]
        self.assertEqual(
            [sample["frames"] for sample in collector.by_thread[(0, 1)]],
            [idle_frames] * 10)
        self.assertEqual(
            [sample["frames"][0]["funcname"]
             for sample in collector.by_thread[(0, 2)]],
            ["even", "odd"] * 5)

    def test_invalid_path_error_preserves_pathlib(self):
        """Missing path: OSError carries the original path object, not a string."""
        missing = pathlib.Path("/i/do/not/exist")
        with self.assertRaises(FileNotFoundError) as cm:
            _remote_debugging.BinaryReader(missing)
        self.assertEqual(os.fspath(cm.exception.filename), os.fspath(missing))

    def test_writer_handles_empty_stack_first_sample(self):
        """BinaryWriter.write_sample tolerates an empty stack on a fresh thread.

        Regression test for gh-149342: an empty first stack equals the empty
        stack a new thread starts from, but it must still be written as a
        stack (STACK_REPLACE_SUFFIX 0 0) rather than a repeat. Goes through
        BinaryWriter.write_sample directly so the test cannot be masked by
        any Python-level filtering.
        """
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        writer = _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0)
        empty_sample = [
            make_interpreter(
                0, [make_thread(99, [], status=THREAD_STATUS_UNKNOWN)]
            )
        ]
        # First sample for a fresh thread has empty frame_info — the exact
        # scenario that exposes the bug.
        writer.write_sample(empty_sample, 1000)
        writer.write_sample(empty_sample, 2000)
        # Mix in a real sample after the repeated empty stack.
        real_sample = [
            make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]
        writer.write_sample(real_sample, 3000)
        writer.finalize()

        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(reader_collector)
        # The empty stack is written once and then repeated; the file must
        # replay all three samples.
        self.assertEqual(count, 3)

    def test_writer_handles_mixed_empty_and_real_first_sample(self):
        """First sample with one empty + one real thread roundtrips through C."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        writer = _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0)
        sample = [
            make_interpreter(
                0,
                [
                    make_thread(1, [make_frame("a.py", 1, "f")]),
                    make_thread(99, [], status=THREAD_STATUS_UNKNOWN),
                ],
            )
        ]
        # Two samples so RLE state is exercised.
        writer.write_sample(sample, 1000)
        writer.write_sample(sample, 2000)
        writer.finalize()

        # Replay must succeed without raising RuntimeError, and the real
        # thread's frames must round-trip.
        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            reader.replay_samples(reader_collector)
        self.assertIn((0, 1), reader_collector.by_thread)
        self.assertEqual(len(reader_collector.by_thread[(0, 1)]), 2)

    def test_writer_total_samples_after_finalize_matches_reader(self):
        """BinaryWriter.total_samples after finalize() matches the reader's count."""
        # Five IDENTICAL samples stay in the open block, as one repeat, until
        # finalize() flushes it; total_samples must still count all of them.
        # Keep the samples identical to preserve coverage. See gh-149342.
        samples = [
            [make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])]
        ] * 5
        filename, writer_collector = self.write_binary_file(samples)
        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            replayed = reader.replay_samples(reader_collector)
        self.assertEqual(writer_collector.total_samples, len(samples))
        self.assertEqual(writer_collector.total_samples, replayed)

    def test_open_block_flushes_on_finalize(self):
        """finalize() writes the open block before the writer is destroyed."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        frame = make_frame("rle_boundary.py", 1, "hot")
        sample = [make_interpreter(0, [make_thread(1, [frame])])]

        writer = _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0)
        for i in range(10_000):
            writer.write_sample(sample, i * 1000)
        self.assertGreater(writer.get_stats()["repeat_records"], 0)

        writer.finalize()

        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            replayed = reader.replay_samples(reader_collector)

        self.assertEqual(replayed, 10_000)
        self.assertEqual(writer.total_samples, 10_000)

    def test_writer_total_samples_after_context_manager_matches_reader(self):
        """total_samples after `with BinaryWriter(...)` matches the reader's count.

        Regression for the asymmetry between finalize() and __exit__ in
        module.c: __exit__ also calls binary_writer_finalize and must
        preserve cached_total_samples like finalize() does, otherwise the
        getter returns 0 once self->writer is NULL.
        """
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        sample = [
            make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]
        with _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0) as w:
            for i in range(5):
                w.write_sample(sample, i * 1000)
        self.assertEqual(w.total_samples, 5)

        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(reader_collector), 5)

    def test_writer_total_samples_after_close_returns_zero(self):
        """close() discards data; total_samples reflects no cached count."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        w = _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0)
        sample = [
            make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]
        for i in range(5):
            w.write_sample(sample, i * 1000)
        w.close()
        self.assertEqual(w.total_samples, 0)

    def test_binary_collector_stops_gracefully_on_overflow(self):
        """OverflowError from the writer stops collection via the running
        protocol instead of propagating and corrupting the file.
        See gh-151292."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        collector = BinaryCollector(filename, 1000, compression="none")
        self.assertTrue(collector.running)

        sample = [
            make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]

        # Collect real samples first, then hit the limit.
        for i in range(3):
            collector.collect(sample, timestamp_us=(i + 1) * 1000)
        self.assertTrue(collector.running)

        bad = [make_interpreter(2**32, sample[0].threads)]
        with captured_stderr() as stderr:
            collector.collect(bad, timestamp_us=4000)
            collector.collect(sample, timestamp_us=5000)

        self.assertFalse(collector.running)
        self.assertTrue(collector._writer.limit_reached)
        self.assertEqual(stderr.getvalue().count("Warning:"), 1)
        self.assertIn("interpreter_id", stderr.getvalue())

        collector.export(None)

        self.assertEqual(collector.total_samples, 3)

        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(reader_collector), 3)

    def test_interpreter_id_overflow_rejected(self):
        """An interpreter_id wider than u32 raises OverflowError before any
        writer state is mutated: subsequent valid samples are still accepted
        and finalize produces a readable file."""
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        good = [
            make_interpreter(0, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]
        bad = [
            make_interpreter(2**32, [make_thread(1, [make_frame("a.py", 1, "f")])])
        ]

        writer = _remote_debugging.BinaryWriter(filename, 1000, 0, compression=0)
        writer.write_sample(good, 1000)
        with self.assertRaises(OverflowError):
            writer.write_sample(bad, 2000)
        writer.write_sample(good, 3000)
        writer.finalize()
        self.assertEqual(writer.total_samples, 2)

        reader_collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(reader_collector), 2)

    def test_writer_finalizes_after_format_limit(self):
        for compression in (0, 1) if ZSTD_AVAILABLE else (0,):
            with self.subTest(compression=compression):
                with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
                    filename = f.name
                self.temp_files.append(filename)
                good = [make_interpreter(0, [
                    make_thread(1, [make_frame("a.py", 1, "f")])
                ])]
                bad = [make_interpreter(2**32, good[0].threads)]
                writer = _remote_debugging.BinaryWriter(
                    filename, 1000, 0, compression=compression
                )
                with self.assertRaises(OverflowError):
                    with writer:
                        writer.write_sample(good, 1000)
                        writer.write_sample(good, 2000)
                        # The first interpreter is committed before the limit.
                        writer.write_sample(good + bad, 3000)
                self.assertEqual(writer.total_samples, 3)
                with BinaryReader(filename) as reader:
                    self.assertEqual(reader.replay_samples(RawCollector()), 3)

    def test_collector_does_not_swallow_unrelated_overflow(self):
        class BadStatus:
            def __index__(self):
                raise OverflowError("status conversion failed")

        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)
        collector = BinaryCollector(filename, 1000, compression="none")
        self.addCleanup(collector._writer.close)
        sample = [make_interpreter(0, [make_thread(1, [], BadStatus())])]
        with captured_stderr() as stderr:
            with self.assertRaisesRegex(OverflowError, "status conversion failed"):
                collector.collect(sample, timestamp_us=1000)
        self.assertEqual(stderr.getvalue(), "")
        self.assertFalse(collector._writer.limit_reached)
        with self.assertRaisesRegex(ValueError, "broken"):
            collector.export()
        with self.assertRaisesRegex(ValueError, "broken"):
            collector._writer.write_sample([], 2000)
        # Closing a broken writer must not attempt to finalize it.
        collector.__exit__(None, None, None)

    def test_collector_finalizes_after_external_exception(self):
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)
        with self.assertRaisesRegex(RuntimeError, "sampling failed"):
            with BinaryCollector(filename, 1000, compression="none") as collector:
                collector.collect([make_interpreter(0, [make_thread(1, [])])])
                raise RuntimeError("sampling failed")
        self.assertEqual(collector.total_samples, 1)
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(RawCollector()), 1)

    @unittest.skipUnless(os.path.exists("/dev/full"), "requires /dev/full")
    def test_finalize_failure_breaks_writer(self):
        writer = _remote_debugging.BinaryWriter("/dev/full", 1000, 0)
        self.addCleanup(writer.close)
        writer.write_sample([make_interpreter(0, [make_thread(1, [])])], 1000)
        with self.assertRaises(OSError):
            writer.finalize()
        self.assertFalse(writer.limit_reached)
        with self.assertRaisesRegex(ValueError, "broken"):
            writer.finalize()
        with self.assertRaisesRegex(ValueError, "broken"):
            writer.write_sample([], 2000)


class TestBinaryFormatValidation(BinaryFormatTestBase):
    """Tests for malformed binary files."""

    HDR_OFF_VERSION = 4
    HDR_OFF_SAMPLES = 28
    HDR_OFF_THREADS = 36
    HDR_OFF_FRAME_TABLE = 48
    FILE_FOOTER_SIZE = 32
    FTR_OFF_STRINGS = 0
    FTR_OFF_FRAMES = 4

    def test_replay_rejects_more_threads_than_declared(self):
        """Replay rejects files with more unique threads than the header declares."""
        threads = [
            make_thread(1, [make_frame("t1.py", 10, "t1")]),
            make_thread(2, [make_frame("t2.py", 20, "t2")]),
        ]
        samples = [[make_interpreter(0, threads)]]
        filename = self.create_binary_file(samples, compression="none")

        with open(filename, "r+b") as raw:
            raw.seek(self.HDR_OFF_THREADS)
            raw.write(struct.pack("=I", 1))

        with BinaryReader(filename) as reader:
            self.assertEqual(reader.get_info()["thread_count"], 1)
            with self.assertRaises(ValueError) as cm:
                reader.replay_samples(RawCollector())
            self.assertEqual(
                str(cm.exception),
                "Invalid thread count: sample data contains more unique "
                "threads than declared in header (declared 1, found at least 2)",
            )

    def test_replay_rejects_sample_count_mismatch(self):
        """Replay rejects files whose decoded samples disagree with the header."""
        samples = [[make_interpreter(0, [
            make_thread(1, [make_frame("sample.py", 10, "sample")])
        ])]]
        filename = self.create_binary_file(samples, compression="none")

        with open(filename, "r+b") as raw:
            raw.seek(self.HDR_OFF_SAMPLES)
            raw.write(struct.pack("=Q", 2))

        with BinaryReader(filename) as reader:
            self.assertEqual(reader.get_info()["sample_count"], 2)
            with self.assertRaises(ValueError) as cm:
                reader.replay_samples(RawCollector())
            self.assertEqual(
                str(cm.exception),
                "Sample count mismatch: header declares 2 samples "
                "but replay decoded 1",
            )

    def test_replay_rejects_trailing_partial_sample_header(self):
        """Replay rejects partial sample bytes instead of silently stopping."""
        filename = self.create_binary_file([], compression="none")
        sample_data_end = self.FILE_HEADER_PLACEHOLDER_SIZE + 1

        with open(filename, "r+b") as raw:
            raw.seek(self.HDR_OFF_STR_TABLE)
            raw.write(struct.pack("=Q", sample_data_end))
            raw.seek(self.HDR_OFF_FRAME_TABLE)
            raw.write(struct.pack("=Q", sample_data_end))

        with BinaryReader(filename) as reader:
            with self.assertRaises(ValueError) as cm:
                reader.replay_samples(RawCollector())
            self.assertEqual(str(cm.exception),
                             "Invalid sample block: truncated header")

    # Minimum on-disk size of one table entry (see binary_io.h).
    MIN_STRING_ENTRY_SIZE = 1
    MIN_FRAME_ENTRY_SIZE = 7

    def _read_offset(self, filename, hdr_off):
        with open(filename, "rb") as raw:
            raw.seek(hdr_off)
            return struct.unpack("=Q", raw.read(8))[0]

    def _patch_footer_count(self, filename, ftr_off, value):
        size = os.path.getsize(filename)
        with open(filename, "r+b") as raw:
            raw.seek(size - self.FILE_FOOTER_SIZE + ftr_off)
            raw.write(struct.pack("=I", value))

    def test_open_rejects_string_count_larger_than_file(self):
        """Open rejects a footer string count larger than the file."""
        samples = [[make_interpreter(0, [
            make_thread(1, [make_frame("s.py", 10, "s")])
        ])]]
        filename = self.create_binary_file(samples, compression="none")
        size = os.path.getsize(filename)
        str_off = self._read_offset(filename, self.HDR_OFF_STR_TABLE)
        max_strings = (size - str_off) // self.MIN_STRING_ENTRY_SIZE
        self._patch_footer_count(filename, self.FTR_OFF_STRINGS, 0xFFFFFFFF)

        with self.assertRaises(ValueError) as cm:
            with BinaryReader(filename):
                pass
        self.assertEqual(
            str(cm.exception),
            f"Invalid string count 4294967295 exceeds maximum "
            f"possible {max_strings}",
        )

    def test_open_rejects_frame_count_larger_than_file(self):
        """Open rejects a footer frame count larger than the file."""
        samples = [[make_interpreter(0, [
            make_thread(1, [make_frame("f.py", 10, "f")])
        ])]]
        filename = self.create_binary_file(samples, compression="none")
        size = os.path.getsize(filename)
        frame_off = self._read_offset(filename, self.HDR_OFF_FRAME_TABLE)
        max_frames = (size - frame_off) // self.MIN_FRAME_ENTRY_SIZE
        self._patch_footer_count(filename, self.FTR_OFF_FRAMES, 0xFFFFFFFF)

        # On a 32-bit build this count overflows the allocation size, so the
        # size_t overflow guard (OverflowError) fires before the count check.
        with self.assertRaises((ValueError, OverflowError)) as cm:
            with BinaryReader(filename):
                pass
        if isinstance(cm.exception, ValueError):
            self.assertEqual(
                str(cm.exception),
                f"Invalid frame count 4294967295 exceeds maximum "
                f"possible {max_frames}",
            )

    def test_open_accepts_frame_count_at_capacity_boundary(self):
        """A frame count at the file-size cap opens; one more is rejected."""
        samples = [[make_interpreter(0, [
            make_thread(1, [make_frame("f.py", 10, "f")])
        ])]]
        filename = self.create_binary_file(samples, compression="none")
        size = os.path.getsize(filename)
        frame_off = self._read_offset(filename, self.HDR_OFF_FRAME_TABLE)
        max_frames = (size - frame_off) // self.MIN_FRAME_ENTRY_SIZE

        self._patch_footer_count(filename, self.FTR_OFF_FRAMES, max_frames)
        with BinaryReader(filename):
            pass

        self._patch_footer_count(filename, self.FTR_OFF_FRAMES, max_frames + 1)
        with self.assertRaises(ValueError) as cm:
            with BinaryReader(filename):
                pass
        self.assertEqual(
            str(cm.exception),
            f"Invalid frame count {max_frames + 1} exceeds maximum "
            f"possible {max_frames}",
        )

    def test_open_rejects_older_format_version(self):
        """Open rejects version 1 files and reports version 2."""
        filename = self.create_binary_file([], compression="none")
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.get_info()["version"], 2)
        with open(filename, "r+b") as raw:
            raw.seek(self.HDR_OFF_VERSION)
            raw.write(struct.pack("=I", 1))
        with self.assertRaisesRegex(
                ValueError,
                r"Unsupported format version 1 \(this reader supports "
                r"version 2\)"):
            with BinaryReader(filename):
                pass

    def test_sample_count_reads_full_64_bits(self):
        """sample_count values requiring the upper 32 bits decode correctly."""
        filename = self.create_binary_file([], compression="none")
        big_count = 0x1_0002_0003

        with open(filename, "r+b") as raw:
            raw.seek(self.HDR_OFF_SAMPLES)
            raw.write(struct.pack("=Q", big_count))

        with BinaryReader(filename) as reader:
            self.assertEqual(reader.get_info()["sample_count"], big_count)

    def test_sample_count_boundary_values(self):
        """Values above the old u32 ceiling decode fine."""
        filename = self.create_binary_file([], compression="none")

        for value in (0xFFFFFFFF - 1, 0xFFFFFFFF, 0xFFFFFFFF + 1):
            with self.subTest(value=value):
                with open(filename, "r+b") as raw:
                    raw.seek(self.HDR_OFF_SAMPLES)
                    raw.write(struct.pack("=Q", value))

                with BinaryReader(filename) as reader:
                    self.assertEqual(reader.get_info()["sample_count"], value)

    # Block column indices and encodings (see binary_io.h).
    MAX_BLOCK_SIZE = 1024 * 1024

    def _write_single_block(self, num_samples=1, instructions=b"\x01\x00\x00",
                            frame_ids=b"", *, header_samples=None,
                            file_samples=None, stream_count=1,
                            stream_table=None, stream_ids=None,
                            residuals=None, statuses=b"", status_flags=(0, 0),
                            frame_width=1, descriptors=None, magic=b"TBLK",
                            truncate=None, frame_count=1):
        """Write a file whose only block is built from the given parts.

        By default every sample belongs to stream 0, has status 0 and is
        exactly one sample interval after the previous one, and the only
        stack is empty (STACK_REPLACE_SUFFIX 0 0). The frame table holds
        *frame_count* frames: frame 0 is in "a.py" and frame N in "N.py".
        *descriptors* maps a column index to (encoding, decoded size) to
        replace its raw descriptor.
        """
        frames = [make_frame("a.py", 1, "f")]
        frames += [make_frame(f"{index}.py", 1, "f")
                   for index in range(1, frame_count)]
        samples = [[make_interpreter(0, [make_thread(1, frames[i:i + 256])])]
                   for i in range(0, frame_count, 256)]
        filename = self.create_binary_file(samples, compression="none")
        data = bytearray(pathlib.Path(filename).read_bytes())
        sample_end = struct.unpack_from("=Q", data, self.HDR_OFF_STR_TABLE)[0]
        if stream_table is None:
            stream_table = b"".join(struct.pack("=QI", thread, 0)
                                    for thread in range(1, stream_count + 1))
        columns = (
            stream_table,
            b"\x00" * num_samples if stream_ids is None else stream_ids,
            b"\x00" * num_samples if residuals is None else residuals,
            statuses,
            instructions,
            frame_ids,
        )
        if header_samples is None:
            header_samples = num_samples
        block = (magic + encode_varint(header_samples)
                 + b"\x01"  # base timestamp
                 + bytes([frame_width, *status_flags]))
        payload = b""
        for index, column in enumerate(columns):
            encoding, decoded_size = (descriptors or {}).get(
                index, (0, len(column)))
            block += encode_varint(decoded_size) + bytes([encoding])
            if encoding == self.COLUMN_ZSTD:
                block += encode_varint(len(column))
            if encoding == self.COLUMN_CONSTANT:
                column = column[:1]
            payload += column
        block = (block + payload)[:truncate]
        data[self.FILE_HEADER_PLACEHOLDER_SIZE:sample_end] = block
        shift = len(block) - (sample_end - self.FILE_HEADER_PLACEHOLDER_SIZE)
        if file_samples is None:
            file_samples = num_samples
        struct.pack_into("=Q", data, self.HDR_OFF_SAMPLES, file_samples)
        struct.pack_into("=I", data, self.HDR_OFF_THREADS,
                         max(len(stream_table) // 12, 1))
        for offset in (self.HDR_OFF_STR_TABLE, self.HDR_OFF_FRAME_TABLE):
            old_offset = struct.unpack_from("=Q", data, offset)[0]
            struct.pack_into("=Q", data, offset, old_offset + shift)
        struct.pack_into("=Q", data, len(data) - 24, len(data))
        pathlib.Path(filename).write_bytes(data)
        return filename

    def assert_block_rejected(self, filename, message):
        with BinaryReader(filename) as reader:
            with self.assertRaises(ValueError) as cm:
                reader.replay_samples(RawCollector())
        self.assertEqual(str(cm.exception), f"Invalid sample block: {message}")

    def test_replay_accepts_hand_built_block(self):
        """Replay accepts a block built by hand from its columns."""
        # STACK_REPLACE_SUFFIX 0 0, then STACK_REPEAT 2.
        filename = self._write_single_block(3, b"\x01\x00\x00\x00\x02")
        collector = RawCollector()
        with BinaryReader(filename) as reader:
            self.assertEqual(reader.replay_samples(collector), 3)
        self.assertEqual(len(collector.by_thread[(0, 1)]), 3)

    def test_replay_decodes_every_frame_id_width(self):
        """Frame IDs decode correctly at every byte width."""
        # STACK_REPLACE_SUFFIX 0 1 reads one frame ID, stored in one byte
        # plane per byte of the width, low byte first.
        for width, frame_id in ((1, 43), (2, 299), (3, 299), (4, 299)):
            with self.subTest(width=width):
                filename = self._write_single_block(
                    1, b"\x01\x00\x01", frame_id.to_bytes(width, "little"),
                    frame_width=width, frame_count=300)
                collector = RawCollector()
                with BinaryReader(filename) as reader:
                    self.assertEqual(reader.replay_samples(collector), 1)
                frames = collector.by_thread[(0, 1)][0]["frames"]
                self.assertEqual([frame["filename"] for frame in frames],
                                 [f"{frame_id}.py"])

    def test_replay_rejects_reentrant_replay(self):
        """Replaying from inside the same reader's collector raises."""
        filename = self._write_single_block(
            2, b"\x01\x00\x00\x00\x01")  # REPEAT 1

        with BinaryReader(filename) as reader:
            class ReentrantCollector:
                def collect(self, stack_frames, timestamps_us):
                    reader.replay_samples(RawCollector())

            with self.assertRaisesRegex(RuntimeError, "already replaying"):
                reader.replay_samples(ReentrantCollector())
            self.assertEqual(reader.replay_samples(RawCollector()), 2)

    def test_replay_rejects_invalid_block(self):
        """Replay rejects every kind of malformed block."""
        frames = self.COLUMN_FRAME_IDS
        cases = [
            ("wrong magic", dict(magic=b"XBLK")),
            # Magic, sample count and base timestamp, then 1 of 3 flag bytes
            ("truncated header", dict(truncate=7)),
            ("no samples", dict(header_samples=0)),
            ("frame ID width out of range", dict(frame_width=0)),
            ("frame ID width out of range", dict(frame_width=5)),
            ("common and varying status flags overlap",
             dict(status_flags=(1, 1))),
            # Magic, sample count, base timestamp, 3 flag bytes and the
            # first decoded size
            ("truncated column descriptor", dict(truncate=10)),
            ("unknown column encoding", dict(descriptors={frames: (3, 0)})),
            ("zstd column in an uncompressed file",
             dict(descriptors={self.COLUMN_STACK_INSTRUCTIONS:
                               (self.COLUMN_ZSTD, 3)})),
            ("encoded column is empty",
             dict(descriptors={frames: (self.COLUMN_CONSTANT, 0)})),
            ("decoded columns exceed the block limit",
             dict(frame_ids=b"\x00",
                  descriptors={frames: (self.COLUMN_CONSTANT,
                                        self.MAX_BLOCK_SIZE)})),
            # Rejected before anything is allocated for the samples
            ("more samples than stored stream indices",
             dict(header_samples=0xFFFFFFFF)),
            ("column extends past the sample section", dict(truncate=-1)),
            ("stream table has a partial entry", dict(stream_table=b"0" * 11)),
            ("stream count out of range", dict(stream_table=b"")),
            ("frame ID column has a partial value",
             dict(frame_width=2, frame_ids=b"\x00")),
            ("status bitmaps have the wrong size", dict(status_flags=(0, 1))),
            ("status bitmap padding is not zero",
             dict(status_flags=(0, 1), statuses=b"\x02")),
            ("stream table repeats a thread",
             dict(stream_table=struct.pack("=QI", 1, 0) * 2)),
            ("sample refers to an unknown stream", dict(stream_ids=b"\x01")),
            ("stream index column has trailing data",
             dict(num_samples=2, header_samples=1)),
            ("timestamp column has trailing data",
             dict(residuals=b"\x00\x00")),
            ("frame index out of range",
             dict(instructions=b"\x01\x00\x01", frame_ids=b"\x05")),
            # Frame IDs 65536 and 2**24 only have a byte in the third or
            # fourth plane
            ("frame index out of range",
             dict(instructions=b"\x01\x00\x01", frame_width=3,
                  frame_ids=(1 << 16).to_bytes(3, "little"),
                  frame_count=300)),
            ("frame index out of range",
             dict(instructions=b"\x01\x00\x01", frame_width=4,
                  frame_ids=(1 << 24).to_bytes(4, "little"),
                  frame_count=300)),
            ("unknown stack instruction", dict(instructions=b"\x03")),
            ("first stack of a stream cannot repeat",
             dict(instructions=b"\x00\x01")),
            ("repeat count out of range",
             dict(num_samples=2, instructions=b"\x01\x00\x00\x00\x00")),
            ("stack prefix out of range", dict(instructions=b"\x01\x01\x00")),
            ("stack uses more frame IDs than stored",
             dict(instructions=b"\x01\x00\x02", frame_ids=b"\x00")),
            # STACK_REUSE 0 257
            ("stack is too deep", dict(instructions=b"\x02\x00\x81\x02")),
            ("reused stack is not stored earlier",
             dict(instructions=b"\x02\x00\x01", frame_ids=b"\x00")),
            ("stack instruction column has trailing data",
             dict(instructions=b"\x01\x00\x00\x01")),
            ("frame ID column has unused values", dict(frame_ids=b"\x00")),
            ("stream table has an unused entry", dict(stream_count=2)),
            ("repeat count extends past the block",
             dict(num_samples=2, instructions=b"\x01\x00\x00\x00\x02")),
            ("more samples than the file header declares",
             dict(file_samples=0)),
        ]
        for message, kwargs in cases:
            with self.subTest(message, **kwargs):
                filename = self._write_single_block(**kwargs)
                self.assert_block_rejected(filename, message)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_replay_rejects_invalid_zstd_column(self):
        """Replay rejects corrupt and mismatched zstd columns."""
        zstd = import_helper.import_module("compression.zstd")
        instructions = b"\x01\x00\x00"
        frame = zstd.compress(
            instructions,
            options={zstd.CompressionParameter.checksum_flag: 1})
        bad_checksum = frame[:-1] + bytes([frame[-1] ^ 1])
        cases = [
            ("Corrupted zstd frame header", instructions, len(instructions)),
            ("zstd frame size does not match the decoded size",
             frame, len(instructions) + 1),
            ("zstd decompression error: ", bad_checksum, len(instructions)),
        ]
        for message, stored, decoded_size in cases:
            with self.subTest(message):
                filename = self._write_single_block(
                    instructions=stored,
                    descriptors={self.COLUMN_STACK_INSTRUCTIONS:
                                 (self.COLUMN_ZSTD, decoded_size)})
                with open(filename, "r+b") as raw:
                    raw.seek(56)  # compression type
                    raw.write(struct.pack("=I", 1))
                with BinaryReader(filename) as reader:
                    with self.assertRaises(ValueError) as cm:
                        reader.replay_samples(RawCollector())
                self.assertStartsWith(str(cm.exception), message)

    def test_replay_propagates_callback_errors(self):
        """Errors raised by the collector or progress callback propagate."""
        filename = self._write_single_block(
            2000, b"\x01\x00\x00\x00\xcf\x0f")  # REPEAT 1999

        class FailingCollector:
            def collect(self, stack_frames, timestamps_us):
                raise ZeroDivisionError

        def failing_progress(current, total):
            if current:
                raise ZeroDivisionError

        with BinaryReader(filename) as reader:
            with self.assertRaises(ZeroDivisionError):
                reader.replay_samples(FailingCollector())
            with self.assertRaises(ZeroDivisionError):
                reader.replay_samples(RawCollector(), failing_progress)
            self.assertEqual(reader.replay_samples(RawCollector()), 2000)


class TestBinaryEncodings(BinaryFormatTestBase):
    """Tests specifically targeting different stack encodings."""

    def test_stack_first_sample_pattern(self):
        """First sample stores its complete stack and roundtrips."""
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

    def test_stack_shared_outer_frames_pattern(self):
        """Samples keeping outer frames roundtrip (STACK_REPLACE_SUFFIX)."""
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

    def test_stack_pop_push_pattern(self):
        """Samples that pop frames and push new ones roundtrip."""
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

    def test_rle_alternating_status_batches_correctly(self):
        """A repeat record whose status alternates every sample replays as N
        single-status batches with the right cumulative timestamps."""
        class BatchCollector:
            def __init__(self):
                self.batches = []

            def collect(self, stack_frames, timestamps_us):
                for interp in stack_frames:
                    for thread in interp.threads:
                        self.batches.append(
                            (thread.status, list(timestamps_us))
                        )

            def export(self, filename):
                pass

        num_samples = 2000
        frame = make_frame("rle.py", 42, "rle_func")
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
            filename = f.name
        self.temp_files.append(filename)

        writer = BinaryCollector(filename, 1000, compression="none")
        expected = []
        for i in range(num_samples):
            status = THREAD_STATUS_HAS_GIL if i % 2 else 0
            ts = 1000 + i
            expected.append((status, [ts]))
            sample = [
                make_interpreter(0, [make_thread(1, [frame], status)])
            ]
            writer.collect(sample, timestamp_us=ts)
        writer.export(None)

        collector = BatchCollector()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(collector)

        self.assertEqual(count, num_samples)
        self.assertEqual(len(collector.batches), num_samples)
        self.assertEqual(collector.batches, expected)


    def test_rle_long_run_splits_batches(self):
        # Construct one block whose repeat run is longer than a replay batch.
        num_samples = 8194
        filename = self.create_binary_file([], compression="none")
        data = bytearray(pathlib.Path(filename).read_bytes())
        instructions = (b"\x01\x00\x00"  # STACK_REPLACE_SUFFIX 0 0
                        + b"\x00\x81\x40")  # STACK_REPEAT 8193
        record = (b"TBLK"
                  + b"\x82\x40"  # 8194 samples as a varint
                  + b"\x01"  # base timestamp
                  + b"\x01\x00\x00"  # frame width, status flags
                  + b"\x0c\x00"  # stream table: 12 raw bytes
                  + b"\x82\x40\x02"  # stream indices: constant
                  + b"\x82\x40\x02"  # timestamp residuals: constant
                  + b"\x00\x00"  # status bitmaps: empty
                  + bytes([len(instructions), 0])  # raw instructions
                  + b"\x00\x00"  # frame IDs: empty
                  + struct.pack("=QI", 1, 0)
                  + b"\x00"  # every sample is in stream 0
                  + b"\x00"  # every residual is zero
                  + instructions)
        data[64:64] = record
        struct.pack_into("=Q", data, 12, 0)  # start timestamp
        struct.pack_into("=Q", data, 20, 1)  # sample interval
        struct.pack_into("=Q", data, 28, num_samples)
        struct.pack_into("=I", data, 36, 1)  # thread count
        for offset in (40, 48):  # string and frame table offsets
            old_offset = struct.unpack_from("=Q", data, offset)[0]
            struct.pack_into("=Q", data, offset, old_offset + len(record))
        struct.pack_into("=Q", data, len(data) - 24, len(data))
        pathlib.Path(filename).write_bytes(data)

        collector = mock.Mock()
        with BinaryReader(filename) as reader:
            count = reader.replay_samples(collector)
        batches = [call.args[1] for call in collector.collect.call_args_list]
        self.assertEqual(count, num_samples)
        self.assertEqual([len(batch) for batch in batches], [1, 8192, 1])
        self.assertEqual([ts for batch in batches for ts in batch],
                         list(range(1, num_samples + 1)))


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
                # Add frames on top (keeps the outer frames)
                extra = random.randint(1, 3)
                frames = [
                    make_frame(f"top{i}_{j}.py", j, f"top{j}")
                    for j in range(extra)
                ]
                frames.extend(base_frames[: random.randint(2, 4)])
            else:
                # Pop some frames and push new ones
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


class TestBinaryReplayToFlamegraph(BinaryFormatTestBase):
    def test_replay_includes_persisted_stats(self):
        frames = [
            make_frame("hot.py", 99, "hot_func"),
            make_frame("main.py", 1, "main"),
        ]
        samples = [
            [
                make_interpreter(
                    0,
                    [make_thread(1, frames, THREAD_STATUS_HAS_GIL)],
                )
            ]
            for _ in range(5)
        ]
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as file:
            bin_path = file.name
        self.temp_files.append(bin_path)
        collector = BinaryCollector(bin_path, 2000, compression="none")
        for sample in samples:
            collector.collect(sample)
        collector.set_stats(
            2000, 1.25, 4.0, error_rate=2.5, missed_samples=1.5
        )
        collector.export(None)

        with BinaryReader(bin_path) as reader:
            info = reader.get_info()
        self.assertEqual(info["duration_sec"], 1.25)
        self.assertEqual(info["sample_rate"], 4.0)
        self.assertEqual(info["error_rate"], 2.5)
        self.assertEqual(info["missed_samples"], 1.5)

        with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as file:
            html_path = file.name
        self.temp_files.append(html_path)

        convert_binary_to_format(bin_path, html_path, "flamegraph")

        with open(html_path, encoding="utf-8") as file:
            content = file.read()
        self.assertIn('"duration_sec": 1.25', content)
        self.assertIn('"sample_rate": 4.0', content)
        self.assertIn('"error_rate": 2.5', content)
        self.assertIn('"missed_samples": 1.5', content)

    @unittest.skipUnless(ZSTD_AVAILABLE, "zstd compression not available")
    def test_measured_stats_with_zstd(self):
        """Measured statistics are read back when zstd is enabled."""
        frames = [make_frame(f"pkg/module_{i:03}.py", i + 1, f"func_{i:03}")
                  for i in range(200)]
        sample = [make_interpreter(0, [make_thread(1, frames)])]
        filename = self.temp_filename()
        collector = BinaryCollector(filename, 2000, compression="zstd")
        collector.collect(sample, timestamp_us=1000)
        collector.set_stats(2000, 1.25, 4.0, error_rate=2.5,
                            missed_samples=1.5)
        collector.export(None)

        with BinaryReader(filename) as reader:
            info = reader.get_info()
            self.assertEqual(info["duration_sec"], 1.25)
            self.assertEqual(info["missed_samples"], 1.5)
            self.assertEqual(reader.replay_samples(RawCollector()), 1)

    def test_legacy_binary_has_no_measured_stats(self):
        frame = make_frame("hot.py", 99, "hot_func")
        bin_path = self.create_binary_file([
            [make_interpreter(0, [make_thread(1, [frame])])]
        ])

        with BinaryReader(bin_path) as reader:
            info = reader.get_info()

        self.assertIsNone(info["duration_sec"])
        self.assertIsNone(info["sample_rate"])
        self.assertIsNone(info["error_rate"])
        self.assertIsNone(info["missed_samples"])

    def test_original_stats_extension_remains_readable(self):
        frame = make_frame("hot.py", 99, "hot_func")
        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as file:
            bin_path = file.name
        self.temp_files.append(bin_path)
        collector = BinaryCollector(bin_path, 2000, compression="none")
        collector.collect([
            make_interpreter(0, [make_thread(1, [frame])])
        ])
        collector.set_stats(2000, 1.25, 4.0)
        collector.export(None)

        with open(bin_path, "rb") as file:
            data = file.read()
        stats = data[-88:-32]
        footer = bytearray(data[-32:])
        old_stats = stats[:16] + b"TACHSTAT" + struct.pack("=II", 1, 32)
        old_data = bytearray(data[:-88] + old_stats + footer)
        struct.pack_into("=Q", old_data, -24, len(old_data))
        with open(bin_path, "wb") as file:
            file.write(old_data)

        with BinaryReader(bin_path) as reader:
            info = reader.get_info()
        self.assertEqual(info["duration_sec"], 1.25)
        self.assertEqual(info["sample_rate"], 4.0)
        self.assertIsNone(info["error_rate"])
        self.assertIsNone(info["missed_samples"])


class TestBinaryReplayToJsonl(BinaryFormatTestBase):
    """Tests for binary -> JSONL replay via convert_binary_to_format."""

    def _replay_to_jsonl(self, samples, interval=1000, mode=None):
        bin_path = self.create_binary_file(
            samples, interval=interval, mode=mode
        )
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            jsonl_path = f.name
        self.temp_files.append(jsonl_path)

        convert_binary_to_format(bin_path, jsonl_path, "jsonl")

        with open(jsonl_path, "r", encoding="utf-8") as f:
            return [json.loads(line) for line in f]

    def test_binary_replay_to_jsonl_basic(self):
        """Replay a small .bin to JSONL: meta/end shape, samples_total, run_id."""
        frame = make_frame("hot.py", 99, "hot_func")
        samples = [
            [make_interpreter(0, [make_thread(1, [frame])])]
            for _ in range(5)
        ]
        records = self._replay_to_jsonl(samples, interval=2000)
        meta, _, frame_defs, _, end = jsonl_tables(records)

        self.assertEqual(meta["sample_interval_usec"], 2000)
        self.assertEqual(end["samples_total"], 5)

        run_ids = {r["run_id"] for r in records}
        self.assertEqual(len(run_ids), 1)
        self.assertRegex(next(iter(run_ids)), r"^[0-9a-f]{32}$")

        self.assertEqual(len(frame_defs), 1)
        self.assertEqual(frame_defs[0]["line"], 99)

    def test_binary_replay_to_jsonl_preserves_mode(self):
        frame = make_frame("hot.py", 99, "hot_func")
        records = self._replay_to_jsonl(
            [[make_interpreter(0, [make_thread(1, [frame])])]],
            mode=PROFILING_MODE_CPU,
        )

        meta = next(record for record in records if record["type"] == "meta")
        self.assertEqual(meta["mode"], "cpu")

    def test_binary_replay_to_jsonl_rle_weight_propagation(self):
        """RLE-batched identical samples land as a single agg entry with the right total."""
        frame = make_frame("rle.py", 42, "rle_func")
        samples = [
            [make_interpreter(0, [make_thread(1, [frame])])]
            for _ in range(50)
        ]
        records = self._replay_to_jsonl(samples)
        _, _, _, agg, end = jsonl_tables(records)

        self.assertEqual(end["samples_total"], 50)
        self.assertEqual(agg["entries"], [
            {"frame_id": 0, "self": 50, "cumulative": 50},
        ])

    def test_binary_replay_to_jsonl_omits_unavailable_columns(self):
        """Columns the binary recorder did not capture are omitted, not 0."""
        # make_frame defaults column/end_column to 0; pass column=-1 / end_column=-1
        # so the binary side records LOCATION_NOT_AVAILABLE.
        frame = make_frame("nocol.py", 7, "no_col", column=-1, end_column=-1)
        samples = [[make_interpreter(0, [make_thread(1, [frame])])]]
        records = self._replay_to_jsonl(samples)
        _, _, frame_defs, _, _ = jsonl_tables(records)

        self.assertEqual(len(frame_defs), 1)
        fd = frame_defs[0]
        self.assertEqual(fd["line"], 7)
        self.assertNotIn("col", fd)
        self.assertNotIn("end_col", fd)


if __name__ == "__main__":
    unittest.main()
