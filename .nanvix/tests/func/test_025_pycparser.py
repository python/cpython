"""Test: pycparser"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import pycparser
    assert hasattr(pycparser, "parse_file")
    print("pycparser: PASS")
except Exception as e:
    print(f"pycparser: FAIL: {e}")
    sys.exit(1)
