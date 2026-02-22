"""Test: catalogue"""
import sys
sys.stdout.reconfigure(line_buffering=True)
try:
    import catalogue
    reg = catalogue.create("test", "funcs")
    @reg.register("double")
    def double(x): return x * 2
    assert reg.get("double")(5) == 10
    print("catalogue: PASS")
except Exception as e:
    print(f"catalogue: FAIL: {e}")
    sys.exit(1)
