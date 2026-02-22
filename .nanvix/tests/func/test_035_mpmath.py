"""Test: mpmath"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from mpmath import mp, mpf
    mp.dps = 50
    assert str(mp.pi).startswith("3.14159")
    print("mpmath: PASS")
except Exception as e:
    print(f"mpmath: FAIL: {e}")
    sys.exit(1)
