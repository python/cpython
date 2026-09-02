class C:
    """This is a class summary that consists of a very long single line, exceeding the recommended PEP 8 limit.

    The rest of the docstring body, separated from the summary by a blank line, can also contain very long lines.
    """
    def meth(self):
        """This is a method summary that consists of a very long single line, exceeding the recommended PEP 8 limit.

        The rest of the docstring body, separated from the summary by a blank line, can also contain very long lines.
        """

    @property
    def prop(self):
        """This is a property summary that consists of a very long single line, exceeding the recommended PEP 8 limit.

        The rest of the docstring body, separated from the summary by a blank line, can also contain very long lines.
        """

def func(self):
    """This is a function summary that consists of a very long single line, exceeding the recommended PEP 8 limit.

    The rest of the docstring body, separated from the summary by a blank line, can also contain very long lines.
    """

data = C()
data.__doc__ = """This is a data summary that consists of a very long single line, exceeding the recommended PEP 8 limit.

The rest of the docstring body, separated from the summary by a blank line, can also contain very long lines.
"""
