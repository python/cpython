"""Exmaple file to be parsed for the parsermodule example.

The classes and functions in this module exist only to exhibit the ability
of the handling information extraction from nested definitions using parse
trees.  They shouldn't interest you otherwise!
"""

class Simple:
    "This class does very little."

    def method(self):
        "This method does almost nothing."
        return 1

    class Nested:
        "This is a nested class."

        def nested_method(self):
            "Method of Nested class."
            def nested_function():
                "Function in method of Nested class."
                pass
            return nested_function

def function():
    "This function lives at the module level."
    return 0
