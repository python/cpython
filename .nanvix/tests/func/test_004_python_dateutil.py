"""Test: python-dateutil"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from dateutil.parser import parse
    from dateutil.relativedelta import relativedelta
    d = parse("2024-01-15")
    d2 = d + relativedelta(months=3)
    assert d2.month == 4
    print("python-dateutil: PASS")
except Exception as e:
    print(f"python-dateutil: FAIL: {e}")
    sys.exit(1)
