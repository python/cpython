"""Test: ctypes"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import ctypes
    x = ctypes.c_int(42)
    assert x.value == 42
    s = ctypes.c_char_p(b"hello")
    assert s.value == b"hello"
    print("ctypes: PASS")
except Exception as e:
    print(f"ctypes: FAIL: {e}")
    sys.exit(1)
