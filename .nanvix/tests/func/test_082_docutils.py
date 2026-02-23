"""Test: docutils"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from docutils.core import publish_parts
    parts = publish_parts("Hello **world**", writer_name="html")
    assert "world" in parts["body"]
    print("docutils: PASS")
except Exception as e:
    print(f"docutils: FAIL: {e}")
    sys.exit(1)
