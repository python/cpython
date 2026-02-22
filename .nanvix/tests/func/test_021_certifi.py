"""Test: certifi"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import certifi
    path = certifi.where()
    assert path.endswith(".pem")
    print("certifi: PASS")
except Exception as e:
    print(f"certifi: FAIL: {e}")
    sys.exit(1)
