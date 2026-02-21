"""Batch 13: html5lib, webencodings, docutils"""
import sys
results = []

# webencodings
try:
    import webencodings
    assert webencodings.lookup("utf-8") is not None
    results.append(("webencodings", "PASS"))
except Exception as e: results.append(("webencodings", f"FAIL: {e}"))

# html5lib
try:
    import html5lib
    doc = html5lib.parse("<p>hello</p>")
    assert doc is not None
    results.append(("html5lib", "PASS"))
except Exception as e: results.append(("html5lib", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
