"""Test: flatbuffers"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import flatbuffers
    b = flatbuffers.Builder(256)
    assert b is not None
    print("flatbuffers: PASS")
except Exception as e:
    print(f"flatbuffers: FAIL: {e}")
    sys.exit(1)
