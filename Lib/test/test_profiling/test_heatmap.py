"""Tests for the heatmap collector (profiling.sampling)."""

import os
import shutil
import tempfile
import unittest
from pathlib import Path

from profiling.sampling.heatmap_collector import (
    HeatmapCollector,
    get_python_path_info,
    extract_module_name,
)

from test.support import captured_stdout, captured_stderr


# =============================================================================
# Unit Tests for Public Helper Functions
# =============================================================================

class TestPathInfoFunctions(unittest.TestCase):
    """Test public helper functions for path information."""

    def test_get_python_path_info_returns_dict(self):
        """Test that get_python_path_info returns a dictionary with expected keys."""
        path_info = get_python_path_info()

        self.assertIsInstance(path_info, dict)
        self.assertIn('stdlib', path_info)
        self.assertIn('site_packages', path_info)
        self.assertIn('sys_path', path_info)

    def test_get_python_path_info_stdlib_is_path_or_none(self):
        """Test that stdlib is either a Path object or None."""
        path_info = get_python_path_info()

        if path_info['stdlib'] is not None:
            self.assertIsInstance(path_info['stdlib'], Path)

    def test_get_python_path_info_site_packages_is_list(self):
        """Test that site_packages is a list."""
        path_info = get_python_path_info()

        self.assertIsInstance(path_info['site_packages'], list)
        for item in path_info['site_packages']:
            self.assertIsInstance(item, Path)

    def test_get_python_path_info_sys_path_is_list(self):
        """Test that sys_path is a list of Path objects."""
        path_info = get_python_path_info()

        self.assertIsInstance(path_info['sys_path'], list)
        for item in path_info['sys_path']:
            self.assertIsInstance(item, Path)

    def test_extract_module_name_with_none(self):
        """Test extract_module_name with None filename."""
        path_info = get_python_path_info()
        module_name, module_type = extract_module_name(None, path_info)

        self.assertEqual(module_name, 'unknown')
        self.assertEqual(module_type, 'other')

    def test_extract_module_name_with_empty_string(self):
        """Test extract_module_name with empty filename."""
        path_info = get_python_path_info()
        module_name, module_type = extract_module_name('', path_info)

        self.assertEqual(module_name, 'unknown')
        self.assertEqual(module_type, 'other')

    def test_extract_module_name_with_stdlib_file(self):
        """Test extract_module_name with a standard library file."""
        path_info = get_python_path_info()

        # Use os module as a known stdlib file
        if path_info['stdlib']:
            stdlib_file = str(path_info['stdlib'] / 'os.py')
            module_name, module_type = extract_module_name(stdlib_file, path_info)

            self.assertEqual(module_type, 'stdlib')
            self.assertIn('os', module_name)

    def test_extract_module_name_with_project_file(self):
        """Test extract_module_name with a project file."""
        path_info = get_python_path_info()

        # Create a mock project file path
        if path_info['sys_path']:
            # Use current directory as project path
            project_file = '/some/project/path/mymodule.py'
            module_name, module_type = extract_module_name(project_file, path_info)

            # Should classify as 'other' if not in sys.path
            self.assertIn(module_type, ['project', 'other'])

    def test_extract_module_name_removes_py_extension(self):
        """Test that .py extension is removed from module names."""
        path_info = get_python_path_info()

        # Test with a simple .py file
        module_name, module_type = extract_module_name('/path/to/test.py', path_info)

        # Module name should not contain .py
        self.assertNotIn('.py', module_name)

    def test_extract_module_name_with_special_files(self):
        """Test extract_module_name with special filenames like <string>."""
        path_info = get_python_path_info()

        special_files = ['<string>', '<stdin>', '[eval]']
        for special_file in special_files:
            module_name, module_type = extract_module_name(special_file, path_info)
            self.assertEqual(module_type, 'other')


# =============================================================================
# Unit Tests for HeatmapCollector Public API
# =============================================================================

