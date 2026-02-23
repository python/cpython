"""Test: tzdata"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import tzdata
    print("tzdata: PASS")
except Exception as e:
    print(f"tzdata: FAIL: {e}")
    sys.exit(1)
