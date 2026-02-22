"""Test: pylatex"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    from pylatex import Document, Section
    doc = Document()
    with doc.create(Section("Test")): pass
    assert doc is not None
    print("pylatex: PASS")
except Exception as e:
    print(f"pylatex: FAIL: {e}")
    sys.exit(1)
