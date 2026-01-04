"""Tests for sampling profiler collector components."""

import json
import marshal
import opcode
import os
import tempfile
import unittest

try:
    import _remote_debugging  # noqa: F401
    from profiling.sampling.pstats_collector import PstatsCollector
    from profiling.sampling.stack_collector import (
        CollapsedStackCollector,
        FlamegraphCollector,
    )
    from profiling.sampling.gecko_collector import GeckoCollector
    from profiling.sampling.collector import extract_lineno, normalize_location
    from profiling.sampling.opcode_utils import get_opcode_info, format_opcode
    from profiling.sampling.constants import (
        PROFILING_MODE_WALL,
        PROFILING_MODE_CPU,
        DEFAULT_LOCATION,
    )
    from _remote_debugging import (
        THREAD_STATUS_HAS_GIL,
        THREAD_STATUS_ON_CPU,
        THREAD_STATUS_GIL_REQUESTED,
    )
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import captured_stdout, captured_stderr

from .mocks import MockFrameInfo, MockThreadInfo, MockInterpreterInfo, LocationInfo
from .helpers import close_and_unlink


class TestSampleProfilerComponents(unittest.TestCase):
    """Unit tests for individual profiler components."""

    def test_mock_frame_info_with_empty_and_unicode_values(self):
        """Test MockFrameInfo handles empty strings, unicode characters, and very long names correctly."""
        # Test with empty strings
        frame = MockFrameInfo("", 0, "")
        self.assertEqual(frame.filename, "")
        self.assertEqual(frame.location.lineno, 0)
        self.assertEqual(frame.funcname, "")

        # Test with unicode characters
        frame = MockFrameInfo("文件.py", 42, "函数名")
        self.assertEqual(frame.filename, "文件.py")
        self.assertEqual(frame.funcname, "函数名")

        # Test with very long names
        long_filename = "x" * 1000 + ".py"
        long_funcname = "func_" + "x" * 1000
        frame = MockFrameInfo(long_filename, 999999, long_funcname)
        self.assertEqual(frame.filename, long_filename)
        self.assertEqual(frame.location.lineno, 999999)
        self.assertEqual(frame.funcname, long_funcname)

    def test_pstats_collector_with_extreme_intervals_and_empty_data(self):
        """Test PstatsCollector handles zero/large intervals, empty frames, None thread IDs, and duplicate frames."""
        # Test with zero interval
        collector = PstatsCollector(sample_interval_usec=0)
        self.assertEqual(collector.sample_interval_usec, 0)

        # Test with very large interval
        collector = PstatsCollector(sample_interval_usec=1000000000)
        self.assertEqual(collector.sample_interval_usec, 1000000000)

        # Test collecting empty frames list
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect([])
        self.assertEqual(len(collector.result), 0)

        # Test collecting frames with None thread id
        test_frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(None, [MockFrameInfo("file.py", 10, "func", None)])],
            )
        ]
        collector.collect(test_frames)
        # Should still process the frames
        self.assertEqual(len(collector.result), 1)

        # Test collecting duplicate frames in same sample (recursive function)
        test_frames = [
            MockInterpreterInfo(
                0,  # interpreter_id
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 10, "func1"),  # Duplicate (recursion)
                        ],
                    )
                ],
            )
        ]
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(test_frames)
        # Should count only once per sample to avoid over-counting recursive functions
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["cumulative_calls"], 1
        )

    def test_pstats_collector_single_frame_stacks(self):
        """Test PstatsCollector with single-frame call stacks to trigger len(frames) <= 1 branch."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Test with exactly one frame (should trigger the <= 1 condition)
        single_frame = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("single.py", 10, "single_func")]
                    )
                ],
            )
        ]
        collector.collect(single_frame)

        # Should record the single frame with inline call
        self.assertEqual(len(collector.result), 1)
        single_key = ("single.py", 10, "single_func")
        self.assertIn(single_key, collector.result)
        self.assertEqual(collector.result[single_key]["direct_calls"], 1)
        self.assertEqual(collector.result[single_key]["cumulative_calls"], 1)

        # Test with empty frames (should also trigger <= 1 condition)
        empty_frames = [MockInterpreterInfo(0, [MockThreadInfo(1, [])])]
        collector.collect(empty_frames)

        # Should not add any new entries
        self.assertEqual(
            len(collector.result), 1
        )  # Still just the single frame

        # Test mixed single and multi-frame stacks
        mixed_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [MockFrameInfo("single2.py", 20, "single_func2")],
                    ),  # Single frame
                    MockThreadInfo(
                        2,
                        [  # Multi-frame stack
                            MockFrameInfo("multi.py", 30, "multi_func1"),
                            MockFrameInfo("multi.py", 40, "multi_func2"),
                        ],
                    ),
                ],
            ),
        ]
        collector.collect(mixed_frames)

        # Should have recorded all functions
        self.assertEqual(
            len(collector.result), 4
        )  # single + single2 + multi1 + multi2

        # Verify single frame handling
        single2_key = ("single2.py", 20, "single_func2")
        self.assertIn(single2_key, collector.result)
        self.assertEqual(collector.result[single2_key]["direct_calls"], 1)
        self.assertEqual(collector.result[single2_key]["cumulative_calls"], 1)

        # Verify multi-frame handling still works
        multi1_key = ("multi.py", 30, "multi_func1")
        multi2_key = ("multi.py", 40, "multi_func2")
        self.assertIn(multi1_key, collector.result)
        self.assertIn(multi2_key, collector.result)
        self.assertEqual(collector.result[multi1_key]["direct_calls"], 1)
        self.assertEqual(
            collector.result[multi2_key]["cumulative_calls"], 1
        )  # Called from multi1

    def test_collapsed_stack_collector_with_empty_and_deep_stacks(self):
        """Test CollapsedStackCollector handles empty frames, single-frame stacks, and very deep call stacks."""
        collector = CollapsedStackCollector(1000)

        # Test with empty frames
        collector.collect([])
        self.assertEqual(len(collector.stack_counter), 0)

        # Test with single frame stack
        test_frames = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [MockFrameInfo("file.py", 10, "func")])]
            )
        ]
        collector.collect(test_frames)
        self.assertEqual(len(collector.stack_counter), 1)
        (((path, thread_id), count),) = collector.stack_counter.items()
        self.assertEqual(path, (("file.py", 10, "func"),))
        self.assertEqual(thread_id, 1)
        self.assertEqual(count, 1)

        # Test with very deep stack
        deep_stack = [MockFrameInfo(f"file{i}.py", i, f"func{i}") for i in range(100)]
        test_frames = [MockInterpreterInfo(0, [MockThreadInfo(1, deep_stack)])]
        collector = CollapsedStackCollector(1000)
        collector.collect(test_frames)
        # One aggregated path with 100 frames (reversed)
        (((path_tuple, thread_id),),) = (collector.stack_counter.keys(),)
        self.assertEqual(len(path_tuple), 100)
        self.assertEqual(path_tuple[0], ("file99.py", 99, "func99"))
        self.assertEqual(path_tuple[-1], ("file0.py", 0, "func0"))
        self.assertEqual(thread_id, 1)

    def test_pstats_collector_basic(self):
        """Test basic PstatsCollector functionality."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Test empty state
        self.assertEqual(len(collector.result), 0)
        self.assertEqual(len(collector.stats), 0)

        # Test collecting sample data
        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 20, "func2"),
                        ],
                    )
                ],
            )
        ]
        collector.collect(test_frames)

        # Should have recorded calls for both functions
        self.assertEqual(len(collector.result), 2)
        self.assertIn(("file.py", 10, "func1"), collector.result)
        self.assertIn(("file.py", 20, "func2"), collector.result)

        # Top-level function should have direct call
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["direct_calls"], 1
        )
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["cumulative_calls"], 1
        )

        # Calling function should have cumulative call but no direct calls
        self.assertEqual(
            collector.result[("file.py", 20, "func2")]["cumulative_calls"], 1
        )
        self.assertEqual(
            collector.result[("file.py", 20, "func2")]["direct_calls"], 0
        )

    def test_pstats_collector_create_stats(self):
        """Test PstatsCollector stats creation."""
        collector = PstatsCollector(
            sample_interval_usec=1000000
        )  # 1 second intervals

        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 20, "func2"),
                        ],
                    )
                ],
            )
        ]
        collector.collect(test_frames)
        collector.collect(test_frames)  # Collect twice

        collector.create_stats()

        # Check stats format: (direct_calls, cumulative_calls, tt, ct, callers)
        func1_stats = collector.stats[("file.py", 10, "func1")]
        self.assertEqual(func1_stats[0], 2)  # direct_calls (top of stack)
        self.assertEqual(func1_stats[1], 2)  # cumulative_calls
        self.assertEqual(
            func1_stats[2], 2.0
        )  # tt (total time - 2 samples * 1 sec)
        self.assertEqual(func1_stats[3], 2.0)  # ct (cumulative time)

        func2_stats = collector.stats[("file.py", 20, "func2")]
        self.assertEqual(
            func2_stats[0], 0
        )  # direct_calls (never top of stack)
        self.assertEqual(
            func2_stats[1], 2
        )  # cumulative_calls (appears in stack)
        self.assertEqual(func2_stats[2], 0.0)  # tt (no direct calls)
        self.assertEqual(func2_stats[3], 2.0)  # ct (cumulative time)

    def test_collapsed_stack_collector_basic(self):
        collector = CollapsedStackCollector(1000)

        # Test empty state
        self.assertEqual(len(collector.stack_counter), 0)

        # Test collecting sample data
        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        collector.collect(test_frames)

        # Should store one reversed path
        self.assertEqual(len(collector.stack_counter), 1)
        (((path, thread_id), count),) = collector.stack_counter.items()
        expected_tree = (("file.py", 20, "func2"), ("file.py", 10, "func1"))
        self.assertEqual(path, expected_tree)
        self.assertEqual(thread_id, 1)
        self.assertEqual(count, 1)

    def test_collapsed_stack_collector_export(self):
        collapsed_out = tempfile.NamedTemporaryFile(delete=False)
        self.addCleanup(close_and_unlink, collapsed_out)

        collector = CollapsedStackCollector(1000)

        test_frames1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [MockFrameInfo("other.py", 5, "other_func")])]
            )
        ]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        with captured_stdout(), captured_stderr():
            collector.export(collapsed_out.name)
        # Check file contents
        with open(collapsed_out.name, "r") as f:
            content = f.read()

        lines = content.strip().split("\n")
        self.assertEqual(len(lines), 2)  # Two unique stacks

        # Check collapsed format: tid:X;file:func:line;file:func:line count
        stack1_expected = "tid:1;file.py:func2:20;file.py:func1:10 2"
        stack2_expected = "tid:1;other.py:other_func:5 1"

        self.assertIn(stack1_expected, lines)
        self.assertIn(stack2_expected, lines)

    def test_flamegraph_collector_basic(self):
        """Test basic FlamegraphCollector functionality."""
        collector = FlamegraphCollector(1000)

        # Empty collector should produce 'No Data'
        data = collector._convert_to_flamegraph_format()
        # With string table, name is now an index - resolve it using the strings array
        strings = data.get("strings", [])
        name_index = data.get("name", 0)
        resolved_name = (
            strings[name_index]
            if isinstance(name_index, int) and 0 <= name_index < len(strings)
            else str(name_index)
        )
        self.assertIn(resolved_name, ("No Data", "No significant data"))

        # Test collecting sample data
        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        collector.collect(test_frames)

        # Convert and verify structure: func2 -> func1 with counts = 1
        data = collector._convert_to_flamegraph_format()
        # Expect promotion: root is the single child (func2), with func1 as its only child
        strings = data.get("strings", [])
        name_index = data.get("name", 0)
        name = (
            strings[name_index]
            if isinstance(name_index, int) and 0 <= name_index < len(strings)
            else str(name_index)
        )
        self.assertIsInstance(name, str)
        self.assertTrue(name.startswith("Program Root: "))
        self.assertIn("func2 (file.py:20)", name)  # formatted name
        children = data.get("children", [])
        self.assertEqual(len(children), 1)
        child = children[0]
        child_name_index = child.get("name", 0)
        child_name = (
            strings[child_name_index]
            if isinstance(child_name_index, int)
            and 0 <= child_name_index < len(strings)
            else str(child_name_index)
        )
        self.assertIn("func1 (file.py:10)", child_name)  # formatted name
        self.assertEqual(child["value"], 1)

    def test_flamegraph_collector_export(self):
        """Test flamegraph HTML export functionality."""
        flamegraph_out = tempfile.NamedTemporaryFile(
            suffix=".html", delete=False
        )
        self.addCleanup(close_and_unlink, flamegraph_out)

        collector = FlamegraphCollector(1000)

        # Create some test data (use Interpreter/Thread objects like runtime)
        test_frames1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [MockFrameInfo("other.py", 5, "other_func")])]
            )
        ]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        # Export flamegraph
        with captured_stdout(), captured_stderr():
            collector.export(flamegraph_out.name)

        # Verify file was created and contains valid data
        self.assertTrue(os.path.exists(flamegraph_out.name))
        self.assertGreater(os.path.getsize(flamegraph_out.name), 0)

        # Check file contains HTML content
        with open(flamegraph_out.name, "r", encoding="utf-8") as f:
            content = f.read()

        # Should be valid HTML
        self.assertIn("<!doctype html>", content.lower())
        self.assertIn("<html", content)
        self.assertIn("Tachyon Profiler - Flamegraph", content)
        self.assertIn("d3-flame-graph", content)

        # Should contain the data
        self.assertIn('"name":', content)
        self.assertIn('"value":', content)
        self.assertIn('"children":', content)

    def test_gecko_collector_basic(self):
        """Test basic GeckoCollector functionality."""
        collector = GeckoCollector(1000)

        # Test empty state
        self.assertEqual(len(collector.threads), 0)
        self.assertEqual(collector.sample_count, 0)
        self.assertEqual(len(collector.global_strings), 1)  # "(root)"

        # Test collecting sample data
        test_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")],
                    )
                ],
            )
        ]
        collector.collect(test_frames)

        # Should have recorded one thread and one sample
        self.assertEqual(len(collector.threads), 1)
        self.assertEqual(collector.sample_count, 1)
        self.assertIn(1, collector.threads)

        profile_data = collector._build_profile()

        # Verify profile structure
        self.assertIn("meta", profile_data)
        self.assertIn("threads", profile_data)
        self.assertIn("shared", profile_data)

        # Check shared string table
        shared = profile_data["shared"]
        self.assertIn("stringArray", shared)
        string_array = shared["stringArray"]
        self.assertGreater(len(string_array), 0)

        # Should contain our functions in the string array
        self.assertIn("func1", string_array)
        self.assertIn("func2", string_array)

        # Check thread data structure
        threads = profile_data["threads"]
        self.assertEqual(len(threads), 1)
        thread_data = threads[0]

        # Verify thread structure
        self.assertIn("samples", thread_data)
        self.assertIn("funcTable", thread_data)
        self.assertIn("frameTable", thread_data)
        self.assertIn("stackTable", thread_data)

        # Verify samples
        samples = thread_data["samples"]
        self.assertEqual(len(samples["stack"]), 1)
        self.assertEqual(len(samples["time"]), 1)
        self.assertEqual(samples["length"], 1)

        # Verify function table structure and content
        func_table = thread_data["funcTable"]
        self.assertIn("name", func_table)
        self.assertIn("fileName", func_table)
        self.assertIn("lineNumber", func_table)
        self.assertEqual(func_table["length"], 2)  # Should have 2 functions

        # Verify actual function content through string array indices
        func_names = []
        for idx in func_table["name"]:
            func_name = (
                string_array[idx]
                if isinstance(idx, int) and 0 <= idx < len(string_array)
                else str(idx)
            )
            func_names.append(func_name)

        self.assertIn("func1", func_names, f"func1 not found in {func_names}")
        self.assertIn("func2", func_names, f"func2 not found in {func_names}")

        # Verify frame table
        frame_table = thread_data["frameTable"]
        self.assertEqual(
            frame_table["length"], 2
        )  # Should have frames for both functions
        self.assertEqual(len(frame_table["func"]), 2)

        # Verify stack structure
        stack_table = thread_data["stackTable"]
        self.assertGreater(stack_table["length"], 0)
        self.assertGreater(len(stack_table["frame"]), 0)

    def test_gecko_collector_export(self):
        """Test Gecko profile export functionality."""
        gecko_out = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
        self.addCleanup(close_and_unlink, gecko_out)

        collector = GeckoCollector(1000)

        test_frames1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("file.py", 10, "func1"), MockFrameInfo("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [MockFrameInfo("other.py", 5, "other_func")])]
            )
        ]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        # Export gecko profile
        with captured_stdout(), captured_stderr():
            collector.export(gecko_out.name)

        # Verify file was created and contains valid data
        self.assertTrue(os.path.exists(gecko_out.name))
        self.assertGreater(os.path.getsize(gecko_out.name), 0)

        # Check file contains valid JSON
        with open(gecko_out.name, "r") as f:
            profile_data = json.load(f)

        # Should be valid Gecko profile format
        self.assertIn("meta", profile_data)
        self.assertIn("threads", profile_data)
        self.assertIn("shared", profile_data)

        # Check meta information
        self.assertIn("categories", profile_data["meta"])
        self.assertIn("interval", profile_data["meta"])

        # Check shared string table
        self.assertIn("stringArray", profile_data["shared"])
        self.assertGreater(len(profile_data["shared"]["stringArray"]), 0)

        # Should contain our functions
        string_array = profile_data["shared"]["stringArray"]
        self.assertIn("func1", string_array)
        self.assertIn("func2", string_array)
        self.assertIn("other_func", string_array)

    def test_gecko_collector_markers(self):
        """Test Gecko profile markers for GIL and CPU state tracking."""
        collector = GeckoCollector(1000)

        # Status combinations for different thread states
        HAS_GIL_ON_CPU = (
            THREAD_STATUS_HAS_GIL | THREAD_STATUS_ON_CPU
        )  # Running Python code
        NO_GIL_ON_CPU = THREAD_STATUS_ON_CPU  # Running native code
        WAITING_FOR_GIL = THREAD_STATUS_GIL_REQUESTED  # Waiting for GIL

        # Simulate thread state transitions
        collector.collect(
            [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            1,
                            [MockFrameInfo("test.py", 10, "python_func")],
                            status=HAS_GIL_ON_CPU,
                        )
                    ],
                )
            ]
        )

        collector.collect(
            [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            1,
                            [MockFrameInfo("test.py", 15, "wait_func")],
                            status=WAITING_FOR_GIL,
                        )
                    ],
                )
            ]
        )

        collector.collect(
            [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            1,
                            [MockFrameInfo("test.py", 20, "python_func2")],
                            status=HAS_GIL_ON_CPU,
                        )
                    ],
                )
            ]
        )

        collector.collect(
            [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            1,
                            [MockFrameInfo("native.c", 100, "native_func")],
                            status=NO_GIL_ON_CPU,
                        )
                    ],
                )
            ]
        )

        profile_data = collector._build_profile()

        # Verify we have threads with markers
        self.assertIn("threads", profile_data)
        self.assertEqual(len(profile_data["threads"]), 1)
        thread_data = profile_data["threads"][0]

        # Check markers exist
        self.assertIn("markers", thread_data)
        markers = thread_data["markers"]

        # Should have marker arrays
        self.assertIn("name", markers)
        self.assertIn("startTime", markers)
        self.assertIn("endTime", markers)
        self.assertIn("category", markers)
        self.assertGreater(
            markers["length"], 0, "Should have generated markers"
        )

        # Get marker names from string table
        string_array = profile_data["shared"]["stringArray"]
        marker_names = [string_array[idx] for idx in markers["name"]]

        # Verify we have different marker types
        marker_name_set = set(marker_names)

        # Should have "Has GIL" markers (when thread had GIL)
        self.assertIn(
            "Has GIL", marker_name_set, "Should have 'Has GIL' markers"
        )

        # Should have "No GIL" markers (when thread didn't have GIL)
        self.assertIn(
            "No GIL", marker_name_set, "Should have 'No GIL' markers"
        )

        # Should have "On CPU" markers (when thread was on CPU)
        self.assertIn(
            "On CPU", marker_name_set, "Should have 'On CPU' markers"
        )

        # Should have "Waiting for GIL" markers (when thread was waiting)
        self.assertIn(
            "Waiting for GIL",
            marker_name_set,
            "Should have 'Waiting for GIL' markers",
        )

        # Verify marker structure
        for i in range(markers["length"]):
            # All markers should be interval markers (phase = 1)
            self.assertEqual(
                markers["phase"][i], 1, f"Marker {i} should be interval marker"
            )

            # All markers should have valid time range
            start_time = markers["startTime"][i]
            end_time = markers["endTime"][i]
            self.assertLessEqual(
                start_time,
                end_time,
                f"Marker {i} should have valid time range",
            )

            # All markers should have valid category
            self.assertGreaterEqual(
                markers["category"][i],
                0,
                f"Marker {i} should have valid category",
            )

    def test_pstats_collector_export(self):
        collector = PstatsCollector(
            sample_interval_usec=1000000
        )  # 1 second intervals

        test_frames1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 20, "func2"),
                        ],
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 20, "func2"),
                        ],
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [MockFrameInfo("other.py", 5, "other_func")]
                    )
                ],
            )
        ]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        pstats_out = tempfile.NamedTemporaryFile(
            suffix=".pstats", delete=False
        )
        self.addCleanup(close_and_unlink, pstats_out)
        collector.export(pstats_out.name)

        # Check file can be loaded with marshal
        with open(pstats_out.name, "rb") as f:
            stats_data = marshal.load(f)

        # Should be a dictionary with the sampled marker
        self.assertIsInstance(stats_data, dict)
        self.assertIn(("__sampled__",), stats_data)
        self.assertTrue(stats_data[("__sampled__",)])

        # Should have function data
        function_entries = [
            k for k in stats_data.keys() if k != ("__sampled__",)
        ]
        self.assertGreater(len(function_entries), 0)

        # Check specific function stats format: (cc, nc, tt, ct, callers)
        func1_key = ("file.py", 10, "func1")
        func2_key = ("file.py", 20, "func2")
        other_key = ("other.py", 5, "other_func")

        self.assertIn(func1_key, stats_data)
        self.assertIn(func2_key, stats_data)
        self.assertIn(other_key, stats_data)

        # Check func1 stats (should have 2 samples)
        func1_stats = stats_data[func1_key]
        self.assertEqual(func1_stats[0], 2)  # total_calls
        self.assertEqual(func1_stats[1], 2)  # nc (non-recursive calls)
        self.assertEqual(func1_stats[2], 2.0)  # tt (total time)
        self.assertEqual(func1_stats[3], 2.0)  # ct (cumulative time)

    def test_flamegraph_collector_stats_accumulation(self):
        """Test that FlamegraphCollector accumulates stats across samples."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # First sample
        stack_frames_1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_HAS_GIL),
                    MockThreadInfo(2, [MockFrameInfo("b.py", 2, "func_b")], status=THREAD_STATUS_ON_CPU),
                ],
            )
        ]
        collector.collect(stack_frames_1)
        self.assertEqual(collector.thread_status_counts["has_gil"], 1)
        self.assertEqual(collector.thread_status_counts["on_cpu"], 1)
        self.assertEqual(collector.thread_status_counts["total"], 2)

        # Second sample
        stack_frames_2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_GIL_REQUESTED),
                    MockThreadInfo(2, [MockFrameInfo("b.py", 2, "func_b")], status=THREAD_STATUS_HAS_GIL),
                    MockThreadInfo(3, [MockFrameInfo("c.py", 3, "func_c")], status=THREAD_STATUS_ON_CPU),
                ],
            )
        ]
        collector.collect(stack_frames_2)

        # Should accumulate
        self.assertEqual(collector.thread_status_counts["has_gil"], 2)  # 1 + 1
        self.assertEqual(collector.thread_status_counts["on_cpu"], 2)   # 1 + 1
        self.assertEqual(collector.thread_status_counts["gil_requested"], 1)  # 0 + 1
        self.assertEqual(collector.thread_status_counts["total"], 5)  # 2 + 3

        # Test GC sample tracking
        stack_frames_gc = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("~", 0, "<GC>")], status=THREAD_STATUS_HAS_GIL),
                ],
            )
        ]
        collector.collect(stack_frames_gc)
        self.assertEqual(collector.samples_with_gc_frames, 1)

        # Another sample without GC
        collector.collect(stack_frames_1)
        self.assertEqual(collector.samples_with_gc_frames, 1)  # Still 1

        # Another GC sample
        collector.collect(stack_frames_gc)
        self.assertEqual(collector.samples_with_gc_frames, 2)

    def test_flamegraph_collector_per_thread_stats(self):
        """Test per-thread statistics tracking in FlamegraphCollector."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Multiple threads with different states
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_HAS_GIL),
                    MockThreadInfo(2, [MockFrameInfo("b.py", 2, "func_b")], status=THREAD_STATUS_ON_CPU),
                    MockThreadInfo(3, [MockFrameInfo("c.py", 3, "func_c")], status=THREAD_STATUS_GIL_REQUESTED),
                ],
            )
        ]
        collector.collect(stack_frames)

        # Check per-thread stats
        self.assertIn(1, collector.per_thread_stats)
        self.assertIn(2, collector.per_thread_stats)
        self.assertIn(3, collector.per_thread_stats)

        # Thread 1: has GIL
        self.assertEqual(collector.per_thread_stats[1]["has_gil"], 1)
        self.assertEqual(collector.per_thread_stats[1]["on_cpu"], 0)
        self.assertEqual(collector.per_thread_stats[1]["total"], 1)

        # Thread 2: on CPU
        self.assertEqual(collector.per_thread_stats[2]["has_gil"], 0)
        self.assertEqual(collector.per_thread_stats[2]["on_cpu"], 1)
        self.assertEqual(collector.per_thread_stats[2]["total"], 1)

        # Thread 3: waiting
        self.assertEqual(collector.per_thread_stats[3]["gil_requested"], 1)
        self.assertEqual(collector.per_thread_stats[3]["total"], 1)

        # Test accumulation across samples
        stack_frames_2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 2, "func_b")], status=THREAD_STATUS_ON_CPU),
                ],
            )
        ]
        collector.collect(stack_frames_2)

        self.assertEqual(collector.per_thread_stats[1]["has_gil"], 1)
        self.assertEqual(collector.per_thread_stats[1]["on_cpu"], 1)
        self.assertEqual(collector.per_thread_stats[1]["total"], 2)

    def test_flamegraph_collector_percentage_calculations(self):
        """Test that percentage calculations are correct in exported data."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Create scenario: 60% GIL held, 40% not held
        for i in range(6):
            stack_frames = [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func")], status=THREAD_STATUS_HAS_GIL),
                    ],
                )
            ]
            collector.collect(stack_frames)

        for i in range(4):
            stack_frames = [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func")], status=THREAD_STATUS_ON_CPU),
                    ],
                )
            ]
            collector.collect(stack_frames)

        # Export to get calculated percentages
        data = collector._convert_to_flamegraph_format()
        thread_stats = data["stats"]["thread_stats"]

        self.assertAlmostEqual(thread_stats["has_gil_pct"], 60.0, places=1)
        self.assertAlmostEqual(thread_stats["on_cpu_pct"], 40.0, places=1)
        self.assertEqual(thread_stats["total"], 10)

    def test_flamegraph_collector_mode_handling(self):
        """Test that profiling mode is correctly passed through to exported data."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Collect some data
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func")], status=THREAD_STATUS_HAS_GIL),
                ],
            )
        ]
        collector.collect(stack_frames)

        # Set stats with mode
        collector.set_stats(
            sample_interval_usec=1000,
            duration_sec=1.0,
            sample_rate=1000.0,
            mode=PROFILING_MODE_CPU
        )

        data = collector._convert_to_flamegraph_format()
        self.assertEqual(data["stats"]["mode"], PROFILING_MODE_CPU)

    def test_flamegraph_collector_zero_samples_edge_case(self):
        """Test that collector handles zero samples gracefully."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Export without collecting any samples
        data = collector._convert_to_flamegraph_format()

        # Should return a valid structure with no data
        self.assertIn("name", data)
        self.assertEqual(data["value"], 0)
        self.assertIn("children", data)
        self.assertEqual(len(data["children"]), 0)

    def test_flamegraph_collector_json_structure_includes_stats(self):
        """Test that exported JSON includes thread_stats and per_thread_stats."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Collect some data with multiple threads
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_HAS_GIL),
                    MockThreadInfo(2, [MockFrameInfo("b.py", 2, "func_b")], status=THREAD_STATUS_ON_CPU),
                ],
            )
        ]
        collector.collect(stack_frames)

        # Set stats
        collector.set_stats(
            sample_interval_usec=1000,
            duration_sec=1.0,
            sample_rate=1000.0,
            mode=PROFILING_MODE_WALL
        )

        # Export and verify structure
        data = collector._convert_to_flamegraph_format()

        # Check that stats object exists and contains expected fields
        self.assertIn("stats", data)
        stats = data["stats"]

        # Verify thread_stats exists and has expected structure
        self.assertIn("thread_stats", stats)
        thread_stats = stats["thread_stats"]
        self.assertIn("has_gil_pct", thread_stats)
        self.assertIn("on_cpu_pct", thread_stats)
        self.assertIn("gil_requested_pct", thread_stats)
        self.assertIn("gc_pct", thread_stats)
        self.assertIn("total", thread_stats)

        # Verify per_thread_stats exists and has data for both threads
        self.assertIn("per_thread_stats", stats)
        per_thread_stats = stats["per_thread_stats"]
        self.assertIn(1, per_thread_stats)
        self.assertIn(2, per_thread_stats)

        # Check per-thread structure
        for thread_id in [1, 2]:
            thread_data = per_thread_stats[thread_id]
            self.assertIn("has_gil_pct", thread_data)
            self.assertIn("on_cpu_pct", thread_data)
            self.assertIn("gil_requested_pct", thread_data)
            self.assertIn("gc_pct", thread_data)
            self.assertIn("total", thread_data)

    def test_flamegraph_collector_per_thread_gc_percentage(self):
        """Test that per-thread GC percentage uses total samples as denominator."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        # Create 10 samples total:
        # - Thread 1 appears in all 10 samples, has GC in 2 of them
        # - Thread 2 appears in only 5 samples, has GC in 1 of them

        # First 5 samples: both threads, thread 1 has GC in 2
        for i in range(5):
            has_gc = i < 2  # First 2 samples have GC for thread 1
            frames_1 = [MockFrameInfo("~", 0, "<GC>")] if has_gc else [MockFrameInfo("a.py", 1, "func_a")]
            stack_frames = [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(1, frames_1, status=THREAD_STATUS_HAS_GIL),
                        MockThreadInfo(2, [MockFrameInfo("b.py", 2, "func_b")], status=THREAD_STATUS_ON_CPU),
                    ],
                )
            ]
            collector.collect(stack_frames)

        # Next 5 samples: only thread 1, thread 2 appears in first of these with GC
        for i in range(5):
            if i == 0:
                # Thread 2 appears in this sample with GC
                stack_frames = [
                    MockInterpreterInfo(
                        0,
                        [
                            MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_HAS_GIL),
                            MockThreadInfo(2, [MockFrameInfo("~", 0, "<GC>")], status=THREAD_STATUS_ON_CPU),
                        ],
                    )
                ]
            else:
                # Only thread 1
                stack_frames = [
                    MockInterpreterInfo(
                        0,
                        [
                            MockThreadInfo(1, [MockFrameInfo("a.py", 1, "func_a")], status=THREAD_STATUS_HAS_GIL),
                        ],
                    )
                ]
            collector.collect(stack_frames)

        # Set stats and export
        collector.set_stats(
            sample_interval_usec=1000,
            duration_sec=1.0,
            sample_rate=1000.0,
            mode=PROFILING_MODE_WALL
        )

        data = collector._convert_to_flamegraph_format()
        per_thread_stats = data["stats"]["per_thread_stats"]

        # Thread 1: appeared in 10 samples, had GC in 2
        # GC percentage should be 2/10 = 20% (using total samples, not thread appearances)
        self.assertEqual(collector.per_thread_stats[1]["gc_samples"], 2)
        self.assertEqual(collector.per_thread_stats[1]["total"], 10)
        self.assertAlmostEqual(per_thread_stats[1]["gc_pct"], 20.0, places=1)

        # Thread 2: appeared in 6 samples, had GC in 1
        # GC percentage should be 1/10 = 10% (using total samples, not thread appearances)
        self.assertEqual(collector.per_thread_stats[2]["gc_samples"], 1)
        self.assertEqual(collector.per_thread_stats[2]["total"], 6)
        self.assertAlmostEqual(per_thread_stats[2]["gc_pct"], 10.0, places=1)


class TestRecursiveFunctionHandling(unittest.TestCase):
    """Tests for correct handling of recursive functions in cumulative stats."""

    def test_pstats_collector_recursive_function_single_sample(self):
        """Test that recursive functions are counted once per sample, not per occurrence."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Simulate a recursive function appearing 5 times in one sample
        recursive_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                        ],
                    )
                ],
            )
        ]
        collector.collect(recursive_frames)

        location = ("test.py", 10, "recursive_func")
        # Should count as 1 cumulative call (present in 1 sample), not 5
        self.assertEqual(collector.result[location]["cumulative_calls"], 1)
        # Direct calls should be 1 (top of stack)
        self.assertEqual(collector.result[location]["direct_calls"], 1)

    def test_pstats_collector_recursive_function_multiple_samples(self):
        """Test cumulative counting across multiple samples with recursion."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Sample 1: recursive function at depth 3
        sample1 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                        ],
                    )
                ],
            )
        ]
        # Sample 2: recursive function at depth 2
        sample2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                        ],
                    )
                ],
            )
        ]
        # Sample 3: recursive function at depth 4
        sample3 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                            MockFrameInfo("test.py", 10, "recursive_func"),
                        ],
                    )
                ],
            )
        ]

        collector.collect(sample1)
        collector.collect(sample2)
        collector.collect(sample3)

        location = ("test.py", 10, "recursive_func")
        # Should count as 3 cumulative calls (present in 3 samples)
        # Not 3+2+4=9 which would be the buggy behavior
        self.assertEqual(collector.result[location]["cumulative_calls"], 3)
        self.assertEqual(collector.result[location]["direct_calls"], 3)

    def test_pstats_collector_mixed_recursive_and_nonrecursive(self):
        """Test a call stack with both recursive and non-recursive functions."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Stack: main -> foo (recursive x3) -> bar
        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 50, "bar"),       # top of stack
                            MockFrameInfo("test.py", 20, "foo"),      # recursive
                            MockFrameInfo("test.py", 20, "foo"),      # recursive
                            MockFrameInfo("test.py", 20, "foo"),      # recursive
                            MockFrameInfo("test.py", 10, "main"),     # bottom
                        ],
                    )
                ],
            )
        ]
        collector.collect(frames)

        # bar: 1 cumulative (in stack), 1 direct (top)
        self.assertEqual(collector.result[("test.py", 50, "bar")]["cumulative_calls"], 1)
        self.assertEqual(collector.result[("test.py", 50, "bar")]["direct_calls"], 1)

        # foo: 1 cumulative (counted once despite 3 occurrences), 0 direct
        self.assertEqual(collector.result[("test.py", 20, "foo")]["cumulative_calls"], 1)
        self.assertEqual(collector.result[("test.py", 20, "foo")]["direct_calls"], 0)

        # main: 1 cumulative, 0 direct
        self.assertEqual(collector.result[("test.py", 10, "main")]["cumulative_calls"], 1)
        self.assertEqual(collector.result[("test.py", 10, "main")]["direct_calls"], 0)

    def test_pstats_collector_cumulative_percentage_cannot_exceed_100(self):
        """Test that cumulative percentage stays <= 100% even with deep recursion."""
        collector = PstatsCollector(sample_interval_usec=1000000)  # 1 second for easy math

        # Collect 10 samples, each with recursive function at depth 100
        for _ in range(10):
            frames = [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            1,
                            [MockFrameInfo("test.py", 10, "deep_recursive")] * 100,
                        )
                    ],
                )
            ]
            collector.collect(frames)

        location = ("test.py", 10, "deep_recursive")
        # Cumulative calls should be 10 (number of samples), not 1000
        self.assertEqual(collector.result[location]["cumulative_calls"], 10)

        # Verify stats calculation gives correct percentage
        collector.create_stats()
        stats = collector.stats[location]
        # stats format: (direct_calls, cumulative_calls, total_time, cumulative_time, callers)
        cumulative_calls = stats[1]
        self.assertEqual(cumulative_calls, 10)

    def test_pstats_collector_different_lines_same_function_counted_separately(self):
        """Test that different line numbers in same function are tracked separately."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Function with multiple line numbers (e.g., different call sites within recursion)
        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("test.py", 15, "func"),  # line 15
                            MockFrameInfo("test.py", 12, "func"),  # line 12
                            MockFrameInfo("test.py", 15, "func"),  # line 15 again
                            MockFrameInfo("test.py", 10, "func"),  # line 10
                        ],
                    )
                ],
            )
        ]
        collector.collect(frames)

        # Each unique (file, line, func) should be counted once
        self.assertEqual(collector.result[("test.py", 15, "func")]["cumulative_calls"], 1)
        self.assertEqual(collector.result[("test.py", 12, "func")]["cumulative_calls"], 1)
        self.assertEqual(collector.result[("test.py", 10, "func")]["cumulative_calls"], 1)


class TestLocationHelpers(unittest.TestCase):
    """Tests for location handling helper functions."""

    def test_extract_lineno_from_location_info(self):
        """Test extracting lineno from LocationInfo namedtuple."""
        loc = LocationInfo(42, 45, 0, 10)
        self.assertEqual(extract_lineno(loc), 42)

    def test_extract_lineno_from_tuple(self):
        """Test extracting lineno from plain tuple."""
        loc = (100, 105, 5, 20)
        self.assertEqual(extract_lineno(loc), 100)

    def test_extract_lineno_from_none(self):
        """Test extracting lineno from None (synthetic frames)."""
        self.assertEqual(extract_lineno(None), 0)

    def test_normalize_location_with_location_info(self):
        """Test normalize_location passes through LocationInfo."""
        loc = LocationInfo(10, 15, 0, 5)
        result = normalize_location(loc)
        self.assertEqual(result, loc)

    def test_normalize_location_with_tuple(self):
        """Test normalize_location passes through tuple."""
        loc = (10, 15, 0, 5)
        result = normalize_location(loc)
        self.assertEqual(result, loc)

    def test_normalize_location_with_none(self):
        """Test normalize_location returns DEFAULT_LOCATION for None."""
        result = normalize_location(None)
        self.assertEqual(result, DEFAULT_LOCATION)
        self.assertEqual(result, (0, 0, -1, -1))


class TestOpcodeFormatting(unittest.TestCase):
    """Tests for opcode formatting utilities."""

    def test_get_opcode_info_standard_opcode(self):
        """Test get_opcode_info for a standard opcode."""
        # LOAD_CONST is a standard opcode
        load_const = opcode.opmap.get('LOAD_CONST')
        if load_const is not None:
            info = get_opcode_info(load_const)
            self.assertEqual(info['opname'], 'LOAD_CONST')
            self.assertEqual(info['base_opname'], 'LOAD_CONST')
            self.assertFalse(info['is_specialized'])

    def test_get_opcode_info_unknown_opcode(self):
        """Test get_opcode_info for an unknown opcode."""
        info = get_opcode_info(999)
        self.assertEqual(info['opname'], '<999>')
        self.assertEqual(info['base_opname'], '<999>')
        self.assertFalse(info['is_specialized'])

    def test_format_opcode_standard(self):
        """Test format_opcode for a standard opcode."""
        load_const = opcode.opmap.get('LOAD_CONST')
        if load_const is not None:
            formatted = format_opcode(load_const)
            self.assertEqual(formatted, 'LOAD_CONST')

    def test_format_opcode_specialized(self):
        """Test format_opcode for a specialized opcode shows base in parens."""
        if not hasattr(opcode, '_specialized_opmap'):
            self.skipTest("No specialized opcodes in this Python version")
        if not hasattr(opcode, '_specializations'):
            self.skipTest("No specialization info in this Python version")

        # Find any specialized opcode to test
        for base_name, variants in opcode._specializations.items():
            if not variants:
                continue
            variant_name = variants[0]
            variant_opcode = opcode._specialized_opmap.get(variant_name)
            if variant_opcode is None:
                continue
            formatted = format_opcode(variant_opcode)
            # Should show: VARIANT_NAME (BASE_NAME)
            self.assertIn(variant_name, formatted)
            self.assertIn(f'({base_name})', formatted)
            return

        self.skipTest("No specialized opcodes found")

    def test_format_opcode_unknown(self):
        """Test format_opcode for an unknown opcode."""
        formatted = format_opcode(999)
        self.assertEqual(formatted, '<999>')


class TestLocationInCollectors(unittest.TestCase):
    """Tests for location tuple handling in each collector."""

    def _make_frames_with_location(self, location, opcode=None):
        """Create test frames with a specific location."""
        frame = MockFrameInfo("test.py", 0, "test_func", opcode)
        # Override the location
        frame.location = location
        return [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]

    def test_pstats_collector_with_location_info(self):
        """Test PstatsCollector handles LocationInfo properly."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Frame with LocationInfo
        frame = MockFrameInfo("test.py", 42, "my_function")
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        # Should extract lineno from location
        key = ("test.py", 42, "my_function")
        self.assertIn(key, collector.result)
        self.assertEqual(collector.result[key]["direct_calls"], 1)

    def test_pstats_collector_with_none_location(self):
        """Test PstatsCollector handles None location (synthetic frames)."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Create frame with None location (like GC frame)
        frame = MockFrameInfo("~", 0, "<GC>")
        frame.location = None  # Synthetic frame has no location
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        # Should use lineno=0 for None location
        key = ("~", 0, "<GC>")
        self.assertIn(key, collector.result)

    def test_collapsed_stack_with_location_info(self):
        """Test CollapsedStackCollector handles LocationInfo properly."""
        collector = CollapsedStackCollector(1000)

        frame1 = MockFrameInfo("main.py", 10, "main")
        frame2 = MockFrameInfo("utils.py", 25, "helper")
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame1, frame2], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        # Check that linenos were extracted correctly
        self.assertEqual(len(collector.stack_counter), 1)
        (path, _), count = list(collector.stack_counter.items())[0]
        # Reversed order: helper at top, main at bottom
        self.assertEqual(path[0], ("utils.py", 25, "helper"))
        self.assertEqual(path[1], ("main.py", 10, "main"))

    def test_flamegraph_collector_with_location_info(self):
        """Test FlamegraphCollector handles LocationInfo properly."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        frame = MockFrameInfo("app.py", 100, "process_data")
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        data = collector._convert_to_flamegraph_format()
        # Verify the function name includes lineno from location
        strings = data.get("strings", [])
        name_found = any("process_data" in s and "100" in s for s in strings if isinstance(s, str))
        self.assertTrue(name_found, f"Expected to find 'process_data' with line 100 in {strings}")

    def test_gecko_collector_with_location_info(self):
        """Test GeckoCollector handles LocationInfo properly."""
        collector = GeckoCollector(sample_interval_usec=1000)

        frame = MockFrameInfo("server.py", 50, "handle_request")
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        profile = collector._build_profile()
        # Check that the function was recorded
        self.assertEqual(len(profile["threads"]), 1)
        thread_data = profile["threads"][0]
        string_array = profile["shared"]["stringArray"]

        # Verify function name is in string table
        self.assertIn("handle_request", string_array)


