"""Test: itsdangerous"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from itsdangerous import URLSafeSerializer
    s = URLSafeSerializer("secret")
    signed = s.dumps({"user": "test"})
    data = s.loads(signed)
    assert data["user"] == "test"
    print("itsdangerous: PASS")
except Exception as e:
    print(f"itsdangerous: FAIL: {e}")
    sys.exit(1)
