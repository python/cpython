"""Test: six"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import six
    assert six.PY3
    assert six.text_type is str
    print("six: PASS")
except Exception as e:
    print(f"six: FAIL: {e}")
    sys.exit(1)
