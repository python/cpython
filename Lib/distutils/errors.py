"""distutils.errors

Provides exceptions used by the Distutils modules.  Note that Distutils
modules may raise standard exceptions; in particular, SystemExit is
usually raised for errors that are obviously the end-user's fault
(eg. bad command-line arguments).

This module safe to use in "from ... import *" mode; it only exports
symbols whose names start with "Distutils" and end with "Error"."""

# created 1999/03/03, Greg Ward

__rcsid__ = "$Id$"

import types

if type (RuntimeError) is types.ClassType:

    # DistutilsError is the root of all Distutils evil.
    class DistutilsError (Exception):
        pass

    # DistutilsModuleError is raised if we are unable to load an expected
    # module, or find an expected class within some module
    class DistutilsModuleError (DistutilsError):
        pass

    # DistutilsClassError is raised if we encounter a distribution or command
    # class that's not holding up its end of the bargain.
    class DistutilsClassError (DistutilsError):
        pass

    # DistutilsGetoptError (help me -- I have JavaProgrammersDisease!) is
    # raised if the option table provided to fancy_getopt is bogus.
    class DistutilsGetoptError (DistutilsError):
        pass

    # DistutilsArgError is raised by fancy_getopt in response to getopt.error;
    # distutils.core then turns around and raises SystemExit from that.  (Thus
    # client code should never see DistutilsArgError.)
    class DistutilsArgError (DistutilsError):
        pass

    # DistutilsFileError is raised for any problems in the filesystem:
    # expected file not found, etc.
    class DistutilsFileError (DistutilsError):
        pass

    # DistutilsOptionError is raised anytime an attempt is made to access
    # (get or set) an option that does not exist for a particular command
    # (or for the distribution itself).
    class DistutilsOptionError (DistutilsError):
        pass

    # DistutilsValueError is raised anytime an option value (presumably
    # provided by setup.py) is invalid.
    class DistutilsValueError (DistutilsError):
        pass

    # DistutilsPlatformError is raised when we find that we don't
    # know how to do something on the current platform (but we do
    # know how to do it on some platform).
    class DistutilsPlatformError (DistutilsError):
        pass

    # DistutilsExecError is raised if there are any problems executing
    # an external program
    class DistutilsExecError (DistutilsError):
        pass

# String-based exceptions
else:
    DistutilsError = 'DistutilsError'
    DistutilsModuleError = 'DistutilsModuleError'
    DistutilsClassError = 'DistutilsClassError'
    DistutilsGetoptError = 'DistutilsGetoptError'
    DistutilsArgError = 'DistutilsArgError'
    DistutilsFileError = 'DistutilsFileError'
    DistutilsOptionError = 'DistutilsOptionError'
    DistutilsValueError = 'DistutilsValueError'
    DistutilsPlatformError = 'DistutilsPlatformError'
    DistutilsExecError = 'DistutilsExecError'
    
del types
