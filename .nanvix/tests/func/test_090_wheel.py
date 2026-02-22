"""Test: wheel"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import wheel
    assert hasattr(wheel, "__version__")
    print("wheel: PASS")
except Exception as e:
    print(f"wheel: FAIL: {e}")
    sys.exit(1)
