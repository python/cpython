"""Test: langcodes"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from langcodes import Language
    lang = Language.get("en")
    assert lang.language == "en"
    print("langcodes: PASS")
except Exception as e:
    print(f"langcodes: FAIL: {e}")
    sys.exit(1)
