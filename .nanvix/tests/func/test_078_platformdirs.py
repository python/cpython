"""Test: platformdirs"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from platformdirs import user_data_dir
    d = user_data_dir("test")
    assert isinstance(d, str) and len(d) > 0
    print("platformdirs: PASS")
except Exception as e:
    print(f"platformdirs: FAIL: {e}")
    sys.exit(1)
