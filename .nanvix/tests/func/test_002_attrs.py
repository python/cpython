"""Test: attrs"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import attr
    @attr.s(auto_attribs=True)
    class Point: x: int; y: int
    p = Point(1, 2)
    assert p.x == 1 and p.y == 2
    print("attrs: PASS")
except Exception as e:
    print(f"attrs: FAIL: {e}")
    sys.exit(1)
