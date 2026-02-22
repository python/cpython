"""Test: packaging"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from packaging.version import Version
    v = Version("3.12.3")
    assert str(v) == "3.12.3"
    assert v > Version("3.11.0")
    print("packaging: PASS")
except Exception as e:
    print(f"packaging: FAIL: {e}")
    sys.exit(1)
