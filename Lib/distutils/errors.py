"""distutils.errors

Provides exceptions used by the Distutils modules.  Note that Distutils
modules may raise standard exceptions; in particular, SystemExit is
usually raised for errors that are obviously the end-user's fault
(eg. bad command-line arguments).

This module safe to use in "from ... import *" mode; it only exports
symbols whose names start with "Distutils" and end with "Error"."""

# created 1999/03/03, Greg Ward

__revision__ = "$Id$"

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

    # DistutilsOptionError is raised for syntactic/semantic errors in
    # command options, such as use of mutually conflicting options, or
    # inconsistent options, badly-spelled values, etc.  No distinction is
    # made between option values originating in the setup script, the
    # command line, config files, or what-have-you.
    class DistutilsOptionError (DistutilsError):
        pass

    # DistutilsSetupError is raised for errors that can be definitely
    # blamed on the setup script, such as invalid keyword arguments to
    # 'setup()'.
    class DistutilsSetupError (DistutilsError):
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

    # DistutilsInternalError is raised on internal inconsistencies
    # or impossibilities
    class DistutilsInternalError (DistutilsError):
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
    DistutilsPlatformError = 'DistutilsPlatformError'
    DistutilsExecError = 'DistutilsExecError'
    DistutilsInternalError = 'DistutilsInternalError'
    
del types
