from functools import cached_property

class Parent:
    @cached_property
    def foo(self):
        "this is the docstring for foo"
        pass

class Child(Parent):
    @cached_property
    def foo(self):
        pass
