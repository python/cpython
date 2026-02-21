"""Batch 12: Heavy imports (separate to avoid OOM)"""
import sys
results = []

# tifffile (heavy, test alone)
try:
    import tifffile
    results.append(("tifffile", "PASS"))
except Exception as e: results.append(("tifffile", f"FAIL: {e}"))

# imageio
try:
    import imageio.v3 as iio
    results.append(("imageio", "PASS"))
except Exception as e: results.append(("imageio", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
