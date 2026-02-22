"""Test: dataclasses-json"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from dataclasses import dataclass
    from dataclasses_json import dataclass_json
    @dataclass_json
    @dataclass
    class Person:
        name: str
        age: int
    p = Person.from_json('{"name": "test", "age": 25}')
    assert p.name == "test"
    print("dataclasses-json: PASS")
except Exception as e:
    print(f"dataclasses-json: FAIL: {e}")
    sys.exit(1)
