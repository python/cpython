import sys

def filter(module_name, imported_name, from_list):
    assert module_name == __name__
    assert imported_name == "test.test_import.data.lazy_imports.basic2"
    assert from_list == ('f',)
    return False

sys.set_lazy_imports_filter(filter)

lazy from test.test_import.data.lazy_imports.basic2 import f
