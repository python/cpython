"""Test: filelock"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from filelock import FileLock
    lock = FileLock("/tmp/test.lock")
    assert lock is not None
    print("filelock: PASS")
except Exception as e:
    print(f"filelock: FAIL: {e}")
    sys.exit(1)
