"""Test: ply"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import ply.lex
    import ply.yacc
    assert hasattr(ply.lex, "lex")
    print("ply: PASS")
except Exception as e:
    print(f"ply: FAIL: {e}")
    sys.exit(1)