class TestHeatmapCollectorInit(unittest.TestCase):
    """Test HeatmapCollector initialization."""

    def test_init_creates_empty_data_structures(self):
        """Test that __init__ creates empty data structures."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # Check that data structures are initialized
        self.assertIsInstance(collector.line_samples, dict)
        self.assertIsInstance(collector.file_samples, dict)
        self.assertIsInstance(collector.line_self_samples, dict)
        self.assertIsInstance(collector.file_self_samples, dict)
        self.assertIsInstance(collector.call_graph, dict)
        self.assertIsInstance(collector.callers_graph, dict)
        self.assertIsInstance(collector.function_definitions, dict)
        self.assertIsInstance(collector.edge_samples, dict)

        # Check that they're empty
        self.assertEqual(len(collector.line_samples), 0)
        self.assertEqual(len(collector.file_samples), 0)
        self.assertEqual(len(collector.line_self_samples), 0)
        self.assertEqual(len(collector.file_self_samples), 0)

    def test_init_sets_total_samples_to_zero(self):
        """Test that total samples starts at zero."""
        collector = HeatmapCollector(sample_interval_usec=100)
        self.assertEqual(collector._total_samples, 0)

    def test_init_gets_path_info(self):
        """Test that path info is retrieved during init."""
        collector = HeatmapCollector(sample_interval_usec=100)
        self.assertIsNotNone(collector._path_info)
        self.assertIn('stdlib', collector._path_info)


class TestHeatmapCollectorSetStats(unittest.TestCase):
    """Test HeatmapCollector.set_stats() method."""

    def test_set_stats_stores_all_parameters(self):
        """Test that set_stats stores all provided parameters."""
        collector = HeatmapCollector(sample_interval_usec=100)

        collector.set_stats(
            sample_interval_usec=500,
            duration_sec=10.5,
            sample_rate=99.5,
            error_rate=0.5
        )

        self.assertEqual(collector.stats['sample_interval_usec'], 500)
        self.assertEqual(collector.stats['duration_sec'], 10.5)
        self.assertEqual(collector.stats['sample_rate'], 99.5)
        self.assertEqual(collector.stats['error_rate'], 0.5)

    def test_set_stats_includes_system_info(self):
        """Test that set_stats includes Python and platform info."""
        collector = HeatmapCollector(sample_interval_usec=100)
        collector.set_stats(sample_interval_usec=100, duration_sec=1.0, sample_rate=100.0)

        self.assertIn('python_version', collector.stats)
        self.assertIn('python_implementation', collector.stats)
        self.assertIn('platform', collector.stats)

    def test_set_stats_accepts_kwargs(self):
        """Test that set_stats accepts additional kwargs."""
        collector = HeatmapCollector(sample_interval_usec=100)

        collector.set_stats(
            sample_interval_usec=100,
            duration_sec=1.0,
            sample_rate=100.0,
            custom_key='custom_value',
            another_key=42
        )

        self.assertEqual(collector.stats['custom_key'], 'custom_value')
        self.assertEqual(collector.stats['another_key'], 42)

    def test_set_stats_with_none_error_rate(self):
        """Test set_stats with error_rate=None."""
        collector = HeatmapCollector(sample_interval_usec=100)
        collector.set_stats(sample_interval_usec=100, duration_sec=1.0, sample_rate=100.0)

        self.assertIn('error_rate', collector.stats)
        self.assertIsNone(collector.stats['error_rate'])


class TestHeatmapCollectorProcessFrames(unittest.TestCase):
    """Test HeatmapCollector.process_frames() method."""

    def test_process_frames_increments_total_samples(self):
        """Test that process_frames increments total samples count."""
        collector = HeatmapCollector(sample_interval_usec=100)

        initial_count = collector._total_samples
        frames = [('file.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        self.assertEqual(collector._total_samples, initial_count + 1)

    def test_process_frames_records_line_samples(self):
        """Test that process_frames records line samples."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('test.py', 5, 'test_func')]
        collector.process_frames(frames, thread_id=1)

        # Check that line was recorded
        self.assertIn(('test.py', 5), collector.line_samples)
        self.assertEqual(collector.line_samples[('test.py', 5)], 1)

    def test_process_frames_records_multiple_lines_in_stack(self):
        """Test that process_frames records all lines in a stack."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [
            ('file1.py', 10, 'func1'),
            ('file2.py', 20, 'func2'),
            ('file3.py', 30, 'func3')
        ]
        collector.process_frames(frames, thread_id=1)

        # All frames should be recorded
        self.assertIn(('file1.py', 10), collector.line_samples)
        self.assertIn(('file2.py', 20), collector.line_samples)
        self.assertIn(('file3.py', 30), collector.line_samples)

    def test_process_frames_distinguishes_self_samples(self):
        """Test that process_frames distinguishes self (leaf) samples."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [
            ('leaf.py', 5, 'leaf_func'),  # This is the leaf (top of stack)
            ('caller.py', 10, 'caller_func')
        ]
        collector.process_frames(frames, thread_id=1)

        # Leaf should have self sample
        self.assertIn(('leaf.py', 5), collector.line_self_samples)
        self.assertEqual(collector.line_self_samples[('leaf.py', 5)], 1)

        # Caller should NOT have self sample
        self.assertNotIn(('caller.py', 10), collector.line_self_samples)

    def test_process_frames_accumulates_samples(self):
        """Test that multiple calls accumulate samples."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('file.py', 10, 'func')]

        collector.process_frames(frames, thread_id=1)
        collector.process_frames(frames, thread_id=1)
        collector.process_frames(frames, thread_id=1)

        self.assertEqual(collector.line_samples[('file.py', 10)], 3)
        self.assertEqual(collector._total_samples, 3)

    def test_process_frames_ignores_invalid_frames(self):
        """Test that process_frames ignores invalid frames."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # These should be ignored
        invalid_frames = [
            ('<string>', 1, 'test'),
            ('[eval]', 1, 'test'),
            ('', 1, 'test'),
            (None, 1, 'test'),
            ('__init__', 0, 'test'),  # Special invalid frame
        ]

        for frame in invalid_frames:
            collector.process_frames([frame], thread_id=1)

        # Should not record these invalid frames
        for frame in invalid_frames:
            if frame[0]:
                self.assertNotIn((frame[0], frame[1]), collector.line_samples)

    def test_process_frames_builds_call_graph(self):
        """Test that process_frames builds call graph relationships."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [
            ('callee.py', 5, 'callee_func'),
            ('caller.py', 10, 'caller_func')
        ]
        collector.process_frames(frames, thread_id=1)

        # Check that call relationship was recorded
        caller_key = ('caller.py', 10)
        self.assertIn(caller_key, collector.call_graph)

        # Check callers graph
        callee_key = ('callee.py', 5)
        self.assertIn(callee_key, collector.callers_graph)

    def test_process_frames_records_function_definitions(self):
        """Test that process_frames records function definition locations."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('module.py', 42, 'my_function')]
        collector.process_frames(frames, thread_id=1)

        self.assertIn(('module.py', 'my_function'), collector.function_definitions)
        self.assertEqual(collector.function_definitions[('module.py', 'my_function')], 42)

    def test_process_frames_tracks_edge_samples(self):
        """Test that process_frames tracks edge sample counts."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [
            ('callee.py', 5, 'callee'),
            ('caller.py', 10, 'caller')
        ]

        # Process same call stack multiple times
        collector.process_frames(frames, thread_id=1)
        collector.process_frames(frames, thread_id=1)

        # Check that edge count is tracked
        self.assertGreater(len(collector.edge_samples), 0)

    def test_process_frames_handles_empty_frames(self):
        """Test that process_frames handles empty frame list."""
        collector = HeatmapCollector(sample_interval_usec=100)

        initial_count = collector._total_samples
        collector.process_frames([], thread_id=1)

        # Should still increment total samples
        self.assertEqual(collector._total_samples, initial_count + 1)

    def test_process_frames_with_file_samples_dict(self):
        """Test that file_samples dict is properly populated."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('test.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        self.assertIn('test.py', collector.file_samples)
        self.assertIn(10, collector.file_samples['test.py'])
        self.assertEqual(collector.file_samples['test.py'][10], 1)


