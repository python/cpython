"""Test: plumbum"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from plumbum import local
    assert local is not None
    print("plumbum: PASS")
except Exception as e:
    print(f"plumbum: FAIL: {e}")
    sys.exit(1)
