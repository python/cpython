"""Tests for sampling profiler collector components."""

import json
import marshal
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
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import captured_stdout, captured_stderr

from .mocks import MockFrameInfo, MockThreadInfo, MockInterpreterInfo
from .helpers import close_and_unlink


class TestSampleProfilerComponents(unittest.TestCase):
    """Unit tests for individual profiler components."""

    def test_mock_frame_info_with_empty_and_unicode_values(self):
        """Test MockFrameInfo handles empty strings, unicode characters, and very long names correctly."""
        # Test with empty strings
        frame = MockFrameInfo("", 0, "")
        self.assertEqual(frame.filename, "")
        self.assertEqual(frame.lineno, 0)
        self.assertEqual(frame.funcname, "")
        self.assertIn("filename=''", repr(frame))

        # Test with unicode characters
        frame = MockFrameInfo("文件.py", 42, "函数名")
        self.assertEqual(frame.filename, "文件.py")
        self.assertEqual(frame.funcname, "函数名")

        # Test with very long names
        long_filename = "x" * 1000 + ".py"
        long_funcname = "func_" + "x" * 1000
        frame = MockFrameInfo(long_filename, 999999, long_funcname)
        self.assertEqual(frame.filename, long_filename)
        self.assertEqual(frame.lineno, 999999)
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
                [MockThreadInfo(None, [MockFrameInfo("file.py", 10, "func")])],
            )
        ]
        collector.collect(test_frames)
        # Should still process the frames
        self.assertEqual(len(collector.result), 1)

        # Test collecting duplicate frames in same sample
        test_frames = [
            MockInterpreterInfo(
                0,  # interpreter_id
                [
                    MockThreadInfo(
                        1,
                        [
                            MockFrameInfo("file.py", 10, "func1"),
                            MockFrameInfo("file.py", 10, "func1"),  # Duplicate
                        ],
                    )
                ],
            )
        ]
        collector = PstatsCollector(sample_interval_usec=1000)
        collector.collect(test_frames)
        # Should count both occurrences
        self.assertEqual(
            collector.result[("file.py", 10, "func1")]["cumulative_calls"], 2
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
                0, [MockThreadInfo(1, [("file.py", 10, "func")])]
            )
        ]
        collector.collect(test_frames)
        self.assertEqual(len(collector.stack_counter), 1)
        (((path, thread_id), count),) = collector.stack_counter.items()
        self.assertEqual(path, (("file.py", 10, "func"),))
        self.assertEqual(thread_id, 1)
        self.assertEqual(count, 1)

        # Test with very deep stack
        deep_stack = [(f"file{i}.py", i, f"func{i}") for i in range(100)]
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
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
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
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [("other.py", 5, "other_func")])]
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
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
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
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [("other.py", 5, "other_func")])]
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
        self.assertIn("Python Performance Flamegraph", content)
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
                        [("file.py", 10, "func1"), ("file.py", 20, "func2")],
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
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        1, [("file.py", 10, "func1"), ("file.py", 20, "func2")]
                    )
                ],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(
                0, [MockThreadInfo(1, [("other.py", 5, "other_func")])]
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
        try:
            from _remote_debugging import (
                THREAD_STATUS_HAS_GIL,
                THREAD_STATUS_ON_CPU,
                THREAD_STATUS_GIL_REQUESTED,
            )
        except ImportError:
            THREAD_STATUS_HAS_GIL = 1 << 0
            THREAD_STATUS_ON_CPU = 1 << 1
            THREAD_STATUS_GIL_REQUESTED = 1 << 3

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
                            [("test.py", 10, "python_func")],
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
                            [("test.py", 15, "wait_func")],
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
                            [("test.py", 20, "python_func2")],
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
                            [("native.c", 100, "native_func")],
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
