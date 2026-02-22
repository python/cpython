"""Batch 6: ML/NLP infra (part 1: lightweight imports)"""
import sys
results = []

# cloudpathlib
try:
    from cloudpathlib import CloudPath
    results.append(("cloudpathlib", "PASS"))
except Exception as e: results.append(("cloudpathlib", f"FAIL: {e}"))

# langcodes
try:
    import langcodes
    lang = langcodes.Language.get("en")
    assert lang.language == "en"
    results.append(("langcodes", "PASS"))
except Exception as e: results.append(("langcodes", f"FAIL: {e}"))

# namex
try:
    import namex
    results.append(("namex", "PASS"))
except Exception as e: results.append(("namex", f"FAIL: {e}"))

# google-pasta
try:
    import pasta
    tree = pasta.parse("x = 1\ny = 2")
    src = pasta.dump(tree)
    assert "x = 1" in src
    results.append(("google-pasta", "PASS"))
except Exception as e: results.append(("google-pasta", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