class TestHeatmapCollectorExport(unittest.TestCase):
    """Test HeatmapCollector.export() method."""

    def setUp(self):
        """Set up test directory."""
        self.test_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.test_dir)

    def test_export_creates_output_directory(self):
        """Test that export creates the output directory."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # Add some data
        frames = [('test.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        output_path = os.path.join(self.test_dir, 'heatmap_output')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        self.assertTrue(os.path.exists(output_path))
        self.assertTrue(os.path.isdir(output_path))

    def test_export_creates_index_html(self):
        """Test that export creates index.html."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('test.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        output_path = os.path.join(self.test_dir, 'heatmap_output')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        index_path = os.path.join(output_path, 'index.html')
        self.assertTrue(os.path.exists(index_path))

    def test_export_creates_file_htmls(self):
        """Test that export creates individual file HTMLs."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('test.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        output_path = os.path.join(self.test_dir, 'heatmap_output')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        # Check for file_XXXX.html files
        html_files = [f for f in os.listdir(output_path)
                      if f.startswith('file_') and f.endswith('.html')]
        self.assertGreater(len(html_files), 0)

    def test_export_with_empty_data(self):
        """Test export with no data collected."""
        collector = HeatmapCollector(sample_interval_usec=100)

        output_path = os.path.join(self.test_dir, 'empty_output')

        # Should handle empty data gracefully
        with captured_stdout(), captured_stderr():
            collector.export(output_path)

    def test_export_handles_html_suffix(self):
        """Test that export handles .html suffix in output path."""
        collector = HeatmapCollector(sample_interval_usec=100)

        frames = [('test.py', 10, 'func')]
        collector.process_frames(frames, thread_id=1)

        # Path with .html suffix should be stripped
        output_path = os.path.join(self.test_dir, 'output.html')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        # Should create directory without .html
        expected_dir = os.path.join(self.test_dir, 'output')
        self.assertTrue(os.path.exists(expected_dir))

    def test_export_with_multiple_files(self):
        """Test export with multiple files."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # Add samples for multiple files
        collector.process_frames([('file1.py', 10, 'func1')], thread_id=1)
        collector.process_frames([('file2.py', 20, 'func2')], thread_id=1)
        collector.process_frames([('file3.py', 30, 'func3')], thread_id=1)

        output_path = os.path.join(self.test_dir, 'multi_file')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        # Should create HTML for each file
        html_files = [f for f in os.listdir(output_path)
                      if f.startswith('file_') and f.endswith('.html')]
        self.assertGreaterEqual(len(html_files), 3)

    def test_export_index_contains_file_references(self):
        """Test that index.html contains references to profiled files."""
        collector = HeatmapCollector(sample_interval_usec=100)
        collector.set_stats(sample_interval_usec=100, duration_sec=1.0, sample_rate=100.0)

        frames = [('mytest.py', 10, 'my_func')]
        collector.process_frames(frames, thread_id=1)

        output_path = os.path.join(self.test_dir, 'test_output')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        index_path = os.path.join(output_path, 'index.html')
        with open(index_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Should contain reference to the file
        self.assertIn('mytest', content)

    def test_export_file_html_has_line_numbers(self):
        """Test that exported file HTML contains line numbers."""
        collector = HeatmapCollector(sample_interval_usec=100)

        # Create a temporary Python file
        temp_file = os.path.join(self.test_dir, 'temp_source.py')
        with open(temp_file, 'w') as f:
            f.write('def test():\n    pass\n')

        frames = [(temp_file, 1, 'test')]
        collector.process_frames(frames, thread_id=1)

        output_path = os.path.join(self.test_dir, 'line_test')

        with captured_stdout(), captured_stderr():
            collector.export(output_path)

        # Find the generated file HTML
        html_files = [f for f in os.listdir(output_path)
                      if f.startswith('file_') and f.endswith('.html')]

        if html_files:
            with open(os.path.join(output_path, html_files[0]), 'r', encoding='utf-8') as f:
                content = f.read()

            # Should have line-related content
            self.assertIn('line-', content)


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
