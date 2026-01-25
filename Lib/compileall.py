"""Module/script to byte-compile all .py files to .pyc files.

When called as a script with arguments, this compiles the directories
given as arguments recursively; the -l option prevents it from
recursing into directories.

Without arguments, it compiles all modules on sys.path, without
recursing into subdirectories.  (Even though it should do so for
packages -- for now, you'll have to deal with packages separately.)

See module py_compile for details of the actual byte-compilation.
"""

import filecmp
import importlib.util
import multiprocessing
import os
import py_compile
import re
import struct
import sys
from argparse import ArgumentParser, Namespace
from collections.abc import Iterable
from concurrent.futures.process import _check_system_limits
from functools import partial
from multiprocessing.context import ForkServerContext
from pathlib import Path
from py_compile import PycInvalidationMode
from re import Match, Pattern
from typing import Iterator

__all__ = ["compile_dir", "compile_file", "compile_path"]


def _walk_dir(
    dir: os.PathLike | str,
    maxlevels: int,
    quiet: int = 0,
) -> Iterable[str]:
    """Recursively walk a directory up to a given depth,
    yielding file paths.

    Args:
        dir (os.PathLike): Directory to walk.
        maxlevels (int): Recursion depth. 0 means no recursion.
        quiet (int, optional): Verbosity level. 0 = print messages,
            1 = suppress normal messages, 2 = suppress warnings and
            messages. Defaults to 0.

    Yields:
        str: Full path to each file found in the directory tree.
    """
    # Convert PathLike objects to string for printing
    if quiet < 2 and isinstance(dir, os.PathLike):
        dir = os.fspath(dir)
    # Print the directory we are listing unless quiet mode >= 1
    if not quiet:
        print("Listing {!r}...".format(dir))

    # Attempt to list all entries in the directory
    names: list[str] = []
    try:
        names = os.listdir(dir)
    except OSError:
        # If listing fails, print a warning unless quiet mode >= 2
        if quiet < 2:
            print(f"Can't list {dir!r}")

    # Sort entries for consistent order
    names.sort()

    # Iterate over all names in the directory
    for name in names:
        # Skip bytecode cache directories
        if name != "__pycache__":
            # Build the full path of the entry
            fullname = os.path.join(dir, name)
            # If entry is a file, yield its path
            if not os.path.isdir(fullname):
                yield fullname
            # If entry is a directory, decide whether to recurse
            elif (
                # Only recurse if maxlevels allows
                maxlevels > 0
                # Skip "." and ".."
                and name not in (os.curdir, os.pardir)
                # Confirm it's a directory
                and os.path.isdir(fullname)
                # Skip symbolic links
                and not os.path.islink(fullname)
            ):
                # Recursively yield from subdirectory,
                # decrementing maxlevels
                yield from _walk_dir(
                    fullname,
                    maxlevels=maxlevels - 1,
                    quiet=quiet,
                )


