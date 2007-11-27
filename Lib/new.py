"""Create new objects of various types.  Deprecated.

This module is no longer required except for backward compatibility.
Objects of most types can now be created by calling the type object.
"""
from warnings import warn as _warn
_warn("The 'new' module is not supported in 3.x, use the 'types' module "
    "instead.", DeprecationWarning, 2)

from types import ClassType as classobj
from types import FunctionType as function
from types import InstanceType as instance
from types import MethodType as instancemethod
from types import ModuleType as module

# CodeType is not accessible in restricted execution mode
try:
    from types import CodeType as code
except ImportError:
    pass
