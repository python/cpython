def func():
    pass
func.__module__ = None

class A:
    def method(self):
        pass
    method.__module__ = None
