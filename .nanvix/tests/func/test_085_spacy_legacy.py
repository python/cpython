"""Test: spacy-legacy"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import spacy_legacy
    assert spacy_legacy is not None
    print("spacy-legacy: PASS")
except Exception as e:
    print(f"spacy-legacy: FAIL: {e}")
    sys.exit(1)
