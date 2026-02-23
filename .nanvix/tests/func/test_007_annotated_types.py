"""Test: annotated-types"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from annotated_types import Gt
    gt = Gt(0)
    assert gt.gt == 0
    print("annotated-types: PASS")
except Exception as e:
    print(f"annotated-types: FAIL: {e}")
    sys.exit(1)
