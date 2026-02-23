"""Test: wasabi"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from wasabi import msg
    assert msg is not None
    print("wasabi: PASS")
except Exception as e:
    print(f"wasabi: FAIL: {e}")
    sys.exit(1)
