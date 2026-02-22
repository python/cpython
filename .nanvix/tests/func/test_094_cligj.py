"""Test: cligj"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import cligj
    assert hasattr(cligj, "verbose_opt")
    print("cligj: PASS")
except Exception as e:
    print(f"cligj: FAIL: {e}")
    sys.exit(1)
