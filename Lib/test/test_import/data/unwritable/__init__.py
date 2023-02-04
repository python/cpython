import sys

class MyMod(object):
    __slots__ = ['__builtins__', '__cached__', '__doc__',
                 '__file__', '__loader__', '__name__',
                 '__package__', '__path__', '__spec__']
    def __init__(self):
        for attr in self.__slots__:
            setattr(self, attr, globals()[attr])


sys.modules[__name__] = MyMod()
