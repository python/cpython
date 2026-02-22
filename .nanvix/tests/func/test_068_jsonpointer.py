"""Test: jsonpointer"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from jsonpointer import resolve_pointer
    doc = {"a": {"b": [1, 2, 3]}}
    assert resolve_pointer(doc, "/a/b/1") == 2
    print("jsonpointer: PASS")
except Exception as e:
    print(f"jsonpointer: FAIL: {e}")
    sys.exit(1)
