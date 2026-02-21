"""Batch 18: Package metadata + registries"""
import sys
results = []

# setuptools
try:
    import setuptools
    assert setuptools.__version__
    results.append(("setuptools", "PASS"))
except Exception as e: results.append(("setuptools", f"FAIL: {e}"))

# wheel
try:
    import wheel
    results.append(("wheel", "PASS"))
except Exception as e: results.append(("wheel", f"FAIL: {e}"))

# catalogue
try:
    import catalogue
    reg = catalogue.create("test", "ops", entry_points=False)
    reg.register("add", func=lambda a, b: a + b)
    assert reg.get("add")(1, 2) == 3
    results.append(("catalogue", "PASS"))
except Exception as e: results.append(("catalogue", f"FAIL: {e}"))

# smart-open
try:
    import smart_open
    results.append(("smart-open", "PASS"))
except Exception as e: results.append(("smart-open", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
