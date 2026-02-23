"""Test: markupsafe"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from markupsafe import Markup, escape
    assert str(escape("<b>")) == "&lt;b&gt;"
    m = Markup("<b>bold</b>")
    assert "bold" in m
    print("markupsafe: PASS")
except Exception as e:
    print(f"markupsafe: FAIL: {e}")
    sys.exit(1)
