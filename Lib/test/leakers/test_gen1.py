import gc

# Taken from test_generators

def f():
    try:
        yield
    except GeneratorExit:
        yield "foo!"

def inner_leak():
    g = f()
    g.next()

def leak():
    inner_leak()
    gc.collect()
    gc.collect()
    gc.collect()
