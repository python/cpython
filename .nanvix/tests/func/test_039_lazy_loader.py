"""Test: lazy-loader"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import lazy_loader
    assert hasattr(lazy_loader, "attach")
    print("lazy-loader: PASS")
except Exception as e:
    print(f"lazy-loader: FAIL: {e}")
    sys.exit(1)
