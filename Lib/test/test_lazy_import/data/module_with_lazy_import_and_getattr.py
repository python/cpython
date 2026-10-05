lazy import test.test_lazy_import.data.basic2 as basic2

def __getattr__(name):
    return f"from_getattr:{name}"
