"""Test: chardet"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import chardet
    r = chardet.detect(b"hello world")
    assert r["encoding"] is not None
    print("chardet: PASS")
except Exception as e:
    print(f"chardet: FAIL: {e}")
    sys.exit(1)