def compile_dir(
    dir: str | os.PathLike,
    maxlevels: int | None = None,
    ddir: str | os.PathLike | None = None,
    force: bool = False,
    rx: Pattern | None = None,
    quiet: int = 0,
    legacy: bool = False,
    optimize: int | list[int] = -1,
    workers: int = 1,
    invalidation_mode: PycInvalidationMode | None = None,
    *,
    stripdir: str | os.PathLike | None = None,
    prependdir: str | os.PathLike | None = None,
    limit_sl_dest: str | os.PathLike | None = None,
    hardlink_dupes: bool = False,
) -> bool:
    """Byte-compile all Python modules in a directory tree.

    Args:
        dir (str | os.PathLike): Root directory to compile.
        maxlevels (int | None, optional): Maximum recursion depth.
            Defaults to sys.getrecursionlimit().
        ddir (str | None, optional): Prepend directory for compiled
            bytecode paths. Cannot be combined with stripdir or
            prependdir.
        force (bool, optional): If True, force recompilation even if
            bytecode timestamps are up-to-date. Defaults to False.
        rx (Pattern | None, optional): Regex pattern to filter files.
            Defaults to None.
        quiet (int, optional): Verbosity level: 0 = normal, 1 = errors
            only, 2 = silent. Defaults to 0.
        legacy (bool, optional): If True, produce legacy `.pyc` files
            instead of PEP 3147 layout. Defaults to False.
            optimize (int | list[int], optional): Optimization level(s)
            for compilation. -1 uses the current interpreter level.
            Defaults to -1.
        workers (int | None, optional): Number of parallel workers. 0
            or None lets ProcessPoolExecutor choose. Defaults to 1.
        invalidation_mode (PycInvalidationMode | None, optional): How to
            check pyc up-to-dateness. Defaults to None.
        stripdir (str | None, optional): Part of path to remove from
            source paths.
        prependdir (str | None, optional): Path to prepend to source
            paths after stripdir is applied.
        limit_sl_dest (str | None, optional): Ignore symlinks pointing
            outside this path.
        hardlink_dupes (bool, optional): Hardlink duplicated pyc files.
        Defaults to False.

    Returns:
        bool: True if all compilations succeeded, False if any failed.
    """
    ProcessPoolExecutor = None
    success: bool = True
    mp_context: ForkServerContext | None = None

    # Check for conflicting directory options
    if ddir is not None and (stripdir is not None or prependdir is not None):
        error_text: str = (
            "Destination dir (ddir) cannot be used "
            "in combination with stripdir or prependdir"
        )
        raise ValueError(error_text)

    # If ddir is specified, convert it to
    # stripdir/prependdir internally
    if ddir is not None:
        stripdir = dir
        prependdir = ddir
        # Clear ddir to avoid confusion
        ddir = None

    # Validate workers argument
    if workers < 0:
        raise ValueError("workers must be greater or equal to 0")

    # Prepare for multiprocessing if requested
    if workers != 1:
        try:
            # Ensuring that system supports ProcessPoolExecutor
            _check_system_limits()
        except NotImplementedError:
            # Fallback to single-threaded if not supported
            workers = 1
        else:
            # Import here only if needed
            from concurrent.futures import ProcessPoolExecutor

    # Set default recursion depth if not provided
    maxlevels = sys.getrecursionlimit() if maxlevels is None else maxlevels
    # Generate all files to compile
    files: Iterable[str] = _walk_dir(dir, quiet=quiet, maxlevels=maxlevels)

    # Use parallel compilation if requested and supported
    if workers != 1 and ProcessPoolExecutor is not None:
        # Use 'forkserver' method on Unix to safely spawn processes
        if multiprocessing.get_start_method() == "fork":
            mp_context = multiprocessing.get_context("forkserver")
        # Let ProcessPoolExecutor choose default
        # number of workers if 0 or None
        max_workers = workers or None
        # Execute compilation in parallel
        with ProcessPoolExecutor(
            max_workers=max_workers, mp_context=mp_context
        ) as executor:
            results: Iterator[bool] = executor.map(
                partial(
                    compile_file,
                    ddir=ddir,
                    force=force,
                    rx=rx,
                    quiet=quiet,
                    legacy=legacy,
                    optimize=optimize,
                    invalidation_mode=invalidation_mode,
                    stripdir=stripdir,
                    prependdir=prependdir,
                    limit_sl_dest=limit_sl_dest,
                    hardlink_dupes=hardlink_dupes,
                ),
                files,
                chunksize=4,
            )
            # Reduce results to single success flag
            success = min(results, default=True)
    else:
        # Single-threaded fallback
        for file in files:
            compiled: bool = compile_file(
                fullname=file,
                ddir=ddir,
                force=force,
                rx=rx,
                quiet=quiet,
                legacy=legacy,
                optimize=optimize,
                invalidation_mode=invalidation_mode,
                stripdir=stripdir,
                prependdir=prependdir,
                limit_sl_dest=limit_sl_dest,
                hardlink_dupes=hardlink_dupes,
            )
            # Mark overall success False if compilation fails
            if not compiled:
                success = False
    # Return True if all files compiled successfully
    return success


