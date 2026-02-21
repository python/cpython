"""Batch 11: NumPy and NumPy-dependent packages"""
import sys
results = []

# typing-inspect (lightweight, run before heavy NumPy-dependent packages)
try:
    from typing_inspect import is_optional_type
    from typing import Optional
    assert is_optional_type(Optional[int])
    results.append(("typing-inspect", "PASS"))
except Exception as e: results.append(("typing-inspect", f"FAIL: {e}"))

# mypy-extensions (lightweight, run before heavy NumPy-dependent packages)
try:
    import mypy_extensions
    results.append(("mypy-extensions", "PASS"))
except Exception as e: results.append(("mypy-extensions", f"FAIL: {e}"))

# numpy
try:
    import numpy as np
    a = np.array([1.0, 2.0, 3.0, 4.0, 5.0])
    assert np.sum(a) == 15.0
    assert abs(np.mean(a) - 3.0) < 1e-10
    assert abs(np.std(a) - np.sqrt(2.0)) < 1e-10
    m = np.array([[1, 2], [3, 4]])
    assert abs(np.linalg.det(m) - (-2.0)) < 1e-10
    inv = np.linalg.inv(m)
    identity = m @ inv
    assert abs(identity[0][0] - 1.0) < 1e-10
    rng = np.random.default_rng(42)
    r = rng.random(5)
    assert len(r) == 5 and all(0 <= x <= 1 for x in r)
    fft = np.fft.fft([1, 0, 1, 0])
    assert abs(fft[0] - 2.0) < 1e-10
    results.append(("numpy", "PASS"))
except Exception as e: results.append(("numpy", f"FAIL: {e}"))

# numpy-financial
try:
    import numpy_financial as npf
    pv = npf.pv(0.05/12, 10*12, -100, -100)
    assert abs(pv - 9488.85) < 1.0
    irr = npf.irr([-100, 39, 59, 55, 20])
    assert abs(irr - 0.2809) < 0.01
    results.append(("numpy-financial", "PASS"))
except Exception as e: results.append(("numpy-financial", f"FAIL: {e}"))

# patsy
try:
    from patsy import dmatrix
    import numpy as np
    data = {"x": np.array([1, 2, 3, 4, 5])}
    dm = dmatrix("x", data)
    assert dm.shape == (5, 2)
    results.append(("patsy", "PASS"))
except Exception as e: results.append(("patsy", f"FAIL: {e}"))

# isodate
try:
    import isodate
    d = isodate.parse_duration("P1Y2M3DT4H5M6S")
    assert d.years == 1 and d.months == 2
    results.append(("isodate", "PASS"))
except Exception as e: results.append(("isodate", f"FAIL: {e}"))

# platformdirs
try:
    import platformdirs
    d = platformdirs.user_data_dir("myapp", "myorg")
    assert len(d) > 0
    results.append(("platformdirs", "PASS"))
except Exception as e: results.append(("platformdirs", f"FAIL: {e}"))

# pandoc
try:
    import pandoc
    results.append(("pandoc", "PASS"))
except Exception as e: results.append(("pandoc", f"FAIL: {e}"))

# regex (requires L3 dotted name patching for _regex C extension)
try:
    import regex
    m = regex.match(r"\p{L}+", "Hello")
    assert m.group() == "Hello"
    assert regex.findall(r"\d+", "a1b22c333") == ["1", "22", "333"]
    results.append(("regex", "PASS"))
except Exception as e: results.append(("regex", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
