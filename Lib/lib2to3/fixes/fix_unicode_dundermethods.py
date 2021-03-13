"""Fixer to convert name of __unicode__ class methods to __str__, unless it exists already.
"""
# Author: Bart Broere


from lib2to3.fixer_base import BaseFix
from lib2to3.pytree import Leaf


class FixUnicodeDundermethods(BaseFix):
    def match(self, node):
        if isinstance(node, Leaf):
            try:
                if node.value == "__unicode__" and node.prev_sibling.value == "def":
                    return True
            except AttributeError:
                return False
        return False

    def transform(self, node, results):
        try:
            for element in node.parent.parent.parent.leaves():
                if element.value == "class":
                    if not self._method_exists(node.parent.parent.parent, "__str__"):
                        node.value = "__str__"
                    return node
                break
        except AttributeError:
            return None
        return None

    def _method_exists(self, class_node, method_name):
        for element in class_node.leaves():
            for element_j in element.leaves():
                if element_j.value == "def":
                    if element_j.next_sibling.value == method_name:
                        return True
        return False