def compile_file(
    fullname: str | os.PathLike,
    ddir: str | os.PathLike | None = None,
    force: bool = False,
    rx: re.Pattern | None = None,
    quiet: int = 0,
    legacy: bool = False,
    optimize: int | list[int] = -1,
    invalidation_mode: PycInvalidationMode | None = None,
    *,
    stripdir: str | os.PathLike | None = None,
    prependdir: str | os.PathLike | None = None,
    limit_sl_dest: str | os.PathLike | None = None,
    hardlink_dupes: bool = False,
) -> bool:
    """Byte-compile a single Python source file (.py) into .pyc.

    Args:
        fullname: Path to the source `.py` file.
        ddir: If set, directory to embed into the bytecode path.
        force: Recompile even if timestamps indicate up-to-date
            bytecode.
        rx: Regex pattern; skip file if it matches.
        quiet: Verbosity level (0 = all messages, 1 = errors only,
            2 = silent).
        legacy: Use legacy pre-PEP3147 pyc paths.
        optimize: Single int or list of optimization levels (-1 =
            interpreter default).
        invalidation_mode: Mode for checking pyc up-to-dateness.
        stripdir: Path prefix to remove from the source path.
        prependdir: Path prefix to prepend after stripdir processing.
        limit_sl_dest: Ignore symlinks pointing outside this directory.
        hardlink_dupes: Hardlink identical pyc files for multiple
            optimization levels.

    Returns:
        bool: True if compilation succeeded or was skipped; False if
            errors occurred.
    """

    # Check for incompatible path options
    if ddir is not None and (stripdir is not None or prependdir is not None):
        raise ValueError(
            (
                "Destination dir (ddir) cannot be used "
                "in combination with stripdir or prependdir"
            )
        )
    head: str
    tail: str
    cfile: str
    dfile: str | None = None
    success: bool = True
    fullname = os.fspath(fullname)
    stripdir = os.fspath(stripdir) if stripdir is not None else None
    name: str = os.path.basename(fullname)

    # Get destination file if ddir is provided
    if ddir is not None:
        dfile = os.path.join(ddir, name)

    # Get stripdir by removing the prefix from fullname
    if stripdir is not None:
        # Path parts for comparison
        fullname_parts: list[str] = fullname.split(os.path.sep)
        stripdir_parts: list[str] = stripdir.split(os.path.sep)

        # Check if stripdir is a prefix of the source path
        if stripdir_parts != fullname_parts[: len(stripdir_parts)]:
            if quiet < 2:
                print(
                    "The stripdir path {!r} is not a valid prefix for "
                    "source path {!r}; ignoring".format(stripdir, fullname)
                )
        else:
            dfile = os.path.join(*fullname_parts[len(stripdir_parts) :])

    if prependdir is not None:
        # If no destination yet, use full source path
        dfile = os.path.join(prependdir, dfile or fullname)

    # Cast optimize to list
    if isinstance(optimize, int):
        optimize = [optimize]

    # Use set() to remove duplicates.
    # Use sorted() to create pyc files in a deterministic order.
    optimize = sorted(set(optimize))

    # Validate hardlinking option
    if hardlink_dupes and len(optimize) < 2:
        raise ValueError(
            "Hardlinking of duplicated bytecode makes sense "
            "only for more than one optimization level"
        )

    # Skip file if regex matches
    if rx is not None and rx.search(fullname):
        return success

    # Skip file if symlink points outside limit_sl_dest
    if limit_sl_dest is not None and os.path.islink(fullname):
        if (
            Path(limit_sl_dest).resolve()
            not in Path(fullname).resolve().parents
        ):
            return success

    # optimization file map
    opt_cfiles: dict[int, str] = {}

    if os.path.isfile(fullname):
        for opt_level in optimize:
            if legacy:
                # For legacy mode append "c" to the filename (.py → .pyc)
                opt_cfiles[opt_level] = fullname + "c"
            else:
                if opt_level >= 0:
                    # If optimization level is >=1,
                    # convert to string; else use ""
                    opt: int | str = opt_level if opt_level >= 1 else ""
                    cfile = importlib.util.cache_from_source(
                        fullname,
                        optimization=opt,
                    )
                    opt_cfiles[opt_level] = cfile
                else:
                    # Otherwise, proceed without optimization
                    cfile = importlib.util.cache_from_source(fullname)
                    opt_cfiles[opt_level] = cfile

        head, tail = name[:-3], name[-3:]
        # Only process .py files
        if tail == ".py":
            # Skiping file if already up-to-date and force is False
            if not force:
                try:
                    # Get the modification time of the source file
                    mtime: int = int(os.stat(fullname).st_mtime)
                    # Construct the expected pyc header
                    # (magic number + timestamp)
                    expect: bytes = struct.pack(
                        "<4sLL",
                        importlib.util.MAGIC_NUMBER,
                        0,
                        mtime & 0xFFFF_FFFF,
                    )
                    # Check all compiled files for each
                    # optimization level
                    for cfile in opt_cfiles.values():
                        with open(cfile, "rb") as chandle:
                            # Reading the header bytes
                            actual: bytes = chandle.read(12)
                        if expect != actual:
                            # If the header does not match,
                            # we need to recompile
                            break
                    else:
                        # All cache files are up-to-date
                        # no need to recompile
                        return success

                except OSError as e:
                    # Report the error
                    if quiet < 2:
                        error_message: str = (
                            "Warning: cannot check up-to-date "
                            f"status for {fullname!r}: {e}"
                        )
                        print(error_message)

            if not quiet:
                print("Compiling {!r}...".format(fullname))

            try:
                # Compile for each optimization level
                for index, opt_level in enumerate(optimize):
                    cfile = opt_cfiles[opt_level]
                    # Perform compilation
                    ok: str | None = py_compile.compile(
                        fullname,
                        cfile,
                        dfile,
                        True,
                        optimize=opt_level,
                        invalidation_mode=invalidation_mode,
                    )
                    if index > 0 and hardlink_dupes:
                        # Get the previous compiled file
                        previous_cfile: str = opt_cfiles[optimize[index - 1]]
                        # Compare the files byte-by-byte
                        if filecmp.cmp(cfile, previous_cfile, shallow=False):
                            # If files are identical,
                            # remove the duplicate file
                            os.unlink(cfile)
                            # Create a hard link to the previous file
                            os.link(previous_cfile, cfile)

            # Handle compilation errors from py_compile
            except py_compile.PyCompileError as err:
                success = False
                if quiet >= 2:
                    return success
                print(
                    "*** "
                    if quiet == 0
                    else f"*** Error compiling {fullname!r}..."
                )
                # Escape non-printable characters in the error message
                encoding: str = sys.stdout.encoding or sys.getdefaultencoding()
                msg: str = err.msg.encode(
                    encoding, errors="backslashreplace"
                ).decode(encoding)
                print(msg)

            # Catch other common errors and print error message
            except (SyntaxError, UnicodeError, OSError) as e:
                success = False
                if quiet >= 2:
                    return success
                print(
                    "*** "
                    if quiet == 0
                    else f"*** Error compiling {fullname!r}..."
                )
                print(f"{e.__class__.__name__}: {e}")

            else:
                if ok == 0:
                    success = False

    # Return result of the compilation
    return success


