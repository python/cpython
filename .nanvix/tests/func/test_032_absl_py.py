"""Test: absl-py"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from absl import logging
    logging.info("test")
    print("absl-py: PASS")
except Exception as e:
    print(f"absl-py: FAIL: {e}")
    sys.exit(1)
