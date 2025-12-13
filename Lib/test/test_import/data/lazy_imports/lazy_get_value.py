lazy import test.test_import.data.lazy_imports.basic2 as basic2

def f():
    x = globals()
    return x['basic2'].resolve()

f()
