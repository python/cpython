"""Test: blinker"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import blinker
    assert hasattr(blinker, "Signal")
    print("blinker: PASS")
except Exception as e:
    print(f"blinker: FAIL: {e}")
    sys.exit(1)
