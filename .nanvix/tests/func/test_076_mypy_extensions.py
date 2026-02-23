"""Test: mypy-extensions"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import mypy_extensions
    assert hasattr(mypy_extensions, "TypedDict")
    print("mypy-extensions: PASS")
except Exception as e:
    print(f"mypy-extensions: FAIL: {e}")
    sys.exit(1)
