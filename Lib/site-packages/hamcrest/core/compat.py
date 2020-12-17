__author__ = "Per Fagrell"
__copyright__ = "Copyright 2013 hamcrest.org"
__license__ = "BSD, see License.txt"

__all__ = ['is_callable']

import sys

# callable was not part of py3k until 3.2, so we create this
# generic is_callable to use callable if possible, otherwise
# we use generic homebrew.
if sys.version_info[0] == 3 and sys.version_info[1] < 2:
    def is_callable(function):
        """Return whether the object is callable (i.e., some kind of function)."""
        if function is None:
            return False
        return any("__call__" in klass.__dict__ for klass in type(function).__mro__)
else:
    is_callable = callable
