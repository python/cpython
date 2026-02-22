"""Test: striprtf"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from striprtf.striprtf import rtf_to_text
    assert rtf_to_text("{\\rtf1 hello}").strip() == "hello"
    print("striprtf: PASS")
except Exception as e:
    print(f"striprtf: FAIL: {e}")
    sys.exit(1)
