"""
Some demos with 0-based and 1-based indicies, and closed slices.
"""

########################################

class SLICER(object):
    """
    class of SLICE, used to convert an expression in brackets / braces
    into a slice(...) or extension.
    """
    def __getitem__(self, arg):
        return arg

SLICE = SLICER()

########################################

def test(*args):
    import ast
    for code in args:
        print(code, '->', end=' ')
        try:
            print(ast.unparse(ast.parse(code)))
        except Exception as e:
            print(e.__class__.__name__)

def tests():
    brackets = [
        "a[i]",
        "a[i:]",
        "a[i:j]",
        "a[i:j:k]",
        "a[i::k]",
        "a[:j]",
        "a[:j:k]",
        "a[::k]",
        "a[:]",
        "a[::]",
        "a[i:j:]",
        "a[i:j:k:]",
        "a[:j:]",
        "a[:j:k:]",
    ]
    test(*brackets)
    print()
    braces = [code.replace("[", "{").replace("]", "}") for code in brackets]
    test(*braces)

########################################

import builtins

class RANGE(object):
    """
    Enables to write range[i:j:k] in addition to range(i, j, k).
    Main interest: also works with closed slices.
    """
    def __call__(self, *args):
        return builtins.range(*args)

    def __getitem__(self, arg):
        if isinstance(arg, slice):
            step = 1 if arg.step is None else arg.step
            return builtins.range(arg.start, arg.stop, step)
        else:
            raise ValueError("only range[slice] allowed")

range = RANGE()

########################################
