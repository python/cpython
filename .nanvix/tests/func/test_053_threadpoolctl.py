"""Test: threadpoolctl"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from threadpoolctl import threadpool_info
    info = threadpool_info()
    assert isinstance(info, list)
    print("threadpoolctl: PASS")
except Exception as e:
    print(f"threadpoolctl: FAIL: {e}")
    sys.exit(1)
