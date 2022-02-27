# line 1
def wrap_many(*foo):
    def wrapper(func):
        return func
    return wrapper

@wrap_many(lambda: bool(True), lambda: True)
def func8():
    return 9
