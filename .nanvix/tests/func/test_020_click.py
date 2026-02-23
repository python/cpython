"""Test: click"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import click
    @click.command()
    @click.option("--name", default="World")
    def hello(name): pass
    assert hello is not None
    print("click: PASS")
except Exception as e:
    print(f"click: FAIL: {e}")
    sys.exit(1)
