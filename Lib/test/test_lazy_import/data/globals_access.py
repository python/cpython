# Test that globals() returns lazy proxy objects without reifying
lazy import test.test_lazy_import.data.basic2 as basic2

def get_from_globals():
    g = globals()
    return g['basic2']

def get_direct():
    return basic2
