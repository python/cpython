"""Test: namex"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import namex
    assert hasattr(namex, "export")
    print("namex: PASS")
except Exception as e:
    print(f"namex: FAIL: {e}")
    sys.exit(1)
