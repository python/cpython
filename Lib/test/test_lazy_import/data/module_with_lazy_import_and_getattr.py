lazy import test.test_lazy_import.data.basic2 as basic2
lazy from test.test_lazy_import.data.basic2 import f

def __getattr__(name):
    return f"from_getattr:{name}"
