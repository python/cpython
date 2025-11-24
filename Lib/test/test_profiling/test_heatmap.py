"""Tests for the heatmap collector (profiling.sampling)."""

import os
import shutil
import tempfile
import unittest

from profiling.sampling.stack_collector import HeatmapCollector

from test.support import captured_stdout, captured_stderr


class MockFrameInfo:
    """Mock FrameInfo for testing since the real one isn't accessible."""

    def __init__(self, filename, lineno, funcname):
        self.filename = filename
        self.lineno = lineno
        self.funcname = funcname

    def __repr__(self):
        return f"MockFrameInfo(filename='{self.filename}', lineno={self.lineno}, funcname='{self.funcname}')"


class MockThreadInfo:
    """Mock ThreadInfo for testing since the real one isn't accessible."""

    def __init__(self, thread_id, frame_info):
        self.thread_id = thread_id
        self.frame_info = frame_info

    def __repr__(self):
        return f"MockThreadInfo(thread_id={self.thread_id}, frame_info={self.frame_info})"


class MockInterpreterInfo:
    """Mock InterpreterInfo for testing since the real one isn't accessible."""

    def __init__(self, interpreter_id, threads):
        self.interpreter_id = interpreter_id
        self.threads = threads

    def __repr__(self):
        return f"MockInterpreterInfo(interpreter_id={self.interpreter_id}, threads={self.threads})"


class TestHeatmapCollector(unittest.TestCase):
    """Tests for HeatmapCollector functionality."""

    def test_heatmap_collector_basic(self):
        """Test basic HeatmapCollector functionality."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # Test empty state
        self.assertEqual(len(collector.file_samples), 0)
        self.assertEqual(len(collector.line_samples), 0)

        # Test collecting sample data
        test_frames = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(
                    1,
                    [("file.py", 10, "func1"), ("file.py", 20, "func2")],
                )]
            )
        ]
        collector.collect(test_frames)

        # Should have recorded samples for the file
        self.assertGreater(len(collector.line_samples), 0)
        self.assertIn("file.py", collector.file_samples)

        # Check that line samples were recorded
        file_data = collector.file_samples["file.py"]
        self.assertGreater(len(file_data), 0)

    def test_heatmap_collector_export(self):
        """Test heatmap HTML export functionality."""
        heatmap_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, heatmap_dir)

        collector = HeatmapCollector(sample_interval_usec=100)

        # Create test data with multiple files
        test_frames1 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [("file.py", 10, "func1"), ("file.py", 20, "func2")])],
            )
        ]
        test_frames2 = [
            MockInterpreterInfo(
                0,
                [MockThreadInfo(1, [("file.py", 10, "func1"), ("file.py", 20, "func2")])],
            )
        ]  # Same stack
        test_frames3 = [
            MockInterpreterInfo(0, [MockThreadInfo(1, [("other.py", 5, "other_func")])])
        ]

        collector.collect(test_frames1)
        collector.collect(test_frames2)
        collector.collect(test_frames3)

        # Export heatmap
        with (captured_stdout(), captured_stderr()):
            collector.export(heatmap_dir)

        # Verify index.html was created
        index_path = os.path.join(heatmap_dir, "index.html")
        self.assertTrue(os.path.exists(index_path))
        self.assertGreater(os.path.getsize(index_path), 0)

        # Check index contains HTML content
        with open(index_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Should be valid HTML
        self.assertIn("<!doctype html>", content.lower())
        self.assertIn("<html", content)
        self.assertIn("Tachyon Profiler", content)

        # Should contain file references
        self.assertIn("file.py", content)
        self.assertIn("other.py", content)

        # Verify individual file HTMLs were created
        file_htmls = [f for f in os.listdir(heatmap_dir) if f.startswith("file_") and f.endswith(".html")]
        self.assertGreater(len(file_htmls), 0)

        # Check one of the file HTMLs
        file_html_path = os.path.join(heatmap_dir, file_htmls[0])
        with open(file_html_path, "r", encoding="utf-8") as f:
            file_content = f.read()

        # Should contain heatmap styling and JavaScript
        self.assertIn("line-sample", file_content)
        self.assertIn("nav-btn", file_content)


if __name__ == "__main__":
    unittest.main()
