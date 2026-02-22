"""Test: toolz"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from toolz import partition, merge
    assert list(partition(2, [1,2,3,4])) == [(1,2), (3,4)]
    assert merge({"a":1}, {"b":2}) == {"a":1, "b":2}
    print("toolz: PASS")
except Exception as e:
    print(f"toolz: FAIL: {e}")
    sys.exit(1)
