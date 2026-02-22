"""Test: markdown-it-py"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from markdown_it import MarkdownIt
    md = MarkdownIt()
    html = md.render("# Hello")
    assert "<h1>" in html
    print("markdown-it-py: PASS")
except Exception as e:
    print(f"markdown-it-py: FAIL: {e}")
    sys.exit(1)
