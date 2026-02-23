"""Test: isodate"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import isodate
    d = isodate.parse_duration("P1Y2M3D")
    assert d.years == 1 and d.months == 2 and d.days == 3
    print("isodate: PASS")
except Exception as e:
    print(f"isodate: FAIL: {e}")
    sys.exit(1)
