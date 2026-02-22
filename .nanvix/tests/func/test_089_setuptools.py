"""Test: setuptools"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import setuptools
    assert hasattr(setuptools, "setup")
    print("setuptools: PASS")
except Exception as e:
    print(f"setuptools: FAIL: {e}")
    sys.exit(1)
