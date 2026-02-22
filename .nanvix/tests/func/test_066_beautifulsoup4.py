"""Test: beautifulsoup4"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from bs4 import BeautifulSoup
    soup = BeautifulSoup("<html><body><p>Hello</p></body></html>", "html.parser")
    assert soup.find("p").text == "Hello"
    print("beautifulsoup4: PASS")
except Exception as e:
    print(f"beautifulsoup4: FAIL: {e}")
    sys.exit(1)
