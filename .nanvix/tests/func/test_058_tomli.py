"""Test: tomli"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import tomli
    data = tomli.loads('[project]\nname = "test"\nversion = "1.0"')
    assert data["project"]["name"] == "test"
    print("tomli: PASS")
except Exception as e:
    print(f"tomli: FAIL: {e}")
    sys.exit(1)
