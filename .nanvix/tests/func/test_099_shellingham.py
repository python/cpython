"""Test: shellingham"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import shellingham
    assert hasattr(shellingham, "detect_shell")
    print("shellingham: PASS")
except Exception as e:
    print(f"shellingham: FAIL: {e}")
    sys.exit(1)
