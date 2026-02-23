"""Test: fonttools"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from fontTools.ttLib import TTFont
    assert TTFont is not None
    print("fonttools: PASS")
except Exception as e:
    print(f"fonttools: FAIL: {e}")
    sys.exit(1)
