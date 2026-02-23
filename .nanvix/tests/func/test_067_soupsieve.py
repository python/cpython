"""Test: soupsieve"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import soupsieve
    assert hasattr(soupsieve, "select")
    print("soupsieve: PASS")
except Exception as e:
    print(f"soupsieve: FAIL: {e}")
    sys.exit(1)