class TestOpcodeHandling(unittest.TestCase):
    """Tests for opcode field handling in collectors."""

    def test_frame_with_opcode(self):
        """Test MockFrameInfo properly stores opcode."""
        frame = MockFrameInfo("test.py", 10, "my_func", opcode=90)
        self.assertEqual(frame.opcode, 90)
        # Verify tuple representation includes opcode
        self.assertEqual(frame[3], 90)
        self.assertEqual(len(frame), 4)

    def test_frame_without_opcode(self):
        """Test MockFrameInfo with no opcode defaults to None."""
        frame = MockFrameInfo("test.py", 10, "my_func")
        self.assertIsNone(frame.opcode)
        self.assertIsNone(frame[3])

    def test_collectors_ignore_opcode_for_key_generation(self):
        """Test that collectors use (filename, lineno, funcname) as key, not opcode."""
        collector = PstatsCollector(sample_interval_usec=1000)

        # Same function, different opcodes
        frame1 = MockFrameInfo("test.py", 10, "func", opcode=90)
        frame2 = MockFrameInfo("test.py", 10, "func", opcode=100)

        frames1 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame1], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        frames2 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame2], status=THREAD_STATUS_HAS_GIL)]
            )
        ]

        collector.collect(frames1)
        collector.collect(frames2)

        # Should be counted as same function (opcode not in key)
        key = ("test.py", 10, "func")
        self.assertIn(key, collector.result)
        self.assertEqual(collector.result[key]["direct_calls"], 2)


