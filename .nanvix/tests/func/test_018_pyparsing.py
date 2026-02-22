"""Test: pyparsing"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from pyparsing import Word, alphas
    g = Word(alphas)
    assert g.parseString("hello")[0] == "hello"
    print("pyparsing: PASS")
except Exception as e:
    print(f"pyparsing: FAIL: {e}")
    sys.exit(1)
