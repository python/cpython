"""Test: wrapt"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import wrapt
    @wrapt.decorator
    def my_decorator(wrapped, instance, args, kwargs):
        return wrapped(*args, **kwargs)
    @my_decorator
    def hello(): return "hello"
    assert hello() == "hello"
    print("wrapt: PASS")
except Exception as e:
    print(f"wrapt: FAIL: {e}")
    sys.exit(1)