def compile_path(
    skip_curdir: int = 1,
    maxlevels: int = 0,
    force: bool = False,
    quiet: int = 0,
    legacy: bool = False,
    optimize: int = -1,
    invalidation_mode: PycInvalidationMode | None = None,
) -> bool:
    """Byte-compile all Python modules on `sys.path`.

    Args:
        skip_curdir (int, optional): If true, skip the current directory
            (`os.curdir`). Defaults to 1.
        maxlevels (int, optional): Maximum recursion depth when
            compiling subdirectories. 0 means unlimited. Defaults to 0.
        force (bool, optional): If True, force recompilation even if
            bytecode is up-to-date. Defaults to False.
        quiet (int, optional): Verbosity level. 0 = normal, 1 = suppress
            messages, 2 = suppress warnings and messages. Defaults to 0.
        legacy (bool, optional): If True, generate legacy `.pyc` files
            instead of `__pycache__` layout. Defaults to False.
        optimize (int, optional): Optimization level for bytecode:
            -1 = current interpreter optimization,
             0 = no optimization,
             1 = basic,
             2 = advanced. Defaults to -1.
        invalidation_mode (PycInvalidationMode | None, optional):
            Controls the invalidation mode for bytecode caching. See
            `compile_dir()` docs. Defaults to None.

    Returns:
        bool: True if all compilations succeeded, False if any failed.
    """
    success: bool = True

    for path in sys.path:
        # Skip empty or current directory if requested
        if (not path or path == os.curdir) and skip_curdir:
            if quiet < 2:
                print("Skipping current directory")
            continue

        # Compile this directory
        success = success and compile_dir(
            path,
            maxlevels,
            ddir=None,
            force=force,
            quiet=quiet,
            legacy=legacy,
            optimize=optimize,
            invalidation_mode=invalidation_mode,
        )

    return success


