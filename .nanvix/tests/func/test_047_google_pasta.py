"""Test: google-pasta"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import pasta
    tree = pasta.parse("x = 1")
    code = pasta.dump(tree)
    assert "x = 1" in code
    print("google-pasta: PASS")
except Exception as e:
    print(f"google-pasta: FAIL: {e}")
    sys.exit(1)
