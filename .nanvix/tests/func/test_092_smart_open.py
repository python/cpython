"""Test: smart-open"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from smart_open import open as smart_open_fn
    assert smart_open_fn is not None
    print("smart-open: PASS")
except Exception as e:
    print(f"smart-open: FAIL: {e}")
    sys.exit(1)
