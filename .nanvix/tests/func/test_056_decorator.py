"""Test: decorator"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import decorator
    @decorator.decorator
    def trace(f, *args, **kwargs):
        return f(*args, **kwargs)
    @trace
    def double(x): return x * 2
    assert double(5) == 10
    print("decorator: PASS")
except Exception as e:
    print(f"decorator: FAIL: {e}")
    sys.exit(1)
