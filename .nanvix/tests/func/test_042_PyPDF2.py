"""Test: PyPDF2"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import PyPDF2
    assert hasattr(PyPDF2, "PdfReader")
    print("PyPDF2: PASS")
except Exception as e:
    print(f"PyPDF2: FAIL: {e}")
    sys.exit(1)
