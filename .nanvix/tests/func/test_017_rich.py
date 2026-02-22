"""Test: rich"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from rich.console import Console
    from rich.text import Text
    c = Console(file=__import__("io").StringIO())
    c.print(Text("hello"))
    print("rich: PASS")
except Exception as e:
    print(f"rich: FAIL: {e}")
    sys.exit(1)
