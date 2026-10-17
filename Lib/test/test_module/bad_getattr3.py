def __getattr__(name):
    if name != 'delgetattr':
        raise AttributeError
    del globals()['__getattr__']
    raise AttributeError
