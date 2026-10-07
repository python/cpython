import sys

def filter(module_name, imported_name, from_list):
    assert module_name == __name__
    assert imported_name == "test.test_lazy_import.data.basic2"
    return False

sys.set_lazy_imports_filter(filter)

lazy import test.test_lazy_import.data.basic2 as basic2
