"""Test: entrypoints"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import entrypoints
    assert hasattr(entrypoints, "get_group_all")
    print("entrypoints: PASS")
except Exception as e:
    print(f"entrypoints: FAIL: {e}")
    sys.exit(1)
