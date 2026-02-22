"""Test: tabulate"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from tabulate import tabulate
    table = [["Alice", 24], ["Bob", 30]]
    out = tabulate(table, headers=["Name", "Age"])
    assert "Alice" in out and "Bob" in out
    print("tabulate: PASS")
except Exception as e:
    print(f"tabulate: FAIL: {e}")
    sys.exit(1)
