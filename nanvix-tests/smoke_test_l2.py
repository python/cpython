"""Layer 2 smoke test: validate built-in CPython modules (L1 deps only).

Tests the modules that are compiled into cpython via Layer 1 libraries:
  zlib, bz2, _ssl/hashlib, _sqlite3, _ctypes, pyexpat, _elementtree
Plus basic stdlib functionality: sys, os, json, collections, math, io.
"""
import sys

sys.stdout.reconfigure(line_buffering=True)
print(f"Python {sys.version}")
print(f"Built-in modules: {len(sys.builtin_module_names)}")
print(f"Platform: {sys.platform}")

results = []

# ── Core stdlib ──────────────────────────────────────────────────────────────

# sys
try:
    assert sys.version_info >= (3, 12)
    results.append(("sys", "PASS"))
except Exception as e:
    results.append(("sys", f"FAIL: {e}"))

# os
try:
    import os
    assert hasattr(os, "getcwd")
    results.append(("os", "PASS"))
except Exception as e:
    results.append(("os", f"FAIL: {e}"))

# json
try:
    import json
    data = json.loads('{"key": "value", "num": 42}')
    assert data["key"] == "value" and data["num"] == 42
    assert json.dumps(data, sort_keys=True) == '{"key": "value", "num": 42}'
    results.append(("json", "PASS"))
except Exception as e:
    results.append(("json", f"FAIL: {e}"))

# collections
try:
    from collections import OrderedDict, Counter, defaultdict
    c = Counter("abracadabra")
    assert c["a"] == 5
    results.append(("collections", "PASS"))
except Exception as e:
    results.append(("collections", f"FAIL: {e}"))

# math
try:
    import math
    assert abs(math.pi - 3.141592653589793) < 1e-10
    assert math.sqrt(144) == 12.0
    results.append(("math", "PASS"))
except Exception as e:
    results.append(("math", f"FAIL: {e}"))

# io
try:
    import io
    buf = io.StringIO()
    buf.write("hello")
    assert buf.getvalue() == "hello"
    results.append(("io", "PASS"))
except Exception as e:
    results.append(("io", f"FAIL: {e}"))

# ── L1-enabled built-in modules ─────────────────────────────────────────────

# zlib (L1: zlib)
try:
    import zlib
    data = b"hello world" * 100
    compressed = zlib.compress(data)
    assert zlib.decompress(compressed) == data
    results.append(("zlib", "PASS"))
except Exception as e:
    results.append(("zlib", f"FAIL: {e}"))

# bz2 (L1: bzip2)
try:
    import bz2
    data = b"hello bzip2 world" * 50
    compressed = bz2.compress(data)
    assert bz2.decompress(compressed) == data
    results.append(("bz2", "PASS"))
except Exception as e:
    results.append(("bz2", f"FAIL: {e}"))

# hashlib / _ssl (L1: openssl)
try:
    import hashlib
    h = hashlib.sha256(b"hello").hexdigest()
    assert len(h) == 64
    results.append(("hashlib", "PASS"))
except Exception as e:
    results.append(("hashlib", f"FAIL: {e}"))

# sqlite3 (L1: sqlite)
try:
    import sqlite3
    conn = sqlite3.connect(":memory:")
    cur = conn.cursor()
    cur.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, val TEXT)")
    cur.execute("INSERT INTO test VALUES (1, 'hello')")
    cur.execute("SELECT val FROM test WHERE id=1")
    assert cur.fetchone()[0] == "hello"
    conn.close()
    results.append(("sqlite3", "PASS"))
except Exception as e:
    results.append(("sqlite3", f"FAIL: {e}"))

# ctypes (L1: libffi)
try:
    import ctypes
    assert hasattr(ctypes, "c_int")
    v = ctypes.c_int(42)
    assert v.value == 42
    results.append(("ctypes", "PASS"))
except Exception as e:
    results.append(("ctypes", f"FAIL: {e}"))

# pyexpat / xml.parsers.expat (L1: libexpat)
try:
    import xml.parsers.expat
    parser = xml.parsers.expat.ParserCreate()
    elements = []
    parser.StartElementHandler = lambda name, attrs: elements.append(name)
    parser.Parse(b"<root><child/></root>", True)
    assert elements == ["root", "child"]
    results.append(("pyexpat", "PASS"))
except Exception as e:
    results.append(("pyexpat", f"FAIL: {e}"))

# xml.etree.ElementTree (uses _elementtree C accelerator, L1: libexpat)
try:
    import xml.etree.ElementTree as ET
    root = ET.fromstring("<root><child>text</child></root>")
    assert root.find("child").text == "text"
    results.append(("xml.etree", "PASS"))
except Exception as e:
    results.append(("xml.etree", f"FAIL: {e}"))

# ── Summary ──────────────────────────────────────────────────────────────────

print()
for name, status in results:
    print(f"  {name}: {status}")

passed = sum(1 for _, s in results if s == "PASS")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{passed}/{len(results)} passed, {failed} failed")

if failed == 0:
    print("Smoke L2 passed!")
else:
    print(f"Smoke L2 FAILED ({failed} failures)")
