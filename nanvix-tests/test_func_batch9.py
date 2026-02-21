"""Batch 9: Web framework, serialization, config"""
import sys
results = []

# pyyaml
try:
    import yaml
    data = yaml.safe_load("name: test\nvalue: 42")
    assert data["name"] == "test" and data["value"] == 42
    dumped = yaml.dump({"key": [1, 2, 3]})
    assert "key:" in dumped
    results.append(("pyyaml", "PASS"))
except Exception as e: results.append(("pyyaml", f"FAIL: {e}"))

# flask
try:
    from flask import Flask
    app = Flask(__name__)
    @app.route("/")
    def index(): return "Hello"
    with app.test_client() as c:
        r = c.get("/")
        assert r.status_code == 200
        assert b"Hello" in r.data
    results.append(("flask", "PASS"))
except Exception as e: results.append(("flask", f"FAIL: {e}"))

# msgpack
try:
    import msgpack
    packed = msgpack.packb({"key": "value", "num": 42})
    unpacked = msgpack.unpackb(packed)
    assert unpacked.get(b"key", unpacked.get("key")) in (b"value", "value")
    results.append(("msgpack", "PASS"))
except Exception as e: results.append(("msgpack", f"FAIL: {e}"))

# blinker
try:
    from blinker import signal
    sig = signal("test-signal")
    received = []
    def _on_signal(sender, **kw): received.append(sender)
    sig.connect(_on_signal)
    sig.send("hello")
    assert received == ["hello"]
    results.append(("blinker", "PASS"))
except Exception as e: results.append(("blinker", f"FAIL: {e}"))

# python-dotenv
try:
    import python_dotenv
    results.append(("python-dotenv", "PASS"))
except Exception as e:
    try:
        import dotenv
        results.append(("python-dotenv", "PASS"))
    except Exception as e2: results.append(("python-dotenv", f"FAIL: {e2}"))

# plumbum
try:
    from plumbum import local
    results.append(("plumbum", "PASS"))
except Exception as e: results.append(("plumbum", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
