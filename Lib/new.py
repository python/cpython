"""Create new objects of various types.  Deprecated.

This module is no longer required except for backward compatibility.
Objects of most types can now be created by calling the type object.
"""

from types import ClassType as classobj
from types import FunctionType as function
from types import MethodType as instancemethod
from types import ModuleType as module
from types import CodeType as code
