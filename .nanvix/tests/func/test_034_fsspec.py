"""Test: fsspec"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import fsspec
    fs = fsspec.filesystem("memory")
    assert fs is not None
    print("fsspec: PASS")
except Exception as e:
    print(f"fsspec: FAIL: {e}")
    sys.exit(1)
