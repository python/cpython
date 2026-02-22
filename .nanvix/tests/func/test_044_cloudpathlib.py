"""Test: cloudpathlib"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from cloudpathlib import CloudPath
    assert CloudPath is not None
    print("cloudpathlib: PASS")
except Exception as e:
    print(f"cloudpathlib: FAIL: {e}")
    sys.exit(1)
