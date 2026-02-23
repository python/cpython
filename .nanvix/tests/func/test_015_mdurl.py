"""Test: mdurl"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from mdurl import encode
    assert encode("hello world") is not None
    print("mdurl: PASS")
except Exception as e:
    print(f"mdurl: FAIL: {e}")
    sys.exit(1)