def add_parser_arguments(
    parser: ArgumentParser,
    invalidation_modes: list[str],
) -> None:
    """Register all command-line arguments for the compilation utility.

    Args:
        parser (ArgumentParser): Argument parser to which options will
            be added.
        invalidation_modes (list[str]): List of allowed invalidation
            mode names.

    Returns:
        None
    """

    # register -l arg
    parser.add_argument(
        "-l",
        action="store_const",
        const=0,
        default=None,
        dest="maxlevels",
        help="don't recurse into subdirectories",
    )
    # register -r arg
    parser.add_argument(
        "-r",
        type=int,
        dest="recursion",
        help=(
            "control the maximum recursion level. "
            "if `-l` and `-r` options are specified, "
            "then `-r` takes precedence."
        ),
    )
    # register -f arg
    parser.add_argument(
        "-f",
        action="store_true",
        dest="force",
        help="force rebuild even if timestamps are up to date",
    )
    # register -q arg
    parser.add_argument(
        "-q",
        action="count",
        dest="quiet",
        default=0,
        help=(
            "output only error messages; -qq will suppress "
            "the error messages as well."
        ),
    )
    # register -b arg
    parser.add_argument(
        "-b",
        action="store_true",
        dest="legacy",
        help="use legacy (pre-PEP3147) compiled file locations",
    )
    # register -d arg
    parser.add_argument(
        "-d",
        metavar="DESTDIR",
        dest="ddir",
        default=None,
        help=(
            "directory to prepend to file paths for use in "
            "compile-time tracebacks and in runtime "
            "tracebacks in cases where the source file is "
            "unavailable"
        ),
    )
    # register -s arg
    parser.add_argument(
        "-s",
        metavar="STRIPDIR",
        dest="stripdir",
        default=None,
        help=(
            "part of path to left-strip from path "
            "to source file - for example buildroot. "
            "`-d` and `-s` options cannot be "
            "specified together."
        ),
    )
    # register -p arg
    parser.add_argument(
        "-p",
        metavar="PREPENDDIR",
        dest="prependdir",
        default=None,
        help=(
            "path to add as prefix to path "
            "to source file - for example / to make "
            "it absolute when some part is removed "
            "by `-s` option. "
            "`-d` and `-p` options cannot be "
            "specified together."
        ),
    )
    # register -x arg
    parser.add_argument(
        "-x",
        metavar="REGEXP",
        dest="rx",
        default=None,
        help=(
            "skip files matching the regular expression; "
            "the regexp is searched for in the full path "
            "of each file considered for compilation"
        ),
    )
    # register -i arg
    parser.add_argument(
        "-i",
        metavar="FILE",
        dest="flist",
        help=(
            "add all the files and directories listed in "
            "FILE to the list considered for compilation; "
            'if "-", names are read from stdin'
        ),
    )
    # register compile_dest arg
    parser.add_argument(
        "compile_dest",
        metavar="FILE|DIR",
        nargs="*",
        help=(
            "zero or more file and directory names "
            "to compile; if no arguments given, defaults "
            "to the equivalent of -l sys.path"
        ),
    )
    # register -j arg
    parser.add_argument(
        "-j",
        "--workers",
        default=1,
        type=int,
        help="Run compileall concurrently",
    )
    # register --invalidation-mode arg
    parser.add_argument(
        "--invalidation-mode",
        choices=sorted(invalidation_modes),
        help=(
            "set .pyc invalidation mode; defaults to "
            '"checked-hash" if the SOURCE_DATE_EPOCH '
            "environment variable is set, and "
            '"timestamp" otherwise.'
        ),
    )
    # register -o arg
    parser.add_argument(
        "-o",
        action="append",
        type=int,
        dest="opt_levels",
        help=(
            "Optimization levels to run compilation with. "
            "Default is -1 which uses the optimization level "
            "of the Python interpreter itself (see -O)."
        ),
    )
    # register -e arg
    parser.add_argument(
        "-e",
        metavar="DIR",
        dest="limit_sl_dest",
        help="Ignore symlinks pointing outsite of the DIR",
    )
    # register --hardlink-dupes arg
    parser.add_argument(
        "--hardlink-dupes",
        action="store_true",
        dest="hardlink_dupes",
        help="Hardlink duplicated pyc files",
    )


