import importlib

def filter(module_name, imported_name, from_list):
    assert module_name == __name__
    assert imported_name == "test.test_import.data.lazy_imports.basic2"
    assert from_list == ['f']
    return True

importlib.set_lazy_imports(None, filter)

lazy from import test.test_import.data.lazy_imports.basic2 import f
