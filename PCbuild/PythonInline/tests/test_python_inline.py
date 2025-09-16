#!/usr/bin/env python3
"""
Test script for Python Inline Runner functionality
"""

import subprocess
import sys
import os
import tempfile

def run_python_inline(args, expected_output=None, should_fail=False):
    """Run python_inline with given args and check result"""
    cmd = ["python_inline.exe"] + args
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if should_fail:
            if result.returncode == 0:
                print(f"? Expected failure but command succeeded: {' '.join(cmd)}")
                return False
            else:
                print(f"? Command failed as expected: {' '.join(cmd)}")
                return True
        else:
            if result.returncode != 0:
                print(f"? Command failed: {' '.join(cmd)}")
                print(f"   stderr: {result.stderr}")
                return False
            
            if expected_output and expected_output not in result.stdout:
                print(f"? Expected output not found: {' '.join(cmd)}")
                print(f"   Expected: {expected_output}")
                print(f"   Got: {result.stdout}")
                return False
            
            print(f"? Command succeeded: {' '.join(cmd)}")
            if result.stdout.strip():
                print(f"   Output: {result.stdout.strip()}")
            return True
            
    except subprocess.TimeoutExpired:
        print(f"? Command timed out: {' '.join(cmd)}")
        return False
    except FileNotFoundError:
        print(f"? python_inline.exe not found. Make sure it's built and in PATH or current directory.")
        return False

def test_basic_functionality():
    """Test basic functionality"""
    print("Testing basic functionality...")
    
    tests = [
        # Test help
        (["-h"], "Usage: python_inline"),
        
        # Test version
        (["-v"], "Python"),
        
        # Test simple script
        (["-c", "print('Hello, World!')"], "Hello, World!"),
        
        # Test math
        (["-c", "print(2 + 2)"], "4"),
        
        # Test sys.argv
        (["-c", "import sys; print(len(sys.argv))", "arg1", "arg2"], "3"),
        
        # Test multiline script
        (["-c", "x = 10\ny = 20\nprint(x + y)"], "30"),
        
        # Test import
        (["-c", "import math; print(int(math.pi))"], "3"),
    ]
    
    passed = 0
    total = len(tests)
    
    for args, expected in tests:
        if run_python_inline(args, expected):
            passed += 1
    
    print(f"\nBasic functionality: {passed}/{total} tests passed")
    return passed == total

def test_file_functionality():
    """Test file execution functionality"""
    print("\nTesting file functionality...")
    
    # Create a temporary Python file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
        f.write("""
import sys
print(f"Script executed with {len(sys.argv)} arguments")
for i, arg in enumerate(sys.argv):
    print(f"  argv[{i}]: {arg}")
""")
        temp_file = f.name
    
    try:
        # Test file execution
        success = run_python_inline(
            ["--file=" + temp_file, "test_arg1", "test_arg2"], 
            "Script executed with 3 arguments"
        )
        
        print(f"\nFile functionality: {'PASSED' if success else 'FAILED'}")
        return success
        
    finally:
        # Clean up
        try:
            os.unlink(temp_file)
        except:
            pass

def test_error_conditions():
    """Test error conditions"""
    print("\nTesting error conditions...")
    
    error_tests = [
        # Test missing script
        ([], True),
        
        # Test missing argument for -c
        (["-c"], True),
        
        # Test invalid option
        (["--invalid-option"], True),
        
        # Test non-existent file
        (["--file=nonexistent.py"], True),
    ]
    
    passed = 0
    total = len(error_tests)
    
    for args, should_fail in error_tests:
        if run_python_inline(args, should_fail=should_fail):
            passed += 1
    
    print(f"\nError conditions: {passed}/{total} tests passed")
    return passed == total

def main():
    """Run all tests"""
    print("Python Inline Runner Test Suite")
    print("=" * 40)
    
    all_passed = True
    
    # Test basic functionality
    if not test_basic_functionality():
        all_passed = False
    
    # Test file functionality
    if not test_file_functionality():
        all_passed = False
    
    # Test error conditions
    if not test_error_conditions():
        all_passed = False
    
    print("\n" + "=" * 40)
    if all_passed:
        print("?? All tests PASSED!")
        return 0
    else:
        print("? Some tests FAILED!")
        return 1

if __name__ == "__main__":
    sys.exit(main())