def prepare_parser_args(
    parser: ArgumentParser,
    args: Namespace,
) -> tuple[int, PycInvalidationMode | None]:
    """Normalization and validation of command-line arguments.

    It post-processes raw argparse values by compiling regular
    expressions, resolving defaults, validating incompatible options,
    converting string values into their corresponding enum types and
    determines the effective recursion depth and invalidation mode.

    Args:
        parser (ArgumentParser): Argument parser instance.
        args (Namespace): Cli arguments to be normalized and validated.

    Returns:
        tuple[int, PycInvalidationMode | None]:
            - maxlevels: Recursion depth for directory traversal.
            - invalidation_mode: Selected bytecode invalidation mode,
                or None.
    """

    # Default invalidation mode
    invalidation_mode: PycInvalidationMode | None = None

    if args.rx:
        args.rx = re.compile(args.rx)

    # Empty symlink limit destination to None
    if args.limit_sl_dest == "":
        args.limit_sl_dest = None

    # Effective recursion depth:
    # explicit --recursion overrides --maxlevels
    if args.recursion is not None:
        maxlevels = args.recursion
    else:
        maxlevels = args.maxlevels

    # Default optimization level to
    # interpreter level if not specified
    if args.opt_levels is None:
        args.opt_levels = [-1]

    if len(args.opt_levels) == 1 and args.hardlink_dupes:
        parser.error(
            "Hardlinking of duplicated bytecode makes sense "
            "only for more than one optimization level."
        )

    # Destination directory (-d) cannot be
    # combined with stripdir or prependdir
    if args.ddir is not None and (
        args.stripdir is not None or args.prependdir is not None
    ):
        parser.error("-d cannot be used in combination with -s or -p")

    # Convert invalidation mode string to
    # PycInvalidationMode enum
    if args.invalidation_mode:
        ivl_mode = args.invalidation_mode.replace("-", "_").upper()
        invalidation_mode = PycInvalidationMode[ivl_mode]

    return maxlevels, invalidation_mode


