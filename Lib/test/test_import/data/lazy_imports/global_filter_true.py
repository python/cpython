import sys

def filter(module_name, imported_name, from_list):
    assert module_name == __name__
    assert imported_name == "test.test_import.data.lazy_imports.basic2"
    return True

sys.set_lazy_imports("normal")
sys.set_lazy_imports_filter(filter)

lazy import test.test_import.data.lazy_imports.basic2 as basic2
