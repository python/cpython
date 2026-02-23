"""Test: click-plugins"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import click_plugins
    assert hasattr(click_plugins, "with_plugins")
    print("click-plugins: PASS")
except Exception as e:
    print(f"click-plugins: FAIL: {e}")
    sys.exit(1)
