# Test that lazy from import with multiple names only reifies accessed names
lazy from test.test_import.data.lazy_imports.basic2 import f, x

def get_globals():
    return globals()
