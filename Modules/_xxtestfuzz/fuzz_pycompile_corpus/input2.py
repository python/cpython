class Foo(metaclass=42):
    __slots__ = ['x']
    pass

foo = Foo()
