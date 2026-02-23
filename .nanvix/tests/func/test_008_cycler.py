"""Test: cycler"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from cycler import cycler
    c = cycler(color=['r', 'g', 'b'])
    assert len(c) == 3
    print("cycler: PASS")
except Exception as e:
    print(f"cycler: FAIL: {e}")
    sys.exit(1)
