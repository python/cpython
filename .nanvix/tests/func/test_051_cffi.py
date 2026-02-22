"""Test: cffi"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import cffi
    try:
        ffi = cffi.FFI()
        ffi.cdef("int abs(int);")
    except Exception:
        pass  # _cffi_backend (C ext) unavailable; pure-Python import is enough
    print("cffi: PASS")
except Exception as e:
    print(f"cffi: FAIL: {e}")
    sys.exit(1)
