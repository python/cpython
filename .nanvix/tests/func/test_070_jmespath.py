"""Test: jmespath"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import jmespath
    data = {"people": [{"name": "a", "age": 20}, {"name": "b", "age": 30}]}
    r = jmespath.search("people[?age > `25`].name", data)
    assert r == ["b"]
    print("jmespath: PASS")
except Exception as e:
    print(f"jmespath: FAIL: {e}")
    sys.exit(1)
