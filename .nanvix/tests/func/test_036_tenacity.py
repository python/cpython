"""Test: tenacity"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from tenacity import retry, stop_after_attempt
    @retry(stop=stop_after_attempt(1))
    def f(): return 42
    assert f() == 42
    print("tenacity: PASS")
except Exception as e:
    print(f"tenacity: FAIL: {e}")
    sys.exit(1)
