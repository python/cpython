"""Test: termcolor"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from termcolor import colored
    text = colored("hello", "red")
    assert "hello" in text
    print("termcolor: PASS")
except Exception as e:
    print(f"termcolor: FAIL: {e}")
    sys.exit(1)
