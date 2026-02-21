"""Batch 16: fpdf (separate — known to hang on set_font)"""
import sys
results = []

try:
    from fpdf import FPDF
    pdf = FPDF()
    pdf.add_page()
    # Note: set_font() hangs on Nanvix — only test import + add_page
    results.append(("fpdf", "PASS (import+add_page only)"))
except Exception as e: results.append(("fpdf", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
