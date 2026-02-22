"""Test: msgpack"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import msgpack
    data = {"key": "value", "num": 42, "lst": [1, 2, 3]}
    packed = msgpack.packb(data)
    unpacked = msgpack.unpackb(packed, raw=False)
    assert unpacked["key"] == "value"
    print("msgpack: PASS")
except Exception as e:
    print(f"msgpack: FAIL: {e}")
    sys.exit(1)
