# Test __lazy_modules__ with from imports
__lazy_modules__ = ['test.test_import.data.lazy_imports.basic2']
from test.test_import.data.lazy_imports.basic2 import x, f

def get_x():
    return x
