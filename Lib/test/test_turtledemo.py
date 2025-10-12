import unittest
import os
import importlib.util


class TurtledemoCommandLineTest(unittest.TestCase):
    """Test command line interface for turtledemo module."""

    def setUp(self):
        """Set up test fixtures."""
        self.demo_dir = os.path.join(os.path.dirname(__file__), '..', 'turtledemo')
        self.main_file = os.path.join(self.demo_dir, '__main__.py')

    def test_turtledemo_directory_exists(self):
        """Test that turtledemo directory exists."""
        self.assertTrue(os.path.exists(self.demo_dir), "turtledemo directory exists")
        self.assertTrue(os.path.isdir(self.demo_dir), "turtledemo is a directory")

    def test_main_file_exists(self):
        """Test that __main__.py exists."""
        self.assertTrue(os.path.exists(self.main_file), "__main__.py exists")

    def test_demo_files_exist(self):
        """Test that demo files exist."""
        if os.path.exists(self.demo_dir):
            demo_files = [f for f in os.listdir(self.demo_dir)
                         if f.endswith('.py') and not f.startswith('_')]
            self.assertGreater(len(demo_files), 0, f"found {len(demo_files)} demo files")

            # Check for known demo files
            expected_demos = ['bytedesign.py', 'chaos.py', 'clock.py', 'colormixer.py', 'forest.py']
            for demo in expected_demos:
                self.assertIn(demo, demo_files, f"demo file {demo} exists")

    def test_module_import(self):
        """Test that turtledemo module can be imported."""
        try:
            spec = importlib.util.spec_from_file_location("turtledemo",
                                                         os.path.join(self.demo_dir, '__init__.py'))
            if spec and spec.loader:
                turtledemo = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(turtledemo)
                self.assertTrue(True, "turtledemo module imported successfully")
            else:
                self.fail("could not create spec for turtledemo")
        except Exception as e:
            self.fail(f"turtledemo import failed: {e}")

    def test_main_module_structure(self):
        """Test __main__ module structure."""
        try:
            with open(self.main_file, 'r') as f:
                content = f.read()

            # Check for key functions and classes
            self.assertIn('def main():', content, "main function defined")
            self.assertIn('class DemoWindow', content, "DemoWindow class defined")
            self.assertIn('def getExampleEntries():', content, "getExampleEntries function defined")
            self.assertIn('if __name__ == \'__main__\':', content, "__main__ guard present")

            # Check for imports
            self.assertIn('import sys', content, "sys import present")
            self.assertIn('import os', content, "os import present")
            self.assertIn('from tkinter import', content, "tkinter import present")

        except Exception as e:
            self.fail(f"failed to read __main__.py: {e}")

    def test_individual_demo_files_structure(self):
        """Test individual demo files structure."""
        demo_files_to_check = ['bytedesign.py', 'chaos.py', 'clock.py']
        for demo_file in demo_files_to_check:
            with self.subTest(demo_file=demo_file):
                demo_path = os.path.join(self.demo_dir, demo_file)
                if os.path.exists(demo_path):
                    try:
                        with open(demo_path, 'r') as f:
                            content = f.read()
                        self.assertIn('def main():', content, f"{demo_file} has main function")
                        has_main_guard = ('if __name__ == \'__main__\':' in content or
                                          'if __name__ == "__main__":' in content)
                        self.assertTrue(has_main_guard, f"{demo_file} has __main__ guard")
                    except Exception as e:
                        self.fail(f"failed to read {demo_file}: {e}")

    def test_configuration_files(self):
        """Test configuration files exist."""
        config_file = os.path.join(self.demo_dir, 'turtle.cfg')
        self.assertTrue(os.path.exists(config_file), "turtle.cfg exists")

    def test_main_file_syntax(self):
        """Test that __main__.py is valid Python code."""
        try:
            with open(self.main_file, 'r') as f:
                content = f.read()

            # Try to compile it
            compile(content, self.main_file, 'exec')
            # If we get here, compilation succeeded
            self.assertTrue(True, "__main__.py is valid Python code")

        except SyntaxError as e:
            self.fail(f"__main__.py has syntax error: {e}")
        except Exception as e:
            self.fail(f"failed to validate __main__.py: {e}")

    def test_documentation_strings(self):
        """Test for documentation strings."""
        try:
            with open(self.main_file, 'r') as f:
                content = f.read()

            # Check for module docstring
            has_docstring = (content.startswith('"""') or content.startswith("'''"))
            self.assertTrue(has_docstring, "__main__.py has module docstring")

            # Check for function docstrings
            self.assertIn('"""', content, "contains docstrings")

        except Exception as e:
            self.fail(f"failed to check docstrings: {e}")

    def test_cli_entry_point(self):
        """Test CLI entry point."""
        try:
            with open(self.main_file, 'r') as f:
                content = f.read()

            # Check that main() is called when run as script
            self.assertIn('main()', content, "main() is called in __main__")

        except Exception as e:
            self.fail(f"failed to check CLI entry point: {e}")


if __name__ == '__main__':
    unittest.main()
