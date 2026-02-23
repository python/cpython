"""Test: markdown"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import markdown
    html = markdown.markdown("# Hello")
    assert "<h1>" in html
    print("markdown: PASS")
except Exception as e:
    print(f"markdown: FAIL: {e}")
    sys.exit(1)
