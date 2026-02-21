"""Batch 5: Document packages (part 1)"""
import sys
results = []

# pypdf
try:
    from pypdf import PdfReader
    results.append(("pypdf", "PASS"))
except Exception as e: results.append(("pypdf", f"FAIL: {e}"))

# PyPDF2
try:
    import PyPDF2
    results.append(("PyPDF2", "PASS"))
except Exception as e: results.append(("PyPDF2", f"FAIL: {e}"))

# xlsxwriter
try:
    import xlsxwriter, io
    buf = io.BytesIO()
    wb = xlsxwriter.Workbook(buf)
    ws = wb.add_worksheet()
    ws.write(0, 0, "Hello")
    ws.write(0, 1, 42)
    wb.close()
    assert len(buf.getvalue()) > 0
    results.append(("xlsxwriter", "PASS"))
except Exception as e: results.append(("xlsxwriter", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
