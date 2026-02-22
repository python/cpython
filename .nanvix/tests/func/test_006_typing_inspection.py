"""Test: typing-inspection"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from typing_inspection import typing_objects
    print("typing-inspection: PASS")
except Exception as e:
    print(f"typing-inspection: FAIL: {e}")
    sys.exit(1)
