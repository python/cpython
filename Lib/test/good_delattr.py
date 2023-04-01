foo = 1

def __delattr__(name):
    if name == 'foo':
        raise AttributeError("Read-only attribute")
    del globals()[name]
