"""Test: charset-normalizer"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from charset_normalizer import from_bytes
    r = from_bytes(b"hello")
    assert r is not None
    print("charset-normalizer: PASS")
except Exception as e:
    print(f"charset-normalizer: FAIL: {e}")
    sys.exit(1)
