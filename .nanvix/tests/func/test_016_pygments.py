"""Test: pygments"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from pygments import highlight
    from pygments.lexers import PythonLexer
    from pygments.formatters import HtmlFormatter
    r = highlight("x = 1", PythonLexer(), HtmlFormatter())
    assert "x" in r
    print("pygments: PASS")
except Exception as e:
    print(f"pygments: FAIL: {e}")
    sys.exit(1)
