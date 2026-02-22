"""Batch 7: Networking (part 1)"""
import sys
results = []

# urllib3
try:
    import urllib3
    http = urllib3.PoolManager()
    results.append(("urllib3", "PASS"))
except Exception as e: results.append(("urllib3", f"FAIL: {e}"))

# requests
try:
    import requests
    s = requests.Session()
    assert s is not None
    results.append(("requests", "PASS"))
except Exception as e: results.append(("requests", f"FAIL: {e}"))

# werkzeug
try:
    from werkzeug.datastructures import Headers
    h = Headers()
    h.add("Content-Type", "text/html")
    assert h.get("Content-Type") == "text/html"
    results.append(("werkzeug", "PASS"))
except Exception as e: results.append(("werkzeug", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
