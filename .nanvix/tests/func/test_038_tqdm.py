"""Test: tqdm"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from tqdm import tqdm
    r = list(tqdm(range(10), disable=True))
    assert r == list(range(10))
    print("tqdm: PASS")
except Exception as e:
    print(f"tqdm: FAIL: {e}")
    sys.exit(1)
