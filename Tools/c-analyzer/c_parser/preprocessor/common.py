import contextlib
import distutils.ccompiler
import logging
import os
import shlex
import subprocess
import sys

from ..info import FileInfo, SourceLine
from .errors import (
    PreprocessorFailure,
    ErrorDirectiveError,
    MissingDependenciesError,
    OSMismatchError,
)


logger = logging.getLogger(__name__)


# XXX Add aggregate "source" class(es)?
#  * expose all lines as single text string
#  * expose all lines as sequence
#  * iterate all lines


def run_cmd(argv, *,
            #capture_output=True,
            stdout=subprocess.PIPE,
            #stderr=subprocess.STDOUT,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
            **kwargs
            ):
    if isinstance(stderr, str) and stderr.lower() == 'stdout':
        stderr = subprocess.STDOUT

    kw = dict(locals())
    kw.pop('argv')
    kw.pop('kwargs')
    kwargs.update(kw)

    # Remove LANG environment variable: the C parser doesn't support GCC
    # localized messages
    env = dict(os.environ)
    env.pop('LANG', None)

    proc = subprocess.run(argv, env=env, **kwargs)
    return proc.stdout


def preprocess(tool, filename, **kwargs):
    argv = _build_argv(tool, filename, **kwargs)
    logger.debug(' '.join(shlex.quote(v) for v in argv))

    # Make sure the OS is supported for this file.
    if (_expected := is_os_mismatch(filename)):
        error = None
        raise OSMismatchError(filename, _expected, argv, error, TOOL)

    # Run the command.
    with converted_error(tool, argv, filename):
        # We use subprocess directly here, instead of calling the
        # distutil compiler object's preprocess() method, since that
        # one writes to stdout/stderr and it's simpler to do it directly
        # through subprocess.
        return run_cmd(argv)


def _build_argv(
    tool,
    filename,
    incldirs=None,
    macros=None,
    preargs=None,
    postargs=None,
    executable=None,
    compiler=None,
):
    compiler = distutils.ccompiler.new_compiler(
        compiler=compiler or tool,
    )
    if executable:
        compiler.set_executable('preprocessor', executable)

    argv = None
    def _spawn(_argv):
        nonlocal argv
        argv = _argv
    compiler.spawn = _spawn
    compiler.preprocess(
        filename,
        macros=[tuple(v) for v in macros or ()],
        include_dirs=incldirs or (),
        extra_preargs=preargs or (),
        extra_postargs=postargs or (),
    )
    return argv


@contextlib.contextmanager
def converted_error(tool, argv, filename):
    try:
        yield
    except subprocess.CalledProcessError as exc:
        convert_error(
            tool,
            argv,
            filename,
            exc.stderr,
            exc.returncode,
        )


def convert_error(tool, argv, filename, stderr, rc):
    error = (stderr.splitlines()[0], rc)
    if (_expected := is_os_mismatch(filename, stderr)):
        logger.debug(stderr.strip())
        raise OSMismatchError(filename, _expected, argv, error, tool)
    elif (_missing := is_missing_dep(stderr)):
        logger.debug(stderr.strip())
        raise MissingDependenciesError(filename, (_missing,), argv, error, tool)
    elif '#error' in stderr:
        # XXX Ignore incompatible files.
        error = (stderr.splitlines()[1], rc)
        logger.debug(stderr.strip())
        raise ErrorDirectiveError(filename, argv, error, tool)
    else:
        # Try one more time, with stderr written to the terminal.
        try:
            output = run_cmd(argv, stderr=None)
        except subprocess.CalledProcessError:
            raise PreprocessorFailure(filename, argv, error, tool)


def is_os_mismatch(filename, errtext=None):
    # See: https://docs.python.org/3/library/sys.html#sys.platform
    actual = sys.platform
    if actual == 'unknown':
        raise NotImplementedError

    if errtext is not None:
        if (missing := is_missing_dep(errtext)):
            matching = get_matching_oses(missing, filename)
            if actual not in matching:
                return matching
    return False


def get_matching_oses(missing, filename):
    # OSX
    if 'darwin' in filename or 'osx' in filename:
        return ('darwin',)
    elif missing == 'SystemConfiguration/SystemConfiguration.h':
        return ('darwin',)

    # Windows
    elif missing in ('windows.h', 'winsock2.h'):
        return ('win32',)

    # other
    elif missing == 'sys/ldr.h':
        return ('aix',)
    elif missing == 'dl.h':
        # XXX The existence of Python/dynload_dl.c implies others...
        # Note that hpux isn't actual supported any more.
        return ('hpux', '???')

    # unrecognized
    else:
        return ()


def is_missing_dep(errtext):
    if 'No such file or directory' in errtext:
        missing = errtext.split(': No such file or directory')[0].split()[-1]
        return missing
    return False
