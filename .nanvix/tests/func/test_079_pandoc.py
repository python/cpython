"""Test: pandoc"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import pandoc
    assert hasattr(pandoc, "read")
    print("pandoc: PASS")
except Exception as e:
    print(f"pandoc: FAIL: {e}")
    sys.exit(1)
