"""Test: immutabledict"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from immutabledict import immutabledict
    d = immutabledict({"a": 1, "b": 2})
    assert d["a"] == 1
    print("immutabledict: PASS")
except Exception as e:
    print(f"immutabledict: FAIL: {e}")
    sys.exit(1)
