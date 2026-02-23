"""Test: xlsxwriter"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import xlsxwriter, io
    buf = io.BytesIO()
    wb = xlsxwriter.Workbook(buf)
    ws = wb.add_worksheet()
    ws.write(0, 0, "hello")
    wb.close()
    assert buf.tell() > 0
    print("xlsxwriter: PASS")
except Exception as e:
    print(f"xlsxwriter: FAIL: {e}")
    sys.exit(1)
