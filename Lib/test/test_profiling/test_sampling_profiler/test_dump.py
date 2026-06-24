"""Tests for one-shot sampling profiler stack dumps."""

from collections import namedtuple
import os
import opcode
import tempfile
import unittest

import _colorize

from profiling.sampling.constants import (
    THREAD_STATUS_GIL_REQUESTED,
    THREAD_STATUS_HAS_EXCEPTION,
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_MAIN_THREAD,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_UNKNOWN,
)
from profiling.sampling.dump import format_stack_dump

from .mocks import (
    LocationInfo as StructseqLocationInfo,
    MockAwaitedInfo,
    MockCoroInfo,
    MockFrameInfo,
    MockInterpreterInfo,
    MockTaskInfo,
    MockThreadInfo,
)

try:
    import _remote_debugging  # noqa: F401
except ImportError:
    _remote_debugging = None


StructseqInterpreterInfo = namedtuple(
    "StructseqInterpreterInfo",
    ["interpreter_id", "threads"],
)
StructseqThreadInfo = namedtuple(
    "StructseqThreadInfo",
    ["thread_id", "status", "frame_info"],
)
StructseqFrameInfo = namedtuple(
    "StructseqFrameInfo",
    ["filename", "location", "funcname", "opcode"],
)


class TestStackDumpFormatting(unittest.TestCase):
    def test_format_stack_dump_single_thread(self):
        frames = [
            MockFrameInfo("leaf.py", 10, "leaf"),
            MockFrameInfo("root.py", 1, "root"),
        ]
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        frames,
                        status=(
                            THREAD_STATUS_MAIN_THREAD
                            | THREAD_STATUS_HAS_GIL
                            | THREAD_STATUS_ON_CPU
                        ),
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(main thread, has GIL, on CPU; most recent call last):",
            output,
        )
        self.assertNotIn("Interpreter 0", output)
        self.assertLess(
            output.index('File "root.py", line 1, in root'),
            output.index('File "leaf.py", line 10, in leaf'),
        )
        self.assertIn('  File "root.py", line 1, in root', output)
        self.assertIn('  File "leaf.py", line 10, in leaf', output)
        self.assertNotIn("\x1b[", output)

    def test_format_stack_dump_with_structseq_tuples(self):
        stack_frames = [
            StructseqInterpreterInfo(
                0,
                [
                    StructseqThreadInfo(
                        123,
                        THREAD_STATUS_HAS_GIL,
                        [
                            StructseqFrameInfo(
                                "file.py",
                                StructseqLocationInfo(5, 5, -1, -1),
                                "func",
                                None,
                            )
                        ],
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(has GIL; most recent call last):",
            output,
        )
        self.assertIn('  File "file.py", line 5, in func', output)

    def test_format_stack_dump_shows_interpreter_ids_when_multiple(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(100, [MockFrameInfo("main.py", 1, "main")])],
            ),
            MockInterpreterInfo(
                1,
                [MockThreadInfo(200, [MockFrameInfo("sub.py", 2, "sub")])],
            ),
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 100 "
            "(interpreter 0; idle; most recent call last):",
            output,
        )
        self.assertIn(
            "Stack dump, thread 200 "
            "(interpreter 1; idle; most recent call last):",
            output,
        )

    def test_format_stack_dump_omits_unknown_status(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [MockFrameInfo("file.py", 5, "func")],
                        status=THREAD_STATUS_UNKNOWN,
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 (most recent call last):",
            output,
        )
        self.assertNotIn("unknown", output)

    def test_format_stack_dump_omits_unknown_when_other_status_exists(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [MockFrameInfo("file.py", 5, "func")],
                        status=THREAD_STATUS_UNKNOWN | THREAD_STATUS_ON_CPU,
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(on CPU; most recent call last):",
            output,
        )
        self.assertNotIn("unknown", output)

    def test_format_stack_dump_labels_known_idle_status(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [MockFrameInfo("file.py", 5, "func")],
                        status=0,
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(idle; most recent call last):",
            output,
        )

    def test_format_stack_dump_status_does_not_add_idle_to_waiting_thread(self):
        status = (
            THREAD_STATUS_MAIN_THREAD
            | THREAD_STATUS_GIL_REQUESTED
            | THREAD_STATUS_HAS_EXCEPTION
        )
        stack_frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(123, [MockFrameInfo("file.py", 5, "func")], status=status)],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(main thread, waiting for GIL, has exception; most recent call last):",
            output,
        )
        self.assertNotIn("idle", output)

    def test_format_stack_dump_formats_opcode_name(self):
        load_const = opcode.opmap["LOAD_CONST"]
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [
                            StructseqFrameInfo(
                                "file.py",
                                StructseqLocationInfo(5, 5, -1, -1),
                                "func",
                                load_const,
                            )
                        ],
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn("opcode=LOAD_CONST", output)
        self.assertNotIn(f"opcode={load_const}", output)

    def test_format_stack_dump_formats_unknown_opcode(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [
                            StructseqFrameInfo(
                                "file.py",
                                StructseqLocationInfo(5, 5, -1, -1),
                                "func",
                                999,
                            )
                        ],
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn("opcode=<999>", output)

    def test_format_stack_dump_prefers_qualname_attribute(self):
        class Frame:
            filename = "file.py"
            location = StructseqLocationInfo(5, 5, -1, -1)
            funcname = "inner"
            qualname = "Outer.inner"
            opcode = None

        stack_frames = [
            MockInterpreterInfo(0, [MockThreadInfo(123, [Frame()])])
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn('  File "file.py", line 5, in Outer.inner', output)
        self.assertNotIn("in inner", output)

    def test_format_stack_dump_filters_internal_frames(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [
                            MockFrameInfo("user.py", 10, "user"),
                            MockFrameInfo("_sync_coordinator.py", 20, "internal"),
                        ],
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn('  File "user.py", line 10, in user', output)
        self.assertNotIn("_sync_coordinator.py", output)

    @unittest.skipIf(_remote_debugging is None, "requires _remote_debugging")
    def test_format_stack_dump_async_task(self):
        task = MockTaskInfo(
            task_id=1,
            task_name="Task-1",
            coroutine_stack=[
                MockCoroInfo(
                    "Task-1",
                    [MockFrameInfo("task.py", 5, "waiter")],
                )
            ],
        )
        stack_frames = [MockAwaitedInfo(thread_id=123, awaited_by=[task])]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn(
            "Stack dump for PID 42, thread 123 "
            "(task 1; most recent call last):",
            output,
        )
        self.assertIn('  File "task.py", line 5, in waiter', output)

    def test_format_stack_dump_strips_source_like_traceback(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [
                            StructseqFrameInfo(
                                __file__,
                                StructseqLocationInfo(1, 1, -1, -1),
                                "<module>",
                                None,
                            )
                        ],
                    )
                ],
            )
        ]

        output = format_stack_dump(stack_frames, pid=42, colorize=False)

        self.assertIn('  File "', output)
        self.assertIn('", line 1, in <module>', output)
        self.assertIn('"""Tests for one-shot sampling profiler stack dumps."""', output)
        self.assertNotIn("^^^^^^^^", output)

    def test_format_stack_dump_highlights_source_range(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [
                    MockThreadInfo(
                        123,
                        [
                            StructseqFrameInfo(
                                __file__,
                                StructseqLocationInfo(1, 1, 0, 3),
                                "<module>",
                                None,
                            )
                        ],
                    )
                ],
            )
        ]
        theme = _colorize.get_theme(force_color=True).profiler_dump

        output = format_stack_dump(stack_frames, pid=42, colorize=True)

        self.assertIn(
            f"{theme.source_highlight}\"\"\"{theme.reset}",
            output,
        )
        self.assertNotIn("^^^^^^^^", _colorize.decolor(output))

    def test_format_stack_dump_highlights_source_range_after_trimming(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            filename = os.path.join(tmp_dir, "target.py")
            with open(filename, "w", encoding="utf-8") as file:
                file.write("    result = call(arg)\n")
            stack_frames = [
                MockInterpreterInfo(
                    0,
                    [
                        MockThreadInfo(
                            123,
                            [
                                StructseqFrameInfo(
                                    filename,
                                    StructseqLocationInfo(1, 1, 13, 17),
                                    "<module>",
                                    None,
                                )
                            ],
                        )
                    ],
                )
            ]
            theme = _colorize.get_theme(force_color=True).profiler_dump

            output = format_stack_dump(stack_frames, pid=42, colorize=True)

        self.assertIn(f"{theme.source_highlight}call{theme.reset}", output)
        self.assertIn("\n    result = call(arg)\n", _colorize.decolor(output))
        self.assertNotIn("\n        result = call(arg)\n", _colorize.decolor(output))

    def test_format_stack_dump_empty(self):
        output = format_stack_dump([], pid=42, colorize=False)

        self.assertEqual("No Python stacks found for PID 42\n", output)

    def test_format_stack_dump_colorized(self):
        stack_frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(123, [MockFrameInfo("file.py", 5, "func")])],
            )
        ]
        theme = _colorize.get_theme(force_color=True).profiler_dump

        output = format_stack_dump(stack_frames, pid=42, colorize=True)

        self.assertIn(theme.header, output)
        self.assertIn(theme.filename, output)
        self.assertIn(theme.reset, output)


if __name__ == "__main__":
    unittest.main()
