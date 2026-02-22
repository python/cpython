"""Test: typing-inspect"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from typing_inspect import is_optional_type
    from typing import Optional
    assert is_optional_type(Optional[int])
    print("typing-inspect: PASS")
except Exception as e:
    print(f"typing-inspect: FAIL: {e}")
    sys.exit(1)
