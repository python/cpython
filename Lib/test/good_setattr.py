foo = 1

def __setattr__(name, value):
    if name == 'foo':
        raise AttributeError("Read-only attribute")
    globals()[name] = value