class TestGeckoOpcodeMarkers(unittest.TestCase):
    """Tests for GeckoCollector opcode interval markers."""

    def test_gecko_collector_opcodes_disabled_by_default(self):
        """Test that opcode tracking is disabled by default."""
        collector = GeckoCollector(sample_interval_usec=1000)
        self.assertFalse(collector.opcodes_enabled)

    def test_gecko_collector_opcodes_enabled(self):
        """Test that opcode tracking can be enabled."""
        collector = GeckoCollector(sample_interval_usec=1000, opcodes=True)
        self.assertTrue(collector.opcodes_enabled)

    def test_gecko_opcode_state_tracking(self):
        """Test that GeckoCollector tracks opcode state changes."""
        collector = GeckoCollector(sample_interval_usec=1000, opcodes=True)

        # First sample with opcode 90 (RAISE_VARARGS)
        frame1 = MockFrameInfo("test.py", 10, "func", opcode=90)
        frames1 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame1], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames1)

        # Should start tracking this opcode state
        self.assertIn(1, collector.opcode_state)
        state = collector.opcode_state[1]
        self.assertEqual(state[0], 90)  # opcode
        self.assertEqual(state[1], 10)  # lineno
        self.assertEqual(state[3], "func")  # funcname

    def test_gecko_opcode_state_change_emits_marker(self):
        """Test that opcode state change emits an interval marker."""
        collector = GeckoCollector(sample_interval_usec=1000, opcodes=True)

        # First sample: opcode 90
        frame1 = MockFrameInfo("test.py", 10, "func", opcode=90)
        frames1 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame1], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames1)

        # Second sample: different opcode 100
        frame2 = MockFrameInfo("test.py", 10, "func", opcode=100)
        frames2 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame2], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames2)

        # Should have emitted a marker for the first opcode
        thread_data = collector.threads[1]
        markers = thread_data["markers"]
        # At least one marker should have been added
        self.assertGreater(len(markers["name"]), 0)

    def test_gecko_opcode_markers_not_emitted_when_disabled(self):
        """Test that no opcode markers when opcodes=False."""
        collector = GeckoCollector(sample_interval_usec=1000, opcodes=False)

        frame1 = MockFrameInfo("test.py", 10, "func", opcode=90)
        frames1 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame1], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames1)

        frame2 = MockFrameInfo("test.py", 10, "func", opcode=100)
        frames2 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame2], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames2)

        # opcode_state should not be tracked
        self.assertEqual(len(collector.opcode_state), 0)

    def test_gecko_opcode_with_none_opcode(self):
        """Test that None opcode doesn't cause issues."""
        collector = GeckoCollector(sample_interval_usec=1000, opcodes=True)

        # Frame with no opcode (None)
        frame = MockFrameInfo("test.py", 10, "func", opcode=None)
        frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [frame], status=THREAD_STATUS_HAS_GIL)]
            )
        ]
        collector.collect(frames)

        # Should track the state but opcode is None
        self.assertIn(1, collector.opcode_state)
        self.assertIsNone(collector.opcode_state[1][0])


