"""This is a test module for test_pydoc"""

__author__ = "Benjamin Peterson"
__credits__ = "Nobody"
__version__ = "1.2.3.4"


class A:
    """Hello and goodbye"""
    def __init__():
        """Wow, I have no function!"""
        pass

class B(object):
    NO_MEANING = "eggs"
    pass

def doc_func():
    """
    This function solves all of the world's problems:
    hunger
    lack of Python
    war
    """

def nodoc_func():
    pass
