"""Test: pytz"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import pytz
    tz = pytz.timezone('US/Eastern')
    assert tz is not None
    print("pytz: PASS")
except Exception as e:
    print(f"pytz: FAIL: {e}")
    sys.exit(1)
