"""Batch 10: Data validation, JSON, HTML"""
import sys
results = []

# beautifulsoup4
try:
    from bs4 import BeautifulSoup
    soup = BeautifulSoup("<html><body><p>Hello</p></body></html>", "html.parser")
    assert soup.find("p").text == "Hello"
    results.append(("beautifulsoup4", "PASS"))
except Exception as e: results.append(("beautifulsoup4", f"FAIL: {e}"))

# soupsieve
try:
    import soupsieve
    results.append(("soupsieve", "PASS"))
except Exception as e: results.append(("soupsieve", f"FAIL: {e}"))

# jsonpointer
try:
    from jsonpointer import resolve_pointer
    doc = {"foo": {"bar": [1, 2, 3]}}
    assert resolve_pointer(doc, "/foo/bar/1") == 2
    results.append(("jsonpointer", "PASS"))
except Exception as e: results.append(("jsonpointer", f"FAIL: {e}"))

# jsonpatch
try:
    import jsonpatch
    doc = {"foo": "bar"}
    patch = jsonpatch.JsonPatch([{"op": "add", "path": "/baz", "value": 42}])
    result = patch.apply(doc)
    assert result["baz"] == 42
    results.append(("jsonpatch", "PASS"))
except Exception as e: results.append(("jsonpatch", f"FAIL: {e}"))

# jmespath
try:
    import jmespath
    data = {"people": [{"name": "a", "age": 20}, {"name": "b", "age": 30}]}
    names = jmespath.search("people[*].name", data)
    assert names == ["a", "b"]
    results.append(("jmespath", "PASS"))
except Exception as e: results.append(("jmespath", f"FAIL: {e}"))

# marshmallow
try:
    from marshmallow import Schema, fields
    class UserSchema(Schema):
        name = fields.Str(required=True)
        age = fields.Int()
    s = UserSchema()
    result = s.load({"name": "test", "age": 25})
    assert result["name"] == "test"
    results.append(("marshmallow", "PASS"))
except Exception as e: results.append(("marshmallow", f"FAIL: {e}"))

# dataclasses-json
try:
    from dataclasses_json import dataclass_json
    from dataclasses import dataclass
    @dataclass_json
    @dataclass
    class P:
        name: str
        age: int
    p = P.from_json('{"name":"A","age":30}')
    assert p.name == "A" and p.age == 30
    results.append(("dataclasses-json", "PASS"))
except Exception as e: results.append(("dataclasses-json", f"FAIL: {e}"))

# tabulate
try:
    from tabulate import tabulate
    t = tabulate([["Alice", 24], ["Bob", 19]], headers=["Name", "Age"])
    assert "Alice" in t and "Name" in t
    results.append(("tabulate", "PASS"))
except Exception as e: results.append(("tabulate", f"FAIL: {e}"))

# termcolor
try:
    from termcolor import colored
    s = colored("hello", "red")
    assert "hello" in s
    results.append(("termcolor", "PASS"))
except Exception as e: results.append(("termcolor", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
