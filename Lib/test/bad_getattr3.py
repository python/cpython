def __getattr__(name):
    if name != 'delgetattr':
        raise AttributeError('Only deletion')
    del globals()['__getattr__']
    return 'OK, deleted'
