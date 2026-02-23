"""Test: ordered-set"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from ordered_set import OrderedSet
    s = OrderedSet([3, 1, 2, 1])
    assert list(s) == [3, 1, 2]
    print("ordered-set: PASS")
except Exception as e:
    print(f"ordered-set: FAIL: {e}")
    sys.exit(1)