class TestCollectorFrameFormat(unittest.TestCase):
    """Tests verifying all collectors handle the 4-element frame format."""

    def _make_sample_frames(self):
        """Create sample frames with full format: (filename, location, funcname, opcode)."""
        return [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("app.py", 100, "main", opcode=90),
                            MockFrameInfo("utils.py", 50, "helper", opcode=100),
                            MockFrameInfo("lib.py", 25, "process", opcode=None),
                        ],
                        status=THREAD_STATUS_HAS_GIL,
                    )
                ],
            )
        ]

    def test_pstats_collector_frame_format(self):
        """Test PstatsCollector with 4-element frame format."""
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(self._make_sample_frames())

        # All three functions should be recorded
        self.assertEqual(len(collector.result), 3)
        self.assertIn(("app.py", 100, "main"), collector.result)
        self.assertIn(("utils.py", 50, "helper"), collector.result)
        self.assertIn(("lib.py", 25, "process"), collector.result)

    def test_collapsed_stack_frame_format(self):
        """Test CollapsedStackCollector with 4-element frame format."""
        collector = CollapsedStackCollector(sample_interval_usec=1000)
        collector.collect(self._make_sample_frames())

        self.assertEqual(len(collector.stack_counter), 1)
        (path, _), _ = list(collector.stack_counter.items())[0]
        # 3 frames in the path (reversed order)
        self.assertEqual(len(path), 3)

    def test_flamegraph_collector_frame_format(self):
        """Test FlamegraphCollector with 4-element frame format."""
        collector = FlamegraphCollector(sample_interval_usec=1000)
        collector.collect(self._make_sample_frames())

        data = collector._convert_to_flamegraph_format()
        # Should have processed the frames
        self.assertIn("children", data)

    def test_gecko_collector_frame_format(self):
        """Test GeckoCollector with 4-element frame format."""
        collector = GeckoCollector(sample_interval_usec=1000)
        collector.collect(self._make_sample_frames())

        profile = collector._build_profile()
        # Should have one thread with the frames
        self.assertEqual(len(profile["threads"]), 1)
        thread = profile["threads"][0]
        # Should have recorded 3 functions
        self.assertEqual(thread["funcTable"]["length"], 3)


