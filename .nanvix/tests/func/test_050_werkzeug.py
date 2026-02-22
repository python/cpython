"""Test: werkzeug"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from werkzeug.test import Client
    from werkzeug.wrappers import Response
    def app(environ, start_response):
        r = Response("Hello", status=200)
        return r(environ, start_response)
    c = Client(app)
    r = c.get("/")
    assert r.status_code == 200
    print("werkzeug: PASS")
except Exception as e:
    print(f"werkzeug: FAIL: {e}")
    sys.exit(1)
