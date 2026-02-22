"""Test: typing-extensions"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from typing_extensions import TypedDict
    class Movie(TypedDict): name: str; year: int
    m: Movie = {"name": "Test", "year": 2024}
    print("typing-extensions: PASS")
except Exception as e:
    print(f"typing-extensions: FAIL: {e}")
    sys.exit(1)
