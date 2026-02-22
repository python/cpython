"""Test: flask"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from flask import Flask
    app = Flask(__name__)
    @app.route("/")
    def index(): return "Hello"
    with app.test_client() as c:
        r = c.get("/")
        assert r.status_code == 200
        assert b"Hello" in r.data
    print("flask: PASS")
except Exception as e:
    print(f"flask: FAIL: {e}")
    sys.exit(1)
