"""Exceptions used throughout the package.

Submodules of packaging may raise exceptions defined in this module as
well as standard exceptions; in particular, SystemExit is usually raised
for errors that are obviously the end-user's fault (e.g. bad
command-line arguments).
"""


class PackagingError(Exception):
    """The root of all Packaging evil."""


class PackagingModuleError(PackagingError):
    """Unable to load an expected module, or to find an expected class
    within some module (in particular, command modules and classes)."""


class PackagingClassError(PackagingError):
    """Some command class (or possibly distribution class, if anyone
    feels a need to subclass Distribution) is found not to be holding
    up its end of the bargain, ie. implementing some part of the
    "command "interface."""


class PackagingGetoptError(PackagingError):
    """The option table provided to 'fancy_getopt()' is bogus."""


class PackagingArgError(PackagingError):
    """Raised by fancy_getopt in response to getopt.error -- ie. an
    error in the command line usage."""


class PackagingFileError(PackagingError):
    """Any problems in the filesystem: expected file not found, etc.
    Typically this is for problems that we detect before IOError or
    OSError could be raised."""


class PackagingOptionError(PackagingError):
    """Syntactic/semantic errors in command options, such as use of
    mutually conflicting options, or inconsistent options,
    badly-spelled values, etc.  No distinction is made between option
    values originating in the setup script, the command line, config
    files, or what-have-you -- but if we *know* something originated in
    the setup script, we'll raise PackagingSetupError instead."""


class PackagingSetupError(PackagingError):
    """For errors that can be definitely blamed on the setup script,
    such as invalid keyword arguments to 'setup()'."""


class PackagingPlatformError(PackagingError):
    """We don't know how to do something on the current platform (but
    we do know how to do it on some platform) -- eg. trying to compile
    C files on a platform not supported by a CCompiler subclass."""


class PackagingExecError(PackagingError):
    """Any problems executing an external program (such as the C
    compiler, when compiling C files)."""


class PackagingInternalError(PackagingError):
    """Internal inconsistencies or impossibilities (obviously, this
    should never be seen if the code is working!)."""


class PackagingTemplateError(PackagingError):
    """Syntax error in a file list template."""


class PackagingByteCompileError(PackagingError):
    """Byte compile error."""


class PackagingPyPIError(PackagingError):
    """Any problem occuring during using the indexes."""


# Exception classes used by the CCompiler implementation classes
class CCompilerError(Exception):
    """Some compile/link operation failed."""


class PreprocessError(CCompilerError):
    """Failure to preprocess one or more C/C++ files."""


class CompileError(CCompilerError):
    """Failure to compile one or more C/C++ source files."""


class LibError(CCompilerError):
    """Failure to create a static library from one or more C/C++ object
    files."""


class LinkError(CCompilerError):
    """Failure to link one or more C/C++ object files into an executable
    or shared library file."""


class UnknownFileError(CCompilerError):
    """Attempt to process an unknown file type."""


class MetadataMissingError(PackagingError):
    """A required metadata is missing"""


class MetadataConflictError(PackagingError):
    """Attempt to read or write metadata fields that are conflictual."""


class MetadataUnrecognizedVersionError(PackagingError):
    """Unknown metadata version number."""


class IrrationalVersionError(Exception):
    """This is an irrational version."""
    pass


class HugeMajorVersionNumError(IrrationalVersionError):
    """An irrational version because the major version number is huge
    (often because a year or date was used).

    See `error_on_huge_major_num` option in `NormalizedVersion` for details.
    This guard can be disabled by setting that option False.
    """
    pass


class InstallationException(Exception):
    """Base exception for installation scripts"""


class InstallationConflict(InstallationException):
    """Raised when a conflict is detected"""
