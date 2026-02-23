"""Test: idna"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import idna
    encoded = idna.encode("example.com")
    assert encoded == b"example.com"
    print("idna: PASS")
except Exception as e:
    print(f"idna: FAIL: {e}")
    sys.exit(1)
