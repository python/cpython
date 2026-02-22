"""Batch 14: docutils (very heavy import, runs alone)"""
import sys
results = []

try:
    import docutils
    results.append(("docutils", "PASS"))
except Exception as e: results.append(("docutils", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
