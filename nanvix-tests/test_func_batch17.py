"""Batch 17: spaCy infra + wrapt"""
import sys
results = []

# spacy-legacy
try:
    import spacy_legacy
    results.append(("spacy-legacy", "PASS"))
except Exception as e: results.append(("spacy-legacy", f"FAIL: {e}"))

# spacy-loggers
try:
    import spacy_loggers
    results.append(("spacy-loggers", "PASS"))
except Exception as e: results.append(("spacy-loggers", f"FAIL: {e}"))

# wasabi
try:
    from wasabi import msg
    results.append(("wasabi", "PASS"))
except Exception as e: results.append(("wasabi", f"FAIL: {e}"))

# wrapt
try:
    import wrapt
    @wrapt.decorator
    def my_dec(wrapped, instance, args, kwargs):
        return wrapped(*args, **kwargs)
    @my_dec
    def add(a, b): return a + b
    assert add(2, 3) == 5
    results.append(("wrapt", "PASS"))
except Exception as e: results.append(("wrapt", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
