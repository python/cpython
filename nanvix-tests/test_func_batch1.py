"""Batch 1: Core utility packages"""
import sys
results = []

# packaging
try:
    from packaging.version import Version
    v = Version("3.12.3")
    assert str(v) == "3.12.3"
    assert v > Version("3.11.0")
    results.append(("packaging", "PASS"))
except Exception as e: results.append(("packaging", f"FAIL: {e}"))

# attrs
try:
    import attr
    @attr.s(auto_attribs=True)
    class Point: x: int; y: int
    p = Point(1, 2)
    assert p.x == 1 and p.y == 2
    results.append(("attrs", "PASS"))
except Exception as e: results.append(("attrs", f"FAIL: {e}"))

# six
try:
    import six
    assert six.PY3
    assert six.text_type is str
    results.append(("six", "PASS"))
except Exception as e: results.append(("six", f"FAIL: {e}"))

# python-dateutil
try:
    from dateutil.parser import parse
    from dateutil.relativedelta import relativedelta
    d = parse("2024-01-15")
    d2 = d + relativedelta(months=3)
    assert d2.month == 4
    results.append(("python-dateutil", "PASS"))
except Exception as e: results.append(("python-dateutil", f"FAIL: {e}"))

# typing-extensions
try:
    from typing_extensions import TypedDict
    class Movie(TypedDict): name: str; year: int
    m: Movie = {"name": "Test", "year": 2024}
    results.append(("typing-extensions", "PASS"))
except Exception as e: results.append(("typing-extensions", f"FAIL: {e}"))

# typing-inspection
try:
    from typing_inspection import typing_objects
    results.append(("typing-inspection", "PASS"))
except Exception as e: results.append(("typing-inspection", f"FAIL: {e}"))

# annotated-types
try:
    from annotated_types import Gt
    gt = Gt(0)
    assert gt.gt == 0
    results.append(("annotated-types", "PASS"))
except Exception as e: results.append(("annotated-types", f"FAIL: {e}"))

# cycler
try:
    from cycler import cycler
    c = cycler(color=['r', 'g', 'b'])
    assert len(c) == 3
    results.append(("cycler", "PASS"))
except Exception as e: results.append(("cycler", f"FAIL: {e}"))

# pytz
try:
    import pytz
    tz = pytz.timezone('US/Eastern')
    assert tz is not None
    results.append(("pytz", "PASS"))
except Exception as e: results.append(("pytz", f"FAIL: {e}"))

# tzdata
try:
    import tzdata
    results.append(("tzdata", "PASS"))
except Exception as e: results.append(("tzdata", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
