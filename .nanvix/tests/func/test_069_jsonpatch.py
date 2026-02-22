"""Test: jsonpatch"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import jsonpatch
    doc = {"a": 1}
    patch = jsonpatch.JsonPatch([{"op": "add", "path": "/b", "value": 2}])
    result = patch.apply(doc)
    assert result == {"a": 1, "b": 2}
    print("jsonpatch: PASS")
except Exception as e:
    print(f"jsonpatch: FAIL: {e}")
    sys.exit(1)
