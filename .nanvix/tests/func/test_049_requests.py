"""Test: requests"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import requests
    assert hasattr(requests, "get")
    s = requests.Session()
    assert s is not None
    print("requests: PASS")
except Exception as e:
    print(f"requests: FAIL: {e}")
    sys.exit(1)
