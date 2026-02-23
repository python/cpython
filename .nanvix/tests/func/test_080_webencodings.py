"""Test: webencodings"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from webencodings import lookup
    info = lookup("utf-8")
    assert info is not None
    print("webencodings: PASS")
except Exception as e:
    print(f"webencodings: FAIL: {e}")
    sys.exit(1)
