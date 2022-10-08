from __future__ import annotations

a:int=3
b:str="foo"

class MyClass:
    a:int=4
    b:str="bar"
    def __init__(self, a, b):
        self.a = a
        self.b = b
    def __eq__(self, other):
        return isinstance(other, MyClass) and self.a == other.a and self.b == other.b

def function(a:int, b:str) -> MyClass:
    return MyClass(a, b)


def function2(a:int, b:"str", c:MyClass) -> MyClass:
    pass


def function3(a:"int", b:"str", c:"MyClass"):
    pass


class UnannotatedClass:
    pass

def unannotated_function(a, b, c): pass

class MyClassWithLocalAnnotations:
    mytype = int
    x: mytype
