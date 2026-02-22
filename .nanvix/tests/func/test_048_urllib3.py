"""Test: urllib3"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import urllib3
    assert hasattr(urllib3, "HTTPConnectionPool")
    print("urllib3: PASS")
except Exception as e:
    print(f"urllib3: FAIL: {e}")
    sys.exit(1)
