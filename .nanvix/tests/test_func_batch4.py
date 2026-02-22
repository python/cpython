"""Batch 4: Utilities (light packages only)"""
import sys
results = []

# absl-py
try:
    from absl import logging as absl_logging
    absl_logging.set_verbosity(absl_logging.INFO)
    results.append(("absl-py", "PASS"))
except Exception as e: results.append(("absl-py", f"FAIL: {e}"))

# filelock
try:
    import tempfile, os
    from filelock import FileLock
    p = os.path.join(tempfile.gettempdir(), "test.lock")
    lock = FileLock(p, timeout=1)
    with lock: pass
    results.append(("filelock", "PASS"))
except Exception as e: results.append(("filelock", f"FAIL: {e}"))

# fsspec
try:
    import fsspec
    fs = fsspec.filesystem("memory")
    fs.mkdir("/test")
    assert fs.exists("/test")
    results.append(("fsspec", "PASS"))
except Exception as e: results.append(("fsspec", f"FAIL: {e}"))

# mpmath
try:
    import mpmath as mp
    mp.dps = 50
    pi = mp.pi
    assert str(pi).startswith("3.14159265")
    results.append(("mpmath", "PASS"))
except Exception as e: results.append(("mpmath", f"FAIL: {e}"))

# tenacity
try:
    from tenacity import retry, stop_after_attempt
    _counter = {"n": 0}
    @retry(stop=stop_after_attempt(3))
    def flaky():
        _counter["n"] += 1
        if _counter["n"] < 3: raise ValueError("retry")
        return "ok"
    assert flaky() == "ok"
    assert _counter["n"] == 3
    results.append(("tenacity", "PASS"))
except Exception as e: results.append(("tenacity", f"FAIL: {e}"))

# toolz
try:
    from toolz import partition
    assert list(partition(2, [1, 2, 3, 4])) == [(1, 2), (3, 4)]
    results.append(("toolz", "PASS"))
except Exception as e: results.append(("toolz", f"FAIL: {e}"))

# tqdm
try:
    from tqdm import tqdm
    r = list(tqdm(range(5), disable=True))
    assert r == [0, 1, 2, 3, 4]
    results.append(("tqdm", "PASS"))
except Exception as e: results.append(("tqdm", f"FAIL: {e}"))

# lazy-loader
try:
    import lazy_loader
    results.append(("lazy-loader", "PASS"))
except Exception as e: results.append(("lazy-loader", f"FAIL: {e}"))

# entrypoints
try:
    import entrypoints
    results.append(("entrypoints", "PASS"))
except Exception as e: results.append(("entrypoints", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
