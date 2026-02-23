"""Test: pypdf"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from pypdf import PdfReader
    assert hasattr(PdfReader, "pages")
    print("pypdf: PASS")
except Exception as e:
    print(f"pypdf: FAIL: {e}")
    sys.exit(1)
