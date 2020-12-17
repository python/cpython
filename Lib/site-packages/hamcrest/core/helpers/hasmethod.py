__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


def hasmethod(obj: object, methodname: str) -> bool:
    """Does ``obj`` have a method named ``methodname``?"""

    if not hasattr(obj, methodname):
        return False
    method = getattr(obj, methodname)
    return callable(method)
