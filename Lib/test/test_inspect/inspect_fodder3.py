from functools import cached_property

# docstring in parent, inherited in child
class ParentInheritDoc:
    @cached_property
    def foo(self):
        """docstring for foo defined in parent"""

class ChildInheritDoc(ParentInheritDoc):
    pass

class ChildInheritDefineDoc(ParentInheritDoc):
    @cached_property
    def foo(self):
        pass

# Redefine foo as something other than cached_property
class ChildPropertyFoo(ParentInheritDoc):
    @property
    def foo(self):
        """docstring for the property foo"""

class ChildMethodFoo(ParentInheritDoc):
    def foo(self):
        """docstring for the method foo"""

# docstring in child but not parent
class ParentNoDoc:
    @cached_property
    def foo(self):
        pass

class ChildNoDoc(ParentNoDoc):
    pass

class ChildDefineDoc(ParentNoDoc):
    @cached_property
    def foo(self):
        """docstring for foo defined in child"""
