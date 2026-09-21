lazy import test.test_lazy_import.data.basic2 as basic2

def f():
    x = globals()
    return x['basic2'].resolve()

f()
