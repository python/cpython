"""Test: more-itertools"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from more_itertools import flatten, chunked
    assert list(flatten([[1, 2], [3, 4]])) == [1, 2, 3, 4]
    assert list(chunked([1, 2, 3, 4, 5], 2)) == [[1, 2], [3, 4], [5]]
    print("more-itertools: PASS")
except Exception as e:
    print(f"more-itertools: FAIL: {e}")
    sys.exit(1)
