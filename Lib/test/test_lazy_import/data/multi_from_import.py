# Test that lazy from import with multiple names only reifies accessed names
lazy from test.test_lazy_import.data.basic2 import f, x

def get_globals():
    return globals()
