"""Test: colorama"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from colorama import Fore, Style
    text = f"{Fore.RED}red{Style.RESET_ALL}"
    assert "red" in text
    print("colorama: PASS")
except Exception as e:
    print(f"colorama: FAIL: {e}")
    sys.exit(1)
