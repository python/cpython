"""Test: fpdf"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from fpdf import FPDF
    pdf = FPDF()
    pdf.add_page()
    pdf.set_font("Helvetica", size=12)
    pdf.cell(200, 10, "Hello World")
    # fpdf 1.7.2 output() writes to stdout; just verify creation worked
    assert pdf.page > 0
    print("fpdf: PASS")
except Exception as e:
    print(f"fpdf: FAIL: {e}")
    sys.exit(1)
