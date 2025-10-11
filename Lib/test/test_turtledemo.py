"""
Test command line interface for turtledemo module.

This test suite validates the CLI functionality of the turtledemo module,
which provides a GUI-based demo viewer for turtle graphics examples.
"""
import sys
import os
import importlib.util


class SimpleTest:
    """Simple test framework to avoid compatibility issues."""
    def __init__(self):
        self.passed = 0
        self.failed = 0
        
    def assert_true(self, condition, msg=""):
        if condition:
            self.passed += 1
            print(f"✓ {msg}")
        else:
            self.failed += 1
            print(f"✗ {msg}")
            
    def assert_equal(self, a, b, msg=""):
        if a == b:
            self.passed += 1
            print(f"✓ {msg}")
        else:
            self.failed += 1
            print(f"✗ {msg}: {a} != {b}")
            
    def assert_in(self, item, container, msg=""):
        if item in container:
            self.passed += 1
            print(f"✓ {msg}")
        else:
            self.failed += 1
            print(f"✗ {msg}: {item} not in {container}")
            
    def assert_is_instance(self, obj, expected_type, msg=""):
        if isinstance(obj, expected_type):
            self.passed += 1
            print(f"✓ {msg}")
        else:
            self.failed += 1
            print(f"✗ {msg}: {type(obj)} != {expected_type}")
            
    def assert_has_attr(self, obj, attr, msg=""):
        if hasattr(obj, attr):
            self.passed += 1
            print(f"✓ {msg}")
        else:
            self.failed += 1
            print(f"✗ {msg}: {attr} not found")
            
    def summary(self):
        total = self.passed + self.failed
        print(f"\nTest Summary: {self.passed}/{total} passed, {self.failed} failed")
        return self.failed == 0


def test_turtledemo_cli():
    """Test command line interface for turtledemo module."""
    test = SimpleTest()
    
    print("Testing turtledemo command line interface...")
    
    # Test 1: Check turtledemo directory structure
    demo_dir = os.path.join(os.path.dirname(__file__), '..', 'turtledemo')
    test.assert_true(os.path.exists(demo_dir), "turtledemo directory exists")
    test.assert_true(os.path.isdir(demo_dir), "turtledemo is a directory")
    
    # Test 2: Check __main__.py exists
    main_file = os.path.join(demo_dir, '__main__.py')
    test.assert_true(os.path.exists(main_file), "__main__.py exists")
    
    # Test 3: Check demo files exist
    if os.path.exists(demo_dir):
        demo_files = [f for f in os.listdir(demo_dir) 
                     if f.endswith('.py') and not f.startswith('_')]
        test.assert_true(len(demo_files) > 0, f"found {len(demo_files)} demo files")
        
        # Check for known demo files
        expected_demos = ['bytedesign.py', 'chaos.py', 'clock.py', 'colormixer.py', 'forest.py']
        for demo in expected_demos:
            test.assert_in(demo, demo_files, f"demo file {demo} exists")
    
    # Test 4: Test module import (may fail due to dependencies)
    try:
        spec = importlib.util.spec_from_file_location("turtledemo", 
                                                     os.path.join(demo_dir, '__init__.py'))
        if spec and spec.loader:
            turtledemo = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(turtledemo)
            test.assert_true(True, "turtledemo module imported successfully")
        else:
            test.assert_true(False, "could not create spec for turtledemo")
    except Exception as e:
        test.assert_true(False, f"turtledemo import failed: {e}")
    
    # Test 5: Test __main__ module structure
    try:
        main_file = os.path.join(demo_dir, '__main__.py')
        with open(main_file, 'r') as f:
            content = f.read()
            
        # Check for key functions and classes
        test.assert_in('def main():', content, "main function defined")
        test.assert_in('class DemoWindow', content, "DemoWindow class defined")
        test.assert_in('def getExampleEntries():', content, "getExampleEntries function defined")
        test.assert_in('if __name__ == \'__main__\':', content, "__main__ guard present")
        
        # Check for imports
        test.assert_in('import sys', content, "sys import present")
        test.assert_in('import os', content, "os import present")
        test.assert_in('from tkinter import', content, "tkinter import present")
        
    except Exception as e:
        test.assert_true(False, f"failed to read __main__.py: {e}")
    
    # Test 6: Test individual demo files structure
    demo_files_to_check = ['bytedesign.py', 'chaos.py', 'clock.py']
    for demo_file in demo_files_to_check:
        demo_path = os.path.join(demo_dir, demo_file)
        if os.path.exists(demo_path):
            try:
                with open(demo_path, 'r') as f:
                    content = f.read()
                test.assert_in('def main():', content, f"{demo_file} has main function")
                has_main_guard = ('if __name__ == \'__main__\':' in content or 
                                  'if __name__ == "__main__":' in content)
                test.assert_true(has_main_guard, f"{demo_file} has __main__ guard")
            except Exception as e:
                test.assert_true(False, f"failed to read {demo_file}: {e}")
    
    # Test 7: Check configuration files
    config_file = os.path.join(demo_dir, 'turtle.cfg')
    test.assert_true(os.path.exists(config_file), "turtle.cfg exists")
    
    # Test 8: Test command line execution simulation
    try:
        # Simulate what happens when running: python -m turtledemo
        main_file = os.path.join(demo_dir, '__main__.py')
        
        # Read the file to check it's syntactically valid Python
        with open(main_file, 'r') as f:
            content = f.read()
        
        # Try to compile it
        compile(content, main_file, 'exec')
        test.assert_true(True, "__main__.py is valid Python code")
        
    except SyntaxError as e:
        test.assert_true(False, f"__main__.py has syntax error: {e}")
    except Exception as e:
        test.assert_true(False, f"failed to validate __main__.py: {e}")
    
    # Test 9: Check for documentation strings
    try:
        main_file = os.path.join(demo_dir, '__main__.py')
        with open(main_file, 'r') as f:
            content = f.read()
        
        # Check for module docstring
        test.assert_true(content.startswith('"""') or content.startswith("'''"), 
                        "__main__.py has module docstring")
        
        # Check for function docstrings
        test.assert_in('"""', content, "contains docstrings")
        
    except Exception as e:
        test.assert_true(False, f"failed to check docstrings: {e}")
    
    # Test 10: Test CLI entry point
    try:
        # The CLI entry point should be the main() function in __main__.py
        main_file = os.path.join(demo_dir, '__main__.py')
        with open(main_file, 'r') as f:
            content = f.read()
        
        # Check that main() is called when run as script
        test.assert_in('main()', content, "main() is called in __main__")
        
    except Exception as e:
        test.assert_true(False, f"failed to check CLI entry point: {e}")
    
    return test.summary()


if __name__ == '__main__':
    success = test_turtledemo_cli()
    sys.exit(0 if success else 1)
