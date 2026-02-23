"""Test: defusedxml"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import defusedxml.ElementTree as ET
    root = ET.fromstring("<root><item>test</item></root>")
    assert root.find("item").text == "test"
    print("defusedxml: PASS")
except Exception as e:
    print(f"defusedxml: FAIL: {e}")
    sys.exit(1)
