"""Test: html5lib"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import html5lib
    doc = html5lib.parse("<html><body><p>Hello</p></body></html>")
    assert doc is not None
    print("html5lib: PASS")
except Exception as e:
    print(f"html5lib: FAIL: {e}")
    sys.exit(1)