class TestInternalFrameFiltering(unittest.TestCase):
    """Tests for filtering internal profiler frames from output."""

    def test_filter_internal_frames(self):
        """Test that _sync_coordinator frames are filtered from anywhere in stack."""
        from profiling.sampling.collector import filter_internal_frames

        # Stack with _sync_coordinator in the middle (realistic scenario)
        frames = [
            MockFrameInfo("user_script.py", 10, "user_func"),
            MockFrameInfo("/path/to/_sync_coordinator.py", 100, "main"),
            MockFrameInfo("<frozen runpy>", 87, "_run_code"),
        ]

        filtered = filter_internal_frames(frames)
        self.assertEqual(len(filtered), 2)
        self.assertEqual(filtered[0].filename, "user_script.py")
        self.assertEqual(filtered[1].filename, "<frozen runpy>")

    def test_pstats_collector_filters_internal_frames(self):
        """Test that PstatsCollector filters out internal frames."""
        collector = PstatsCollector(sample_interval_usec=1000)

        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("user_script.py", 10, "user_func"),
                            MockFrameInfo("/path/to/_sync_coordinator.py", 100, "main"),
                            MockFrameInfo("<frozen runpy>", 87, "_run_code"),
                        ],
                        status=THREAD_STATUS_HAS_GIL,
                    )
                ],
            )
        ]
        collector.collect(frames)

        self.assertEqual(len(collector.result), 2)
        self.assertIn(("user_script.py", 10, "user_func"), collector.result)
        self.assertIn(("<frozen runpy>", 87, "_run_code"), collector.result)

    def test_gecko_collector_filters_internal_frames(self):
        """Test that GeckoCollector filters out internal frames."""
        collector = GeckoCollector(sample_interval_usec=1000)

        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("app.py", 50, "run"),
                            MockFrameInfo("/lib/_sync_coordinator.py", 100, "main"),
                        ],
                        status=THREAD_STATUS_HAS_GIL,
                    )
                ],
            )
        ]
        collector.collect(frames)

        profile = collector._build_profile()
        string_array = profile["shared"]["stringArray"]

        # Should not contain _sync_coordinator functions
        for s in string_array:
            self.assertNotIn("_sync_coordinator", s)

    def test_flamegraph_collector_filters_internal_frames(self):
        """Test that FlamegraphCollector filters out internal frames."""
        collector = FlamegraphCollector(sample_interval_usec=1000)

        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("app.py", 50, "run"),
                            MockFrameInfo("/lib/_sync_coordinator.py", 100, "main"),
                            MockFrameInfo("<frozen runpy>", 87, "_run_code"),
                        ],
                        status=THREAD_STATUS_HAS_GIL,
                    )
                ],
            )
        ]
        collector.collect(frames)

        data = collector._convert_to_flamegraph_format()
        strings = data.get("strings", [])

        for s in strings:
            self.assertNotIn("_sync_coordinator", s)

    def test_collapsed_stack_collector_filters_internal_frames(self):
        """Test that CollapsedStackCollector filters out internal frames."""
        collector = CollapsedStackCollector(sample_interval_usec=1000)

        frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("app.py", 50, "run"),
                            MockFrameInfo("/lib/_sync_coordinator.py", 100, "main"),
                        ],
                        status=THREAD_STATUS_HAS_GIL,
                    )
                ],
            )
        ]
        collector.collect(frames)

        # Check that no stack contains _sync_coordinator
        for (call_tree, _), _ in collector.stack_counter.items():
            for filename, _, _ in call_tree:
                self.assertNotIn("_sync_coordinator", filename)