def process_flist(args: Namespace, compile_dests: list[str]) -> bool | None:
    """Read additional compilation targets from a file or stdin.

    Args:
        args (Namespace): Parsed cli arguments. Uses `flist` to locate
            the input source and `quiet` to control error output.
        compile_dests (list[str]): List of compilation targets to
            extend.

    Returns:
        bool | None:
            - False if the file list could not be read.
            - None on success (targets are appended in place).
    """

    try:
        # Use stdin when "-" is specified,
        # otherwise open the provided file.
        with (
            sys.stdin
            if args.flist == "-"
            else open(args.flist, encoding="utf-8")
        ) as f:
            # Read each line, strip whitespace,
            # and treat it as a path.
            for line in f:
                compile_dests.append(line.strip())
    except OSError:
        # Report errors unless quiet mode
        # is set to suppress them.
        if args.quiet < 2:
            print("Error reading file list {}".format(args.flist))

        # Signal failure to the caller.
        return False


def run_compilation(
    compile_dests: list[str],
    args: Namespace,
    maxlevels: int,
    invalidation_mode: PycInvalidationMode | None,
) -> bool:
    """Run the compilation for the given destinations and arguments.

    Args:
        compile_dests (List[str]): List of files or directories to
            compile.
        args: Parsed argument namespace.
        maxlevels (int): Maximum recursion depth for directories.
        invalidation_mode: PycInvalidationMode to use.

    Returns:
        bool: True if all compilations succeeded, False if any failed.
    """
    success: bool = True
    compiled: bool

    try:
        if compile_dests:
            # Compile each destination
            for dest in compile_dests:
                if os.path.isfile(dest):
                    compiled = compile_file(
                        dest,
                        args.ddir,
                        args.force,
                        args.rx,
                        args.quiet,
                        args.legacy,
                        invalidation_mode=invalidation_mode,
                        stripdir=args.stripdir,
                        prependdir=args.prependdir,
                        optimize=args.opt_levels,
                        limit_sl_dest=args.limit_sl_dest,
                        hardlink_dupes=args.hardlink_dupes,
                    )
                    if not compiled:
                        success = False
                else:
                    compiled = compile_dir(
                        dest,
                        maxlevels,
                        args.ddir,
                        args.force,
                        args.rx,
                        args.quiet,
                        args.legacy,
                        workers=args.workers,
                        invalidation_mode=invalidation_mode,
                        stripdir=args.stripdir,
                        prependdir=args.prependdir,
                        optimize=args.opt_levels,
                        limit_sl_dest=args.limit_sl_dest,
                        hardlink_dupes=args.hardlink_dupes,
                    )
                    if not compiled:
                        success = False
            return success
        else:
            # If no destinations provided, compile all sys.path
            return compile_path(
                legacy=args.legacy,
                force=args.force,
                quiet=args.quiet,
                invalidation_mode=invalidation_mode,
            )

    except KeyboardInterrupt:
        if args.quiet < 2:
            print("\n[interrupted]")
        return False


def main() -> bool:
    """Entrypoint for the compileall.

    Returns:
        bool: True if all files compiled successfully, False if any
        errors occurred or if the process was interrupted.
    """

    # Creating the parser
    parser: ArgumentParser = ArgumentParser(
        description="Utilities to support installing Python libraries.",
        color=True,
    )
    # Build list of allowed invalidation modes
    invalidation_modes: list[str] = [
        mode.name.lower().replace("_", "-")
        for mode in py_compile.PycInvalidationMode
    ]
    # Add standard parser arguments
    add_parser_arguments(parser, invalidation_modes)
    # Parse command-line arguments into a Namespace object
    args: Namespace = parser.parse_args()
    # Extract compile destinations
    compile_dests: list[str] = args.compile_dest
    # Prepare max recursion depth and invalidation
    # mode from parser arguments
    maxlevels, invalidation_mode = prepare_parser_args(parser, args)

    # if flist is provided then load it
    if args.flist:
        completed: bool | None = process_flist(args, compile_dests)
        # If result is false, stop immediately
        if completed is False:
            return False

    # Run compilation
    return run_compilation(compile_dests, args, maxlevels, invalidation_mode)


if __name__ == "__main__":
    exit_status = int(not main())
    sys.exit(exit_status)
