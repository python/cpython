"""Test: gast"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import gast, ast
    tree = ast.parse("x = 1")
    gtree = gast.parse("x = 1")
    assert gtree is not None
    print("gast: PASS")
except Exception as e:
    print(f"gast: FAIL: {e}")
    sys.exit(1)
