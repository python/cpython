"""Test: pyyaml"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import yaml
    data = yaml.safe_load("name: test\nvalue: 42")
    assert data["name"] == "test" and data["value"] == 42
    out = yaml.dump({"a": 1, "b": [2, 3]})
    assert "a:" in out
    print("pyyaml: PASS")
except Exception as e:
    print(f"pyyaml: FAIL: {e}")
    sys.exit(1)
