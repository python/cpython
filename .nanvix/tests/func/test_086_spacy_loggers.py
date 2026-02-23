"""Test: spacy-loggers"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import spacy_loggers
    assert spacy_loggers is not None
    print("spacy-loggers: PASS")
except Exception as e:
    print(f"spacy-loggers: FAIL: {e}")
    sys.exit(1)
