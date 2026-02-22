"""Batch 8: FFI, security, utils"""
import sys
results = []

# cffi
try:
    import cffi
    ffi = cffi.FFI()
    ffi.cdef("int abs(int);")
    results.append(("cffi", "PASS"))
except Exception as e: results.append(("cffi", f"FAIL: {e}"))

# ctypes
try:
    import ctypes
    x = ctypes.c_int(42)
    assert x.value == 42
    s = ctypes.c_char_p(b"hello")
    assert s.value == b"hello"
    results.append(("ctypes", "PASS"))
except Exception as e: results.append(("ctypes", f"FAIL: {e}"))

# threadpoolctl
try:
    from threadpoolctl import threadpool_info
    info = threadpool_info()
    assert isinstance(info, list)
    results.append(("threadpoolctl", "PASS"))
except Exception as e: results.append(("threadpoolctl", f"FAIL: {e}"))

# itsdangerous
try:
    from itsdangerous import URLSafeSerializer
    s = URLSafeSerializer("secret")
    signed = s.dumps({"user": "test"})
    data = s.loads(signed)
    assert data["user"] == "test"
    results.append(("itsdangerous", "PASS"))
except Exception as e: results.append(("itsdangerous", f"FAIL: {e}"))

# colorama
try:
    from colorama import Fore, Style
    text = f"{Fore.RED}red{Style.RESET_ALL}"
    assert "red" in text
    results.append(("colorama", "PASS"))
except Exception as e: results.append(("colorama", f"FAIL: {e}"))

# decorator
try:
    import decorator
    @decorator.decorator
    def trace(f, *args, **kwargs):
        return f(*args, **kwargs)
    @trace
    def double(x): return x * 2
    assert double(5) == 10
    results.append(("decorator", "PASS"))
except Exception as e: results.append(("decorator", f"FAIL: {e}"))

# defusedxml
try:
    import defusedxml.ElementTree as ET
    root = ET.fromstring("<root><item>test</item></root>")
    assert root.find("item").text == "test"
    results.append(("defusedxml", "PASS"))
except Exception as e: results.append(("defusedxml", f"FAIL: {e}"))

# tomli
try:
    import tomli
    data = tomli.loads('[project]\nname = "test"\nversion = "1.0"')
    assert data["project"]["name"] == "test"
    results.append(("tomli", "PASS"))
except Exception as e: results.append(("tomli", f"FAIL: {e}"))

# more-itertools
try:
    from more_itertools import flatten, chunked
    assert list(flatten([[1, 2], [3, 4]])) == [1, 2, 3, 4]
    assert list(chunked([1, 2, 3, 4, 5], 2)) == [[1, 2], [3, 4], [5]]
    results.append(("more-itertools", "PASS"))
except Exception as e: results.append(("more-itertools", f"FAIL: {e}"))

for name, status in results:
    print(f"  {name}: {status}")
failed = sum(1 for _, s in results if s.startswith("FAIL"))
print(f"\n{len(results) - failed}/{len(results)} passed")
sys.exit(1 if failed else 0)
